/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/dlt645.h"


static sigset_t sigmask;

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

int main(int argc, char *argv[])
{
	pid_t chldpid;

	chldpid = mf_init(APP_PATH, APP_NAME, dlt645_rcvdata, dlt645_rcvinit);

	do{
		sigsuspend(&sigmask);
	}while(!appexit);

	mf_exit();

	exit(0);
}


int dlt645_rcvdata(int len)
{
	printf("parser-dlt645: received data of %d bytes\n", len);

	return 0;
}

int dlt645_rcvinit(char *buf, int len)
{
	printf("parser-dlt645: received init of %d bytes\n", len);

	return 0;
}



