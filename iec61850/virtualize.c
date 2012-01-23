/*
 * virtualize.c
 *
 *  Created on: 12.07.2011
 *      Author: alex AVAlon
 *
 *  This module make virtualization procedure for all data_types going from/to unitlinks
 *
 */

#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "../common/common.h"
#include "../common/multififo.h"
#include "../common/asdu.h"
#include "iec61850.h"

#define VIRT_ASDU_MAXSIZE 	512
#define RTC_DEV_NAME		"/dev/rtc1"
#define TIME_DEV_L_LIMIT	5*60
#define TIME_DEV_H_LIMIT	29*60

/* Timer parameters */
static uint8_t alarm_t = ALARM_PER;
static time_t time_dev = 0;

/* RTC precision adjustment variables and constants */
static time_t time_adj = 0;
static int p_adj = 0;

#define ADJ_RESOLUTION		3050		// by the Epson RTC driver
#define CLOCK_FREQ			32768		// by the Epson RTC driver
#define ADJ_TIME_DIFF		6*3600		// 6 hours

// DATA
typedef struct _DATAMAP{
	LIST l;
	DOBJ		*mydobj;
	uint32_t	scadaid;		// scadaid  = position in scada asdu, get from DOBJ.options
	uint32_t	meterid;		// use find_by_int, meterid = position in meter asdu
	uint8_t		value_type; 	// for "on fly" converting, get from DOTYPE.ATTR.stVal <-> DOTYPE.ATTR.btype
} ASDU_DATAMAP;

typedef struct _SCADA_ASDU_TYPE{		// Analog Logical Node Type
	LIST l;
	LNTYPE *mylntype;
	ASDU_DATAMAP	*fdmap;
} VIRT_ASDU_TYPE;

// View dataset as ASDU for virtual IED
typedef struct _VIRT_ASDU{		// Analog Logical Node
	LIST l;
	LNODE *myln;
	uint32_t baseoffset;		// Base offset of meter data in iec104 channel
	VIRT_ASDU_TYPE *myasdutype;
} VIRT_ASDU;

typedef struct _SCADA_CH{
	LIST l;
	LDEVICE *myld;
	uint32_t ASDUaddr;			// use find_by_int, get from IED.options
} SCADA_CH;

typedef struct _SCADA_ASDU{
	LIST l;
	VIRT_ASDU *pscada;
} SCADA_ASDU;

// Pointer to full mapping config as text
char *MCFGfile;

// Variables for asdu actions
static LIST fasdu = {NULL, NULL}, fasdutype = {NULL, NULL}, fdm = {NULL, NULL}, fscada = {NULL, NULL}, fscadach = {NULL, NULL};

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *newlist;
	plist->next = malloc(size);
	if (!plist->next){
		printf("IEC61850: malloc error:%d - %s\n",errno, strerror(errno));
		exit(3);
	}

	newlist = plist->next;
	newlist->prev = plist;
	newlist->next = 0;
	return newlist;
}

// Get mapping parameters from special config file 'mainmap.cfg' of real ASDU frames from meters
int get_map_by_name(char *name, uint32_t *mid){
char *p;

	p = strstr(MCFGfile, name);
	if (!p) return -1;
	while((*p != 0xA) && (p != MCFGfile)) p--;
	while(*p <= '0') p++;
	*mid = atoi(p);

	return 0;
}

// Find DA with name and ptr equals to name and ptr DTYPE
// Return int equal string typedef
int get_type_by_name(char *name, char *type){
DTYPE *adtype;
ATTR *dattr = (ATTR*) &fattr;
char *p1;
int ret = 0;

	// Find DTYPE by type
	adtype = (DTYPE*) fdtype.next;
	while(adtype){
		if (!strcmp(adtype->dtype.id, type)) break;
		adtype = adtype->l.next;
	}

	if (adtype){
		// Find ATTR by name & ptr to type
		dattr = (ATTR*) fattr.next;
		while((dattr) && (!ret)){
			if (&adtype->dtype == dattr->attr.pmydatatype){
				// Own type
				if (!strcmp(dattr->attr.name, name)){
					// Name yes. Detect btype and convert to INT
					p1 = dattr->attr.btype;
					if (strstr(p1, "Struct")) ret = 1;
					else ret = -1;
				}
			}
			dattr = dattr->l.next;
		}
	}else ret = -1;

	return ret;
}

void send_sys_msg(int adr, int msg){
ep_data_header edh;
	edh.adr = adr;
	edh.sys_msg = msg;
	edh.len = 0;
	mf_toendpoint((char*) &edh, sizeof(ep_data_header), adr, DIRDN);
}

void start_collect_data()
{
	SCADA_CH *sch = (SCADA_CH *) fscadach.next;

	//	Execute all low level application for devices by LDevice (SCADA_CH)
	//sch = sch->l.next;
	while(sch)
	{
		send_sys_msg(sch->ASDUaddr, EP_MSG_DCOLL_START);

		printf("IEC61850: System message EP_MSG_DCOLL_START sent. Address = %d.\n", sch->ASDUaddr);

		sch = sch->l.next;
	}
}

int get_rtc_time(const char *dev_name, time_t *t_val)
{
	struct rtc_time rtc_tm;
	struct tm time_tm;

	int fd, res = 0;

	fd = open(dev_name, O_RDONLY);

	if (fd == -1) {
		return fd;
	}

	res = ioctl(fd, RTC_RD_TIME, &rtc_tm);

	if(res != -1) {
		time_tm.tm_hour = rtc_tm.tm_hour;
		time_tm.tm_min = rtc_tm.tm_min;
		time_tm.tm_sec = rtc_tm.tm_sec;
		time_tm.tm_mday = rtc_tm.tm_mday;
		time_tm.tm_mon = rtc_tm.tm_mon;
		time_tm.tm_year = rtc_tm.tm_year;

		*t_val = mktime(&time_tm);
	}

	close(fd);

	return res;
}

int set_rtc_time(const char *dev_name, time_t t_val)
{
	struct rtc_time rtc_tm;
	struct tm *time_tm;

	int fd, res = 0;

	time_tm = localtime(&t_val);

	if(!time_tm) return -1;

	rtc_tm.tm_hour = time_tm->tm_hour;
	rtc_tm.tm_min = time_tm->tm_min;
	rtc_tm.tm_sec = time_tm->tm_sec;
	rtc_tm.tm_mday = time_tm->tm_mday;
	rtc_tm.tm_mon = time_tm->tm_mon;
	rtc_tm.tm_year = time_tm->tm_year;

	fd = open(dev_name, O_RDONLY);

	if (fd == -1) {
		return fd;
	}

	res = ioctl(fd, RTC_SET_TIME, &rtc_tm);

	close(fd);

	return res;
}

int get_rtc_adj(const char *dev_name, int *adj) {
	int fd, res = 0;

	fd = open(dev_name, O_RDONLY);

	if (fd == -1) {
		return fd;
	}

	res = ioctl(fd, RTC_PLL_GET, adj);

	close(fd);

	return res;
}

int set_rtc_adj(const char *dev_name, int adj) {
	int fd, res = 0;

	fd = open(dev_name, O_RDONLY);

	if (fd == -1) {
		return fd;
	}

	res = ioctl(fd, RTC_PLL_SET, &adj);

	close(fd);

	return res;
}

void catch_alarm(int sig)
{
	struct timeval cor_time;
	time_t rtc_time, sys_time;
	int res;

	if(time_dev != 0)
	{
		sys_time = time(NULL);

		res = get_rtc_time(RTC_DEV_NAME, &rtc_time);

		time_dev = sys_time - rtc_time;

		if(res != -1 && time_dev != 0)
		{
			if(time_dev > 0)
			{
				cor_time.tv_sec = sys_time - 1;
				cor_time.tv_usec = 0;
				settimeofday(&cor_time, NULL);
				time_dev--;
			}
			else
			{
				cor_time.tv_sec = sys_time + 1;
				cor_time.tv_usec = 0;
				settimeofday(&cor_time, NULL);
				time_dev++;
			}
		}
	}

	signal(sig, catch_alarm);

	alarm(alarm_t);
}

int sync_time(time_t srv_time)
{
	struct timeval tv;
	time_t sys_time, rtc_time, time_diff;
	int res;

	if(time_adj != 0 && srv_time - time_adj > ADJ_TIME_DIFF)
	{
		res = get_rtc_time(RTC_DEV_NAME, &rtc_time);

		if(res == -1) return res;

		// calculation of precision adjustment according to the RX8025 SA/NB application manual
		p_adj += (int)( ( (float)(rtc_time - time_adj)/(float)(srv_time - time_adj)*(float)(CLOCK_FREQ) - (float)(CLOCK_FREQ) ) / (float)(CLOCK_FREQ) * (float)1000000  * (float)3050 ) * (-1);

		res = set_rtc_adj(RTC_DEV_NAME, p_adj);

		time_adj = srv_time;
	}
	else if(time_adj == 0)
	{
		time_adj = srv_time;
	}

	res = set_rtc_time(RTC_DEV_NAME, srv_time);

	printf("IEC61850: RTC synchronization with master host %s.\n", res?"failed":"succeeded");

	sys_time = time(NULL);

	time_diff = sys_time - srv_time;

	if(!time_diff || time_dev)
		return res;
	else
	{
		if(time_diff < 0) time_diff *= -1;

		if(time_diff < TIME_DEV_L_LIMIT || time_diff > TIME_DEV_H_LIMIT)
		{
			tv.tv_sec  = srv_time;
			tv.tv_usec = 0;

			res = settimeofday(&tv, NULL);

			printf("IEC61850: STC synchronization with master host %s. Time diff = %d.\n", res?"failed":"succeeded", (int)time_diff);
		}
		else
		{
			time_dev = sys_time - srv_time;

			printf("IEC61850: Time adjusting started. time_dev = %d\n", (int)time_dev);
		}
	}

	return res;
}

int rcvdata(int len){
char *buff, *sendbuff;
int adr, dir, rdlen, fullrdlen;
int offset;
ep_data_header *edh, *sedh;
data_unit *pdu, *spdu;
VIRT_ASDU *sasdu = (VIRT_ASDU*) fasdu.next;
asdu *pasdu, *psasdu;
ASDU_DATAMAP *pdm;
SCADA_ASDU *actscada;

	buff = malloc(len);

	if(!buff) return -1;

	fullrdlen = mf_readbuffer(buff, len, &adr, &dir);

	printf("IEC61850: Data received. Address = %d, Length = %d %s.\n", adr, len, dir == DIRDN? "from down" : "from up");

	// set offset to zero before loop
	offset = 0;

	while(offset < fullrdlen){
		if(fullrdlen - offset < sizeof(ep_data_header)){
			printf("IEC61850: Found not full ep_data_header\n");
			free(buff);
			return 0;
		}

		edh = (struct ep_data_header *) (buff + offset);				// set start structure

		switch(edh->sys_msg){
		case EP_USER_DATA:
			rdlen = edh->len;
			pasdu = (asdu*) (buff + offset + sizeof(ep_data_header));
			rdlen -= sizeof(asdu);


			printf("IEC61850: Values for ASDU = %d received\n", edh->adr);

			// TODO broadcast request, etc.

			// Making up Buffer for send to SCADA_ASDU.
			// It has data objects having mapping only
			sendbuff = malloc(len);
			sedh = (ep_data_header*) sendbuff;
			sedh->sys_msg = EP_USER_DATA;
			sedh->len = sizeof(asdu);

			psasdu = (asdu*) (sendbuff + sizeof(ep_data_header));
			memcpy(psasdu, pasdu, sizeof(asdu));
			psasdu->size = 0;
			spdu = (data_unit*) ((void*)psasdu + sizeof(asdu));
			pdu = (void*) pasdu + sizeof(asdu);

			while(rdlen > 0){
				if (pdu->id <= (VIRT_ASDU_MAXSIZE - 4)){
	 				// TODO Copy variable to data struct IEC61850
					// TODO Find type of variable and convert type on fly

					// Mapping id
					pdm = (ASDU_DATAMAP*) &fdm;
					while ((pdm) && (pdm->meterid != pdu->id))
					{
						pdm = pdm->l.next;
					}
					if (pdm){
						// TODO Find type of variable & Convert type on fly
						// Remap variable pdu->id -> id (for SCADA_ASDU)

						// find VIRT_ASDU
						while ( (sasdu) && sasdu->myln->ln.pmyld &&
								!(atoi(sasdu->myln->ln.pmyld->inst) == edh->adr &&
								strstr(pdm->mydobj->dobj.pmynodetype->id, sasdu->myln->ln.lntype)) )
						{
							sasdu = sasdu->l.next;
						}

						if(sasdu){
							pdu->id =  sasdu->baseoffset + pdm->scadaid;
							memcpy(spdu, pdu, sizeof(data_unit));
							spdu++;
							psasdu->size++;
							sedh->len += sizeof(data_unit);
							printf("IEC61850: Value = 0x%X. id %d map to SCADA_ASDU id %d (%d). Time = %d\n", pdu->value.ui, pdm->meterid, pdm->scadaid, pdu->id, pdu->time_tag);
						}
						else{
							printf("IEC61850 error: Address ASDU %d not found\n", edh->adr);
						}
					}
//					else{
//						printf("IEC61850: Value = 0x%X. id %d don't map to SCADA_ASDU id. Time = %d\n", pdu->value.ui, pdu->id, pdu->time_tag);
//					}
				}
//				else{
//					printf("IEC61850 error: id %d very big\n", pdu->id);
//				}
				// Next pdu
				pdu++;
				rdlen -= sizeof(data_unit);
			}

			// Send data to all registered SCADA_CHs
			if (psasdu->size){
				actscada = (SCADA_ASDU*) fscada.next;
				while(actscada){
					sedh->adr = atoi(actscada->pscada->myln->ln.pmyld->inst);
					psasdu->adr = sedh->adr;
					mf_toendpoint(sendbuff, sizeof(ep_data_header) + sedh->len, sedh->adr, DIRDN);
					printf("IEC61850: %d data_units sent to SCADA adr = %d\n", psasdu->size, sedh->adr);
					actscada = actscada->l.next;
				}
			}

			free(sendbuff);

			break;

		case EP_MSG_TIME_SYNC:
			sync_time( *(time_t*)(buff + offset + sizeof(ep_data_header)) );

			break;
		}

		// move over the data
		offset += sizeof(ep_data_header);
		offset += edh->len;
	}

	free(buff);

	return 0;
}

int asdu_parser(void){
SCADA_ASDU *actscada;
SCADA_CH *actscadach;
VIRT_ASDU *actasdu;
VIRT_ASDU_TYPE *actasdutype;
ASDU_DATAMAP *actasdudm;

LDEVICE *ald;
LNODE *aln;
LNTYPE *alnt;
DOBJ *adobj;

	printf("ASDU: Start ASDU mapping to parse\n");

	// Create VIRT_ASDU_TYPE list
	actasdutype = (VIRT_ASDU_TYPE*) &fasdutype;
	actasdudm = (ASDU_DATAMAP*) &fdm;
	alnt = (LNTYPE*) flntype.next;
	while(alnt){

		actasdutype = create_next_struct_in_list((LIST*) actasdutype, sizeof(VIRT_ASDU_TYPE));

		actasdutype->fdmap = NULL;

		printf("ASDU: new VIRT_ASDU_TYPE for LNTYPE id=%s \n", alnt->lntype.id);

		// Fill VIRT_ASDU_TYPE
		actasdutype->mylntype = alnt;

		// create ASDU_DATAMAP list
		adobj = actasdutype->mylntype->lntype.pfdobj; // (DOBJ*) fdo.next;

		while(adobj){
			// check if DOBJ belongs to _LNODETYPE
			if(adobj->dobj.pmynodetype == &alnt->lntype){
				if (adobj->dobj.options){
						// creating new DATAMAP and filling
						actasdudm = create_next_struct_in_list((LIST*) actasdudm, sizeof(ASDU_DATAMAP));

						// if point fdmap to first datamap found
						if(actasdutype->fdmap == NULL) actasdutype->fdmap = actasdudm;

						// Fill ASDU_DATAMAP
						actasdudm->mydobj = adobj;
						actasdudm->scadaid = atoi(adobj->dobj.options);
						if (!get_map_by_name(adobj->dobj.name, &actasdudm->meterid)){
							// find by DOType->DA.name = mag => DOType->DA.btype
							actasdudm->value_type = get_type_by_name("mag", adobj->dobj.type);
							printf("ASDU: new SCADA_ASDU_DO for DOBJ name=%s type=%s: %d =>moveto=> %d by type=%d\n",
									adobj->dobj.name, adobj->dobj.type, actasdudm->meterid, actasdudm->scadaid, actasdudm->value_type);
						}else printf("ASDU: new SCADA_ASDU_DO for DOBJ error: Tag not found into mainmap.cfg\n");
				}else printf("ASDU: new SCADA_ASDU_DO for DOBJ (without mapping) name=%s type=%s\n", adobj->dobj.name, adobj->dobj.type);
			}
			// Next DOBJ
			adobj = adobj->l.next;
		}

		printf("ASDU: ready VIRT_ASDU_TYPE for LNTYPE id=%s \n", alnt->lntype.id);

		alnt = alnt->l.next;
	}

	// Create  SCADA_CH list
	actscadach = (SCADA_CH*) &fscadach;
	ald = (LDEVICE*) fld.next;
	while(ald)
	{
		if(ald->ld.inst){
			actscadach = create_next_struct_in_list((LIST*) actscadach, sizeof(SCADA_CH));

			// Fill SCADA_CH
			actscadach->myld = ald;
			actscadach->ASDUaddr = atoi(ald->ld.inst);

			printf("ASDU: ready SCADA_CH addr=%d for LDEVICE inst=%s \n", actscadach->ASDUaddr, ald->ld.inst);
		}

		ald = ald->l.next;
	}

	// Create VIRT_ASDU and SCADA_ASDU lists
	actasdu = (VIRT_ASDU*) &fasdu;
	actscada = (SCADA_ASDU*) &fscada;
	aln = (LNODE*) fln.next;
	while(aln){
		if (aln->ln.lninst){
			actasdu = create_next_struct_in_list((LIST*) actasdu, sizeof(VIRT_ASDU));

			// Fill VIRT_ASDU
			actasdu->myln = aln;
			actasdu->baseoffset = atoi(aln->ln.lninst) * IEC104_CHLENGHT;

			// If 'scada', create SCADA_ASDU
//			if (strstr(actasdu->myln->ln.iedname, "scada")){
			if (atoi(actasdu->myln->ln.lninst) >= SLAVE_START_INST && strstr(actasdu->myln->ln.lnclass, "MMXU")){
				actscada = create_next_struct_in_list((LIST*) actscada, sizeof(SCADA_ASDU));
				actscada->pscada = actasdu;
			}

			// Link to VIRT_ASDU_TYPE
			// Find LNTYPE.id = VIRT_ASDU_TYPE.LN.lntype
			actasdutype =  (VIRT_ASDU_TYPE*) fasdutype.next;
			while(actasdutype){
				alnt = actasdutype->mylntype;
				if (!strcmp(alnt->lntype.id, aln->ln.lntype)) break;
				actasdutype = actasdutype->l.next;
			}
			if (actasdutype){
				actasdu->myasdutype = actasdutype;
				printf("ASDU: VIRT_ASDU %s.%s.%s linked to TYPE %s\n",
						actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass, actasdutype->mylntype->lntype.id);
			}
			else printf("ASDU: VIRT_ASDU %s.%s.%s NOT linked to TYPE\n", actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass);

			// Link ASDU_TYPE to DATAMAP


			printf("ASDU: new VIRT_ASDU offset=%d for LN name=%s.%s.%s type=%s ied=%s\n",
					actasdu->baseoffset, aln->ln.ldinst, aln->ln.lninst, aln->ln.lnclass, aln->ln.lntype, aln->ln.iedname);
		}

		aln = aln->l.next;
	}

	printf("ASDU: End ASDU mapping\n");

	return 0;
}

// Load data to low level
void create_alldo(void){
VIRT_ASDU *sasdu = (VIRT_ASDU *) &fasdu;
ASDU_DATAMAP *pdm;
int adr;
frame_dobj fr_do;

	// Setup of unitlinks for getting DATA OBJECTS
	// get VIRT_ASDU => get LN_TYPE => get DATA_OBJECT list => write list to unitlink
	sasdu = ((VIRT_ASDU *) &fasdu)->l.next;
	while(sasdu){
		if(sasdu->myln->ln.pmyld) // check if pmyld is not NULL
		{
			adr = atoi(sasdu->myln->ln.pmyld->inst);
			// find logical node type
			pdm = sasdu->myasdutype->fdmap;
			while (pdm && strstr(sasdu->myln->ln.lntype, pdm->mydobj->dobj.pmynodetype->id)){
				// write datatypes by sys msg EP_MSG_NEWDOBJ
				fr_do.edh.adr = adr;
				fr_do.edh.len = sizeof(frame_dobj) - sizeof(ep_data_header);
				fr_do.edh.sys_msg = EP_MSG_NEWDOBJ;
				fr_do.id = pdm->meterid;
				strcpy(fr_do.name, pdm->mydobj->dobj.name);

				// write to endpoint
				mf_toendpoint((char*) &fr_do, sizeof(frame_dobj), fr_do.edh.adr, DIRDN);
				usleep(5);	// delay for forming  next event

				pdm = pdm->l.next;
			}
		}

		sasdu = sasdu->l.next;
	}
}

int virt_start(char *appname){
FILE *fmcfg;
int clen, ret;
struct stat fst;
pid_t chldpid;
char *pchld_app_end;

SCADA_CH *sch = (SCADA_CH *) &fscadach;
char *chld_app;
//
// Read mainmap.cfg into memory
	if (stat("/rw/mx00/configs/mainmap.cfg", &fst) == -1){
		printf("IEC Virt: 'mainmap.cfg' file not found\n");
	}
	MCFGfile =  malloc(fst.st_size);
	fmcfg = fopen("/rw/mx00/configs/mainmap.cfg", "r");
	clen = fread(MCFGfile, 1, (size_t) (fst.st_size), fmcfg);
	if (clen != fst.st_size) ret = -1;
	else{
		// Building mapping meter asdu to ssd asdu
		if (asdu_parser()) ret = -1;
		else{
			// Run multififo
			chldpid = mf_init("/rw/mx00/mainapp", appname, rcvdata);
			ret = chldpid;
		}
	}
	free(MCFGfile);
	printf("\n--- Configuration ready --- \n\n");

	//	Execute all low level application for devices by LDevice (SCADA_CH)
	sch = sch->l.next;
	while(sch){

		printf("\n--------------\nIEC Virt: execute for LDevice asdu = %s\n", sch->myld->ld.inst);

		// Create config_device
		chld_app = malloc(strlen(sch->myld->ld.desc) + 1);
		strcpy(chld_app, sch->myld->ld.desc);

		// By agreement with icd and ssd connection
		// ld.desc = unitlink-name/dispather name - device type
		// Unitlinks will starts as realname
		pchld_app_end = strstr(chld_app, "/");
		if (pchld_app_end) *pchld_app_end = 0;	// Makes Unitlink's name as zero-string
//		pchld_app_end++;	// (in future) equals to beginning of text meter description for HMI

		pchld_app_end = strstr(chld_app, ".");
		if (pchld_app_end) *pchld_app_end = 0;

		// New endpoint
		mf_newendpoint(sch->ASDUaddr, chld_app,"/rw/mx00/unitlinks", 0);

		free(chld_app);
		sleep(1);	// Delay for forming next level endpoint

		sch = sch->l.next;
	}

	create_alldo();

	start_collect_data();

	return ret;
}
