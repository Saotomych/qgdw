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

#include "fake-unitlink.h"

#define LISTEN	0x42
#define CONNECT 0x43

struct phy_route *myprs[MAXEP];	// One phy_route for one endpoint
struct phy_route *firstpr;
int maxpr = 0;

fd_set rd_socks;
fd_set wr_socks;
fd_set ex_socks;

char cfgparse(char *key, char *buf){
char *par;
struct in_addr adr;

	par = strstr(key, buf);
	if (par) par+=6;
	else return 0;

	if (strstr("mask",key)){
		inet_aton(par, &adr);
		return adr.s_addr;
	}

	if (strstr("addr", key)){
		inet_aton(par, &adr);
		return adr.s_addr;
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

	return -1;
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
	if (tai.ep_index) pr = myprs[tai.ep_index];
	edh = (struct ep_data_header *) inoti_buf;
	if (!edh->sys_msg){
		// Write data to socket

	}else{
		// System command

	}

	return 0;
}

int rcvinit(ep_init_header *ih){
int i;
struct phy_route *pr;
config_device *cd;

	printf("TCP_LINK HAS READ INIT DATA: %s\n", ih->isstr[0]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", ih->isstr[1]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", ih->isstr[2]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", ih->isstr[3]);
	printf("TCP_LINK HAS READ INIT DATA: %s\n", ih->isstr[4]);

	cd = ih->edc;

	printf("TCP_LINK HAS READ CONFIG_DEVICE: %d\n\n", cd->addr);

	// For connect route struct to socket find equal address ASDU in route struct set
	for (i = 0 ; i < maxpr; i++){
		if (myprs[i]->asdu == cd->addr){ pr = myprs[i]; break;}
	}
	if (i == maxpr) return 0;
	printf("Phylink TCP/IP: route found: addr = %d, num = %d\n", cd->addr, i);
	pr = myprs[i];
	// Create & bind new socket
	pr->socdesc = socket(AF_INET, SOCK_STREAM, 0);	// TCP for this socket
	if (pr->socdesc){
		FD_SET(pr->socdesc, &rd_socks);
		FD_SET(pr->socdesc, &wr_socks);
		FD_SET(pr->socdesc, &ex_socks);
		bind(pr->socdesc, (struct sockaddr *) &pr->sai, sizeof(struct sockaddr_in));
	}

	// listen&accept || connect making in main function

	return 0;
}


int main(int argc, char * argv[]){
pid_t chldpid;
int exit = 0, ret;
struct timeval tv;	// for sockets select

FILE *addrcfg;
char outbuf[251];
char *p;
struct phy_route *pr;

	// Init physical routes structures by phys.cfg file
	firstpr = malloc(sizeof(struct phy_route) * MAXEP);
	addrcfg = fopen("/rw/mx00/configs/phys.cfg", "r");
	if (addrcfg){
		// Create phy_route tables
		do{
			p = fgets(outbuf, 250, addrcfg);
			if (p){
				// Parse string
				myprs[maxpr] = firstpr + sizeof(struct phy_route) * maxpr;
				pr = myprs[maxpr];
				pr->asdu = atoi(p);
				if (pr->asdu){
					pr->mode = cfgparse("-mode", outbuf);
					pr->mask = cfgparse("-mask", outbuf);
					pr->sai.sin_addr.s_addr = htonl(cfgparse("-addr", outbuf));
					pr->sai.sin_port = htons(cfgparse("-port", outbuf));
					pr->ep_index = maxpr;
					pr->socdesc = 0;
					pr->state = 0;
					maxpr++;
				}
			}
		}while(p);
	}
	printf("Phylink TCP/IP: config table ready, %d records\n", maxpr);

	// Init select sets for sockets
	FD_ZERO(&rd_socks);
	FD_ZERO(&wr_socks);
	FD_ZERO(&ex_socks);

	// Init multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	// Call for TEST
	rcvinit(&fakeih);
	// END Call for TEST

	do{
	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(myprs[maxpr-1]->socdesc + 1, &rd_socks, &wr_socks, &ex_socks, &tv);
	    if (ret == -1) printf("Phylink TCP/IP: select error:%d - %s\n",errno, strerror(errno));

		// Cycle select for sockets descriptors

	    // Установка связи
	    // через коннект
	    // через листен

	    // Прием фрейма данных
	    // отправка его на все подключенные каналы

	}while(!exit);

	mf_exit();

	free(firstpr);
	return 0;
}
