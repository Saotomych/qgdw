/*
 *  devlink.c
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

#include "devlink.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <errno.h>

// FIFO[0] - in, FIFO[1] - out
#define FIFOWR	1
#define FIFORD	0
// 1 file for connecting with main application
FILE *file_dev2main[2];
volatile int f_dev2main[2];

// Global config structure
struct config_device devconf_test = {
	(char*) ("m300"),
	(char*) ("iec101"),
	(char*) ("ttyS1-fifo"),
	10,
	12,
};


// Test definitions
struct iec_data_type idt_test = {
	(char*)("test name"),
};

int iec_desc_test[] = {5,4,3,2,1,-1};

struct data_type dattyp_test = {
	NULL,
	&idt_test,
	iec_desc_test,
	0,
	0,
	1,
	2,
};

struct work_data_type wdt_test = {
	NULL,
	0,
	3,
	4,
};

int dummy_proloque (char *buf, int len, char *newbuf){
	return 0;
}

int dummy_epiloque (struct data_type *dt){
	return 0;
}

// Structures for connecting prologue and epiloque to parser
#define SUPPORT_PARSERS		3
static const struct prolepi ppe[] = {
		{"iec101", dummy_proloque, dummy_epiloque},
		{"iec104", dummy_proloque, dummy_epiloque},
		{"dlt645", dummy_proloque, dummy_epiloque},
};

// End test definitions

struct data_type *dtypes[]={&dattyp_test};
struct work_data_type *wdtypes[]={&wdt_test};

u08	devs = 0;		// Актуальное число устройств
u08 devact = 0;		// Текущее устройство для опроса и инициализации
u08 devactrd = 0;		//	Текущее устройство для чтения из парсера приходящих структур
u08 devactrw = 0;		// Текущее устройство для записи в парсер уходящих команд
struct device_link *devlink[10];
//                            struct device_link *devlink[] = {devlink_test};

char prepath[] = {"/rw/mx00"};
char pathparser[] = {"parsers"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char mainlink[] = {"main"};
char initdev[] = {"initdev"};
char parslink[] = {"parser"};

struct hnd_parser *first_reg_prs = 0;
char filename[160];


int main(int argc, char * argv[]){

// Config structure
struct config_device *devconf = &devconf_test;

// Init base structures
struct hnd_parser *prs;
char *mainfilename[2];

char buf[20];

int cnt;
u32 ret;
ssize_t rdlen, wrlen;


	return 0;
}
