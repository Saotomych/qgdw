/*
 * hmi.c
 *
 *  Created on: 16.12.2011
 *      Author: dmitry & Alex AVAlon
 */

#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "hmi.h"
#include "menu.h"

#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )
#define FGCOLOR BLACK
#define BGCOLOR	WHITE

static LIST fldextinfo = {NULL, NULL};
static ldextinfo *actldei = (ldextinfo *) &fldextinfo;

// Synonyms for global variables
// Defvalues included: m700env, about.me
static value defvalues[] = {
		{"APP:LOCALIP", NULL, NULL, STRING, 0},
		{"APP:MAC", NULL, NULL, STRING, 0},
		{"APP:Type", NULL, NULL, STRING, 0},
		{"APP:UPDATE", NULL, NULL, STRING, 0},
		{"APP:UBI", NULL, NULL, STRING, 0},
		{"APP:SFTP", NULL, NULL, STRING, 0},
		{"APP:CRONSW", NULL, NULL, STRING, 0},
		{"APP:CRONCF", NULL, NULL, STRING, 0},
		{"APP:ManDate", NULL, NULL, STRING, 0},
		{"APP:Tester", NULL, NULL, STRING, 0},
		{"APP:SerNum", NULL, NULL, STRING, 0},
		{"APP:ldtypetext", NULL, NULL, 0 ,0},
};

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *newlist;
	plist->next = malloc(size);
	if (!plist->next){
		printf("IEC61850: malloc error:%d - %s\n",errno, strerror(errno));
		exit(3);
	}

	newlist = plist->next;
	newlist->prev = plist;
	newlist->next = 0;
	return newlist;
}

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
				papp = defvalues[i].name + 4;
				if (!strcmp(papp, tbuf)) {
					defvalues[i].val = malloc(strlen(p));
					p[strlen(p)-1] = 0;
					strcpy(defvalues[i].val, p);
					defvalues[i].idtype = INTVAR | TRUEVALUE | STRING;
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
				defvalues[j].idtype = INTVAR | TRUEVALUE | STRING;
				break;
			}
		}
	}
}

// ----------------------------------------------------------------------------------
// Parser of file /rw/mx00/configs/lowlevel.cfg
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

	while(1){
		p = fgets(tbuf, 128, fl);
		if ((p == tbuf) && (*p >= '0')){
			// if find new line
			actldei = create_next_struct_in_list((LIST *) &actldei->l, sizeof(ldextinfo));
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
		}else break;
	}

	return 0;

}

// ----------------------------------------------------------------------------------
void mainloop()
{
	GR_EVENT event;
	//GR_WM_PROPERTIES props;

	while (1) {
 		wm_handle_event(&event);
 		GrGetNextEvent(&event);
 				switch (event.type) {

 				case GR_EVENT_TYPE_EXPOSURE:
 					if (event.exposure.wid == GR_ROOT_WINDOW_ID){
 	 					printf ("Root exposure event 0x%04X\n", event.exposure.wid);
 	 					redraw_screen(&event);
 					}
 					break;

 				case GR_EVENT_TYPE_KEY_DOWN:
 					key_pressed(&event);
 					break;

				case GR_EVENT_TYPE_KEY_UP:
					key_rised(&event);
					break;

				case GR_EVENT_TYPE_UPDATE:
					printf("Window event update\n");
					break;

 				case GR_EVENT_TYPE_CLOSE_REQ:
 					GrClose();
 					exit(0);
 				}
 		}

 	GrClose();
 }

//---------------------------------------------------------------------------------
char prepath[] = {"/rw/mx00"};
char pathul[] = {"unitlinks"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char pathconfig[] = {"configs"};
char mainlink[] = {"main"};
struct stat fst;

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

int main(int argc, char **argv)
{
int i;
char *fname;

	//---*** Init IEC61850 ***---//

	// Make config name
	i = strlen(prepath) + strlen(pathconfig) + strlen(IECCONFIG) + 3;
	fname = malloc(i);
	sprintf(fname, "%s/%s/%s", prepath, pathconfig, IECCONFIG);

	// Parsing cid, create virtualization structures from common iec61850 configuration
	if (cid_build(fname)){
		printf("IEC61850: cid file not found\n");
	}else{
		// Cross connection of IEC structures
		crossconnection();
	}
	free(fname);

	//---*** Init var controller ***---//
	// Parse config files: addr.cfg, lowlevel.cfg, m700env, наличие icd и cid

	// Make fullname for lowlevel.cfg
	i = strlen(prepath) + strlen(pathconfig) + strlen("lowlevel.cfg") + 3;
	fname = malloc(i);
	sprintf(fname, "%s/%s/%s", prepath, pathconfig, "lowlevel.cfg");
	// Parse lowlevel config file
	if (lowlevel_parser(fname)) printf("IEC61850: lowlevel file reading error\n");
	free(fname);

	// Setup environment
	env_parser();

	// Parse about config file
	if (about_parser("/tmp/about/about.me")) printf("IEC61850: about.me file reading error\n");

	// Parse vars from menu.c
	menu_parser(defvalues, sizeof(defvalues) / sizeof (value));

	for(i=0; i < (sizeof(defvalues)/sizeof(value)); i++)
		printf("%02d: %s=%s\n", i, defvalues[i].name, (char*) defvalues[i].val);

	// Register all variables in varcontroller
	vc_init(defvalues, sizeof(defvalues) / sizeof (value));

//---*** Init visual control ***---//
	if (init_menu()){
		printf("Configuration of LNODEs nor found\n");
		exit(1);
	}
	do_openfilemenu("menus/item", MENUFILE);
	draw_menu();
	mainloop();

	return 0;
}

