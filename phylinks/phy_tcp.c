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
		par = strstr(par, "phy_tcp");
		if(par){
			par+=8;

			return atoi(par);
		}
		else{
			return 0;
		}
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
int port;

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
					port = cfgparse("-name", outbuf); // get tcp port number
					if (port){
						pr->sai.sin_family = AF_INET;
						pr->sai.sin_addr.s_addr = cfgparse("-addr", outbuf);	// inet_aton returns net order (big endian)
						pr->sai.sin_port = htons(port);							// convert port number from little endian
						pr->mode = cfgparse("-port", outbuf);					// port mode - LISTEN/CONNECT
						pr->mask = cfgparse("-mask", outbuf);					// inet_aton returns net order (big endian)
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
int ret, i;
struct phy_route *lpr = NULL;

	printf("Phylink TCP/IP: Setup listen for connect from %s to port %d\n", inet_ntoa(pr->sai.sin_addr), htons(pr->sai.sin_port));

	pr->sailist.sin_family      = pr->sai.sin_family;
	pr->sailist.sin_addr.s_addr = INADDR_ANY;
	pr->sailist.sin_port        = pr->sai.sin_port;

	for(i=0; i<maxpr;i++)
	{
		// look if listen port already binded
		if(myprs[i]->mode == LISTEN && myprs[i]->sai.sin_port == pr->sai.sin_port && myprs[i]->lstsocdesc != 0) { lpr = myprs[i]; break; }
	}

	if(lpr)
	{
		pr->lstsocdesc = lpr->lstsocdesc;
		pr->socdesc = 0;
		printf("Phylink TCP/IP: Bind established, listen waiting...\n");
	}
	else
	{
		pr->lstsocdesc = socket(pr->sailist.sin_family, SOCK_STREAM, 0);	// TCP for this socket

		if (pr->lstsocdesc == -1){
			printf("Phylink TCP/IP: Socket error: %d - %s\n",errno, strerror(errno));
			pr->lstsocdesc = 0;

			send_sys_msg(pr, EP_MSG_CONNECT_NACK);

			return 0;
		}

		printf("Phylink TCP/IP: Socket 0x%X SET: addrasdu = %d, mode = 0x%X, ep_up = %d\n", pr->lstsocdesc, pr->asdu, pr->mode, pr->ep_index);

		pr->socdesc = 0;

		// Bind привязывает к локальному адресу
		ret = bind(pr->lstsocdesc, (struct sockaddr *) &pr->sailist, sizeof(struct sockaddr_in));
		if (ret){
			printf("Phylink TCP/IP: Bind error: %d - %s\n",errno, strerror(errno));
			close(pr->lstsocdesc);
			pr->lstsocdesc = 0;

			send_sys_msg(pr, EP_MSG_CONNECT_NACK);
		}else{
			if (listen(pr->lstsocdesc, 10) == -1){
				printf("Phylink TCP/IP: Listen error: %d - %s\n",errno, strerror(errno));
				close(pr->lstsocdesc);
				pr->lstsocdesc = 0;

				send_sys_msg(pr, EP_MSG_CONNECT_NACK);
			}
			else
				printf("Phylink TCP/IP: Bind established, listen waiting...\n");
		}
	}

	return 0;
}

int set_connect(struct phy_route *pr){
int ret;
	printf("Phylink TCP/IP: Connect to %s:%d\n", inet_ntoa(pr->sai.sin_addr), htons(pr->sai.sin_port));

	pr->socdesc = socket(pr->sai.sin_family, SOCK_STREAM, 0);	// TCP for this socket

	if (pr->socdesc == -1){
		printf("Phylink TCP/IP: Socket error: %d - %s\n",errno, strerror(errno));
		pr->socdesc = 0;

		send_sys_msg(pr, EP_MSG_CONNECT_NACK);

		return 0;
	}

	printf("Phylink TCP/IP: Socket 0x%X SET: addrasdu = %d, mode = 0x%X, ep_up = %d\n", pr->socdesc, pr->asdu, pr->mode, pr->ep_index);

	ret = connect(pr->socdesc, (struct sockaddr *) &pr->sai, sizeof(struct sockaddr_in));
	if (ret){
		close_phyroute(pr);
		send_sys_msg(pr, EP_MSG_CONNECT_NACK);
		ret = 0;
		printf("Phylink TCP/IP: Connect error: %d - %s\n", errno, strerror(errno));
	}else{
		fcntl(pr->socdesc, F_SETFL, O_NONBLOCK);	// Set socket as nonblock
		send_sys_msg(pr, EP_MSG_CONNECT_ACK);
		ret = 1;
		printf("Phylink TCP/IP: Connect established.\n");
	}

	return ret;
}

int connect_by_config(struct phy_route *pr){
int ret = 0;
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
			printf("Phylink TCP/IP: System error: this connection not found. addrasdu = %d\n", edh->adr);
			offset += edh->len;
			continue;
		}

		tai.buf = inoti_buf + offset;	// set pointer to begin data

		switch(edh->sys_msg){
		case EP_USER_DATA:	// Write data to socket
				if(rdlen-offset >= edh->len && pr->state == 1) sendall(pr->socdesc, tai.buf, edh->len, 0);
				break;

		case EP_MSG_RECONNECT:	// Disconnect and connect according to connect rules for this endpoint
				printf("Phylink TCP/IP: Reconnect to: %d, local endpoint: %d\n", edh->adr, edh->numep);
				pr->state = EP_MSG_RECONNECT;	// Next work going to main cycle
				break;

		case EP_MSG_CONNECT:
				if (pr->state == 1){
					printf("Phylink TCP/IP: This connect is already set!\n");
				}else{
					printf("Phylink TCP/IP: Connect to: %d, local endpoint: %d\n", edh->adr, edh->numep);
					printf("Phylink TCP/IP: Route found: addr = %d, ep_index = %d\n", edh->adr, pr->ep_index);
					pr->ep_index = edh->numep;

					pr->state = connect_by_config(pr);
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

	printf("Phylink TCP/IP: Fast close all sockets\n");
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
int ret, i, j, rdlen;
SOCKET soclst;
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
		printf("Phylink TCP/IP: Config file not found\n");
		return 0;
	}
	printf("Phylink TCP/IP: Config table ready, %d records\n", maxpr);

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
			if (pr->state == EP_MSG_RECONNECT){
				if (pr->socdesc){
					printf("Phylink TCP/IP: Reconnect socket to asdu %d\n", pr->asdu);
					close_phyroute(pr);
				}else{
					printf("Phylink TCP/IP: Repeat reconnect socket to asdu %d\n", pr->asdu);
				}

				pr->state = connect_by_config(pr);
			}

			// Add connected sockets
			if ((pr->state == 1) && (pr->socdesc)){
				if (pr->socdesc > maxdesc) maxdesc = pr->socdesc;
				FD_SET(pr->socdesc, &rd_socks);
				FD_SET(pr->socdesc, &ex_socks);
			}

			// Add listen sockets
			if (pr->lstsocdesc){
				if (pr->lstsocdesc > maxdesc) maxdesc = pr->lstsocdesc;
				FD_SET(pr->lstsocdesc, &rd_socks);
			}
		}

		tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(maxdesc + 1, &rd_socks, NULL, &ex_socks, &tv);
	    if (ret == -1){
	    	printf("Phylink TCP/IP: Select error: %d - %s\n",errno, strerror(errno));
	    	for(i=0; i < maxpr; i++){
	    		pr = myprs[i];
		    	printf("Phylink TCP/IP: phy_route %d, desc = 0x%X\n", i, pr->socdesc);
	    	}
	    	exit(0);
	    }
	    else
	    if (ret){
	    	for (i=0; i<maxpr; i++){
				pr = myprs[i];

				if (FD_ISSET(pr->lstsocdesc, &rd_socks)){
					// One connection to one port with enabled IP
	    			// Accept connection
	    			printf("Phylink TCP/IP: Incoming connection ...\n");
	    			sinlen = sizeof(sin);
	    			soclst = accept(pr->lstsocdesc, (struct sockaddr *) &sin, &sinlen);
	    			if (soclst == -1){
	    				printf("Phylink TCP/IP: Accept error:%d - %s\n",errno, strerror(errno));
	    				send_sys_msg(pr, EP_MSG_CONNECT_NACK);
	    				pr->state = 0;
	    			}else{
	    		    	for (j=i; j<maxpr; j++){
	    		    		if(myprs[j]->mode != LISTEN) continue;

	    		    		pr = myprs[j];

		    				if (pr->sai.sin_addr.s_addr == sin.sin_addr.s_addr){
		    					// Close old socket
		    					if (pr->socdesc) close_phyroute(pr);
		    					// Set new socket
		    					pr->socdesc = soclst;
		    					fcntl(pr->socdesc, F_SETFL, O_NONBLOCK);	// Set socket as nonblock
		    					send_sys_msg(pr, EP_MSG_CONNECT_ACK);
		    					pr->state = 1;
		    					printf("Phylink TCP/IP: Socket 0x%X Connection from %s accept OK. \n", pr->socdesc, inet_ntoa(pr->sai.sin_addr));
		    					break;
		    				}
	    		    	}

	    				if(j == maxpr){
	    					printf("Phylink TCP/IP: Client address %s unknown\n" , inet_ntoa(sin.sin_addr));
	    					shutdown(soclst, SHUT_RDWR);
	    					close(soclst);
	    				}
	    			}

	    			// exit loop
    				break;
				}

   	    		if (myprs[i]->state == 1){
			    	if (FD_ISSET(pr->socdesc, &rd_socks)){
		    			// Receive frame
		    			rdlen = recv(pr->socdesc, outbuf + sizeof(ep_data_header), 1024  - sizeof(ep_data_header), 0);
		    			if (rdlen == -1){
				    		send_sys_msg(pr, EP_MSG_CONNECT_LOST);
	    					close_phyroute(pr);
		    				printf("Phylink TCP/IP: Socket read error: %d - %s\n",errno, strerror(errno));
				    		printf("Phylink TCP/IP: Connection to %s closed\n", inet_ntoa(pr->sai.sin_addr));
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

		    			// exit loop
	    				break;
			    	}

			    	if (FD_ISSET(pr->socdesc, &ex_socks)){
			    		send_sys_msg(pr, EP_MSG_CONNECT_LOST);
		    			close_phyroute(pr);
			    		printf("Phylink TCP/IP: Connection with %s lost\n", inet_ntoa(pr->sai.sin_addr));

			    		// exit loop
	    				break;
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
