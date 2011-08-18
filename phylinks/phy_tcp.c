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

void close_phyroute(struct phy_route *pr){
struct linger l = { 1, 0 };
	pr->state = 0;
	setsockopt(pr->socdesc, SOL_SOCKET, SO_LINGER, &l, sizeof(struct linger));
	shutdown(pr->socdesc, SHUT_RDWR);
	close(pr->socdesc);
	pr->socdesc = 0;
}

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
				myprs[maxpr] = (struct phy_route*) (firstpr + maxpr);
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
						pr->lstsocdesc = 0;
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

int set_listen(struct phy_route *pr){
int ret;
	printf("Phylink TCP/IP: Listen for connect %s:%d\n", inet_ntoa(pr->sai.sin_addr), htons(pr->sai.sin_port));
// Bind привязывает к локальному адресу
	pr->sailist.sin_addr.s_addr = INADDR_ANY;
	pr->sailist.sin_port = pr->sai.sin_port;
	pr->lstsocdesc = pr->socdesc;
	pr->socdesc = 0;
	ret = bind(pr->lstsocdesc, (struct sockaddr *) &pr->sailist, sizeof(struct sockaddr_in));
	if (ret){
		printf("Phylink TCP/IP: bind error:%d - %s\n",errno, strerror(errno));
	}else{
		printf("Phylink TCP/IP: bind established, listen waiting...\n");
		if (listen(pr->lstsocdesc, 10) == -1) printf("Phylink TCP/IP: listen error:%d - %s\n",errno, strerror(errno));
		else ret = 1;
	}

	return ret;
}

int set_connect(struct phy_route *pr){
int ret;
	printf("Connect to %s:%d\n", inet_ntoa(pr->sai.sin_addr), htons(pr->sai.sin_port));
	ret = connect(pr->socdesc, (struct sockaddr *) &pr->sai, sizeof(struct sockaddr_in));
	if (ret){
		send_sys_msg(pr, EP_MSG_CONNECT_NACK);
		ret = 0;
		printf("Phylink TCP/IP: connect error:%d - %s\n",errno, strerror(errno));
	}else{
		fcntl(pr->socdesc, F_SETFL, O_NONBLOCK);	// Set socket as nonblock
		send_sys_msg(pr, EP_MSG_CONNECT_ACK);
		ret = 1;
		printf("Phylink TCP/IP: connect established.\n");
	}

	return ret;
}

int connect_by_config(struct phy_route *pr){
int ret = 0;
	pr->sai.sin_family = AF_INET;
	if (pr->mode == CONNECT) ret = set_connect(pr);
	if (pr->mode == LISTEN) ret = set_listen(pr);

	return ret;
}

int rcvdata(int len){
TRANSACTINFO tai;
char *inoti_buf;
struct phy_route *pr;
ep_data_header *edh;
int rdlen, i;
int offset;

	tai.len = len;
	tai.addr = 1;

	inoti_buf = malloc(len);

	rdlen = mf_readbuffer(inoti_buf, len, &tai.addr, &tai.direct);

	// set offset to zero before loop
	offset = 0;

	while(offset < rdlen)
	{
		if(rdlen - offset < sizeof(ep_data_header))
		{
			free(inoti_buf);
			return 0;
		}

		edh = (struct ep_data_header *) (inoti_buf + offset);				// set start structure
		offset += sizeof(ep_data_header);

		pr = NULL;
		// Get phy_route by edh addr
		for(i=0; i < maxpr; i++){
			if (myprs[i]->asdu == edh->adr){ pr = myprs[i]; break;}
		}

		// check if phy_route was found
		if (!pr){
			printf("Phylink TCP/IP error: This connect not found\n");
			offset += edh->len;
			continue;
		}

		tai.buf = inoti_buf + offset;	// set pointer to begin data

		switch(edh->sys_msg){
		case EP_USER_DATA:	// Write data to socket
				if(rdlen-offset >= edh->len) sendall(pr->socdesc, tai.buf, edh->len, 0);
				break;

		case EP_MSG_RECONNECT:	// Disconnect and connect according to connect rules for this endpoint
				printf("Phylink TCP/IP: Reconnect to: %d, local endpoint: %d\n", edh->adr, edh->numep);
				if (pr->state != EP_MSG_CONNECT){
					pr->state = EP_MSG_RECONNECT;	// Next work going to main cycle
					break;
				}

		case EP_MSG_CONNECT:
				if (pr->state == 1){
					printf("Phylink TCP/IP error: This connect setting already\n");
				}else{
					printf("Phylink TCP/IP: Connect to: %d %d\n", edh->adr, edh->numep);
					printf("Phylink TCP/IP: route found: addr = %d, ep_index = %d\n", edh->adr, pr->ep_index);
					pr->ep_index = edh->numep;

					// Create & bind new socket
					pr->socdesc = socket(AF_INET, SOCK_STREAM, 0);	// TCP for this socket
					if (pr->socdesc == -1) printf("Phylink TCP/IP error: socket error:%d - %s\n",errno, strerror(errno));
					else{
						printf("Phylink TCP/IP: Socket 0x%X SET: addrasdu = %d, mode = 0x%X, ep_up = %d\n", pr->socdesc, pr->asdu, pr->mode, pr->ep_index);
						pr->state = connect_by_config(pr);
					}
				}
				break;

		case EP_MSG_CONNECT_CLOSE: // Disconnect and delete endpoint
				pr->state = EP_MSG_CONNECT_CLOSE;		// Demand to close
				break;

		case EP_MSG_QUIT:
				appexit = 1;
				break;
		}

		// move over the data
		offset += edh->len;
	}


	free(inoti_buf);
	return 0;
}

// Program terminating
void sighandler_sigterm(int arg){
int i;
struct phy_route *pr;
struct linger l = { 1, 0 };

	printf("Phylink TCP/IP: fast close all sockets\n");
	for (i=0; i<maxpr; i++){
		pr = myprs[i];
		if ((pr->state) && (pr->socdesc)){
			pr->state = 0;
			usleep(100);		// Delay for really end work with this pr
			close_phyroute(pr);
		}
	}
	exit(0);
}

int main(int argc, char * argv[]){
pid_t chldpid;
int ret, i, rdlen;
struct timeval tv;	// for sockets select
struct linger l = { 1, 0 };

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
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata);

	signal(SIGTERM, sighandler_sigterm);
	signal(SIGINT, sighandler_sigterm);

// Cycle select for sockets descriptors
	do{
	    FD_ZERO(&rd_socks);
	    FD_ZERO(&ex_socks);
		maxdesc = 0;
		for (i=0; i<maxpr; i++){
			pr = myprs[i];

			// Close by demand
			if ((pr->state == EP_MSG_CONNECT_CLOSE) && (pr->socdesc)){
				close_phyroute(pr);
			}

			// Close socket and reconnect for CONNECT mode
			if ((pr->state == EP_MSG_RECONNECT) && (pr->socdesc)){
				printf("IEC61850: Reconnect socket to asdu %d\n", pr->asdu);
				close_phyroute(pr);
	    		if (pr->mode == CONNECT){
	    			pr->socdesc = socket(AF_INET, SOCK_STREAM, 0);	// TCP for this socket
	    			pr->state = set_connect(pr);
	    			if (!pr->state) pr->state = EP_MSG_CONNECT;	// Reconnect demanded
	    		}
			}

			// Add connected sockets
			if ((pr->state == 1) && (pr->socdesc)){
				if (pr->socdesc > maxdesc) maxdesc = pr->socdesc;
				FD_SET(pr->socdesc, &rd_socks);
				FD_SET(pr->socdesc, &ex_socks);
			}

			// Add listen sockets
			if ((pr->state == 1) && (pr->lstsocdesc)){
				if (pr->lstsocdesc > maxdesc) maxdesc = pr->lstsocdesc;
				FD_SET(pr->lstsocdesc, &rd_socks);
			}
		}

		tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(maxdesc + 1, &rd_socks, NULL, &ex_socks, &tv);
	    if (ret == -1){
	    	printf("Phylink TCP/IP error: select error:%d - %s\n",errno, strerror(errno));
	    	for(i=0; i < maxpr; i++){
	    		pr = myprs[i];
		    	printf("Phylink TCP/IP: phy_route %d, desc = 0x%X\n", i, pr->socdesc);
	    	}
	    	exit(0);
	    }
	    else
	    if (ret){
	    	for (i=0; i<maxpr; i++){
	    		if (myprs[i]->state){
    				pr = myprs[i];

    				if (FD_ISSET(pr->lstsocdesc, &rd_socks)){
    					// One connection to one port with enabled IP
		    			// Accept connection
    					if (!pr->socdesc){
    		    			printf("Phylink TCP/IP: accept\n");
    		    			sinlen = sizeof(sin);
    		    			rdlen = accept(pr->lstsocdesc, (struct sockaddr *) &sin, &sinlen);
    		    			if (rdlen == -1){
    		    				printf("Phylink TCP/IP: accept error:%d - %s\n",errno, strerror(errno));
    		    				send_sys_msg(pr, EP_MSG_CONNECT_NACK);
    		    				pr->state = 0;
    		    			}else{
    		    				if (pr->sai.sin_addr.s_addr == sin.sin_addr.s_addr){
    		    					fcntl(pr->socdesc, F_SETFL, O_NONBLOCK);	// Set socket as nonblock
    		    					pr->socdesc = rdlen;
    		    					send_sys_msg(pr, EP_MSG_CONNECT_ACK);
    		    					printf("Phylink TCP/IP: accept OK\n");
    		    				}else{
    		    					close_phyroute(pr);
    		    					printf("Phylink TCP/IP error: client address incorrect\n");
    					    		send_sys_msg(pr, EP_MSG_CONNECT_NACK);
    		    				}
    		    			}
    					}
    				}

			    	if (FD_ISSET(pr->socdesc, &rd_socks)){
		    			// Receive frame
		    			rdlen = recv(pr->socdesc, outbuf + sizeof(ep_data_header), 1024  - sizeof(ep_data_header), 0);
		    			if (rdlen == -1){
		    				printf("Phylink TCP/IP error: socket read error:%d - %s\n",errno, strerror(errno));
		    				if (pr->mode == CONNECT){
		    					close_phyroute(pr);
					    		send_sys_msg(pr, EP_MSG_CONNECT_LOST);
		    				}else{
					    		pr->socdesc = 0;
		    					send_sys_msg(pr, EP_MSG_CONNECT_NACK);
		    				}
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

			    	if (FD_ISSET(pr->socdesc, &ex_socks)){
			    		if (pr->mode == CONNECT){
			    			close_phyroute(pr);
				    		send_sys_msg(pr, EP_MSG_CONNECT_LOST);
			    		}else{
				    		pr->socdesc = 0;
			    			send_sys_msg(pr, EP_MSG_CONNECT_NACK);
			    		}
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
