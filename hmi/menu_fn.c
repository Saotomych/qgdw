/*
 * menu_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: alex
 */

#include "../common/common.h"
#include "../common/iec61850.h"
#include "../common/tarif.h"
#include "../common/ts_print.h"
#include "hmi.h"
#include "menu.h"

extern menu* do_openfilemenu(char *buf);
extern LNODE *actlnode;
extern time_t maintime;		// Actual Time
extern time_t jourtime;		// Time for journal setting
extern time_t tmptime;		// Time for journal setting
extern int intervals[];

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

// Menu of LDEVICES
char* ChangeLN(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
LNODE *pln = (LNODE*) actlnode;
int x, y = 0;
char *lnodefilter = "MMXU";

	pln = (LNODE*) fln.next; x = 0;
	ts_sprintf(pmenu, "main 4 16 151 140\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "keys enter:setlnbyclass\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text %d %d a a Выбор устройства\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = 4;
	while(pln){
		if (!strcmp(pln->ln.lnclass, lnodefilter)){
			ts_sprintf(pmenu, "menu %d %d a a Адр: %s Тип: %s\n", x, y, pln->ln.ldinst, pln->ln.prefix);
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
char *ptxt;
LNODE *pln = (LNODE*) actlnode;
int x, y = 0, inst;

	pln = actlnode; x = 2;
	ts_sprintf(pmenu, "main 4 16 151 140\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "keys enter:setlnbytype\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text %d %d a a Выбор меню\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = 4;

	// Find LLN0

	inst = atoi(pln->ln.ldinst);
	while ((pln) && (pln->ln.prefix) && (inst == atoi(pln->ln.ldinst))) pln=pln->l.prev;

	// Make text menu
	while ((pln) && (inst == atoi(pln->ln.ldinst))){
		ptxt = get_textbylnclass(pln->ln.lnclass);
		if ((ptxt) && (!strstr(txtmenu, ptxt))){
			ts_sprintf(pmenu, "menu %d %d a a %s\n", x, y, ptxt);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
		}
		pln = pln->l.next;
	}

	*pmenu = 0;

	return txtmenu;
}

char* ChangeDate(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
time_t *timel = &tmptime;
struct tm *timetm = localtime(timel);
int i, x, y, maxday, wday, day;

	wday = (7 + timetm->tm_wday - (timetm->tm_mday % 7)) % 7;
	maxday = mons[timetm->tm_mon].mdays;
	// Correct for 'Visokosny' year
	if ((timetm->tm_mon == 1) && (!(timetm->tm_year % 4))) maxday++;

    // Months and years
	ts_sprintf(pmenu, "main 10 35 140 124\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "keys up:dateup down:datedown right:dateright left:dateleft enter:dateenter\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 20 0 100 a %s %d\n", mons[timetm->tm_mon].mrus, 1900+timetm->tm_year);
	pmenu += strlen(pmenu);


	// Days of week
	x = 2;
	for (i = 0; i < 7; i++){
		ts_sprintf(pmenu, "text %d 16 20 a %s\n", x, days[i]);
		pmenu += strlen(pmenu);
		x += 20;
	}

	// Empty days of old month in offset of x
	x = 20 * wday + 2; y = MENUSTEP * 2;
	// Numbers to end of first line days
	day = 1;
	for (i = wday; i < 7; i++){
		ts_sprintf(pmenu, "menu %d %d %d a %d\n", ((day < 10) ? x+6 : x), y, ((day < 10) ? 9 : 16), day);
		pmenu += strlen(pmenu);
		x += 20; day++;
	}

	// Numbers of other lines
	while(day <= maxday){
		x = 2;
		y += MENUSTEP;
		for (i = 0; (i < 7) && (day <= maxday); i++, day++){
			ts_sprintf(pmenu, "menu %d %d %d a %d\n", ((day < 10) ? x+6 : x), y, ((day < 10) ? 9 : 16), day);
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

char* ChangeTime(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
time_t *timel = &tmptime;
struct tm *timetm = localtime(timel);

    // Months and years
	ts_sprintf(pmenu, "main 10 50 140 60\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "keys up:timeup down:timedown right:timeright left:timeleft enter:timeenter\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text 10 0 160 a Установка времени\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 30 25 10 a %d\n", timetm->tm_hour/10);
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 40 25 10 a %d\n", timetm->tm_hour%10);
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text 50 25 10 a :\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 60 25 10 a %d\n", timetm->tm_min/10);
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 70 25 10 a %d\n", timetm->tm_min%10);
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text 80 25 10 a :\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 90 25 10 a %d\n", timetm->tm_sec/10);
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "menu 100 25 10 a %d\n", timetm->tm_sec%10);
	pmenu += strlen(pmenu);

	return txtmenu;
}

char* ChangeTimeMain(char *arg){
	tmptime = maintime;
	return ChangeTime(arg);
}

char* ChangeTimeJour(char *arg){
	tmptime = jourtime;
	return ChangeTime(arg);
}

char* SetDateMain(char *arg){
struct tm *ttm;
menu* actmenu = ((menu*)arg);
int day = atoi(actmenu->pitems[actmenu->num_item]->text);

	ttm	= localtime(&tmptime);
	if (day) ttm->tm_mday = day;

	maintime = mktime(ttm);

	return 0;
}

char* SetDateJour(char *arg){
struct tm *ttm;
menu* actmenu = ((menu*)arg);
int day = atoi(actmenu->pitems[actmenu->num_item]->text);

	ttm	= localtime(&tmptime);
	if (day) ttm->tm_mday = day;

	jourtime = mktime(ttm);

	return 0;
}

char* SetTimeMain(char *arg){
struct  tm *ttm = localtime(&maintime);
menu* actmenu = ((menu*)arg);

	ttm->tm_hour = atoi(actmenu->pitems[1]->text) * 10 + atoi(actmenu->pitems[2]->text);
	ttm->tm_min = atoi(actmenu->pitems[4]->text) * 10 + atoi(actmenu->pitems[5]->text);
	ttm->tm_sec = atoi(actmenu->pitems[7]->text) * 10 + atoi(actmenu->pitems[8]->text);

	maintime = mktime(ttm);

	return 0;
}

char* SetTimeJour(char *arg){
struct  tm *ttm = localtime(&jourtime);
menu* actmenu = ((menu*)arg);

	ttm->tm_hour = atoi(actmenu->pitems[1]->text) * 10 + atoi(actmenu->pitems[2]->text);
	ttm->tm_min = atoi(actmenu->pitems[4]->text) * 10 + atoi(actmenu->pitems[5]->text);
	ttm->tm_sec = atoi(actmenu->pitems[7]->text) * 10 + atoi(actmenu->pitems[8]->text);

	jourtime = mktime(ttm);

	return 0;
}

// Menu of Interval
char* ChangeIntl(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
int i, x = 4, l;

	ts_sprintf(pmenu, "main 10 65 140 60\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "keys right:timeright left:timeleft enter:setinterval\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text 0 0 160 a Установка интервала\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text 50 14 160 a минуты\n");
	pmenu += strlen(pmenu);

	for (i=0; i < 8; i++){
		if (intervals[i] > 9) l = 18;
		else l = 11;
		ts_sprintf(pmenu, "menu %d 40 %d a %d\n", x, l, intervals[i]);
		pmenu += strlen(pmenu);
		x += l + 3;
	}

	return txtmenu;
}

// Menu of Tarif
char* ChangeTarif(char *arg){
char *txtmenu = arg;
char *pmenu = arg;
tarif *ptarif = (tarif*) &ftarif.l.next;
int y = 0;

	ts_sprintf(pmenu, "main 2 60 156 100\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "keys enter:settarif\n");
	pmenu += strlen(pmenu);
	ts_sprintf(pmenu, "text 12 %d a a Выбор тарифа\n", y);
	pmenu += strlen(pmenu);
	y += MENUSTEP;
	ts_sprintf(pmenu, "menu 2 %d a a Тариф - - %s\n", y, ptarif->name);
	pmenu += strlen(pmenu);
	ptarif = ptarif->l.next;
	y += MENUSTEP;
	while(ptarif){
		ts_sprintf(pmenu, "menu 2 %d a a Тариф %d - %s\n", y, ptarif->id, ptarif->name);
		pmenu += strlen(pmenu);
		y += MENUSTEP;
		ptarif = ptarif->l.next;
	}

	*pmenu = 0;

	return txtmenu;
}

// Array of structs "synonym to function"
fact menufactset[] = {
		{"lnmenu", (void*) ChangeLN, NULL},
		{"lntypemenu", (void*) ChangeLNType, NULL},
		{"date", (void*) ChangeDate, NULL},					// For temporary operations with date
		{"maindate", (void*) ChangeDateMain, (void*) SetDateMain},			// Initial operation by main date
		{"jourdate", (void*) ChangeDateJour, (void*) SetDateJour},			// Initial operation by journal date
		{"maintime", (void*) ChangeTimeMain, (void*) SetTimeMain},			// Initial operation by main date
		{"jourtime", (void*) ChangeTimeJour, (void*) SetTimeJour},			// Initial operation by journal date
		{"interval", (void*) ChangeIntl, NULL},
		{"tarif", (void*) ChangeTarif, NULL},
};

//---*** External IP ***---//

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen
static char *txtmenu;
static char idxmenu = -1;
menu* create_menu(char *menuname){
struct stat fst;			// statistics of file
FILE *fmcfg;               //file open
size_t clen;					// lenght of file or array
char *pmenu;
menu *psmenu;
int i;

	// It's memory part will be realloc after the txtmenu building and malloc for new menu structure
//	premenu = malloc(2048);
//	premenu[0] = ' ';
//fst.st_size = 2048;
	// Stat of file, for get size of file
	if (stat(menuname, &fst)){
		txtmenu = malloc(4096);
		txtmenu[0] = 0;
	}else{
		if (S_ISREG(S_IFREG)){
			// Load file to static buffer
			txtmenu = malloc((size_t)(fst.st_size + 2));
			fmcfg = fopen(menuname, "r");
			clen = fread(txtmenu + 1, 1, (size_t) (fst.st_size), fmcfg);
			txtmenu[0] = 0;
			txtmenu[clen+1] = 0;
			fclose(fmcfg);
		}
	}

	// Find and run proloque
	pmenu = strstr(menuname,"/");
	if (pmenu) pmenu++;
	else pmenu = menuname;

	for (i=0; i < sizeof(menufactset) / sizeof(fact); i++){
		if (!strcmp(menufactset[i].action, pmenu)){
			menufactset[i].proloque(txtmenu + strlen(txtmenu) + 1);
			idxmenu = i;
			break;
		}
	}

	// Create menu structures
	psmenu = do_openfilemenu(txtmenu);
	//	txtmenu set as menu.ptxtmenu and free in destroy_menu

	return psmenu;
}

void call_epiloque(menu *actmenu){

	if ((idxmenu != -1) && (menufactset[(int)idxmenu].epiloque))
									menufactset[(int)idxmenu].epiloque(actmenu);

}
