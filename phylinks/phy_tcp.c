/*
 *  phyint.c
 *  Created on: 01.06.2011
 *      Author: Alex AVAlon
 *
 *	Linked data from physical device (socket) through parser to main application
 *
 *	Handles name consists:
 *	- last part of iec device name
 *	- system number from main app
 *
 */

#include "../common/common.h"
#include "../common/multififo.h"
#include "local-phyints.h"

// Test config_device
//#include "fake-unitlink.h"

#define LISTEN	0x42
#define CONNECT 0x43

struct phy_route *myprs[MAXEP];	// One phy_route for one endpoint
struct phy_route *firstpr;
int maxpr = 0;

fd_set rd_socks;
fd_set wr_socks;
fd_set ex_socks;
int maxdesc;

int cfgparse(char *key, char *buf){
char *par;
struct in_addr adr;

	par = strstr(buf, key);
	if (par) par+=6;
	else return 0;

	if (strstr(key, "name")){
		return (int) strstr(par, "phy_tcp");
	}

	if (strstr(key, "mask")){
		inet_aton(par, &adr);
		return adr.s_addr;
	}

	if (strstr(key, "addr")){
		inet_aton(par, &adr);
		return adr.s_addr;
	}

	if (strstr(key, "port")){
		return atoi(par);
	}

	if (strstr(key, "mode")){
		if (strstr(par, "LISTEN")) return LISTEN;
		if (strstr(par, "CONNECT")) return CONNECT;
	}

	return -1;
}

int createroutetable(void){
FILE *addrcfg;
struct phy_route *pr;
char *p;
char outbuf[256];

// Init physical routes structures by phys.cfg file
	firstpr = malloc(sizeof(struct phy_route) * MAXEP);
	addrcfg = fopen("/rw/mx00/configs/phys.cfg", "r");
	if (addrcfg){
		// Create phy_route tables
		do{
			p = fgets(outbuf, 250, addrcfg);
			if (p){
				// Parse string
				myprs[maxpr] = (struct phy_route*) (firstpr + sizeof(struct phy_route) * maxpr);
				pr = myprs[maxpr];
				pr->asdu = atoi(outbuf);
				if (pr->asdu){
					if (cfgparse("-name", outbuf)){
						pr->sai.sin_addr.s_addr = cfgparse("-addr", outbuf);	// inet_aton returns net order (big endian)
						pr->mode = cfgparse("-mode", outbuf);
						pr->mask = cfgparse("-mask", outbuf);
						pr->sai.sin_port = htons(cfgparse("-port", outbuf));		// atoi returns little endian
						pr->ep_index = 0;
						pr->socdesc = 0;
						pr->state = 0;
						maxpr++;
					}
				}
			}
		}while(p);
	}else return -1;

	return 0;
}

int rcvdata(int len){
TRANSACTINFO tai;
char inoti_buf[256];
struct phy_route *pr;
ep_data_header *edh, edout;
int rdlen, ret;

	tai.buf = inoti_buf;
	tai.len = len;
	tai.addr = 0;

	rdlen = mftai_readbuffer(&tai);
	// Get phy_route by index
	if (tai.ep_index) pr = myprs[tai.ep_index-1];
	else return 0;
	edh = (struct ep_data_header *) inoti_buf;
	tai.buf = inoti_buf + sizeof(struct ep_data_header);
	switch(edh->sys_msg){
	case EP_USER_DATA:	// Write data to socket
			while(pr->nowrite);
			send(pr->socdesc, tai.buf, len - sizeof(struct ep_data_header), 0);
			break;

	case EP_MSG_RECONNECT:	// Disconnect and connect according to connect rules for this endpoint

			break;

	case EP_MSG_CONNECT_CLOSE: // Disconnect and delete endpoint

			break;

	case EP_MSG_CONNECT:
		// Create & bind new socket
			pr->socdesc = socket(AF_INET, SOCK_STREAM, 0);	// TCP for this socket
			if (pr->socdesc == -1) printf("Phylink TCP/IP: socket error:%d - %s\n",errno, strerror(errno));
			else{
				pr->sai.sin_family = AF_INET;
				printf("Phylink TCP/IP: Socket 0x%X SET: addrasdu = %d, mode = 0x%X\n", pr->socdesc, pr->asdu, pr->mode);

		// listen&accept || connect making in main function
				if (pr->mode == CONNECT){
					printf("Connect to 0x%s:%d\n", inet_ntoa(pr->sai.sin_addr), htons(pr->sai.sin_port));
					ret = connect(pr->socdesc, (struct sockaddr *) &pr->sai, sizeof(struct sockaddr_in));
					if (ret){
						edout.adr = pr->asdu;
						edout.sys_msg = EP_MSG_CONNECT_NACK;
						edout.len = 0;
						mf_toendpoint_by_index((char*)&edout, sizeof(ep_data_header), pr->ep_index, DIRUP);
						printf("Phylink TCP/IP: connect error:%d - %s\n",errno, strerror(errno));
						return 0;
					}else{
						edout.adr = pr->asdu;
						edout.sys_msg = EP_MSG_CONNECT_ACK;
						edout.len = 0;
						mf_toendpoint_by_index((char*)&edout, sizeof(ep_data_header), pr->ep_index, DIRUP);
						printf("Phylink TCP/IP: connect established.\n");
					}
					pr->state = 1;
				}

				if (pr->mode == LISTEN){
					printf("Listen 0x%X:%d\n", pr->sai.sin_addr.s_addr, htons(pr->sai.sin_port));
				// Bind привязывает к локальному адресу
					pr->sai.sin_addr.s_addr = INADDR_ANY;
					ret = bind(pr->socdesc, (struct sockaddr *) &pr->sai, sizeof(struct sockaddr));
					if (ret){
						printf("Phylink TCP/IP: bind error:%d - %s\n",errno, strerror(errno));
						return 0;
					}
					else printf("Phylink TCP/IP: bind established, listen waiting...\n");
					listen(pr->socdesc, 256);
					pr->state = 1;
				}
			}

			break;
	}

	return 0;
}

int rcvinit(ep_init_header *ih){
int i;
struct phy_route *pr;

#ifdef _DEBUG
	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[0]);
	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[1]);
	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[2]);
	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[3]);
	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[4]);
#endif

	printf("Phylink TCP/IP: HAS READ CONFIG_DEVICE: %d\n\n", ih->addr);

	// For connect route struct to socket find equal address ASDU in route struct set
	for (i = 0 ; i < maxpr; i++){
		if (myprs[i]->asdu == ih->addr){ pr = myprs[i]; break;}
	}
	if (i == maxpr){
		printf("This connect not found\n");
		return 0;	// Route not found
	}
	if (myprs[i]->state){
		printf("This connect setting already\n");
		return 0;	// Route init already
	}
	printf("Phylink TCP/IP: route found: addr = %d, num = %d\n", ih->addr, i);
	pr = myprs[i];
	pr->ep_index = ih->numch;

	return 0;
}


int main(int argc, char * argv[]){
pid_t chldpid;
int exit = 0, ret, i;
struct timeval tv;	// for sockets select

char outbuf[1024];
struct phy_route *pr;
ep_data_header *edh;

	if (createroutetable() == -1){
		printf("Phylink TCP/IP: config file not found\n");
		return 0;
	}
	printf("Phylink TCP/IP: config table ready, %d records\n", maxpr);

	// Init select sets for sockets
	FD_ZERO(&rd_socks);
	FD_ZERO(&wr_socks);
	FD_ZERO(&ex_socks);

	// Init multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	// Call for TEST
//	rcvinit(&fakeih);
	// END Call for TEST

	// Cycle select for sockets descriptors
	do{
		FD_ZERO(&rd_socks);
		FD_ZERO(&wr_socks);
		FD_ZERO(&ex_socks);
		maxdesc = 0;
    	for (i=0; i<maxpr; i++){
    		pr = myprs[i];
    		if ((pr->state) && (pr->socdesc)){
    			if (pr->socdesc > maxdesc)	maxdesc = pr->socdesc;
    			FD_SET(pr->socdesc, &rd_socks);
    			FD_SET(pr->socdesc, &wr_socks);
    			FD_SET(pr->socdesc, &ex_socks);
    		}
    	}

		tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(maxdesc + 1, &rd_socks, NULL, NULL, &tv);
	    if (ret == -1) printf("Phylink TCP/IP: select error:%d - %s\n",errno, strerror(errno));
	    else
	    if (ret){
	    	for (i=0; i<maxpr; i++){
	    		if (myprs[i]->state){
    				pr = myprs[i];
			    	if (FD_ISSET(pr->socdesc, &rd_socks)){
	    				edh = (ep_data_header*) outbuf;
	    				edh->adr = myprs[i]->asdu;
			    		if (pr->mode == LISTEN){
			    			// Accept connection
			    			printf("Phylink TCP/IP: accept\n");
			    			ret = accept(pr->socdesc, NULL, NULL);
			    			if (ret == -1){
			    				printf("Phylink TCP/IP: accept error:%d - %s\n",errno, strerror(errno));

			    				edh->sys_msg = EP_MSG_CONNECT_NACK;
			    				edh->len = 0;
			    				mf_toendpoint_by_index(outbuf, sizeof(ep_data_header), pr->ep_index, DIRUP);
			    				pr->state = 0;
			    			}else{
			    				printf("Phylink TCP/IP: accept OK\n");

			    				pr->socdesc = ret;
			    				pr->mode = LISTEN + CONNECT;

			    				edh->sys_msg = EP_MSG_CONNECT_ACK;
			    				edh->len = 0;
			    				mf_toendpoint_by_index(outbuf, sizeof(ep_data_header), pr->ep_index, DIRUP);
			    			}
			    		}else{
			    			// Receive frame
			    			ret = recv(pr->socdesc, outbuf + sizeof(ep_data_header), 1024, 0);
			    			if (ret == -1){
			    				printf("Phylink TCP/IP: socket read error:%d - %s\n",errno, strerror(errno));
			    				pr->state = 0;
			    			}else{
			    				if (ret){
			    					printf("Phylink TCP/IP: Event desc num %d reading with len = %d\n", i, ret);
			    					// Send frame to endpoint
			    					edh->sys_msg = EP_USER_DATA;
			    					edh->len = ret;
			    					mf_toendpoint_by_index(outbuf, ret + sizeof(ep_data_header), pr->ep_index, DIRUP);
			    				}
			    			}
			    		}
			    	}

			    	if (FD_ISSET(pr->socdesc, &wr_socks)){
			    		// Socket was writing
			    		pr->nowrite = 1;
//			    		printf("Phylink TCP/IP: Event desc num %d writing\n", i);
			    	}

			    	if (FD_ISSET(pr->socdesc, &ex_socks)){
			    		pr->nowrite = 0;
//			    		printf("Phylink TCP/IP: Event desc num %d exception\n", i);
			    	}
	    		}
	    	}
	    }
//	    else printf("Phylink TCP/IP: Timeout\n");

	}while(!exit);

	mf_exit();

	free(firstpr);
	return 0;
}
