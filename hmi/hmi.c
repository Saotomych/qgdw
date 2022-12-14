/*
 * hmi.c
 *
 *  Created on: 16.12.2011
 *      Author: dmitry & Alex AVAlon
 */

#include <linux/input.h>
#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "../common/tarif.h"
#include "../common/ts_print.h"
#include "../common/paths.h"
#include "hmi.h"
#include "menu.h"


#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )
#define FGCOLOR BLACK
#define BGCOLOR	WHITE

LIST fldextinfo = {NULL, NULL};
static ldextinfo *actldei = (ldextinfo *) &fldextinfo;

int fkeyb = 0;

static volatile uint32_t MFMessage = 0;

// Synonyms for global variables
// Defvalues included: m700env, about.me
value defvalues[] = {
		{0, "APP:LOCALIP", NULL, NULL, STRING, 0},
		{1, "APP:MAC", NULL, NULL, STRING, 0},
		{2, "APP:Type", NULL, NULL, STRING, 0},
		{3, "APP:UPDATE", NULL, NULL, STRING, 0},
		{4, "APP:UBI", NULL, NULL, STRING, 0},
		{5, "APP:SFTP", NULL, NULL, STRING, 0},
		{6, "APP:CRONSW", NULL, NULL, STRING, 0},
		{7, "APP:CRONCF", NULL, NULL, STRING, 0},
		{8, "APP:ManDate", NULL, NULL, STRING, 0},
		{9, "APP:Tester", NULL, NULL, STRING, 0},
		{10, "APP:SerNum", NULL, NULL, STRING, 0},
		// Time variables
		{11, "APP:year", NULL, NULL, 0, 0},
		{12, "APP:montext", NULL, NULL, 0, 0},
		{13, "APP:mondig", NULL, NULL, 0, 0},
		{14, "APP:day", NULL, NULL, 0, 0},
		{15, "APP:wday", NULL, NULL, 0, 0},
		{16, "APP:hour", NULL, NULL, 0, 0},
		{17, "APP:min", NULL, NULL, 0, 0},
		{18, "APP:sec", NULL, NULL, 0, 0},
		{19, "APP:jyear", NULL, NULL, 0, 0},
		{20, "APP:jmontext", NULL, NULL, 0, 0},
		{21, "APP:jmondig", NULL, NULL, 0, 0},
		{22, "APP:jday", NULL, NULL, 0, 0},
		{23, "APP:jwday", NULL, NULL, 0, 0},
		{24, "APP:jhour", NULL, NULL, 0, 0},
		{25, "APP:jmin", NULL, NULL, 0, 0},
		{26, "APP:jsec", NULL, NULL, 0, 0},
		// IEC variables
		{27, "APP:lnclasstext", NULL, NULL, 0 ,0},
		// Filter variables
		{28, "APP:interval", NULL, NULL, 0 ,0},
		// Tarif variables
		{29, "APP:tarifid", NULL, "-", 0, STRING},
		{30, "APP:tarifname", NULL, "???? ????????????", 0, STRING},
		// String variables
		{31, "APP:devicetype", NULL, NULL, 0, 0},
		{32, "APP:deviceaddr", NULL, NULL, 0, 0},
		{33, "APP:deviceport", NULL, NULL, 0, 0},
		{34, "APP:devicestat", NULL, NULL, 0, 0},
		{35, "APP:devicecode", NULL, NULL, 0, 0},
		{0, NULL, NULL, NULL, 0, 0},
};

extern LIST* create_next_struct_in_list(LIST *plist, int size);

// ----------------------------------------------------------------------------------
// Parser of file /tmp/about/about.me
int about_parser(char *faname){
FILE *fl;
char *p, *papp;
char tbuf[200];
int i;

// Loading main config file
	fl = fopen(faname, "r");
	if (fl == NULL) return -1;

	do{
		p = fgets(tbuf, 128, fl);
		if (p == tbuf){
			while(*p != '=') p++;
			*p = 0; p++;
			for (i=0; i < (sizeof(defvalues)/sizeof(value)); i++){
				if (defvalues[i].name){
					papp = defvalues[i].name + 4;
					if (!strcmp(papp, tbuf)) {
						defvalues[i].val = malloc(strlen(p));
						p[strlen(p)-1] = 0;
						strcpy(defvalues[i].val, p);
						defvalues[i].idtype = INTERNAL | TRUEVALUE | STRING;
					}
				}
			}
			p = tbuf;
		}
	}while (p == tbuf);

	return 0;
}

// ----------------------------------------------------------------------------------
// Parser of file /tmp/about/setenv.sh
void env_parser(void){
char words[][16] = {
		{"LOCALIP"},
		{"MAC"},
		{"Type"},
		{"UPDATE"},
		{"UBI"},
		{"SFTP"},
		{"CRONSW"},
		{"CRONCF"},
		{"\0"},
};

int i, j;
char *papp;

	for (i=0; words[i][0] != 0; i++){
		for(j=0; j < (sizeof(defvalues)/sizeof(value)); j++){
			papp = defvalues[j].name;
			if (strstr(papp, &words[i][0])){
				defvalues[j].val = getenv(&words[i][0]);
				defvalues[j].idtype = INTERNAL | TRUEVALUE | STRING;
				break;
			}
		}
	}
}

// ----------------------------------------------------------------------------------
// Parser of file lowlevel.cfg
int lowlevel_parser(char *fllname){
char *p;
int i, len = 1;
FILE *fl;
char tbuf[128];

char words[][6] = {
		{"-addr"},
		{"-port"},
		{"-name"},
		{"-sync"},
		{"\0"},
};

// Loading main config file
	fl = fopen(fllname, "r");
	if (fl == NULL) return -1;

	do{	p = fgets(tbuf, 128, fl);
		if ((p == tbuf) && (*p >= '0') && (*p <= '9')){
			// if find new line
			actldei = (ldextinfo*) create_next_struct_in_list((LIST *) &actldei->l, sizeof(ldextinfo));
			actldei->addr = NULL;
			actldei->nameul = NULL;
			actldei->llstring = NULL;
			actldei->portmode = NULL;
			actldei->sync = NULL;
			actldei->asduadr = atoi(tbuf);
			if (actldei->asduadr){
				// ASDU addr is here
				// Create place for full config string
				len = 0;
				while (tbuf[len] >= ' ') len++;
				tbuf[len] = 0;
				actldei->llstring = malloc(len+1);
				strcpy(actldei->llstring, tbuf);

				// Find words in this line
				p = actldei->llstring;
				for (i = 0; words[0][i] != 0; i++){
					p = strstr(actldei->llstring, &words[i][0]);
					if (p){
						p += 6;
						switch(i){
						case 0: actldei->addr = p; break;
						case 1: actldei->portmode = p; break;
						case 2: actldei->nameul = p; break;
						case 3: actldei->sync = p; break;
						}
					}
				}

				// Change all spaces onto 0
				p = actldei->llstring;
				while (*p >= ' '){
					if (*p == ' ') *p = 0;
					p++;
				}

			}
		}
	}while(!feof (fl));

	return 0;

}

void main_switch(GR_EVENT *event){

	switch (event->type) {

	case GR_EVENT_TYPE_EXPOSURE:
		if (event->exposure.wid == GR_ROOT_WINDOW_ID){
			ts_printf(STDOUT_FILENO, TS_DEBUG, "Root exposure event 0x%04X\n", event->exposure.wid);
			redraw_screen(&event);
		}
		break;

	case GR_EVENT_TYPE_KEY_DOWN:
		key_pressed(event);
			break;

	case GR_EVENT_TYPE_KEY_UP:
		key_rised(event);
		break;

	case GR_EVENT_TYPE_UPDATE:
		ts_printf(STDOUT_FILENO, TS_DEBUG, "Window event update\n");
		break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			exit(0);
		}
}

uint32_t setvarbyevent(varevent *ave){
varrec *avr;
char *pstr;
value *actval;

//	ts_printf(STDOUT_FILENO, "HMI: varrec address = 0x%X\n", ave->uid);
//	ts_printf(STDOUT_FILENO, "HMI: varrec value index = %d\n", ave->validx);

	if (ave){

		avr = (varrec*) ave->uid;

		if (ave->uid != avr->uid) return 0;

		avr->prop |= TRUEVALUE;

		actval = &avr->val[ave->validx];
		switch(actval->idtype){
		case QUALITY:
		case INT32:
			*((int32_t*) (actval->val)) = ave->value.i;
			ts_printf(STDOUT_FILENO, TS_DEBUG, "HMI: new value %s(0x%X) = %d\n", avr->name->fc, ave->uid, ave->value.i);
			break;

		case FLOAT32:
			*((float*) (actval->val)) = ave->value.f;
			ts_printf(STDOUT_FILENO, TS_DEBUG, "HMI: new value %s(0x%X) = %.2F\n", avr->name->fc, ave->uid, ave->value.f);
			break;

		case TIMESTAMP:
			*((time_t*) (actval->val)) = (time_t) ave->value.i;
			break;

		case STRING:
			pstr = (char*)((uint32_t) ave + ave->vallen);
			strncpy((char*) (actval->val), pstr, ave->vallen);
//			ts_printf(STDOUT_FILENO, "HMI!!!: get string value %s as %s\n", (char*) (ave->value.i), avr->name->fc);
			break;
		}

		if (avr->ptime){
			avr->ptime[ave->validx] = ave->time;
		}
		else avr->time = ave->time;

	}

	return 0;
}


varevent *get_nextvarevent(ep_data_header *edh, varevent *ave){
int32_t len = (uint32_t) ave - (uint32_t) edh - edh->len - sizeof(ep_data_header) + sizeof(varevent);

	ave++;
	if (len < 0) return ave;

	return 0;
}


// -- Multififo receive data --
int rcvdata(int len){
char *buff;
int  adr, dir;
uint32_t fullrdlen;
uint32_t offset;
ep_data_header *edh;
varevent *ave;

	buff = malloc(len);
	if(!buff) return -1;

	fullrdlen = mf_readbuffer(buff, len, &adr, &dir);

	ts_printf(STDOUT_FILENO, TS_DEBUG, "HMI: Data received. Address = %d, Length = %d %s.\n", adr, fullrdlen, dir == DIRDN? "from down" : "from up");

	// set offset to zero before loop
	offset = 0;

	while(offset < fullrdlen){
		if ((fullrdlen - offset) < sizeof(ep_data_header)){
			ts_printf(STDOUT_FILENO, TS_VERBOSE, "HMI: Found not full ep_data_header\n");
			free(buff);
			return 0;
		}

		edh = (ep_data_header *) (buff + offset);				// set start structure

		// Incoming data will be working
		switch(edh->sys_msg){

		case EP_MSG_VAREVENT:
			ave = (varevent*)((char*) edh + sizeof(ep_data_header));

			do{
				setvarbyevent(ave);
				ave = get_nextvarevent(edh, ave);
			}while(ave);

			MFMessage = GR_EVENT_TYPE_EXPOSURE;		// Event for screen refresh

			break;
		}

		// move over the data
		offset += sizeof(ep_data_header) + edh->len;
	}

	free(buff);

	return 0;
}


// ----------------------------------------------------------------------------------
void mainloop()
{
GR_EVENT event;
//GR_WM_PROPERTIES props;
struct input_event ev[16];
size_t evlen;

	memset(&event, 0, sizeof(GR_EVENT));

	while (1) {
 		wm_handle_event(&event);
 		GrGetNextEventTimeout(&event, 100L);

 		if (fkeyb != -1){
 			evlen = read(fkeyb, ev, sizeof(ev)) / sizeof(struct input_event);
 			if (evlen != 0xFFFFFFF){
				if (ev[0].value){
					printf("KEY: %X, %X, %X\n", ev[0].value, ev[0].type, ev[0].code);
					event.type = GR_EVENT_TYPE_KEY_DOWN;
					event.keystroke.ch = ev[0].code;
					main_switch(&event);
				}
 			}else main_switch(&event);
 		}else main_switch(&event);

 		// Check my events
 		if (MFMessage){
			event.type = MFMessage;
			event.exposure.wid = GR_ROOT_WINDOW_ID;
			MFMessage = 0;
			main_switch(&event);
 		}
	}

 	GrClose();
 }

//---------------------------------------------------------------------------------
static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

int main(int argc, char **argv)
{
int i;
char *fname;
pid_t chldpid;

	init_allpaths();
	mf_semadelete(getpath2fifomain(), "hmi700");

	//---*** Init IEC61850 ***---//
	// Make config name
	i = strlen(getpath2configs()) + strlen(IECCONFIG) + 3;
	fname = malloc(i);
	sprintf(fname, "%s%s", getpath2configs(), IECCONFIG);

	// Parsing cid, create virtualization structures from common iec61850 configuration
	if (cid_build(fname)){
		ts_printf(STDOUT_FILENO, TS_INFO, "IEC61850: cid file not found\n");
	}else{
		// Cross connection of IEC structures
		crossconnection();
	}
	free(fname);

	//---*** Init var controller ***---//
	// Parse config files: addr.cfg, lowlevel.cfg, m700env, ?????????????? icd ?? cid

	// Make fullname for lowlevel.cfg
	i = strlen(getpath2configs()) + strlen("lowlevel.cfg") + 3;
	fname = malloc(i);
	sprintf(fname, "%s%s", getpath2configs(), "lowlevel.cfg");
	// Parse lowlevel config file
	if (lowlevel_parser(fname)) ts_printf(STDOUT_FILENO, TS_INFO, "IEC61850: lowlevel file reading error\n");
	free(fname);

	// Parse Tariffes
	i = strlen(getpath2configs()) + strlen("tarif.xml") + 3;
	fname = malloc(i);
	sprintf(fname, "%s%s", getpath2configs(), "tarif.xml");
	// Parse lowlevel config file
	if (tarif_parser(fname)) ts_printf(STDOUT_FILENO, TS_INFO, "Tarif: configuration file reading error\n");
	free(fname);

	// Setup environment variables from device environment
	env_parser();

	// Parse about config file
	i = strlen(getpath2about()) + strlen("about.me") + 3;
	fname = malloc(i);
	sprintf(fname, "%s%s", getpath2about(), "about.me");
	if (about_parser(fname)) ts_printf(STDOUT_FILENO, TS_INFO, "HMI: about.me file reading error\n");
	free(fname);

	// Parse vars from menu.c
	menu_parser(defvalues, sizeof(defvalues) / sizeof (value));

	// Initialize defvalues indexer
	for(i=0; i < (sizeof(defvalues)/sizeof(value) - 1); i++) defvalues[i].idx = i;

	// Register all variables in varcontroller
	vc_init();

	fkeyb = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);

	// Multififo init
	chldpid = mf_init(getpath2fifomain(), "hmi700", rcvdata);
	// Set endpoint for datasets
	mf_newendpoint(IDHMI, "startiec", getpath2fifomain(), 0);

	//---*** Init visual control ***---//
	if (init_menu()){
		ts_printf(STDOUT_FILENO, TS_INFO, "Configuration of LNODEs not found\n");
		exit(1);
	}
	mainloop();

	return 0;
}
