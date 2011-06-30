/*
 *  phyint.c
 *  Created on: 01.06.2011
 *      Author: alex AAV
 *
 *	Linked data from physical device through parser to main application
 *
 *	Handles name consists:
 *	- last part of iec device name
 *	- system number from main app
 *
 */

#include "../common/common.h"
#include "../common/multififo.h"
#include "local-phyints.h"

#define LISTEN	0x42
#define CONNECT 0x43

struct phy_route *myprs[MAXEP];	// One phy_route for one endpoint
struct phy_route *firstpr;
int maxpr = 0;

fd_set rd_socks;
fd_set wr_socks;
fd_set ex_socks;

// return pointer to string of ip addr
char *parsenextip(FILE *file, char *pstr, char *outbuf){
char *p = 0;
size_t len = 1;
	p = fgets(outbuf, 100, file);
	if (p)
		p = strstr(pstr, outbuf);
	if (p){
		p += 6;					// skip -addr_
		while(*p == ' ') p++;	// skip spaces
	}
	return p;
}

char cfgparse(char *key){
char *par;
struct sockaddr_in adr;

	par = strstr(key, inoti_buf);
	if (par) par+=6;
	else return 0;

	if (strstr("mask",key)){
		inet_aton(par, &adr);
		return adr.s_adr;
	}

	if (strstr("port", key)){
		return atoi(par);
	}

	if (strstr("mode", key)){
		if (strstr("LISTEN", par)) return LISTEN;
		if (strstr("CONNECT", par)) return CONNECT;
	}

	if (strstr("name", key)){
		return (u32) par;
	}
}

char inoti_buf[100];
int rcvdata(int len){
TRANSACTINFO tai;
struct phy_route *pr;
struct ep_data_header *edh;
int rdlen;

	tai.buf = inoti_buf;
	tai.len = len;
	tai.addr = 0;

	rdlen = mftai_readbuffer(&tai);
	// Get phy_route by index
	if (tai->ep_index) pr = myprs[tai->ep_index];
	edh = inoti_buf;
	if (!edh->sysmsg){
		// Write data to socket

	}else{
		// System command

	}


	return 0;
}

int rcvinit(char **buf, int len){
struct phy_route *pr;
struct config_device *cd;
u32 i, addr;
FILE addrcfg;
char *paddr;
struct in_addr fileaddr;

	// Init new phy_route
	myprs[maxpr] = firstpr + sizeof(struct phy_route) * maxpr;
	pr = myprs[maxpr];
	strcpy(pr->name, buf[2]);

	printf("TCP_LINK HAS READ INIT DATA: %s\n", buf[0]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", buf[1]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", buf[2]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", buf[3]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n\n", buf[4]);

	cd = &buf[5];

	pr->sai.sin_addr.s_addr = cd->addr;

	printf("TCP_LINK HAS READ CONFIG_DEVICE: %d %d \n\n", cd->addr, cd->extaddr);

	addrcfg = fopen("/rw/mx00/configs/phys.cfg", "r");
	if (addrcfg){
		// Find socket config by address from file "phys.cfg"
		do{
			paddr = parsenextip(addrcfg, "-addr", inoti_buf);	// return pointer to full string with ip address
			if (!paddr) return 0;	// EOF
			addr = inet_aton(paddr, &fileaddr);
			if (addr){
				if (addr == cd->addr){
					// Parse parameters, Init struct phy_route
					// address OK => inoti_buf have full config string => parse config: -mask -port -mode -name
					pr->mask = cfgparse("-mask");
					pr->sai.sin_port = cfgparse("-port");
					pr->mode = cfgparse("-mode");
//					pr->name = (char*) cfgparse("-name");		// name not need for phy link
					pr->state = 0;
					pr->ep_index = maxpr;
					// Create & bind new socket
					pr->socdesc = socket(AF_INET, SOCK_STREAM, 0);	// TCP for this socket
					if (pr->socdesc){
						FD_SET(pr->socdesc, &rd_socks);
						FD_SET(pr->socdesc, &wr_socks);
						FD_SET(pr->socdesc, &ex_socks);
						bind(pr->socdesc, pr->sai, sizeof(sockaddr_in));
					}else return 0;
					break;
				}
			}
		}while(addr);
		maxpr++;

		// listen&accept || connect making in main function

		fclose(addrcfg);
	}

	return 0;
}


int main(int argc, char * argv[]){
pid_t chldpid;
int exit = 0, i, ret;
struct timeval tv;	// for sockets select

	// Init physical routes structures
	firstpr = malloc(sizeof(struct phy_route) * MAXEP);

	// Init select sets for sockets
	FD_ZERO(&rd_socks);
	FD_ZERO(&wr_socks);
	FD_ZERO(&ex_socks);

	// Init multififo
	chldpid = mf_init("/rw/mx00/devlinks","devlinktest", rcvdata, rcvinit);

	do{
	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(myprs[maxpr-1]->socdesc + 1, &rd_socks, &wr_socks, &ex_socks, &tv);
	    if (ret == -1) printf("%s: select error:%d - %s\n",appname, errno, strerror(errno));

		// Cycle select for sockets descriptors

	}while(!exit);

	mf_exit();

	free(firstpr);
	return 0;
}
