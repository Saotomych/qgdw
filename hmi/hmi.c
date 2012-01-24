/*
 * hmi.c
 *
 *  Created on: 16.12.2011
 *      Author: dmitry
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"
#include "nanowm.h"
#include <string.h>
#include <signal.h>
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

void event_menu()
{
	GR_EVENT event;
	//GR_WM_PROPERTIES props;

	while (1) {
 		//GR_EVENT event;
 		//GrGetNextEvent(&event);
 		//do_paint();

 		wm_handle_event(&event);
 		GrGetNextEvent(&event);
 				switch (event.type) {

 				case GR_EVENT_TYPE_EXPOSURE:
 					factsetting[4].func(&event);
 					break;

 				case GR_EVENT_TYPE_KEY_DOWN:
 					factsetting[5].func(&event);
 					break;

				case GR_EVENT_TYPE_KEY_UP:
 					factsetting[6].func(&event);
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
  init_menu(factsetting, sizeof(factsetting) / sizeof(fact));
  do_openfilemenu();
  draw_menu();
  event_menu();
  return 0;
}

