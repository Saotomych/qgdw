/*
 * tarif.c
 *
 *  Created on: 21.02.2012
 *      Author: Alex AVAlon
 */

#include "common.h"
#include "xml.h"
#include "tarif.h"
#include "ts_print.h"

// Start points for tariffes
LIST fseason, fspecs, fhighday;
tarif ftarif;

season *flastseason = (season*) &fseason;
specday *flastspec = (specday*) &fspecs ;
tarif *flasttarif = (tarif*) &ftarif;
highday *flasthighday = (highday*) &fhighday;
sett *flastworkset, *flastholiset, *flasthighset;

int actdaymode = 0;

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *new;
	plist->next = malloc(size);

	if (!plist->next){
		ts_printf(STDOUT_FILENO, "IEC61850: malloc error:%d - %s\n",errno, strerror(errno));
		exit(3);
	}
	new = plist->next;
	new->prev = plist;
	new->next = 0;
	return new;
}

// In: p - pointer to string
// Out: key - pointer to string with key of XML file
//      par - pointer to string par for this key without ""
static char* get_next_parameter(char *p, char **key, char **par){
int mode=0;

	*key = p;
	while (*p != '>'){
		if (!mode){
			// Key mode
			if (*p == '='){
				mode = 1;	// switch to par mode
				*p = 0;
			}
		}else{
			// Par mode
			if (*p == '"'){
				if (mode == 2){
					*p = 0;
					return p+1;		// key + par ready
				}
				if (mode == 1){
					mode = 2;
					*par = p+1;
				}
			}
		}
		p++;
	}

	return 0;
}

void tarifvars_init(const char *pTag){
	flasthighday = create_next_struct_in_list(&(flasthighday->l), sizeof(season));
	flasthighset = (sett*) &flasthighday->myhgdays;
	memset(&ftarif, 0, sizeof(tarif));
}

void create_season(const char *pTag){
char *p;
char *key=0, *par=0;

	flastseason = create_next_struct_in_list(&(flastseason->l), sizeof(season));

	// Parse parameters for season
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "name")) flastseason->name = par;
			else
			if (strstr((char*) key, "id")) flastseason->id = atoi(par);
			else
			if (strstr((char*) key, "date")){
				flastseason->day = atoi(par);
				flastseason->mnth = atoi(par+3);
			}
		}
	}while(p);

	// create first LISTs for work and holidays
//	flastseason->mywrdays = create_next_struct_in_list(&(flastseason->mywrdays->l), sizeof(LIST));
//	flastseason->myhldays = create_next_struct_in_list(&(flastseason->myhldays->l), sizeof(LIST));
	flastworkset = (sett*) &flastseason->mywrdays;
	flastholiset = (sett*) &flastseason->myhldays;

	ts_printf(STDOUT_FILENO, "Tarif: new season: id=%d name=%s date=%02d.%02d\n",
			flastseason->id, flastseason->name, flastseason->day, flastseason->mnth);

}

void start_workdaysset(const char *pTag){
	actdaymode = WORKDAY;
}

void start_holidaysset(const char *pTag){
	actdaymode = HOLIDAY;
}

void start_highdaysset(const char *pTag){
	actdaymode = HIGHDAY;
}

void add_workday(const char *pTag){
char *p;
char *key=0, *par=0;

	flastspec = create_next_struct_in_list(&(flastspec->l), sizeof(specday));
	flastspec->daytype = WORKDAY;

	// Parse parameters for workday
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "id")) flastspec->id = atoi(par);
			else
			if (strstr((char*) key, "date")){
				flastspec->day = atoi(par);
				flastspec->mnth = atoi(par+3);
			}
		}
	}while(p);

	ts_printf(STDOUT_FILENO, "Tarif: new special working day: id=%d date=%02d.%02d\n", flastspec->id, flastspec->day, flastspec->mnth);

}

void add_holiday(const char *pTag){
char *p;
char *key=0, *par=0;

	flastspec = create_next_struct_in_list(&(flastspec->l), sizeof(specday));
	flastspec->daytype = HOLIDAY;

	// Parse parameters for workday
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "id")) flastspec->id = atoi(par);
			else
			if (strstr((char*) key, "date")){
				flastspec->day = atoi(par);
				flastspec->mnth = atoi(par+3);
			}
		}
	}while(p);

	ts_printf(STDOUT_FILENO, "Tarif: new special holiday: id=%d date=%02d.%02d\n", flastspec->id, flastspec->day, flastspec->mnth);

}

void add_highday(const char *pTag){
char *p;
char *key=0, *par=0;

	flastspec = create_next_struct_in_list(&(flastspec->l), sizeof(specday));
	flastspec->daytype = HIGHDAY;

	// Parse parameters for workday
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "id")) flastspec->id = atoi(par);
			else
			if (strstr((char*) key, "date")){
				flastspec->day = atoi(par);
				flastspec->mnth = atoi(par+3);
			}
		}
	}while(p);

	ts_printf(STDOUT_FILENO, "Tarif: new special highday day: id=%d date=%02d.%02d\n", flastspec->id, flastspec->day, flastspec->mnth);
}

void create_set(const char *pTag){
sett *actset;
char *p;
char *key=0, *par=0;

	if (actdaymode == WORKDAY){
		flastworkset = create_next_struct_in_list(&(flastworkset->l), sizeof(sett));
		actset = flastworkset;
	}

	if (actdaymode == HOLIDAY){
		flastholiset = create_next_struct_in_list(&(flastholiset->l), sizeof(sett));
		actset = flastholiset;
	}

	if (actdaymode == HIGHDAY){
		flasthighset = create_next_struct_in_list(&(flasthighset->l), sizeof(sett));
		actset = flasthighset;
	}

	actset->mytarif = NULL;

	// Parse parameters for set
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "idtarif")) actset->idtarif = atoi(par);
			else
			if (strstr((char*) key, "id")) actset->id = atoi(par);
			else
			if (strstr((char*) key, "time")){
				actset->hour = atoi(par);
				actset->min = atoi(par+3);
			}
		}
	}while(p);

	ts_printf(STDOUT_FILENO, "Tarif: new set for season %d - '%s': id=%d time=%02d:%02d idtarif=%d\n",
			flastseason->id, flastseason->name, actset->id, actset->hour, actset->min, actset->idtarif);

}

void create_tarif(const char *pTag){
char *p;
char *key=0, *par=0;

	flasttarif = create_next_struct_in_list(&(flasttarif->l), sizeof(season));

	// Parse parameters for tarif
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "name")) flasttarif->name = par;
			else
			if (strstr((char*) key, "id")) flasttarif->id = atoi(par);
			else
			if (strstr((char*) key, "money")) flasttarif->money = atof(par);
		}
	}while(p);

	ts_printf(STDOUT_FILENO, "Tarif: new tarif: id=%d name=%s money=%f\n",
			flasttarif->id, flasttarif->name, flasttarif->money);

}

int tarif_parser(char *filename){
char *tfile;
FILE *fcid;
int cidlen, ret = 0;
struct stat fst;

	// Get size of main config file
	if (stat(filename, &fst) == -1){
		ts_printf(STDOUT_FILENO, "Tarif: Config file not found\n");
	}

	tfile = malloc(fst.st_size);

	// Loading main config file
	fcid = fopen(filename, "r");
	cidlen = fread(tfile, 1, (size_t) (fst.st_size), fcid);
	if(!strstr(tfile, "</Tarifs>"))
	{
		ts_printf(STDOUT_FILENO, "Tarif: config file is incomplete\n");
		exit(1);
	}
	if (cidlen == fst.st_size) XMLSelectSource(tfile);
	else ret = -1;

	connect_2tarif();

	return ret;
}

void connect_2tarif(){
season *acts = (season*) fseason.next;
sett *actwset;
sett *acthset;
tarif *actt;

	while(acts){

		actwset = (sett*) acts->mywrdays.next;
		while(actwset){
			actt = (tarif*) &ftarif.l.next;
			while((actt) && (actt->id != actwset->idtarif))	actt = actt->l.next;
			actwset->mytarif = actt;

			ts_printf(STDOUT_FILENO, "Tarif: Workday set id=%d connect to tarif id=%d (0x04%X)\n", actwset->id, actt->id, (int) actwset->mytarif);

			actwset = actwset->l.next;
		}

		acthset = (sett*) acts->myhldays.next;
		while(acthset){
			actt = (tarif*) &ftarif.l.next;
			while((actt) && (actt->id != acthset->idtarif))	actt = actt->l.next;
			acthset->mytarif = actt;

			ts_printf(STDOUT_FILENO, "Tarif: Holiday set id=%d connect to tarif id=%d (0x04%X)\n", acthset->id, actt->id, (int) acthset->mytarif);

			acthset = acthset->l.next;
		}

		acts = acts->l.next;
	}

	acthset = ((highday*) (fhighday.next))->myhgdays.next;
	while(acthset){
		actt = (tarif*) &ftarif.l.next;
		while((actt) && (actt->id != acthset->idtarif))	actt = actt->l.next;
		acthset->mytarif = actt;

		ts_printf(STDOUT_FILENO, "Tarif: Highday set id=%d connect to tarif id=%d (0x04%X)\n", acthset->id, actt->id, (int) acthset->mytarif);

		acthset = acthset->l.next;
	}
}
