/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/dlt645.h"


static sigset_t sigmask;

int main(int argc, char *argv[])
{
	pid_t chldpid;
	int exit = 0;

	chldpid = mf_init(APP_PATH, APP_NAME, dlt645_rcvdata, dlt645_rcvinit);

	do{
		sigsuspend(&sigmask);
	}while(!exit);

	mf_exit();

	return 0;
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



