#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"
#include "nanowm.h"
#include <signal.h>
#include <string.h>
#include "menu.h"
#include "hmi.h"

#include <sys/types.h>
#include <sys/stat.h>

static item **pitems;      //указатель на структуру пунктов
static menu *num_menu;     //указатель на структуру меню

#define MAIN_WIDTH 160     //параметры главного окна
#define MAIN_HEIGHT 160

#define WIDTH 160          //параметры окна пункта меню
#define HEIGHT MENUSTEP

#define FONT_WIDTH 7
#define FONT_HEIGHT 13

#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )
#define BLUE  MWRGB( 0  , 0  , 128 )
#define CIAN  MWRGB( 0  , 128, 128 )
#define LTGREEN MWRGB( 0  , 255, 0 )

#define FGCOLOR	BLACK
#define BGCOLOR	WHITE
#define FONTNAME "pcf/7x13.pcf.gz"

fact pfactsetting[] ={
		{"doxpaint", f1},
		{"keydown", f2},
		{"keyup", f3},

};
//------------------------------------------------------------------------------------
int do_openfilemenu()
{
	static char *ptxtmenu;     //указатель на массив пунктов меню
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

	if (stat("menus/item", &fst) == -1){
		 		printf("IEC Virt: 'item' file not found\n");
		 		return -1;
		 	}

		    ptxtmenu =  malloc(fst.st_size + 2);
		 	fmcfg = fopen("menus/item", "r");
		 	clen = fread((ptxtmenu+1), 1, (size_t) (fst.st_size), fmcfg);
		 	if (!clen) return -1;
		 	if (clen != fst.st_size) return -1;

		 	//Make ending 0 for string
		 	ptxtmenu[clen+1] = 0;

		 	//Make ending start 0
		 	ptxtmenu[0] = 0;

		 	for (i=1; i<clen; i++)
	        {
	        	if (ptxtmenu[i] == 0xD || ptxtmenu[i] == 0xA) ptxtmenu[i] = 0;
	        	if ((ptxtmenu[i]) && (!ptxtmenu[i-1])) count_item++;
	        }

	         pitems = malloc(count_item * sizeof(item*)); //возвращает указатель на первый байт области памяти структуры пунктов
	         ptxt = ptxtmenu;

	         for (i = 0; i < count_item; i++)
	         {
	            while (!(*ptxt)) ptxt++;

	            pitems[i] = (item*) malloc(sizeof(item));
	            ptxt[4] = 0;
	            pitemtype = ptxt;
	            ptxt += 5;

	            // Item type "MENU"
	            if (!strcmp(pitemtype, "menu")){

	            	pitems[i]->prev_item = last_menuitem;
	            	pitems[i]->next_item = first_menuitem;
	            	pitems[last_menuitem]->next_item = i;
		            pitems[i]->text = ptxt;

		            while ((*ptxt) && ((*ptxt) != '>') && ((*ptxt) != '~')) ptxt++;

		            pitems[i]->next_menu = 0;
		            if (*ptxt == '>'){
		            	*ptxt = 0;
		            	pitems[i]->next_menu = ptxt+1;
		            	ptxt++;
		            }
		            if (*ptxt == '~'){
		               	*ptxt = 0;
		            	pitems[i]->next_menu = ptxt+1;
		            	ptxt++;
		            }
		            last_menuitem = i;
		            count_menu++;
	            }
	            if (!count_menu){
	            	last_menuitem++;
	            	first_menuitem++;
	            }

	            // Item type "TEXT"
	            if (!strcmp(pitemtype, "text")){
		            pitems[i]->text = ptxt;
	            }

	            ptxt += strlen(ptxt);

	         }

	         pitems[first_menuitem]->prev_item = last_menuitem;

	         num_menu = malloc(sizeof(menu));   //возвращает указатель на первый байт блока области памяти структуры меню
	         num_menu->pitems = pitems;
	         num_menu->num_item = first_menuitem;
	         num_menu->first_item = first_menuitem;
	         num_menu->start_item = 0;
	         num_menu->count_item = count_item;

     return 0;
}
//------------------------------------------------------------------------------------
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

			GrText(*main_window, gc, 10, 0, pitem->text, strlen(pitem->text), GR_TFUTF8|GR_TFTOP);

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


			num_menu->main_window = GrNewWindow(GR_ROOT_WINDOW_ID, MENUSTEP, MENUSTEP, MAIN_HEIGHT, MAIN_WIDTH, 1, WHITE, BLACK);
   		    GrMapWindow(num_menu->main_window);

			props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
			props.props = GR_WM_PROPS_BORDER |
					   GR_WM_PROPS_CAPTION |
					   GR_WM_PROPS_CLOSEBOX;
			props.title = "main window";

			GrSetWMProperties(num_menu->main_window, &props);

			GrGetWindowInfo(GR_ROOT_WINDOW_ID, &winfo);
			GrSelectEvents(num_menu->main_window, winfo.eventmask | GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_KEY_DOWN);

			//Paint on the screen windows of item
			int i;
			int y2 = 0; //step item

			for (i=0; i < num_menu->count_item; i++)
			{
				main_window = &(num_menu->pitems[i]->main_window);

				*main_window = GrNewWindow(num_menu->main_window, 0, y2, WIDTH, HEIGHT, 0, WHITE, WHITE);
				y2 = y2+MENUSTEP;

				props.flags = 0;
				props.props = 0;

				GrSetWMProperties(*main_window, &props);
				GrSelectEvents(*main_window, 0);
				GrMapWindow(*main_window);      //Make a window visible on the screen
	        }

			num_menu->font = GrCreateFontEx(FONTNAME, FONT_HEIGHT, FONT_WIDTH, 0);
			GrSetFontAttr(num_menu->font, GR_TFANTIALIAS | GR_TFKERNING, 0);
}

//-------------------------------------------------------------------------------
void f1(void *arg)
{
	//GR_EVENT *event = (GR_EVENT*) arg;
	//int *pi = (int*) arg;
    int i;
	for (i = num_menu->start_item; i < num_menu->count_item; i++)
	{
		do_paint(pitems[i], FGCOLOR, BGCOLOR);
    }
	do_paint(pitems[num_menu->num_item], BGCOLOR, FGCOLOR);
}
//--------------------------------------------------------------------------------
//keyup and keydown
void f2(void *arg)
{
    GR_EVENT *event = (GR_EVENT*) arg;
    GR_WINDOW_INFO winfo;
    int i;
    int y;

	switch(event->keystroke.ch)
	{

	case 0xf802:
					num_menu->num_item = pitems[num_menu->num_item]->prev_item;

					if (num_menu->num_item < num_menu->start_item){
						y = num_menu->first_item * MENUSTEP;
						num_menu->start_item = num_menu->num_item;
						for (i = num_menu->start_item; i < num_menu->count_item; i++){
							GrMoveWindow(pitems[i]->main_window, 0, y);
							y += MENUSTEP;
						}
					}

					if  (num_menu->num_item == (num_menu->count_item - 1)){
						y = num_menu->first_item * MENUSTEP;
						num_menu->start_item = num_menu->num_item - (MAIN_HEIGHT/MENUSTEP - 1) + 1;
						if (num_menu->start_item < num_menu->first_item) num_menu->start_item = num_menu->first_item;
						for (i = num_menu->start_item; i < num_menu->count_item; i++){
							GrMoveWindow(pitems[i]->main_window, 0, y);
							y += MENUSTEP;
						}
					}

					for (i = 0; i < num_menu->count_item; i++) do_paint(pitems[i], FGCOLOR, BGCOLOR);
					do_paint(pitems[num_menu->num_item], BGCOLOR, FGCOLOR);

					break;

	case 0xf803:
					num_menu->num_item = pitems[num_menu->num_item]->next_item;

					if ((((num_menu->num_item-num_menu->start_item) * MENUSTEP) >= (MAIN_HEIGHT - MENUSTEP)) ||
							(num_menu->num_item == num_menu->first_item)) {

						y = num_menu->first_item * MENUSTEP;
						num_menu->start_item = num_menu->num_item - (MAIN_HEIGHT/MENUSTEP - 1) + 1;
						if (num_menu->start_item < num_menu->first_item) num_menu->start_item = num_menu->first_item;
						for (i = num_menu->start_item; i < num_menu->count_item; i++){
							GrMoveWindow(pitems[i]->main_window, 0, y);
							y += MENUSTEP;
						}
						for (i = num_menu->first_item; i < num_menu->start_item; i++){
							GrMoveWindow(pitems[i]->main_window, 0, y);
							y += MENUSTEP;
						}
					}

					for (i = 0; i < num_menu->count_item; i++) do_paint(pitems[i], FGCOLOR, BGCOLOR);
					do_paint(pitems[num_menu->num_item], BGCOLOR, FGCOLOR);

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
