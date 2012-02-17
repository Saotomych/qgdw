/*
 * menu_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: alex
 */

#include "../common/common.h"
#include "../common/iec61850.h"
#include "hmi.h"
#include "menu.h"

extern menu* do_openfilemenu(char *buf);
extern LNODE *actlnode;
extern time_t maintime;		// Actual Time
extern time_t jourtime;		// Time for journal setting
extern time_t tmptime;		// Time for journal setting

static struct _mons{
	char *meng;
	char *mrus;
	int	 mdays;
} mons[12] = {
		{"Jan", "Январь", 31},
		{"Feb", "Февраль", 28},
		{"Mar", "Март", 31},
		{"Apr", "Апрель", 30},
		{"May", "Май", 31},
		{"Jun", "Июнь", 30},
		{"Jul", "Июль", 31},
		{"Aug", "Август", 31},
		{"Sep", "Сентябрь", 30},
		{"Oct", "Октябрь", 31},
		{"Nov", "Ноябрь", 30},
		{"Dec", "Декабрь", 31},
};

static char *days[7] = {
		"ПН",
		"ВТ",
		"СР",
		"ЧТ",
		"ПТ",
		"СБ",
		"BC",
};

//char menutxt[8192];

// Menu of LNODES
char* ChangeLN(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
LNODE *pln = (LNODE*) actlnode;
int x, y = 0;
char *lnodefilter = (char*) pln->ln.lnclass;

	pln = (LNODE*) fln.next; x = 0;
	sprintf(pmenu, "main 16 33 128 126\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "keys enter:setlnbyclass\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "text %d %d a a Выбор устройства\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = MENUSTEP;
	while(pln){
		if (!strcmp(pln->ln.lnclass, lnodefilter)){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
		}
		pln = pln->l.next;
	}

	*pmenu = 0;

	return txtmenu;
}

// Menu of ALL LNODES FROM FILTER CLASSES
char* ChangeLNType(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
LNODE *pln = (LNODE*) actlnode;
int x, y = 0;

	pln = (LNODE*) fln.next; x = 2;
	sprintf(pmenu, "main 16 15 128 145\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "keys enter:setlnbytype\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "text %d %d a a Выбор устройства\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = MENUSTEP;
	while(pln){
		if (!strcmp(pln->ln.lnclass, "MMXU")){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
		}
		pln = pln->l.next;
	}

	pln = (LNODE*) fln.next;
	while(pln){
		if (!strcmp(pln->ln.lnclass, "MSQI")){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
		}
		pln = pln->l.next;
	}

	pln = (LNODE*) fln.next;
	while(pln){
		if (!strcmp(pln->ln.lnclass, "MMTR")){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
		}
		pln = pln->l.next;
	}

	*pmenu = 0;

	return txtmenu;
}

// Menu of Date
//struct tm {
//               int tm_sec;         /* seconds */
//               int tm_min;         /* minutes */
//               int tm_hour;        /* hours */
//               int tm_mday;        /* day of the month */
//               int tm_mon;         /* month */
//               int tm_year;        /* year */
//               int tm_wday;        /* day of the week */
//               int tm_yday;        /* day in the year */
//               int tm_isdst;       /* daylight saving time */
//           };

char* ChangeDate(char *arg){
char txtmenu = arg;
char *pmenu = arg;
time_t *timel = &tmptime;
struct tm *timetm = localtime(timel);
int i, x, y, maxday, wday, day;

	wday = (7 + timetm->tm_wday - (timetm->tm_mday % 7)) % 7;
	maxday = mons[timetm->tm_mon].mdays;
	// Correct for 'Visokosny' year
	if ((timetm->tm_mon == 1) && (!(timetm->tm_year % 4))) maxday++;

    // Months and years
	sprintf(pmenu, "main 10 0 140 130\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "keys up:dateup down:datedown right:dateright left:dateleft enter:dateenter\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "menu 20 0 100 a %s %d\n", mons[timetm->tm_mon].mrus, 1900+timetm->tm_year);
	pmenu += strlen(pmenu);


	// Days of week
	x = 2;
	for (i = 0; i < 7; i++){
		sprintf(pmenu, "text %d 16 20 a %s\n", x, days[i]);
		pmenu += strlen(pmenu);
		x += 20;
	}

	// Empty days of old month in offset of x
	x = 20 * wday + 2; y = MENUSTEP * 2;
	// Numbers to end of first line days
	day = 1;
	for (i = wday; i < 7; i++){
		sprintf(pmenu, "menu %d %d %d a %d\n", ((day < 10) ? x+6 : x), y, ((day < 10) ? 8 : 16), day);
		pmenu += strlen(pmenu);
		x += 20; day++;
	}

	// Numbers of other lines
	while(day <= maxday){
		x = 2;
		y += MENUSTEP;
		for (i = 0; (i < 7) && (day <= maxday); i++, day++){
			sprintf(pmenu, "menu %d %d %d a %d\n", ((day < 10) ? x+6 : x), y, ((day < 10) ? 8 : 16), day);
			pmenu += strlen(pmenu);
			x += 20;
		}
	}

	*pmenu = 0;

	return txtmenu;
}

char* ChangeDateMain(char *arg){
	tmptime = maintime;
	return ChangeDate(arg);
}

char* ChangeDateJour(char *arg){
	tmptime = jourtime;
	return ChangeDate(arg);
}

// Menu of Interval
char* ChangeIntl(char *arg){
char *ptxt;

	return ptxt;
}

// Menu of Tarif
char* ChangeTarif(char *arg){
char *ptxt;

	return ptxt;
}

// Array of structs "synonym to function"
fact menufactset[] = {
		{"lnmenu", (void*) ChangeLN, NULL},
		{"lntypemenu", (void*) ChangeLNType, NULL},
		{"date", (void*) ChangeDate, NULL},					// For temporary operations with date
		{"maindate", (void*) ChangeDateMain, NULL},			// Initial operation by main date
		{"jourdate", (void*) ChangeDateJour, NULL},			// Initial operation by journal date
		{"interval", (void*) ChangeIntl, NULL},
		{"tarif", (void*) ChangeTarif, NULL},
};

//---*** External IP ***---//

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen

menu* create_menu(char *menuname){
FILE *fmcfg;               //file open
struct stat fst;			// statistics of file
int clen;					// lenght of file or array

char *pmenu, *txtmenu;
menu *psmenu;
int i;

	txtmenu = malloc(2048);
	txtmenu[0] = 0;

	// Stat of file, for get size of file
	if (stat(menuname, &fst) == -1){
		printf("IEC Virt: menufile not found\n");
	}else{
		// Load file to static buffer
		fmcfg = fopen(menuname, "r");
		clen = fread(txtmenu + 1, 1, (size_t) (fst.st_size), fmcfg);
		txtmenu[fst.st_size] = 0;
	}

	// Find and run proloque
	pmenu = strstr(menuname,"/");
	if (pmenu) pmenu++;
	else pmenu = menuname;

	for (i=0; i < sizeof(menufactset) / sizeof(fact); i++){
		if (!strcmp(menufactset[i].action, pmenu)){
			(char*) menufactset[i].proloque(txtmenu + strlen(txtmenu) + 1);
			break;
		}
	}

	// Create menu structures
	if (*(txtmenu+1)){
		psmenu = do_openfilemenu(txtmenu);
		if (!psmenu) psmenu = do_openfilemenu("menus/item");
	}

//	free(txtmenu);

	return psmenu;
}

