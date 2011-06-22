/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include "../include/iec104.h"
#include "../../devlinks/devlink.h"

char devlink[]  = "devlink-test";
char parser[]   = "parser-iec104";
char physlink[] = "physlink-test";



struct config_device cd = {
		devlink,
		parser,
		physlink,
		10,
		11,
};

int main(int argc, char *argv[])
{
	pid_t chldpid;
	sigset_t sigmask;

	chldpid = mf_init("/rw/mx00/parsers", "parser-iec104", iec104_rcvdata, iec104_rcvinit);
//	mf_newendpoint(&cd, "/rw/mx00/devlinks");

	signal(SIGQUIT, sighandler_sigquit);
	signal(SIGPWR,  SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGCHLD, sighandler_sigchld);

	sigsuspend(&sigmask);
	printf("parser-iec104: detect stop child process\n");

	mf_exit();

	return 0;
}


int iec104_rcvdata(char *buf, int len)
{
	printf("parser-iec104: received data of %d bytes\n", len);

	return 0;
}

int iec104_rcvinit(char *buf, int len)
{
	printf("parser-iec104: received init of %d bytes\n", len);

	return 0;
}


void sighandler_sigchld(int arg)
{
	printf("parser-iec104: child quit\n");
	return;
}


void sighandler_sigquit(int arg)
{
	printf("parser-iec104: own quit\n");
	mf_exit();
	return;
}









