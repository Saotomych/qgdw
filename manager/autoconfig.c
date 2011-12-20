/*
 * autoconfig.c
 *
 *  Created on: 01.09.2011
 *  Author: Grigoriy Musiyaka
 */

#include "autoconfig.h"
#include "icdxml.h"

LOWREC *lrs[MAXEP] = {NULL};

uint32_t maxrec = 0;

// Speed chains for connect look up
uint32_t spdstty[] = {9600, 2400, 1200, 0};

// List of unitlinks, index depends on scenario ids
char *unitlink_list[] = {NULL, "unitlink-iec104", "unitlink-iec101", "unitlink-m700", "unitlink-dlt645"};

char *Addrfile = NULL;

/* Request-Response frame buffer variables */
static time_t timer_recv = 0;				/* timer for full response from device */
static uint16_t t_recv = RECV_TIMEOUT;		/* timeout for full response from device */

/* Timer parameters */
static uint8_t alarm_t = ALARM_PER;

/* Indexes */
static int32_t speed_idx = 0;				/* current speed index */


// Support functions
void catch_alarm(int sig)
{
	time_t cur_time = time(NULL);

	// check request-response timer
	if(timer_recv > 0 && difftime(cur_time, timer_recv) >= t_recv)
	{
		// tired of waiting for response from device

		// stop request timer
		timer_recv = 0;
#ifdef _DEBUG
		printf("Config Manager: Timer req went off.\n");
#endif
	}

	signal(sig, catch_alarm);

	alarm(alarm_t);
}

// Create lowlevel strings in text buffer
int createllforiec104(LOWREC *lr, char* scfg){
// Example: 967 -addr 84.242.3.213 -name "unitlink-iec104.phy_tcp.2404" -port CONNECT -sync 300 -wack 1
	int ret;

	// Form lowlevel string
	ret = sprintf(scfg, "%d -addr %s -name \"unitlink-iec104.phy_tcp.%d\" -port %s -sync 300 -wack %d\n", lr->asdu, inet_ntoa(lr->sai.sin_addr), lr->sai.sin_port, lr->host_type == HOST_MASTER?"CONNECT":"LISTEN", lr->host_type == HOST_MASTER?1:1);

	return ret;
}

int createllforiec101(LOWREC *lr, char* scfg){
// Example: 967 -addr 967 -name "unitlink-iec101.phy_tty.ttyS3" -port 9600,8N1,NO -sync 300
	int ret;

	// Form lowlevel string
	ret = sprintf(scfg, "%d -addr %llu -name \"unitlink-iec101.phy_tty.ttyS3\" -port %d,8E1,NO -sync 300\n", lr->asdu, lr->link_adr, lr->setspeed);

	return ret;
}

int createllfordlt645(LOWREC *lr, char* scfg){
// Example: 1001 -addr 001083100021 -name "unitlink-dlt645.phy_tty.ttyS3" -port 9600,8E1,NO -sync 300
	int ret;

	// Form lowlevel string
	ret = sprintf(scfg, "%d -addr %12llu -name \"unitlink-dlt645.phy_tty.ttyS3\" -port %d,8E1,NO -sync 300\n", lr->asdu, lr->link_adr, lr->setspeed);

	return ret;
}

int createllformx00(LOWREC *lr, char* scfg){
// Example: 1000 -addr 01 -name "unitlink-m700.phy_tty.ttyS4" -port 9600,8E1,NO -sync 300
	int ret;

	// Form lowlevel string
	ret = sprintf(scfg, "%d -addr %llu -name \"unitlink-m700.phy_tty.ttyS4\" -port %d,8E1,NO -sync 300\n", lr->asdu, lr->link_adr, lr->setspeed);

	return ret;
}

// Create lowrecord, call from addrxml-parser
int createlowrecord(LOWREC *lr){
int ret = -1;

	lrs[maxrec] = malloc(sizeof(LOWREC));
	if (lrs[maxrec]){
		ret = 0;
		lrs[maxrec]->sai.sin_family = AF_INET;
		lrs[maxrec]->sai.sin_addr.s_addr = lr->sai.sin_addr.s_addr;
		lrs[maxrec]->sai.sin_port = lr->sai.sin_port;
		lrs[maxrec]->link_adr = lr->link_adr;
		lrs[maxrec]->asdu = lr->asdu;
		lrs[maxrec]->host_type = lr->host_type;
		lrs[maxrec]->lninst = lr->lninst;
		lrs[maxrec]->scen = lr->scen;
		lrs[maxrec]->scfg = 0;
		lrs[maxrec]->connect = lr->connect;
		lrs[maxrec]->myep = 0;
		lrs[maxrec]->setspeed = 0;
		maxrec++;
	}

	return ret;
}

int loadaddrcfg(char *name){
FILE *addrcfg;
int adrlen, ret = -1;
struct stat fst;

	addrcfg = fopen(name, "r");
	if (addrcfg){
	 	// get size of main config file
		stat(name, &fst);

		Addrfile = malloc(fst.st_size);

		// read main config file
		adrlen = fread(Addrfile, 1, (size_t) (fst.st_size), addrcfg);
		if (adrlen == fst.st_size) ret = 0;
	}
	else
	{
		printf("Config Manager: Addr.cfg file cannot opened\n");
	}

	return ret;
}

int recv_data(int len)
{
	char *buff;
	int adr, dir, i;
	uint32_t offset;
	ep_data_header *ep_header_in;

	buff = (char*) malloc(len);

	if(!buff) return -1;

	len = mf_readbuffer(buff, len, &adr, &dir);

#ifdef _DEBUG
	printf("Config Manager: Data received. Address = %d, Length = %d, Direction = %s.\n", adr, len, dir == DIRDN ? "DIRUP" : "DIRDN");
#endif

	offset = 0;

	while(offset < len)
	{
		if(len - offset < sizeof(ep_data_header))
		{
#ifdef _DEBUG
			printf("Config Manager: ERROR - Looks like ep_data_header missed.\n");
#endif

			free(buff);
			return -1;
		}

		ep_header_in = (ep_data_header*)(buff + offset);
		offset += sizeof(ep_data_header);

#ifdef _DEBUG
		printf("Config Manager: Received ep_data_header - adr = %d, sys_msg = %d, len = %d.\n", ep_header_in->adr, ep_header_in->sys_msg, ep_header_in->len);
#endif

		if(len - offset < ep_header_in->len)
		{
#ifdef _DEBUG
			printf("%s: ERROR - Expected data length %d bytes, received %d bytes.\n", sizeof(ep_data_header) + ep_header_in->len, len - offset);
#endif

			free(buff);
			return -1;
		}

		if(ep_header_in->sys_msg == EP_MSG_DEV_ONLINE)
		{
#ifdef _DEBUG
			printf("Config Manager: System message EP_MSG_DEV_ONLINE received. Address = %d, Length = %d\n", ep_header_in->adr, ep_header_in->len);
#endif

			for(i=0; i<maxrec; i++)
			{
				if(lrs[i]->asdu == ep_header_in->adr)
				{
					lrs[i]->connect = 1;

#ifdef _DEBUG
					printf("Config Manager: Device is Online. Address = %d\n", ep_header_in->adr);
#endif
					break;
				}
			}
		}

		offset += ep_header_in->len;
	}

	free(buff);

	return 0;
}

void send_sys_msg(int adr, int msg){
ep_data_header edh;
	edh.adr = adr;
	edh.sys_msg = msg;
	edh.len = 0;
	mf_toendpoint((char*) &edh, sizeof(ep_data_header), adr, DIRDN);
}

void test_connection_iec104(FILE* fdesc)
{
	int i, f_empty = 1, ret;

	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->scen == IEC104)
		{
			if(!lrs[i]->scfg) lrs[i]->scfg = (char*) calloc(128, sizeof(char));

			createllforiec104(lrs[i], lrs[i]->scfg);

			fprintf(fdesc, "%s", lrs[i]->scfg);

			f_empty = 0;
		}
	}

	fclose(fdesc);

	if(!f_empty)
	{
		// create and start all endpoints for this type of scenario
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == IEC104)
			{
				mf_newendpoint(lrs[i]->asdu, "unitlink-iec104", CHILD_APP_PATH, 0);
				sleep(EP_INIT_TIME);
				send_sys_msg(lrs[i]->asdu, EP_MSG_TEST_CONN);
			}
		}

		// set timeout
		t_recv = RECV_TIMEOUT*2;

		// start response timer
		timer_recv = time(NULL);

		// wait for responses
		while(timer_recv);

		// stop child application
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == IEC104)
			{
				// force child app to quit
				send_sys_msg(lrs[i]->asdu, EP_MSG_QUIT);

				// wait until child app is quit
				for(;;)
				{
					ret = mf_testrunningapp("unitlink-iec104");
					if( ret == 0 || ret == -1 ) break;
					usleep(100000);
				}

				break;
			}
		}
	}
}

void test_connection_iec101(FILE* fdesc)
{

}

void test_connection_dlt645(FILE* fdesc)
{
	int i, f_empty = 1, dev_num = 0, ret;

	// update speed for all devices for current scenario
	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->scen == DLT645)
		{
			lrs[i]->setspeed = spdstty[speed_idx];

			if(!lrs[i]->scfg) lrs[i]->scfg = (char*) calloc(128, sizeof(char));

			createllfordlt645(lrs[i], lrs[i]->scfg);

			fprintf(fdesc, "%s", lrs[i]->scfg);

			f_empty = 0;

			dev_num++;
		}
	}

	fclose(fdesc);

	if(!f_empty)
	{
		// create and start all endpoints for this type of scenario
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == DLT645)
			{
				mf_newendpoint(lrs[i]->asdu, "unitlink-dlt645", CHILD_APP_PATH, speed_idx?1:0);
				// wait for initialization to end
				sleep(EP_INIT_TIME);
			}
		}


		// start connection test
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == DLT645)
			{
				// send only for one endpoint, unitlink will take care about the rest
				send_sys_msg(lrs[i]->asdu, EP_MSG_TEST_CONN);
				break;
			}
		}

		// set timeout
		t_recv = RECV_TIMEOUT*dev_num;

		// start response timer
		timer_recv = time(NULL);

		// wait for responses
		while(timer_recv);

		// stop child application
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == DLT645)
			{
				// force child app to quit
				send_sys_msg(lrs[i]->asdu, EP_MSG_QUIT);

				// wait until child app is quit
				for(;;)
				{
					ret = mf_testrunningapp("unitlink-dlt645");
					if( ret == 0 || ret == -1 ) break;
					usleep(100000);
				}

				break;
			}
		}
	}

	// check if any device is Online
	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->scen == DLT645 && lrs[i]->connect)
		{
			// stop test
			return;
		}
	}

	speed_idx++;

	// check
	if(spdstty[speed_idx] != 0)
	{
		fdesc = fopen(LLEVEL_FILE, "w");

		if(fdesc) test_connection_dlt645(fdesc);
	}
}

void test_connection_mx00(FILE* fdesc)
{
	int i, f_empty = 1, dev_num = 0, ret;

	// update speed for all devices for current scenario
	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->scen == MX00)
		{
			lrs[i]->setspeed = spdstty[speed_idx];

			if(!lrs[i]->scfg) lrs[i]->scfg = (char*) calloc(128, sizeof(char));

			createllformx00(lrs[i], lrs[i]->scfg);

			fprintf(fdesc, "%s", lrs[i]->scfg);

			f_empty = 0;

			dev_num++;
		}
	}

	fclose(fdesc);

	if(!f_empty)
	{
		// create and start all endpoints for this type of scenario
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == MX00)
			{
				mf_newendpoint(lrs[i]->asdu, "unitlink-m700", CHILD_APP_PATH, speed_idx?1:0);
				// wait for initialization to end
				sleep(EP_INIT_TIME);
			}
		}

		// start connection test
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == MX00)
			{
				// send only for one endpoint, unitlink will take care about the rest
				send_sys_msg(lrs[i]->asdu, EP_MSG_TEST_CONN);
				break;
			}
		}

		// set timeout
		t_recv = RECV_TIMEOUT*dev_num;

		// start response timer
		timer_recv = time(NULL);

		// wait for responses
		while(timer_recv);

		// stop child application
		for(i=0; i<maxrec; i++)
		{
			if(lrs[i]->scen == MX00)
			{
				// force child app to quit
				send_sys_msg(lrs[i]->asdu, EP_MSG_QUIT);

				// wait until child app is quit
				for(;;)
				{
					ret = mf_testrunningapp("unitlink-m700");
					if( ret == 0 || ret == -1 ) break;
					usleep(100000);
				}

				break;
			}
		}
	}

	// check if any device is Online
	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->scen == MX00 && lrs[i]->connect)
		{
			// stop test
			return;
		}
	}

	speed_idx++;

	// check
	if(spdstty[speed_idx] != 0)
	{
		fdesc = fopen(LLEVEL_FILE, "w");

		if(fdesc) test_connection_mx00(fdesc);
	}
}

void (*scenfunc[])(FILE* fdesc) = {NULL, test_connection_iec104, test_connection_iec101, test_connection_mx00, test_connection_dlt645};

int main(int argc, char * argv[]){
uint8_t i, scen;
FILE* fdesc;
// Backup previous lowlevel.cfg and ieclevel.icd
	rename(LLEVEL_FILE, "/rw/mx00/configs/lowlevel.bak");
	rename(ICD_FILE,    "/rw/mx00/configs/ieclevel.bak");

	if (loadaddrcfg(ADDR_FILE) == -1) return -1;

	if(!strstr(Addrfile, "</Hardware>"))
	{
		printf("Config Manager: ADDR file is incomplete\n");
		exit(1);
	}

	// get  MAC-address of ethernet card
	get_mac(MAC_FILE);

	// Create lowrecord structures
	XMLSelectSource(Addrfile);

	printf("Config Manager: low records = %d\n", maxrec);

	signal(SIGALRM, catch_alarm);
	alarm(alarm_t);

	// init multififo
	mf_init(APP_PATH, APP_NAME, recv_data);

	// Create lowlevel.cfg for concrete speed level
	// 1: fixed asdu iec104
	// 2: fixed asdu iec101
	// 3: fixed MAC, dynamic asdu, m700
	// 4: dynamic asdu, dlt645
	for (scen = 1; scen < 5; scen++)
	{
		fdesc = fopen(LLEVEL_FILE, "w");

		if(!fdesc) break;

		printf("Config Manager: Begin testing connections for %s devices\n", unitlink_list[scen]);

		// test connection for different scenarios one by one
		scenfunc[scen](fdesc);

		printf("Config Manager: Testing connections for %s devices ended\n", unitlink_list[scen]);

		// set speed index to zero after each test
		speed_idx = 0;
	}

	// write lowlevel.cfg and ieclevel.icd files

	printf("Config Manager: Writing of lowlevel.cfg file started...\n");

	fdesc = fopen(LLEVEL_FILE, "w");

	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->connect)
		{
			fprintf(fdesc, "%s", lrs[i]->scfg);
		}
	}

	fclose(fdesc);

	printf("Config Manager: Writing of ieclevel.icd file started...\n");

	icd_full(ICD_FILE);

	printf("Config Manager: Creating of configuration files finished\n");

	mf_exit();

	return 0;
}
