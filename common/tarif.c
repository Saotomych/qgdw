/*
 * tarif.c
 *
 *  Created on: 21.02.2012
 *      Author: Alex AVAlon
 */

#include "common.h"
#include "xml.h"
#include "tarif.h"

// Start points for tariffes
LIST fseason, fspecs, ftarif, fhighday;

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
		printf("IEC61850: malloc error:%d - %s\n",errno, strerror(errno));
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
	flasthighset = flasthighday->myhgdays;
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
	flastseason->mywrdays = create_next_struct_in_list(&(flastseason->mywrdays->l), sizeof(LIST));
	flastseason->myhldays = create_next_struct_in_list(&(flastseason->myhldays->l), sizeof(LIST));
	flastworkset = flastseason->mywrdays;
	flastholiset = flastseason->myhldays;

	printf("Tarif: new season: id=%d name=%s date=%d.%d\n",
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

}

void add_holiday(const char *pTag){

}

void add_highday(const char *pTag){

}

void create_set(const char *pTag){
sett *actset;

	if (actdaymode == WORKDAY) actset = flastworkset;
	if (actdaymode == HOLIDAY) actset = flastholiset;
	if (actdaymode == HIGHDAY) actset = flasthighset;

	actset = create_next_struct_in_list(&(actset->l), sizeof(sett));

}

void create_tarif(const char *pTag){

}

int tarif_parser(char *filename){
char *tfile;
FILE *fcid;
int cidlen, ret = 0;
struct stat fst;

	// Get size of main config file
	if (stat(filename, &fst) == -1){
		printf("Tarif: Config file not found\n");
	}

	tfile = malloc(fst.st_size);

	// Loading main config file
	fcid = fopen(filename, "r");
	cidlen = fread(tfile, 1, (size_t) (fst.st_size), fcid);
	if(!strstr(tfile, "</Tarifs>"))
	{
		printf("Tarif: config file is incomplete\n");
		exit(1);
	}
	if (cidlen == fst.st_size) XMLSelectSource(tfile);
	else ret = -1;

	return ret;
}

