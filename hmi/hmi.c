/*
 * hmi.c
 *
 *  Created on: 16.12.2011
 *      Author: dmitry
 */
#include <nano-X.h>
#include <nanowm.h>
#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "menu.h"
#include "hmi.h"

#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )
#define FGCOLOR BLACK
#define BGCOLOR	WHITE

LIST fldextinfo = {NULL, NULL};
ldextinfo *actldei = (ldextinfo *) &fldextinfo;
char *llfile;	// Buffer for lowlevel.cfg file

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

int lowlevel_parser(char *fllname){
char *p;
int i, len = 1;
FILE *fl;
char tbuf[128];

char words[][6] = {
		{"-addr"},
		{"-port"},
		{"-name"},
		{"-sync"}
};

// Loading main config file
	fl = fopen(fllname, "r");
	if (fl == NULL) return -1;

	while(len != EOF){
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
				for (i = 0; i < 4; i++){
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
	}

	return 0;

}

void SetNewMenu(char *arg){

}

void SetMainMenu(char *arg){

}

void SetKeydown(char *arg){

}

void HintDraw(char *arg){

}

void EnInfoDraw(char *arg){

}

//Array of struct
fact factsetting[] = {
		{"mainmenu", (void*) SetMainMenu},      //0
		{"newmenu", (void*) SetNewMenu},		//1
		{"hint", (void*) HintDraw},				//2
		{"energyinfo", (void*) EnInfoDraw},		//3
		{"doxpaint", NULL},						//4
		{"keydown", NULL},   		     		//5
		{"keyup", NULL},						//6
};

// Defvalues included: m700env, lowlevel.cfg
value defvalues[] = {
		{"APP:ip", NULL, NULL, 0, 0},
		{"APP:mac", NULL, NULL, 0, 0},
};

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
 						factsetting[4].func(&event);
 					}
 					break;

 				case GR_EVENT_TYPE_KEY_DOWN:
 					printf ("Key down event 0x%04X\n", event.keystroke.ch);
 					factsetting[5].func(&event);
 					break;

				case GR_EVENT_TYPE_KEY_UP:
 					factsetting[6].func(&event);
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
FILE *fl;

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

	// Loading environment, setup to defvalues
		vc_init(defvalues, sizeof(defvalues) / sizeof (value));

//---*** Init visual control ***---//
	init_menu(factsetting, sizeof(factsetting) / sizeof(fact));
	do_openfilemenu("menus/item", MENUFILE);
	draw_menu();
	mainloop();

	return 0;
}

