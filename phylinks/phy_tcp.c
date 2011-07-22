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

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

struct phy_route *myprs[MAXEP];	// One phy_route for one endpoint
struct phy_route *firstpr;
int maxpr = 0;

int desclist = 0;	// Descriptor for listen

void send_sys_msg(struct phy_route *pr, int msg){
ep_data_header edh;
	edh.adr = pr->asdu;
	edh.sys_msg = msg;
	edh.len = 0;
	mf_toendpoint((char*) &edh, sizeof(ep_data_header), pr->asdu, DIRUP);
}

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
	addrcfg = fopen("/rw/mx00/configs/lowlevel.cfg", "r");
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

int sendall(int s, char *buf, int len, int flags){
int total = 0;
int n;

	while(total < len){
        n = send(s, buf+total, len-total, flags);
        if (n == -1) break;
        total += n;
    }

	return (n==-1 ? -1 : total);
}

void set_listen(struct phy_route *pr){
int ret;
	printf("Phylink TCP/IP: Listen for connect 0x%X:%d\n", pr->sai.sin_addr.s_addr, htons(pr->sai.sin_port));
// Bind привязывает к локальному адресу
	pr->sailist.sin_addr.s_addr = INADDR_ANY;
	ret = bind(pr->socdesc, (struct sockaddr *) &pr->sailist, sizeof(struct sockaddr_in));
	if (ret){
		printf("Phylink TCP/IP: bind error:%d - %s\n",errno, strerror(errno));
		return;
	}else printf("Phylink TCP/IP: bind established, listen waiting...\n");
	listen(pr->socdesc, 1);
	pr->state = 1;
}

void set_connect(struct phy_route *pr){
int ret;
	printf("Connect to %s:%d\n", inet_ntoa(pr->sai.sin_addr), htons(pr->sai.sin_port));
	ret = connect(pr->socdesc, (struct sockaddr *) &pr->sai, sizeof(struct sockaddr_in));
	if (ret){
		send_sys_msg(pr, EP_MSG_CONNECT_NACK);
		printf("Phylink TCP/IP: connect error:%d - %s\n",errno, strerror(errno));
	}else{
		fcntl(pr->socdesc, F_SETFL, O_NONBLOCK);	// Set socket as nonblock
		send_sys_msg(pr, EP_MSG_CONNECT_ACK);
		pr->state = 1;
		printf("Phylink TCP/IP: connect established.\n");
	}
}

void connect_by_config(struct phy_route *pr){
	pr->sai.sin_family = AF_INET;
	if (pr->mode == CONNECT) set_connect(pr);
	if (pr->mode == LISTEN) set_listen(pr);
}

int rcvdata(int len){
TRANSACTINFO tai;
char *inoti_buf;
struct phy_route *pr;
ep_data_header *edh;
int rdlen, i;

	tai.len = len;
	tai.addr = 1;

	inoti_buf = malloc(len);

	rdlen = mf_readbuffer(inoti_buf, len, &tai.addr, &tai.direct);
	// Get phy_route by addr
	for(i=0; i < maxpr; i++){
		if (myprs[i]->asdu == tai.addr){ pr = myprs[i]; break;}
	}
	if (i==maxpr){ free(inoti_buf); return 0;}
	edh = (struct ep_data_header *) inoti_buf;				// set start structure
	tai.buf = inoti_buf + sizeof(struct ep_data_header);	// set pointer to begin data
	switch(edh->sys_msg){
	case EP_USER_DATA:	// Write data to socket
			sendall(pr->socdesc, tai.buf, len - sizeof(struct ep_data_header), 0);
			break;

	case EP_MSG_RECONNECT:	// Disconnect and connect according to connect rules for this endpoint
			close(pr->socdesc);
			pr->socdesc = 0;
			pr->state = 0;

	case EP_MSG_CONNECT:
			// Create & bind new socket
			pr->socdesc = socket(AF_INET, SOCK_STREAM, 0);	// TCP for this socket
			if (pr->socdesc == -1) printf("Phylink TCP/IP: socket error:%d - %s\n",errno, strerror(errno));
			else{
				printf("Phylink TCP/IP: Socket 0x%X SET: addrasdu = %d, mode = 0x%X\n", pr->socdesc, pr->asdu, pr->mode);
				connect_by_config(pr);
			}
			break;

	case EP_MSG_CONNECT_CLOSE: // Disconnect and delete endpoint
			close(pr->socdesc);
			break;

	case EP_MSG_QUIT:
			appexit = 1;
			break;
	}

	free(inoti_buf);
	return 0;
}

int rcvinit(ep_init_header *ih){
int i;
struct phy_route *pr;

//#ifdef _DEBUG
//	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[0]);
//	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[1]);
//	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[2]);
//	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[3]);
//	printf("Phylink TCP/IP: HAS READ INIT DATA: %s\n", ih->isstr[4]);
//#endif

	printf("Phylink TCP/IP: HAS READ CONFIG_DEVICE: %d %d\n", ih->addr, ih->numep);

	// For connect route struct to socket find equal address ASDU in route struct set
	for (i = 0 ; i < maxpr; i++){
		if (myprs[i]->asdu == ih->addr){ pr = myprs[i]; break;}
	}
	if (i == maxpr){
		printf("Phylink TCP/IP: This connect not found\n");
		return 0;	// Route not found
	}
	if (pr->state){
		printf("This connect setting already\n");
		return 0;	// Route init already
	}
	pr->ep_index = ih->numep;

	printf("Phylink TCP/IP: route found: addr = %d, num = %d\n", ih->addr, i);
	return 0;
}


int main(int argc, char * argv[]){
pid_t chldpid;
int ret, i, rdlen;
struct timeval tv;	// for sockets select

char outbuf[1024];
struct phy_route *pr;
ep_data_header *edh;
struct sockaddr_in sin;
socklen_t sinlen;

fd_set rd_socks;
fd_set ex_socks;
int maxdesc;

	if (createroutetable() == -1){
		printf("Phylink TCP/IP: config file not found\n");
		return 0;
	}
	printf("Phylink TCP/IP: config table ready, %d records\n", maxpr);

	// Init multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	// Cycle select for sockets descriptors
	do{
	    FD_ZERO(&rd_socks);
	    FD_ZERO(&ex_socks);
		maxdesc = 0;
		for (i=0; i<maxpr; i++){
			pr = myprs[i];
			if ((pr->state) && (pr->socdesc)){
				if (pr->socdesc > maxdesc)	maxdesc = pr->socdesc;
				FD_SET(pr->socdesc, &rd_socks);
				FD_SET(pr->socdesc, &ex_socks);
			}
		}

		tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(maxdesc + 1, &rd_socks, NULL, &ex_socks, &tv);
	    if (ret == -1) printf("Phylink TCP/IP: select error:%d - %s\n",errno, strerror(errno));
	    else
	    if (ret){
	    	for (i=0; i<maxpr; i++){
	    		if (myprs[i]->state){
    				pr = myprs[i];
			    	if (FD_ISSET(pr->socdesc, &rd_socks)){
			    		if (pr->mode == LISTEN){
			    			// Accept connection
			    			printf("Phylink TCP/IP: accept\n");
			    			sinlen = sizeof(sin);
			    			rdlen = accept(pr->socdesc, (struct sockaddr *) &sin, &sinlen);
			    			if (rdlen == -1){
			    				printf("Phylink TCP/IP: accept error:%d - %s\n",errno, strerror(errno));
			    				send_sys_msg(pr, EP_MSG_CONNECT_NACK);
			    				pr->state = 0;
			    			}else{
			    				if (pr->sai.sin_addr.s_addr == sin.sin_addr.s_addr){
			    					fcntl(pr->socdesc, F_SETFL, O_NONBLOCK);	// Set socket as nonblock
			    					pr->socdesc = rdlen;
			    					pr->mode = LISTEN + CONNECT;
			    					send_sys_msg(pr, EP_MSG_CONNECT_ACK);
			    					printf("Phylink TCP/IP: accept OK\n");
			    				}else{
			    					close(pr->socdesc);
			    					set_listen(pr);
			    					printf("Phylink TCP/IP: client address incorrect\n");
			    				}
			    			}
			    		}else{
			    			// Receive frame
			    			rdlen = recv(pr->socdesc, outbuf + sizeof(ep_data_header), 1024  - sizeof(ep_data_header), 0);
			    			if (rdlen == -1){
			    				printf("Phylink TCP/IP: socket read error:%d - %s\n",errno, strerror(errno));
			    				close(pr->socdesc);
			    				pr->state = 0;
			    			}else{
			    				if (rdlen){
						    		printf("Phylink TCP/IP: Reading desc = 0x%X, num = %d, ret = %d, rdlen = %d\n", pr->socdesc, i, ret, rdlen);
			    					// Send data frame to endpoint
				    				edh = (ep_data_header*) outbuf;
				    				edh->adr = pr->asdu;
						    		edh->sys_msg = EP_USER_DATA;
			    					edh->len = rdlen;
			    					mf_toendpoint(outbuf, rdlen + sizeof(ep_data_header), pr->asdu, DIRUP);
			    				}
			    			}
			    		}
			    	}

			    	if (FD_ISSET(pr->socdesc, &ex_socks)){
			    		close(pr->socdesc);
			    		pr->state = 0;
			    		pr->socdesc = 0;
			    		send_sys_msg(pr, EP_MSG_CONNECT_LOST);
			    	}
	    		}
	    	}
	    }
//	    else printf("Phylink TCP/IP: Timeout\n");

	}while(!appexit);

	mf_exit();

	free(firstpr);
	return 0;
}
