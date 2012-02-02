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
{"mainmenu", SetMainMenu},      //0
{"newmenu", SetNewMenu},		//1
{"hint", HintDraw},				//2
{"energyinfo", EnInfoDraw},		//3
{"doxpaint", NULL},				//4
{"keydown", NULL},        		//5
{"keyup", NULL},				//6
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
int main(int argc, char **argv)
{

//---*** Init IEC61850 ***---//
	// Parsing cid, create virtualization structures from common iec61850 configuration
	if (cid_build()){
		printf("IEC61850: cid file not found\n");
		exit(1);
	}
	// Cross connection of IEC structures
	crossconnection();

//---*** Init visual control ***---//
  init_menu(factsetting, sizeof(factsetting) / sizeof(fact));
  do_openfilemenu("menus/item", MENUFILE);
  draw_menu();
  mainloop();
  return 0;
}

