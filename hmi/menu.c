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

GR_GC_ID gc;		       //graphics context id
GR_EVENT event;
GR_WM_PROPERTIES props;
GR_FONT_ID	font;

int count_item = 0;        //number of item

//GR_WINDOW_ID *main_window;

static item **pitems;      //указатель на структуру пунктов
static menu *num_menu;     //указатель на структуру меню

#define MAIN_WIDTH 160     //параметры главного окна
#define MAIN_HEIGHT 120

#define WIDTH 160          //параметры окна пункта меню
#define HEIGHT 20

#define FONT_WIDTH 20
#define FONT_HEIGHT 20

#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )
#define BLUE  MWRGB( 0  , 0  , 128 )
#define CIAN  MWRGB( 0  , 128, 128 )
#define LTGREEN MWRGB( 0  , 255, 0 )

#define FGCOLOR	BLACK
#define BGCOLOR	WHITE
//#define FONTNAME   "7x13B.pcf.gz"
#define FONTNAME "7x13.pcf.gz"
//#define FONTNAME   "screen-rus.bdf"
//#define FONTNAME   "X6x13.bdf"
//#define FONTNAME   "6x13-ISO8859-1.pcf.gz"
//#define FONTNAME   "lubI24.pcf"
//#define FONTNAME    "helvB12.pcf"
//#define FONTNAME	 "times.ttf"
char fontname[200] = FONTNAME;

//void f1(void *arg);

fact pfactsetting[] ={
		{"doxpaint", f1},
		{"keydown", f2},
		{"keyup", f3},

};
//------------------------------------------------------------------------------------
int do_openfilemenu()
{
	static char *ptxtmenu;     //указатель на массив пунктов меню
	char *ptxt;
	FILE *fmcfg;               //file open
	struct stat fst;
	int clen;
	//static item **pitems;     //указатель на структуру пунктов
	int i;
	if (stat("item", &fst) == -1){
		 		printf("IEC Virt: 'item' file not found\n");
		 		return -1;
		 	}

		    ptxtmenu =  malloc(fst.st_size + 2);
		 	fmcfg = fopen("item", "r");
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

	         //if (ptxtmenu[i-1]) count_item++;

	         pitems = malloc(count_item * sizeof(item*)); //возвращает указатель на первый байт области памяти структуры пунктов
	         ptxt = ptxtmenu;

	         for (i=0; i<count_item; i++)
	         {
	            pitems[i] = (item*) malloc(sizeof(item));
	            while (!(*ptxt)) ptxt++;
	            pitems[i]->text = ptxt;
	            //printf("%0xX\n", pitems[i]);
	            //pitems[i]->name_font = ("verdana");

	            while ((*ptxt) && ((*ptxt) != '>') && ((*ptxt) != '~')) ptxt++;

	            pitems[i]->next_menu = 0;
	            if (*ptxt == '>'){
	            	*ptxt = 0;
	            	pitems[i]->next_menu = ptxt+1;
	            	ptxt++;
	                ptxt += strlen(ptxt);
	            }
	            if (*ptxt == '~'){
	               	*ptxt = 0;
	            	pitems[i]->next_menu = ptxt+1;
	            	ptxt++;
	            	ptxt += strlen(ptxt);
	            }

	         }
	           //printf("/n");
	            num_menu = malloc(sizeof(menu));   //возвращает указатель на первый байт блока области памяти структуры меню
	            num_menu->pitems = pitems;
	            num_menu->num_item = 0;
	            num_menu->start_item = 0;

     return 0;
}
//------------------------------------------------------------------------------------
void do_paint(item *pitem, int fg, int bg)
{
	        GR_GC_ID gc = GrNewGC();
			GR_WINDOW_ID *main_window = &(pitem->main_window);
			GR_WINDOW_INFO winfo;
			int y1=0;
			//int	width, height;
			//GR_FONT_INFO finfo;
			//height =  winfo.height / 530;
			//width =  winfo.width / 640;
		    //font = GrCreateFontEx(FONTNAME, height, width, 0);

			GrSetFontAttr(font, GR_TFANTIALIAS|GR_TFKERNING, 0);
			GrSetGCFont(gc, font);

			GrGetWindowInfo(*main_window, &winfo);

			gc = GrNewGC();                                   //Allocate a new graphic context
			GrSetGCUseBackground(gc, GR_FALSE);

			GrSetGCForeground(gc, bg);
			GrFillRect(*main_window, gc, 0, y1, winfo.width, winfo.height);
			GrSetGCForeground(gc, fg);

			GrSetGCBackground(gc, bg);
			GrSetGCFont(gc, font);
			GrSetGCForeground(gc, fg);
		    //GrFillRect(wid, gc, x-1, y-1, finfo.maxwidth+2, finfo.height+2);
		    //GrSetGCForeground(gc, GR_RGB(255, 255, 255));

			GrText(*main_window, gc, 0, y1, pitem->text, strlen(pitem->text), GR_TFUTF8|GR_TFTOP);

			GrDestroyGC(gc);

			y1 = y1+20; //step text
			//sprintf(buf, "%d/%d Item menu", height, width);
			//GrDestroyGC(gc);
			//GrFlushWindow(*main_window);
}
//------------------------------------------------------------------------------------
void draw_menu()
{
  GR_WINDOW_ID *main_window;
		 if (GrOpen() < 0) {
			 fprintf(stderr, "Cannot open graphics\n");
			 exit(1);
			}

			 wm_init();
			 //Paint on the screen window main_window
			 GR_WINDOW_INFO winfo;
			 num_menu->main_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, MAIN_HEIGHT, MAIN_WIDTH, 1, BLUE, BLACK);

			 GrGetWindowInfo(num_menu->main_window, &winfo);

			 gc = GrNewGC();
			 GrSetGCUseBackground(gc, GR_FALSE);

			 GrSetGCForeground(gc, BGCOLOR);
			 GrFillRect(num_menu->main_window, gc, 0, 0, winfo.width, winfo.height);
			 GrSetGCForeground(gc, FGCOLOR);

			 GrMapWindow(num_menu->main_window);
			 GrSetFocus(num_menu->main_window);
			 props.flags = GR_WM_FLAGS_PROPS |
							  GR_WM_FLAGS_TITLE;
			 //props.props = GR_WM_PROPS_NOB4ACKGROUND;
			  props.props = //GR_WM_PROPS_NORAISE |
							  GR_WM_PROPS_BORDER |
							  GR_WM_PROPS_CAPTION |
							  GR_WM_PROPS_CLOSEBOX;
			 props.title = "main window";
			 //props.background = BLACK;

			GrSetWMProperties(num_menu->main_window, &props);
			//GrSetWMProperties(window2, &props);
			//GrSetWMProperties(window3, &props);

			GrSelectEvents(num_menu->main_window, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ); //Select the events this client wants to receive

			//Paint on the screen windows of item
			int i;
			int y2 = 0; //step item

			for (i=num_menu->start_item; i<count_item; i++)
			{
				main_window = &(num_menu->pitems[i]->main_window);

				//if (y2>160) i = 8;

				//num_menu->main_window = GR_ROOT_WINDOW_ID
				*main_window = GrNewWindow(num_menu->main_window, 0, y2, WIDTH, HEIGHT, 1, WHITE, BLUE);
				y2 = y2+20;

				props.flags = GR_WM_FLAGS_PROPS |
						GR_WM_FLAGS_TITLE;
				//props.props = GR_WM_PROPS_NOB4ACKGROUND;
				props.props = //GR_WM_PROPS_NORAISE |
						GR_WM_PROPS_BORDER |
						GR_WM_PROPS_CAPTION |
						GR_WM_PROPS_CLOSEBOX;
				props.title = "menu window";
				//props.background = BLACK;

				GrSetWMProperties(*main_window, &props);
				//GrSetWMProperties(window2, &props);


				GrSelectEvents(*main_window, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_UPDATE | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
						| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP); //Select the events this client wants to receive

				GrMapWindow(*main_window);      //Make a window visible on the screen
				GrSetFocus(*main_window);

	        }

		//gc = GrNewGC();               //Allocate a new graphic context

		font = GrCreateFontEx(FONTNAME, FONT_HEIGHT, FONT_WIDTH, 0);
}
//-------------------------------------------------------------------------------
void f1(void *arg)
{
	//GR_EVENT *event = (GR_EVENT*) arg;
	//int *pi = (int*) arg;
    int i;
	for (i=num_menu->start_item; i<count_item; i++)
	{
		if (num_menu->num_item == i) do_paint(pitems[i], BGCOLOR, FGCOLOR);
		else do_paint(pitems[i], FGCOLOR, BGCOLOR);
    }
}
//--------------------------------------------------------------------------------
//keyup and keydown
void f2(void *arg)
{
    GR_EVENT *event = (GR_EVENT*) arg;
    int i;

	switch(event->keystroke.ch)
	{

	case 0xf802:
					num_menu->num_item--;
					if (num_menu->num_item < 0) num_menu->num_item = 0;

					if (num_menu->num_item < num_menu->start_item){
						num_menu->start_item--;
						draw_menu();
					}
					if (num_menu->start_item < 0) num_menu->start_item = 0;

					for (i=num_menu->start_item; i<count_item; i++)
					{
						    if (num_menu->num_item == i) do_paint(pitems[i], BGCOLOR, FGCOLOR);
							else do_paint(pitems[i], FGCOLOR, BGCOLOR);
					}
                    break;

	case 0xf803:
					num_menu->num_item++;
					//num_menu->last_item = num_menu->num_item;

					//i = num_menu->last_item;
					if (((num_menu->num_item - num_menu->start_item) * 20) > 60){
						num_menu->start_item++;
						draw_menu();
					}

					if (num_menu->num_item >= count_item) num_menu->num_item = count_item-1;


					for (i=num_menu->start_item; i<count_item; i++)
					{
						    if (num_menu->num_item == i) do_paint(pitems[i], BGCOLOR, FGCOLOR);
							else do_paint(pitems[i], FGCOLOR, BGCOLOR);
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
