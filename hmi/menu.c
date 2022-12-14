/*
 * hmi.c
 *
 *  Created on: 16.12.2011
 *      Author: dmitry & Alex AVAlon
 */

#include <nano-X.h>
#include <nanowm.h>
#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "../common/tarif.h"
#include "../common/ts_print.h"
#include "../common/paths.h"
#include "menu.h"

extern struct _lntxt{
	char ln[5];
	char lntext[50];
} lntxts[];

LNODE *actlnode;			// Global variable, change in menu only
tarif *acttarif;			// Global variable, change in menu only

time_t maintime;		// Actual Time
time_t jourtime;		// Time for journal setting
time_t tmptime;			// Time for setting process
int idlnmenuname = 0;
int intervals[] = {1,2,3,5,10,15,20,30,60};
int *pinterval = intervals;		// Time Interval for view journal records
int *ptarifid = NULL;
char *ptarifname = NULL;
char *pdevtype = NULL;
char *pdevaddr = NULL;
char *pdevport = NULL;
char *pdevstat = NULL;
char *pdevcode = NULL;

static struct tm mtime_tm;	// Time for convert of time_l
static struct tm jtime_tm;	// Time for convert of time_l

static char *pmwday;				// Text for weekday, actual
static char *pjwday;				// Text for weekday, journal
static char *pmmon;				// Text for month, actual
static char *pjmon;				// Text for month, journal
static int myear;				// Year actual
static int jyear;				// Year journal
static int m_mon;				// Month as digit = tm_mon + 1, actual
static int j_mon;				// Month as digit, journal

//static char prev_item;		// pointer to item in main menu
static menu *num_menu;     //указатель на структуру меню

static char tmpstring[100];			// For temporary operations
static char *devtypetext;		// Text of device type

static char lnmenunames[10][64];

static value menuvalues[] = {
		// Time variables
		{0, "APP:year", 	&myear, 			"XXXX", 	INT32, 		STRING},
		{0, "APP:montext", &pmmon, 				"XX", 		PTRSTRING, 	STRING},	// Month as full text
		{0, "APP:mondig", 	&m_mon,				"XX", 		INT32DIG2, 	STRING},	// Month as digit with zero
		{0, "APP:day",  	&(mtime_tm.tm_mday),"XX", 		INT32DIG2, 	STRING},	// Day as digit with zero
		{0, "APP:wday", 	&pmwday, 			"XX", 		PTRSTRING, 	STRING},	// Day of week as text (2 symbols)
		{0, "APP:hour", 	&(mtime_tm.tm_hour),"XX", 		INT32DIG2, 	STRING},
		{0, "APP:min", 		&(mtime_tm.tm_min), "XX", 		INT32DIG2, 	STRING},
		{0, "APP:sec", 		&(mtime_tm.tm_sec), "XX", 		INT32DIG2, 	STRING},
		{0, "APP:jyear", 	&jyear, 			"XXXX", 	INT32, 		STRING},
		{0, "APP:jmontext",	&pjmon, 			"XX", 		PTRSTRING, 	STRING},
		{0, "APP:jmondig", 	&j_mon, 			"XX", 		INT32DIG2, 	STRING},	// Month as digit with zero
		{0, "APP:jday", 	&(jtime_tm.tm_mday),"XX", 		INT32DIG2, 	STRING},
		{0, "APP:jwday", 	&pjwday, 			"XX", 		PTRSTRING, 	STRING},	// Day of week as text
		{0, "APP:jhour", 	&(jtime_tm.tm_hour),"XX", 		INT32DIG2,	STRING},
		{0, "APP:jmin", 	&(jtime_tm.tm_min), "XX", 		INT32DIG2,	STRING},
		{0, "APP:jsec", 	&(jtime_tm.tm_sec), "XX", 		INT32DIG2,	STRING},
		// IEC variables
		{0, "APP:lnclasstext", &devtypetext,  	"Тип не выбран", PTRSTRING , STRING},
		{0, "APP:interval", &pinterval, "нет", PTRINT32, STRING},
		// Tarif variables
		{0, "APP:tarifid", &ptarifid, "-", PTRINT32, STRING},
		{0, "APP:tarifname", &ptarifname, "все тарифы", PTRSTRING, STRING},
		// String variables
		{0, "APP:devicetype", &pdevtype, "SCADA", PTRSTRING, STRING},
		{0, "APP:deviceaddr", &pdevaddr, "не найден", PTRSTRING, STRING},
		{0, "APP:deviceport", &pdevport, "не найден", PTRSTRING, STRING},
		{0, "APP:devicestat", &pdevstat, "оффлайн", PTRSTRING, STRING},
		{0, "APP:devicecode", &pdevcode, "нет", PTRSTRING, STRING},
};

//------------------------------------------------------------------------------------
// Function create new text in all windows
static char wintext[40];
static void do_paint(item *pitem, int fg, int bg){
GR_GC_ID gc;
GR_WINDOW_ID *main_window = &(pitem->main_window);
GR_WINDOW_INFO winfo;
int len = sizeof(wintext) - 1;
int idtype;
int i;
struct tm *ttm;

			GrGetWindowInfo(*main_window, &winfo);

			gc = GrNewGC();                                   //Allocate a new graphic context
			GrSetGCUseBackground(gc, GR_FALSE);
			GrSetGCBackground(gc, fg);
			GrSetGCForeground(gc, bg);
			GrFillRect(*main_window, gc, 0, 0, winfo.width, winfo.height);
			GrSetGCForeground(gc, fg);
			GrSetGCFont(gc, num_menu->font);

			if ((pitem->vr) && (pitem->vr->prop & TRUEVALUE)){

				wintext[0] = 0;
				wintext[1] = 0;

				for(i=0; i < pitem->vr->maxval; i++){

					if (!pitem->printtype){

						// PRINT VALUE

						idtype = pitem->vr->val[i].idtype;

						if (idtype & FLOAT32){
								ts_sprintf(wintext, "%s%9.2f%s", pitem->text, *((float*) (pitem->vr->val[i].val)), pitem->endtext);
						}

						if (idtype & INT32){
							ts_sprintf(wintext, "%s%d%s", pitem->text, *((int*) (pitem->vr->val[i].val)), pitem->endtext);
						}

						if (idtype & PTRINT32){
							if (*((int*) (pitem->vr->val[i].val))){
								ts_sprintf(wintext, "%s%d%s", pitem->text, *((int*)*((int*) (pitem->vr->val[i].val))), pitem->endtext);
							}else ts_sprintf(wintext, "%s%s%s", pitem->text, (char*) pitem->vr->val[i].defval, pitem->endtext);
						}

						if (idtype & INT32DIG2){
							ts_sprintf(wintext, "%s%02d%s", pitem->text, *((int*) (pitem->vr->val[i].val)), pitem->endtext);
						}

						if (idtype & INT64){
							ts_sprintf(wintext, "%s%ld%s", pitem->text, *((long*) (pitem->vr->val[i].val)), pitem->endtext);
						}

						if (idtype & STRING){
							strncpy(wintext, pitem->text, sizeof(wintext) - 1);
							len -= strlen(wintext);
							if ((pitem->vr) && (&pitem->vr->val[i])){
								if (pitem->vr->val[i].val) strncat(wintext, (char*) pitem->vr->val[i].val, len);
								else if (pitem->vr->val[i].defval) strncat(wintext, (char*) pitem->vr->val[i].defval, len);
							}
							len = sizeof(wintext) - 1 - strlen(wintext);
							if ((pitem->endtext) && (len > 0)) strncat(wintext, pitem->endtext, len);
							wintext[sizeof(wintext) - 1] = 0;
						}

						if (idtype & PTRSTRING){
							strncpy(wintext, pitem->text, sizeof(wintext) - 1);
							len -= strlen(wintext);
							if ((pitem->vr) && (&pitem->vr->val[i])){
								if ((pitem->vr->val[i].val) && (*((int*) pitem->vr->val[i].val)))
									strncat(wintext, (char*) *((int*) pitem->vr->val[i].val), len);
								else if (pitem->vr->val[i].defval) strncat(wintext, (char*) pitem->vr->val[i].defval, len);
							}
							len = sizeof(wintext) - 1 - strlen(wintext);
							if ((pitem->endtext) && (len > 0)) strncat(wintext, pitem->endtext, len);
							wintext[sizeof(wintext) - 1] = 0;
						}

					}else{

						// PRINT TIME

						if (pitem->vr->ptime){
							if ((int)pitem->vr->ptime[i] != -1){
								ttm = localtime(&(pitem->vr->ptime[i]));
								ts_sprintf(wintext, "%s%02d:%02d%s", pitem->text, ttm->tm_hour, ttm->tm_min, pitem->endtext);
							}else{
								ts_sprintf(wintext, "-----", pitem->text, ttm->tm_hour, ttm->tm_min, pitem->endtext);
							}
						}else{
							ttm = localtime(&(pitem->vr->time));
							ts_sprintf(wintext, "%s%02d:%02d%s", pitem->text, ttm->tm_hour, ttm->tm_min, pitem->endtext);
						}
					}

					GrText(*main_window, gc, 2, MENUSTEP * i, wintext, strlen(wintext), GR_TFUTF8|GR_TFTOP);
				}
			}else GrText(*main_window, gc, 2, 0, pitem->text, strlen(pitem->text), GR_TFUTF8|GR_TFTOP);

			GrDestroyGC(gc);
}
//------------------------------------------------------------------------------------
// Create subwindows for menu items and map all window
static void draw_menu()
{
		if (!num_menu) return;

		 if (GrOpen() < 0) {
			 fprintf(stderr, "Cannot open graphics\n");
			 exit(1);
			}

			wm_init();
			//Paint on the screen window main_window

			GR_WINDOW_ID *main_window;
			GR_WINDOW_INFO winfo;
			GR_WM_PROPERTIES props;

			num_menu->main_window = GrNewWindow(GR_ROOT_WINDOW_ID,
												num_menu->rect.x,
												num_menu->rect.y,
												num_menu->rect.width,
												num_menu->rect.height,
												1, WHITE, BLACK);

   		    GrMapWindow(num_menu->main_window);

			props.flags = GR_WM_FLAGS_PROPS;
			props.props = GR_WM_PROPS_BORDER |
					   GR_WM_PROPS_CAPTION;
			props.title = "";

			GrSetWMProperties(num_menu->main_window, &props);

			GrGetWindowInfo(GR_ROOT_WINDOW_ID, &winfo);
			GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN);
			GrSelectEvents(num_menu->main_window, winfo.eventmask | GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_KEY_DOWN);

			//Paint on the screen windows of item
			int i;

			for (i = 0; i < num_menu->start_item; i++)
			{
				main_window = &(num_menu->pitems[i]->main_window);

				*main_window = GrNewWindow(num_menu->main_window,
										   num_menu->pitems[i]->rect.x,
										   num_menu->pitems[i]->rect.y,
										   num_menu->pitems[i]->rect.width,
										   num_menu->pitems[i]->rect.height,
										   0, WHITE, WHITE);

				props.flags = GR_WM_FLAGS_PROPS;
				props.props = GR_WM_PROPS_BORDER;

				GrSetWMProperties(*main_window, &props);
				GrSelectEvents(*main_window, GR_EVENT_MASK_EXPOSURE);
				GrMapWindow(*main_window);      //Make a window visible on the screen
	        }

			for (i = num_menu->start_item; i < num_menu->count_item; i++)
			{
				main_window = &(num_menu->pitems[i]->main_window);

				*main_window = GrNewWindow(num_menu->main_window,
										   num_menu->pitems[i]->rect.x,
										   num_menu->pitems[i]->rect.y,
										   num_menu->pitems[i]->rect.width,
										   num_menu->pitems[i]->rect.height,
										   0, WHITE, WHITE);

				props.flags = 0;
				props.props = 0;

				GrSetWMProperties(*main_window, &props);
				GrSelectEvents(*main_window, 0);
				GrMapWindow(*main_window);      //Make a window visible on the screen
	        }

			num_menu->font = GrCreateFontEx(num_menu->fontname, FONT_HEIGHT, FONT_WIDTH, 0);
			GrSetFontAttr(num_menu->font, GR_TFANTIALIAS | GR_TFKERNING, 0);

			redraw_screen(NULL);
}

//-------------------------------------------------------------------------------------------
// Draw only visible items and move items
static void redraw_menu(int type){
int i;
int stepy, y, itemy, itemh;

	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->ctrl_height;

	stepy = itemy + itemh - num_menu->rect.height;
	if (!type){
		if (num_menu->num_item == num_menu->first_item) stepy = itemy - num_menu->bgnmenuy;
	}else{
		if (num_menu->num_item < num_menu->pitems[num_menu->num_item]->next_item) stepy = itemy - num_menu->bgnmenuy;
	}

	for (i = num_menu->start_item; i < num_menu->count_item; i++){
		num_menu->pitems[i]->rect.y -= stepy;
		y = num_menu->pitems[i]->rect.y;
		if ((y >= num_menu->bgnmenuy - 10) && (y < num_menu->rect.height)){
			GrMoveWindow(num_menu->pitems[i]->main_window,
					 num_menu->pitems[i]->rect.x,
					 num_menu->pitems[i]->rect.y);
			GrMapWindow(num_menu->pitems[i]->main_window);
		}else GrUnmapWindow(num_menu->pitems[i]->main_window);
	}

	for (i=0; i < num_menu->start_item; i++) GrRaiseWindow(num_menu->pitems[i]->main_window);
}

//-------------------------------------------------------------------------------
// Keys functions

void default_left(GR_EVENT *event){
int ret, num_item;
struct tm *ttm;

	if (num_menu->pitems[num_menu->num_item]->action){
		ret = call_action(event->keystroke.ch, num_menu);
		num_item = num_menu->num_item;

		if (ret == REMAKEMENU){
			num_menu = destroy_menu(num_menu, DIR_SIDEBKW);
			if (!num_menu){
				num_menu = create_menu(lnmenunames[idlnmenuname]);
				call_action(NODIRECT, num_menu);		// Refresh variables
			}
			if (num_menu){
				num_menu->num_item = num_item;
				draw_menu();
			}
		}

		if (ret == REDRAWTIMEJOUR){
			ttm	= localtime(&jourtime);
			memcpy(&jtime_tm, ttm, sizeof(struct tm));
			jyear = 1900 + jtime_tm.tm_year;
			j_mon = jtime_tm.tm_mon + 1;

			vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

			redraw_screen(NULL);
		}

		if (ret == REDRAWTIMEMAIN){
			ttm	= localtime(&maintime);
			memcpy(&mtime_tm, ttm, sizeof(struct tm));
			myear = 1900 + mtime_tm.tm_year;
			m_mon = mtime_tm.tm_mon + 1;
			redraw_screen(NULL);
		}

		if (ret == REDRAW){

			vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

			redraw_screen(NULL);
		}
	}
}

void default_right(GR_EVENT *event){
int ret, num_item;
struct tm *ttm;

	if (num_menu->pitems[num_menu->num_item]->action){
		ret = call_action(event->keystroke.ch, num_menu);
		num_item = num_menu->num_item;

		if (ret == REMAKEMENU){
			num_menu = destroy_menu(num_menu, DIR_SIDEBKW);
			if (!num_menu){
				num_menu = create_menu(lnmenunames[idlnmenuname]);
				call_action(NODIRECT, num_menu);		// Refresh variables
			}
			if (num_menu){
				num_menu->num_item = num_item;
				draw_menu();
			}
		}

		if (ret == REDRAWTIMEJOUR){
			ttm	= localtime(&jourtime);
			memcpy(&jtime_tm, ttm, sizeof(struct tm));
			jyear = 1900 + jtime_tm.tm_year;
			j_mon = jtime_tm.tm_mon + 1;

			vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

			redraw_screen(NULL);
		}

		if (ret == REDRAWTIMEMAIN){
			ttm	= localtime(&maintime);
			memcpy(&mtime_tm, ttm, sizeof(struct tm));
			myear = 1900 + mtime_tm.tm_year;
			m_mon = mtime_tm.tm_mon + 1;
			redraw_screen(NULL);
		}

		if (ret == REDRAW){

			vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

			redraw_screen(NULL);
		}
	}
}

void default_up(GR_EVENT *event){
int itemy, itemh;

	num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;
	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->rect.height;

	if ((itemy > (num_menu->rect.height-10)) || (itemy  < (num_menu->bgnmenuy - 10))) redraw_menu(1);

	redraw_screen(NULL);

}

void default_down(GR_EVENT *event){
int itemy, itemh;

	num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;
	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->rect.height;

	if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

	redraw_screen(NULL);

}

void default_enter(GR_EVENT *event){
	// Select new menu
	if (num_menu->pitems[num_menu->num_item]->next_menu){
		strcpy(tmpstring, getpath2menu());
		strcat(tmpstring, num_menu->pitems[num_menu->num_item]->next_menu);
		num_menu = create_menu(tmpstring);
		call_action(NODIRECT, num_menu);		// Refresh variables
		if (num_menu) draw_menu();
	}
}

void date_left(GR_EVENT *event){
struct tm *ttm;
int itemy, itemh, i;

	if (!num_menu->num_item){
		// Month
		ttm	= localtime(&tmptime);
		if (ttm->tm_mon) ttm->tm_mon--;
		else { ttm->tm_mon = 11; ttm->tm_year--;}
		tmptime = mktime(ttm);
		call_epiloque(num_menu);
		num_menu = destroy_menu(num_menu, DIR_BACKWARD);
		default_enter(NULL);
	}else{
		// Days
		if (num_menu->pitems[num_menu->num_item]->rect.y ==
			num_menu->pitems[(int)num_menu->pitems[num_menu->num_item]->prev_item]->rect.y)
					num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;
		else{
			for (i=0; i<6; i++){
				if (num_menu->pitems[num_menu->num_item]->rect.y ==
					num_menu->pitems[(int)num_menu->pitems[num_menu->num_item]->next_item]->rect.y)
						num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;
				else break;
			}
		}

		itemy = num_menu->pitems[num_menu->num_item]->rect.y;
		itemh = num_menu->pitems[num_menu->num_item]->rect.height;

		if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

		redraw_screen(NULL);
	}

}

void date_right(GR_EVENT *event){
struct tm *ttm;
int itemy, itemh, i;

	if (!num_menu->num_item){
		// Month
		ttm	= localtime(&tmptime);
		if (ttm->tm_mon < 11) ttm->tm_mon++;
		else { ttm->tm_mon = 0; ttm->tm_year++;}
		tmptime = mktime(ttm);
		call_epiloque(num_menu);
		num_menu = destroy_menu(num_menu, DIR_BACKWARD);
		default_enter(NULL);
	}else{
		// Days
		if (num_menu->pitems[num_menu->num_item]->rect.y ==
			num_menu->pitems[(int)num_menu->pitems[num_menu->num_item]->next_item]->rect.y)
					num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;
		else{
			for (i=0; i<6; i++){
				if (num_menu->pitems[num_menu->num_item]->rect.y ==
					num_menu->pitems[(int)num_menu->pitems[num_menu->num_item]->prev_item]->rect.y)
						num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;
				else break;
			}
		}

		itemy = num_menu->pitems[num_menu->num_item]->rect.y;
		itemh = num_menu->pitems[num_menu->num_item]->rect.height;

		if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

		redraw_screen(NULL);
	}

}

void date_up(GR_EVENT *event){
int itemy, itemh, i;
	if (!num_menu->num_item){
		// Month
		default_up(event);
	}else{
		// For days
		for (i=0; i<7; i++){
			if (num_menu->num_item > num_menu->pitems[num_menu->num_item]->prev_item)
					num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;
			else
					{num_menu->num_item = 0; break;}
		}
		itemy = num_menu->pitems[num_menu->num_item]->rect.y;
		itemh = num_menu->pitems[num_menu->num_item]->rect.height;

		if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

		redraw_screen(NULL);

	}
}

void date_down(GR_EVENT *event){
int itemy, itemh, i;

	if (!num_menu->num_item){
		// Month
		default_down(event);
	}else{
		// Days
		for (i=0; i<7; i++){
			if (num_menu->num_item < num_menu->pitems[num_menu->num_item]->next_item)
					num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;
			else
					{num_menu->num_item = 0; break;}
		}
		itemy = num_menu->pitems[num_menu->num_item]->rect.y;
		itemh = num_menu->pitems[num_menu->num_item]->rect.height;

		if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

		redraw_screen(NULL);
	}

}

void date_enter(GR_EVENT *event){
struct tm *ttm;

	call_epiloque(num_menu);
	num_menu = destroy_menu(num_menu, DIR_BACKWARD);

	ttm	= localtime(&jourtime);
	memcpy(&jtime_tm, ttm, sizeof(struct tm));
	jyear = 1900 + jtime_tm.tm_year;
	j_mon = jtime_tm.tm_mon + 1;

	ttm	= localtime(&maintime);
	memcpy(&mtime_tm, ttm, sizeof(struct tm));
	myear = 1900 + mtime_tm.tm_year;
	m_mon = mtime_tm.tm_mon + 1;

	vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

	redraw_screen(NULL);

}

void time_left(GR_EVENT *event){

	num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;
	redraw_screen(NULL);

}

void time_right(GR_EVENT *event){

	num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;
	redraw_screen(NULL);

}

void time_up(GR_EVENT *event){
char *pdig = num_menu->pitems[num_menu->num_item]->text;

	(*pdig)++;
	if ((*pdig) > '9') *pdig = '0';

	// Correct by position
	if ((num_menu->num_item == 1) && ((*pdig) > '2')) *pdig = '0';
	if ((num_menu->num_item == 4) && ((*pdig) > '5')) *pdig = '0';
	if ((num_menu->num_item == 7) && ((*pdig) > '5')) *pdig = '0';

	redraw_screen(NULL);

}

void time_down(GR_EVENT *event){
char *pdig = num_menu->pitems[num_menu->num_item]->text;

	(*pdig)--;
	if ((*pdig) < '0') *pdig = '9';

	// Correct by position
	if ((num_menu->num_item == 1) && ((*pdig) > '2')) *pdig = '2';
	if ((num_menu->num_item == 4) && ((*pdig) > '5')) *pdig = '5';
	if ((num_menu->num_item == 7) && ((*pdig) > '5')) *pdig = '5';

	redraw_screen(NULL);

}

void time_enter(GR_EVENT *event){
struct tm *ttm;

	call_epiloque(num_menu);
	num_menu = destroy_menu(num_menu, DIR_BACKWARD);

	ttm	= localtime(&jourtime);
	memcpy(&jtime_tm, ttm, sizeof(struct tm));
	jyear = 1900 + jtime_tm.tm_year;
	j_mon = jtime_tm.tm_mon + 1;

	ttm	= localtime(&maintime);
	memcpy(&mtime_tm, ttm, sizeof(struct tm));
	myear = 1900 + mtime_tm.tm_year;
	m_mon = mtime_tm.tm_mon + 1;

	vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

	redraw_screen(NULL);
}

void setlnbytype(GR_EVENT *event){
LNODE *pln = actlnode;
char* itemtext = (char*) num_menu->pitems[num_menu->num_item]->text;

	itemtext = get_lnclassbytext(itemtext);

	// Find LLN0
	while((pln) && (pln->ln.lnclass)) pln=pln->l.prev;

	while(pln){
		if (pln->ln.lnclass){
			if (!strcmp(itemtext, pln->ln.lnclass)){
				actlnode = pln;
				break;
			}
		}
		pln = pln->l.next;
	}

	// Return to previous menu
	num_menu = destroy_menu(num_menu, DIR_BACKWARD);
	// Refresh menu by new values
	call_action(NODIRECT, num_menu);		// Refresh variables for set right main menu
	num_menu = destroy_menu(num_menu, DIR_SIDEBKW);
	if (!num_menu){
		num_menu = create_menu(lnmenunames[idlnmenuname]);
		call_action(NODIRECT, num_menu);		// Refresh variables for set all vars
	}
	if (num_menu) draw_menu();
}

void setlnbyclass(GR_EVENT *event){
LNODE *pln = (LNODE*) fln.next;
char* itemtext = (char*) num_menu->pitems[num_menu->num_item]->text + 8;
int adr = atoi(itemtext);	// LD address

	while(pln){
		if (atoi(pln->ln.ldinst) == adr){
				actlnode = pln;
				break;
		}
		pln = pln->l.next;
	}

	// Return to previous menu
	num_menu = destroy_menu(num_menu, DIR_BACKWARD);
	// Refresh menu by new values
	call_action(NODIRECT, num_menu);		// Refresh variables for set right main menu
	num_menu = destroy_menu(num_menu, DIR_SIDEBKW);
	if (!num_menu){
		num_menu = create_menu(lnmenunames[idlnmenuname]);
		call_action(NODIRECT, num_menu);		// Refresh variables for set all vars
	}
	if (num_menu) draw_menu();
}

void setinterval(GR_EVENT *event){
int i = num_menu->num_item - 2;

	num_menu = destroy_menu(num_menu, DIR_BACKWARD);
	pinterval = &intervals[i];

	vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

	redraw_screen(NULL);
}

void settarif(GR_EVENT *event){
int i = num_menu->num_item - 1;
tarif *ptarif = (tarif*) &ftarif;

	while((ptarif) && (i)){
		i--;
		ptarif = ptarif->l.next;
	}
	if (ptarif){
		acttarif = ptarif;
		num_menu = destroy_menu(num_menu, DIR_BACKWARD);
		call_action(NODIRECT, num_menu);		// Refresh variables

		vc_attach_dataset(num_menu->fvarrec, &jourtime, *pinterval, actlnode, LOGGED);

		redraw_screen(NULL);
	}else num_menu = destroy_menu(num_menu, DIR_BACKWARD);

}
//-------------------------------------------------------------------------------
// Draw all items and cursor position as last
void redraw_screen(void *arg){
int i;

//  Full redraw
	for (i = 0; i < num_menu->count_item; i++){
		do_paint(num_menu->pitems[i], FGCOLOR, BGCOLOR);
	}

	do_paint(num_menu->pitems[num_menu->num_item], BGCOLOR, FGCOLOR);

}

// Key working function:
// keyleft and keyright working through function set in action_fn.c file
// keyup and key down working as cursor keys forever
// ENTER it's enter to submenu forever or confirmation of changes forever
// MENU it's return to previous menu without changes forever
void key_pressed(void *arg){
GR_EVENT *event = (GR_EVENT*) arg;

	ts_printf(STDOUT_FILENO, TS_DEBUG, "HMI: Key press 0x%04X\n", event->keystroke.ch);

	switch(event->keystroke.ch){

	case 0x102:
	case 0xf800:	// Key left
					if (num_menu->keyleft) num_menu->keyleft((GR_EVENT*) arg);
					break;

	case 0x103:
	case 0xf801:	// Key right
					if (num_menu->keyright) num_menu->keyright((GR_EVENT*) arg);
					break;

	case 0x101:
	case 0xf802:	// Key up
					if (num_menu->keyup) num_menu->keyup((GR_EVENT*) arg);
					break;

	case 0x104:
	case 0xf803:	// Key down
					if (num_menu->keydown) num_menu->keydown((GR_EVENT*) arg);
					break;

	case 0x106:
	case 0x0D:		// Key ENTER
	case 0x20:		// Dynamic menu change our variable
					if (num_menu->keyenter) num_menu->keyenter((GR_EVENT*) arg);
					break;

	case 0x105:
	case 0x1B:		// Key MENU / ESC
					num_menu = destroy_menu(num_menu, DIR_BACKWARD);
					if (!num_menu){
						num_menu = create_menu("menus/item");
						if (num_menu) draw_menu();
					}else{
						redraw_screen(NULL);
					}
					break;
	}

}

//---------------------------------------------------------------------------------
// After release of key
void key_rised(void *arg)
{

}

//---------------------------------------------------------------------------------
//int init_menu(fact *factsetting, int len)
int init_menu(){
struct tm *ttm;
int i, z;

	if (!fln.next) return 1;

	// Set actual time in vars
	jourtime = time(NULL);
	maintime = time(NULL);
	ttm	= localtime(&maintime);
	memcpy(&jtime_tm, ttm, sizeof(struct tm));
	memcpy(&mtime_tm, ttm, sizeof(struct tm));
	jyear = 1900 + jtime_tm.tm_year;
	myear = 1900 + mtime_tm.tm_year;
	j_mon = jtime_tm.tm_mon + 1;
	m_mon = mtime_tm.tm_mon + 1;

	// Init base menu names
	z = get_quanoftypes();
	for (i=0; i < z; i++){
		ts_sprintf(lnmenunames[i], "%s/item%s", getpath2menu(), lntxts[i].ln);
	}

	// First actual LNODE init
	actlnode = (LNODE*) fln.next;					// Try set first LLN0
	if (!actlnode) exit(1);

	// First actual tarif init
	acttarif = (tarif*) &ftarif;
	acttarif->name = defvalues[30].defval;
	acttarif->id = 0;

	// Set first actual LN
//	actlnode = setdef_lnode(0, num_menu);					// Try set Telemetering / MMXU
//	if (!actlnode) actlnode = setdef_lnode(1, num_menu);	// Try set Telesignal
//	if (!actlnode) actlnode = setdef_lnode(2, num_menu);	// Try set Telecontrol
//	if (!actlnode) exit(1);

	// Open first menu
	num_menu = create_menu(lnmenunames[0]);
	if (num_menu) {
		call_action(NODIRECT, num_menu);		// Refresh variables
		draw_menu();
	}

    return 0;
}

void menu_parser(pvalue vt, int len){
int i, j;

	for (i = 0; i < len; i++){
		for (j = 0; j < sizeof(menuvalues)/sizeof(value); j++){
			if (vt[i].name)
				if (!strcmp(vt[i].name, menuvalues[j].name)) memcpy(&vt[i], &menuvalues[j], sizeof(value));
		}
	}
}

//---------------------------------------------------------------------------------

