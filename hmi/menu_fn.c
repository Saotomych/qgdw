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

struct _mons{
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

char *days[7] = {
		{"ПН"},
		{"ВТ"},
		{"СР"},
		{"ЧТ"},
		{"ПТ"},
		{"СБ"},
		{"BC"},
};

char menutxt[1024];

// Menu of LNODES
char* ChangeLN(char *arg){
char *pmenu = menutxt;
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
char **lnodefilter = (char**) ((int*)arg)[1];
void **dynmenuvar = (void*)(((int*)arg)[3]);
int i = 1; 	// Real parameters for dynmenu begin from 1
int x, y = 0;

	*dynmenuvar = pbln;

	pln = (LNODE*) fln.next; x = 0;
	sprintf(pmenu, "text %d %d a a Выбор устройства\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = MENUSTEP;
	while(pln){
		if (!strcmp(pln->ln.lnclass, *lnodefilter)){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s >item ~changetypeln\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
			// Set pointer to LNODE for the item in future
			dynmenuvar[i] = (int*) pln; i++;
		}
		pln = pln->l.next;
	}

	return menutxt;
}

// Menu of ALL LNODES FROM FILTER CLASSES
char* ChangeLNType(char *arg){
char *pmenu = menutxt;
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
void **dynmenuvar = (void*)(((int*)arg)[3]);
int i = 1; 	// Real parameters for dynmenu begin from 1
int x, y = 0;

	*dynmenuvar = pbln;

	pln = (LNODE*) fln.next; x = 0;
	sprintf(pmenu, "main 40 10 120 150\n");
	pmenu += strlen(pmenu);
	sprintf(pmenu, "text %d %d a a Выбор устройства\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = MENUSTEP;
	while(pln){
		if (!strcmp(pln->ln.lnclass, "MMXU")){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s >item ~changetypeln\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
			// Set pointer to LNODE for the item in future
			dynmenuvar[i] = (int*) pln; i++;
		}
		pln = pln->l.next;
	}

	pln = (LNODE*) fln.next;
	while(pln){
		if (!strcmp(pln->ln.lnclass, "MSQI")){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s >item ~changetypeln\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
			// Set pointer to LNODE for the item in future
			dynmenuvar[i] = (int*) pln; i++;
		}
		pln = pln->l.next;
	}

	pln = (LNODE*) fln.next;
	while(pln){
		if (!strcmp(pln->ln.lnclass, "MMTR")){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s >item ~changetypeln\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
			// Set pointer to LNODE for the item in future
			dynmenuvar[i] = (int*) pln; i++;
		}
		pln = pln->l.next;
	}

	return menutxt;
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
char *pmenu = menutxt;
time_t **timel = (time_t**)(((int*)arg)[4]);
struct tm *timetm = (struct tm**)(((int*)arg)[5]);
int i, x, y, maxday, wday, day;

	wday = (timetm->tm_mday/7)*7-timetm->tm_mday+timetm->tm_wday;
	maxday = mons[timetm->tm_mon].mdays;
	// Correct for 'Visokosny' year
	if ((timetm->tm_mon == 1) && (!(timetm->tm_year % 4))) maxday++;

    // Months and years
	sprintf(pmenu, "main 10 0 140 120\n");
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

	return menutxt;
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
		{"lnmenu", (void*) ChangeLN},
		{"lntypemenu", (void*) ChangeLNType},
		{"date", (void*) ChangeDate},
		{"interval", (void*) ChangeIntl},
		{"tarif", (void*) ChangeTarif},
};

//---*** External IP ***---//

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen
char* create_dynmenu(char *menuname, void *arg){
char *pmenu;
int i;

	pmenu = strstr(menuname,"/");
	if (pmenu) pmenu++;
	else pmenu = menuname;

	for (i=0; i < sizeof(menufactset) / sizeof(fact); i++){
		if (!strcmp(menufactset[i].action, pmenu)){
			pmenu = 0;
			pmenu = (char *) menufactset[i].func(arg);
		}
	}

	return pmenu;
}

