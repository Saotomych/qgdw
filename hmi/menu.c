

#include <nano-X.h>
#include <nanowm.h>
#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "menu.h"
#include "hmi.h"

static char prev_item;		// pointer to item in main menu

static menu *num_menu;     //указатель на структуру меню

char newmenu[40];			// For temporary operations

#define MAIN_WIDTH 160     //параметры главного окна
#define MAIN_HEIGHT 160

#define WIDTH 160          //параметры окна пункта меню
#define HEIGHT MENUSTEP

#define FONT_WIDTH 7
#define FONT_HEIGHT 13

#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )

#define FGCOLOR	BLACK
#define BGCOLOR	WHITE
#define FONTNAME "pcf/7x13.pcf.gz"

fact pfactsetting[] ={
		{"doxpaint", f1},
		{"keydown", f2},
		{"keyup", f3},
};

//------------------------------------------------------------------------------------
int do_openfilemenu(char *buf, int type){
	char *pitemtype;
	char *ptxt;
	FILE *fmcfg;               //file open
	struct stat fst;
	int clen;

	int count_item = 0;
	int count_menu = 0;

	int i;
	char last_menuitem = 0;
	char first_menuitem = 0;

	char *p;

	 	 	num_menu = malloc(sizeof(menu));   //возвращает указатель на первый байт блока области памяти структуры меню

    		switch(type){

			case MENUFILE:

							if (stat(buf, &fst) == -1){
								printf("IEC Virt: menufile not found\n");
								free(num_menu);
								return -1;
							}

							num_menu->ptxtmenu =  malloc(fst.st_size + 2);
						 	fmcfg = fopen(buf, "r");
						 	clen = fread((num_menu->ptxtmenu+1), 1, (size_t) (fst.st_size), fmcfg);
						 	if (!clen) return -1;
						 	if (clen != fst.st_size) return -1;
						 	break;

			case MENUMEM:
							clen = strlen(buf);
						 	if (!clen) return -1;

						 	num_menu->ptxtmenu = malloc(clen + 2);
						 	strcpy((num_menu->ptxtmenu+1), buf);
						 	break;
			}


		 	//Make ending 0 for string
		 	num_menu->ptxtmenu[clen+1] = 0;
		 	//Make ending start 0
		 	num_menu->ptxtmenu[0] = 0;

		 	for (i=1; i<clen; i++)
	        {
	        	if (num_menu->ptxtmenu[i] == 0xD || num_menu->ptxtmenu[i] == 0xA) num_menu->ptxtmenu[i] = 0;
	        	if ((num_menu->ptxtmenu[i]) && (!num_menu->ptxtmenu[i-1])) count_item++;
	        }

		 	num_menu->pitems = malloc(count_item * sizeof(item*)); //возвращает указатель на первый байт области памяти структуры пунктов
		 	ptxt = num_menu->ptxtmenu;
	 	 	num_menu->bgnmenuy = 0;

	         for (i = 0; i < count_item; i++)
	         {
	            while (!(*ptxt)) ptxt++;

	            num_menu->pitems[i] = (item*) malloc(sizeof(item));
	            ptxt[4] = 0;
	            pitemtype = ptxt;
	            ptxt += 5;
	            while (*ptxt == ' ') ptxt++;

	            // Parse RECTangle for item
	            // X
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.x = atoi(ptxt);
	            else num_menu->pitems[i]->rect.x = 0;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // Y
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.y = atoi(ptxt);
	            else num_menu->pitems[i]->rect.y = num_menu->pitems[i-1]->rect.y + num_menu->pitems[i-1]->rect.height;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // W
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.width = atoi(ptxt);
	            else num_menu->pitems[i]->rect.width = MAIN_WIDTH - num_menu->pitems[i]->rect.x;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // H
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.height = atoi(ptxt);
	            else num_menu->pitems[i]->rect.height = MENUSTEP;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // Item type "MENU"
	            if (!strcmp(pitemtype, "menu")){

	            	num_menu->pitems[i]->prev_item = last_menuitem;
	            	num_menu->pitems[i]->next_item = first_menuitem;
	            	num_menu->pitems[(int)last_menuitem]->next_item = i;
		            num_menu->pitems[i]->text = ptxt;

		            while ((*ptxt) && ((*ptxt) != '>') && ((*ptxt) != '~')) ptxt++;

		            num_menu->pitems[i]->next_menu = 0;
		            if (*ptxt == '>'){
		            	*ptxt = 0;
		            	num_menu->pitems[i]->next_menu = ptxt+1;
		            	ptxt++;
		            }
		            if (*ptxt == '~'){
		               	*ptxt = 0;
		               	num_menu->pitems[i]->action = ptxt+1;
		            	ptxt++;
		            }
		            last_menuitem = i;
		            count_menu++;
	            }
	            // For fixed first menu point in items
	            if (!count_menu){
	            	last_menuitem++;
	            	first_menuitem++;
	            }

	            // Item type "TEXT"
	            if (!strcmp(pitemtype, "text")){
		            num_menu->pitems[i]->text = ptxt;
		            if (i) num_menu->pitems[last_menuitem]->rect.height += num_menu->pitems[i]->rect.height;
	            }

	            ptxt += strlen(ptxt);

	            // IF Fixed text is variable
	            p = strstr(num_menu->pitems[i]->text, "&var:");
	            if (p){
	            	*p = 0;
	            	p += 5;
	            	num_menu->pitems[i]->vr = vc_addvarrec(p, (LNODE*)fln.next);
	            	while ((*p != ' ') && (*p)) p++;
	            	num_menu->pitems[i]->endtext = p;
	            }else{
	            	num_menu->pitems[i]->vr = NULL;
	            	num_menu->pitems[i]->endtext = NULL;
	            }

	         }

	         num_menu->pitems[(int) first_menuitem]->prev_item = last_menuitem;

	         num_menu->num_item = first_menuitem;
	         num_menu->first_item = first_menuitem;
	         num_menu->start_item = first_menuitem;
	         num_menu->count_item = count_item;
	         num_menu->bgnmenuy = num_menu->pitems[(int) num_menu->first_item]->rect.y;
     return 0;
}
//------------------------------------------------------------------------------------
static char wintext[40];
void do_paint(item *pitem, int fg, int bg)
{
	        GR_GC_ID gc = GrNewGC();
			GR_WINDOW_ID *main_window = &(pitem->main_window);
			GR_WINDOW_INFO winfo;

			GrGetWindowInfo(*main_window, &winfo);

			gc = GrNewGC();                                   //Allocate a new graphic context
			GrSetGCUseBackground(gc, GR_FALSE);
			GrSetGCBackground(gc, fg);
			GrSetGCForeground(gc, bg);
			GrFillRect(*main_window, gc, 0, 0, winfo.width, winfo.height);
			GrSetGCForeground(gc, fg);

			GrSetGCFont(gc, num_menu->font);

			if (pitem->vr) {
//				if (pitem->vr->prop & ISTRUE)
				if (pitem->vr->prop & INT32){
					sprintf(wintext, "%s%d%s", pitem->text, *((int*) (pitem->vr->val->val)), pitem->endtext);
				}
				if (pitem->vr->prop & INT64){
					sprintf(wintext, "%s%ld%s", pitem->text, *((long*) (pitem->vr->val->val)), pitem->endtext);
				}
				if (!(pitem->vr->prop & STRING)){
					strcpy(wintext, pitem->text);
					if (pitem->vr){
						if (pitem->vr->val) strcat(wintext, (char*) pitem->vr->val->val);
						else if (pitem->vr->val->defval) strcat(wintext, (char*) pitem->vr->val->defval);
					}
					if (pitem->endtext) strcat(wintext, pitem->endtext);
				}
				GrText(*main_window, gc, 3, 0, wintext, strlen(wintext), GR_TFUTF8|GR_TFTOP);
			}else GrText(*main_window, gc, 3, 0, pitem->text, strlen(pitem->text), GR_TFUTF8|GR_TFTOP);

			GrDestroyGC(gc);
}
//------------------------------------------------------------------------------------
void draw_menu()
{
		 if (GrOpen() < 0) {
			 fprintf(stderr, "Cannot open graphics\n");
			 exit(1);
			}

			wm_init();
			//Paint on the screen window main_window

			GR_WINDOW_ID *main_window;
			GR_WINDOW_INFO winfo;
			GR_WM_PROPERTIES props;


			num_menu->main_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, MAIN_HEIGHT, MAIN_WIDTH, 1, WHITE, BLACK);
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

			for (i=0; i < num_menu->count_item; i++)
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

			num_menu->font = GrCreateFontEx(FONTNAME, FONT_HEIGHT, FONT_WIDTH, 0);
			GrSetFontAttr(num_menu->font, GR_TFANTIALIAS | GR_TFKERNING, 0);

}

void redraw_menu(int type){
int i;
int stepy, y, itemy, itemh;

	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->rect.height;

	stepy = itemy + itemh - MAIN_HEIGHT;
	if (!type){
		if (num_menu->num_item == num_menu->first_item) stepy = itemy - num_menu->bgnmenuy;
	}else{
		if (num_menu->num_item < num_menu->pitems[num_menu->num_item]->next_item) stepy = itemy - num_menu->bgnmenuy;
	}

	for (i = num_menu->first_item; i < num_menu->count_item; i++){
		num_menu->pitems[i]->rect.y -= stepy;
		y = num_menu->pitems[i]->rect.y;
		if ((y >= num_menu->bgnmenuy - 10) && (y < MAIN_HEIGHT)){
			GrMoveWindow(num_menu->pitems[i]->main_window,
					 num_menu->pitems[i]->rect.x,
					 num_menu->pitems[i]->rect.y);
			GrMapWindow(num_menu->pitems[i]->main_window);
		}else GrUnmapWindow(num_menu->pitems[i]->main_window);

	}
}

void destroy_menu()
{
int i;

	for (i=0; i < num_menu->count_item; i++){
		GrUnmapWindow(num_menu->pitems[i]->main_window);
		GrDestroyWindow(num_menu->pitems[i]->main_window);
		free(num_menu->pitems[i]);
		num_menu->pitems[i] = 0;
	}
	GrUnmapWindow(num_menu->main_window);
	GrDestroyWindow(num_menu->main_window);
	free(num_menu->ptxtmenu);
	free(num_menu);
	num_menu = 0;
}

//-------------------------------------------------------------------------------
void f1(void *arg){
    int i;

	for (i = 0; i < num_menu->count_item; i++){
		do_paint(num_menu->pitems[i], FGCOLOR, BGCOLOR);
	}

	do_paint(num_menu->pitems[num_menu->num_item], BGCOLOR, FGCOLOR);

}
//--------------------------------------------------------------------------------
//keyup and keydown
void f2(void *arg){
    GR_EVENT *event = (GR_EVENT*) arg;
    int itemy, itemh;

	switch(event->keystroke.ch)
	{

	case 0xf802:	// Key up

					num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;

//					printf("start %d; num %d; count %d\n", num_menu->start_item, num_menu->num_item, num_menu->count_item);

					itemy = num_menu->pitems[num_menu->num_item]->rect.y;
					itemh = num_menu->pitems[num_menu->num_item]->rect.height;

					if ((itemy > (MAIN_HEIGHT-10)) || (itemy  < (num_menu->bgnmenuy - 10))) redraw_menu(1);

					f1(NULL);

					break;

	case 0xf803:	// Key down

					num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;

//					printf("start %d; num %d; count %d\n", num_menu->start_item, num_menu->num_item, num_menu->count_item);

					itemy = num_menu->pitems[num_menu->num_item]->rect.y;
					itemh = num_menu->pitems[num_menu->num_item]->rect.height;

					if ((itemy > (MAIN_HEIGHT-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

					f1(NULL);

					break;

	case 0x0D:		// Key ENTER
	case 0x20:
					prev_item = num_menu->num_item;
					if (num_menu->pitems[num_menu->num_item]->next_menu){
						strcpy(newmenu, "menus/");
						strcat(newmenu, num_menu->pitems[num_menu->num_item]->next_menu);
						destroy_menu();
						if (!do_openfilemenu(newmenu, MENUFILE)){
							draw_menu();
						}else exit (-1);
					}
					break;

	case 0x1B:		// Key MENU / ESC
					destroy_menu();
					if (!do_openfilemenu("menus/item", MENUFILE)){
						draw_menu();
						num_menu->num_item = prev_item;
					}
					break;

	}

}
//---------------------------------------------------------------------------------
void f3(void *arg)
{

}

//---------------------------------------------------------------------------------
int init_menu(fact *factsetting, int len)
{
    int n, m;

	if (factsetting == 0) return 0;

	for (m=0; m < (sizeof(pfactsetting) / sizeof(fact)); m++)
	{
		for (n=0; n<len; n++)
		{
			if (!strcmp(pfactsetting[m].action, factsetting[n].action)){
				factsetting[n].func = pfactsetting[m].func;
				break;
			}
		}
	}
	return 0;
}
//---------------------------------------------------------------------------------
