/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/iec104.h"


char testdata[] = {"Do you see this string? Data received, all right.\0"};

char devlink[]  = "devlinktest";
char parser[]   = APP_NAME;
char physlink[] = "physlink-test";

struct config_device cd = {	devlink, parser, physlink, 10, 11};

static sigset_t sigmask;

int main(int argc, char *argv[])
{
	pid_t chldpid;
	int exit = 0;

	chldpid = mf_init(APP_PATH, APP_NAME, iec104_rcvdata, iec104_rcvinit);

	do{
		sigsuspend(&sigmask);
	}while(!exit);

	mf_exit();

	return 0;
}


int iec104_rcvdata(int len)
{
	char buf[100];
	int addr;
	int dir;

	mf_readbuffer(buf, len, &addr, &dir);
	printf("parser-iec104: HAS READ DATA: %s\n", buf);

	return 0;
}

int iec104_rcvinit(char **buf, int len)
{
	printf("parser-iec104: HAS READ INIT DATA: %s\n", buf[0]);
	printf("parser-iec104: HAS READ INIT DATA: %s\n", buf[1]);
	printf("parser-iec104: HAS READ INIT DATA: %s\n", buf[2]);
	printf("parser-iec104: HAS READ INIT DATA: %s\n", buf[3]);
	printf("parser-iec104: HAS READ INIT DATA: %s\n", buf[4]);

	return 0;
}









