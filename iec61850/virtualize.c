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
#include "../common/ts_print.h"
#include "../common/asdu.h"
#include "../common/iec61850.h"
#include "../common/paths.h"
#include "../common/varcontrol.h"
#include "log_db.h"

#define VIRT_ASDU_MAXSIZE 	512

/* Timer parameters */
static uint8_t alarm_t = ALARM_PER;
static time_t time_dev = 0;

/* RTC precision adjustment variables and constants */
static time_t time_adj = 0;

#define RTC_DEV_NAME		"/dev/rtc1"
#define TIME_DEV_L_LIMIT	5*60
#define TIME_DEV_H_LIMIT	29*60
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
	uint32_t log_rec[LOG_FLD_NUM];
} SCADA_CH;

typedef struct _SCADA_ASDU{
	LIST l;
	VIRT_ASDU *pscada;
} SCADA_ASDU;

// Variables for asdu actions
static LIST fasdu, fasdutype, fdm, fscada, fscadach;

// Last varrec for datasets control
//static varrec *lastvarrec = NULL;


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

		ts_printf(STDOUT_FILENO, "IEC61850: System message EP_MSG_DCOLL_START sent. Address = %d.\n", sch->ASDUaddr);

		sch = sch->l.next;
	}

	// set log_db timers initial values
	log_db_init_timers();
}

// --- Time function ---
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

int is_scada(uint32_t adr)
{
	SCADA_ASDU *scd = (SCADA_ASDU *) fscada.next;

	while(scd)
	{
		if(atoi(scd->pscada->myln->ln.ldinst) == adr) return 1;
		scd = scd->l.next;
	}

	return 0;
}

void catch_alarm(int sig)
{
	struct timeval cor_time;
	time_t rtc_time, sys_time;
	int res, i;
	SCADA_CH *sch;

	sys_time = time(NULL);

	if(time_dev != 0)
	{
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

	for(i=0; i<LOG_SERV_PRM_SIZE; i++)
	{
		if(log_serv_prm_list[i].add_timer > 0 && difftime(sys_time, log_serv_prm_list[i].add_timer) >= log_serv_prm_list[i].add_period)
		{
			sch = (SCADA_CH*) fscadach.next;
			while(sch)
			{
				if(!is_scada(sch->ASDUaddr))
				{
					log_db_add_var_rec(sch->ASDUaddr, sch->log_rec, log_serv_prm_list[i].db_req, log_serv_prm_list[i].db_flds, log_serv_prm_list[i].flds_num);
				}
				sch = sch->l.next;
			}
			// update timer
			log_serv_prm_list[i].add_timer = sys_time;
		}
	}

	for(i=0; i<LOG_SERV_PRM_SIZE; i++)
	{
		if(log_serv_prm_list[i].export_timer > 0 && difftime(sys_time, log_serv_prm_list[i].export_timer) >= log_serv_prm_list[i].export_period)
		{
			char file_name[64] = {0};

			log_db_export_data(log_serv_prm_list[i].db_req, sys_time, file_name);

			log_db_clean_old_data(log_serv_prm_list[i].db_req, sys_time, log_serv_prm_list[i].storage_deep);

			// update timer
			log_serv_prm_list[i].export_timer = sys_time;
		}
	}

	signal(sig, catch_alarm);
	alarm(alarm_t);
}

int sync_time(time_t srv_time)
{
	struct timeval tv;
	time_t sys_time, rtc_time, time_diff;
	int res, p_adj_cur, p_adj;

	if(time_adj != 0 && srv_time - time_adj > ADJ_TIME_DIFF)
	{
		res = get_rtc_time(RTC_DEV_NAME, &rtc_time);

		if(res == -1) return res;

		// calculation of precision adjustment according to the RX8025 SA/NB application manual
		p_adj = (int)( ( (float)(rtc_time - time_adj)/(float)(srv_time - time_adj)*(float)(CLOCK_FREQ) - (float)(CLOCK_FREQ) ) / (float)(CLOCK_FREQ) * 1e9 ) * (-1);

		// get current precision adjustment value
		res = get_rtc_adj(RTC_DEV_NAME, &p_adj_cur);

		if(p_adj != 0 && res != -1)
		{
			ts_printf(STDOUT_FILENO, "IEC61850: RTC precision adjustment needed. rtc_time = %d, srv_time = %d, p_adj_cur = %d, p_adj = %d.\n", (int)rtc_time, (int)srv_time, p_adj_cur, p_adj);

			p_adj += p_adj_cur;

			res = set_rtc_adj(RTC_DEV_NAME, p_adj);

			if(res != -1)
			{
				ts_printf(STDOUT_FILENO, "IEC61850: RTC precision adjustment %s.\n", res?"failed":"succeeded");

				time_adj = srv_time;
			}
		}
	}
	else if(time_adj == 0)
	{
		time_adj = srv_time;
	}

	res = set_rtc_time(RTC_DEV_NAME, srv_time);

	ts_printf(STDOUT_FILENO, "IEC61850: RTC synchronization with master host %s.\n", res?"failed":"succeeded");

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

			ts_printf(STDOUT_FILENO, "IEC61850: STC synchronization with master host %s. Time diff = %d.\n", res?"failed":"succeeded", (int)time_diff);
		}
		else
		{
			time_dev = sys_time - srv_time;

			ts_printf(STDOUT_FILENO, "IEC61850: Time adjusting started. time_dev = %d\n", (int)time_dev);
		}
	}

	return res;
}

// --- End Time function

// --- rcvdata call this function

char *add_header2scada(char *buff, ep_data_header *edh){
data_unit *pdu = (data_unit*) (buff + sizeof(data_unit));
ep_data_header *pdh = (ep_data_header*) buff;

	pdh->adr = 0;
	pdh->sys_msg = EP_USER_DATA;
	pdh->len = sizeof(asdu);

	return (char*) pdu;
}

char *add_header2hmi(char *buff){
varevent *pve = (varevent*) (buff + sizeof(varevent));
ep_data_header *pdh = (ep_data_header*) buff;

	pdh->adr = IDHMI;
	pdh->len = 0;
	pdh->sys_msg = EP_MSG_BOOKEVENT;

	return (char*) pve;
}

data_unit *add_asdu(char *buff, asdu* pasdu){
asdu *psasdu =  (asdu*) buff;

	memcpy(psasdu, pasdu, sizeof(asdu));
	psasdu->size = 0;

	return (data_unit*) (psasdu + sizeof(asdu));
}

// Function find data_unit into map table and add remapped data_unit to SCADA frame
// In: pointer to data_unit from LD, pointer to pointer to new data_unit for SCADA, income ep_data_header
// Return:
// If data_unit was added to frame as new remapped data_unit for SCADA => return 1;
// If data_unit.id wasn't found in map table and don't added to frame for SCADA => return 0;
uint32_t add_dataunit(data_unit *pdu, data_unit **pspdu, ep_data_header *edh){
VIRT_ASDU *sasdu = (VIRT_ASDU*) fasdu.next;
data_unit *spdu = *pspdu;
SCADA_CH *actscadach;
ASDU_DATAMAP *pdm;
asdu *pasdu = (asdu*) (edh + 1);

uint32_t  fld_idx;

	if (pdu->id <= (VIRT_ASDU_MAXSIZE - 4)){

		// save current data unit value for log_db
		if(strlen(pdu->name) > 0){
			actscadach = (SCADA_CH*) fscadach.next;
			while(actscadach && actscadach->ASDUaddr != pasdu->adr)	actscadach = actscadach->l.next;

			if(actscadach){
				// SCADA_CH was found
				fld_idx = log_db_get_fld_idx(pdu->name);
				if(fld_idx >= 0){
					actscadach->log_rec[fld_idx] = pdu->value.ui;
				}
			}
		}

		// Mapping id
		pdm = (ASDU_DATAMAP*) fdm.next;
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
					(!strcmp(pdm->mydobj->dobj.pmynodetype->id, sasdu->myln->ln.lntype))) )
			{
				sasdu = sasdu->l.next;
			}

			if(sasdu){
				memcpy(spdu, pdu, sizeof(data_unit));
				spdu->id =  sasdu->baseoffset + pdm->scadaid;
				(*pspdu)++; 	// Next pdu
//				ts_printf(STDOUT_FILENO, "IEC61850: Value = 0x%X. id %d map to SCADA_ASDU id %d (%d). Time = %d\n", pdu->value.ui, pdm->meterid, pdm->scadaid, pdu->id, pdu->time_tag);
				return 1;
			}else{
				ts_printf(STDOUT_FILENO, "IEC61850 error: Address ASDU %d not found\n", edh->adr);
			}
		}
		else{
			ts_printf(STDOUT_FILENO, "IEC61850 error: Value = 0x%X. id %d don't map to SCADA_ASDU id. Time = %d\n", pdu->value.ui, pdu->id, pdu->time_tag);
		}
	}
	else{
		ts_printf(STDOUT_FILENO, "IEC61850 error: id %d very big\n", pdu->id);
	}


	return 0;
}

uint32_t add_varevent(data_unit *pdu, varevent **ppve, ep_data_header *edh){
varrec* actvr;
varevent *pve = *ppve;
asdu *pasdu = (asdu*) (edh + 1);

		// Find varrec and set event
		actvr = vc_getfirst_varrec();
		while(actvr){
			if (actvr->asdu != pasdu->adr){
				actvr = actvr->l.next;
				continue;
			}
			if (actvr->id == pdu->id){
				actvr->time = pdu->time_tag;
				*((float*)(actvr->val->val)) = pdu->value.f;

//				ts_printf(STDOUT_FILENO, "IEC61850!!!: Variable id %d was find as %s  \n", actvr->id, actvr->name->fc);
				if (actvr->prop & BOOKING){
					pve->value.f = pdu->value.f;
					pve->time = pdu->time_tag;
					pve->vallen = sizeof(float);
					(*ppve)++; 	// Next varevent
//					ts_printf(STDOUT_FILENO, "IEC61850!!!: Variable id %d was send to HMI \n", actvr->id);

					return 1;
				}
			}
			actvr = actvr->l.next;
		}

		return 0;
}

data_unit *get_next_dataunit(data_unit *pdu, ep_data_header *edh){
int32_t len = (uint32_t) pdu - (uint32_t) edh - sizeof(ep_data_header) - sizeof(asdu);

	if (len > 0) return pdu++;

	return 0;
}

// Send data to all registered SCADA_CHs
uint32_t send_asdu2scada(char *sendbuf, uint32_t len){
ep_data_header *sedh = (ep_data_header*) sendbuf;
asdu *psasdu = (asdu*) (sedh + 1);
SCADA_ASDU *actscada;
uint32_t cnt = 0;

	sedh->len = sizeof(asdu) + sizeof(data_unit) * len;
	psasdu->size = len;

	if (len){
		actscada = (SCADA_ASDU*) fscada.next;
		while(actscada){
			sedh->adr = atoi(actscada->pscada->myln->ln.pmyld->inst);
			psasdu->adr = sedh->adr;
			mf_toendpoint(sendbuf, sizeof(ep_data_header) + sedh->len, sedh->adr, DIRDN);
			actscada = actscada->l.next;
			cnt++;
		}
	}

	return cnt;
}

uint32_t send_varevent2hmi(char *sendbuf, uint32_t len){
ep_data_header *eph = (ep_data_header*) sendbuf;
uint32_t cnt;

	eph->len = sizeof(varevent) * len;


	cnt = mf_toendpoint(sendbuf, eph->len + sizeof(ep_data_header), IDHMI, DIRUP);
	if (cnt == -1) return 0;

	return ((cnt - sizeof(ep_data_header)) / sizeof(varevent));
}

// 3. Find LN by Name
varrec *find_varrecbyname(char* pname){
varrec *vr;
LNODE *ln = (LNODE*) fln.next;

	return vr;
}

uint32_t get_logvarevent(char *pname, varbook *vb, varevent **pve){
varevent *ve = *pve;

	return 0;
}

uint32_t get_actvarevent(varrec *actvr, varbook *vb, varevent **pve){
varevent *ve = *pve;

	return 0;
}

char *get_logstring(char *pname){
char *lstr;

	return lstr;
}


int rcvdata(int len){
static uint32_t Transaction = 0;
char *buff;
int offset;
ep_data_header *edh;
char *senddm, *sendve;
varevent *pve;
data_unit *pdu;
data_unit *pdm;
int cntdm, cntve, cntdu;
int adr, dir, fullrdlen;

// For EP_MSG_BOOK & UNBOOK
LNODE *actln;
varrec *actvr;
varbook *avb;
varevent *actve;
uint32_t adr1;
char lnname[40];
char *pname;
int recjour;
time_t time;
uint32_t valtype;
float val;

	buff = malloc(len);

	if(!buff) return -1;

	fullrdlen = mf_readbuffer(buff, len, &adr, &dir);

	ts_printf(STDOUT_FILENO, "IEC61850: Transaction %d\n", Transaction++);
	ts_printf(STDOUT_FILENO, "IEC61850: Data received . Address = %d, Length = %d %s.\n", adr, len, dir == DIRDN? "from down" : "from up");

	// set offset to zero before loop
	offset = 0;

	while(offset < fullrdlen){
		if(fullrdlen - offset < sizeof(ep_data_header)){
			ts_printf(STDOUT_FILENO, "IEC61850 error: Found not full ep_data_header\n");
			free(buff);
			return 0;
		}

		edh = (ep_data_header *) (buff + offset);				// set start structure

		switch(edh->sys_msg){
		case EP_USER_DATA:

			// Making up Buffer for send to SCADA_ASDU.
			// It has data objects having mapping only
			senddm = malloc(len);
			sendve = malloc(len);

			// Init of  buffers
			pdu = (data_unit*) ((uint32_t) edh + sizeof(ep_data_header) + sizeof(asdu));		// Income data_units
			pdm = (data_unit*) add_header2scada(senddm, edh);			// Outgoing data_units
			memcpy( (char*) pdm, 										// ASDU for sending
					(char*) ((uint32_t) edh + sizeof(ep_data_header)),  // Receiving ASDU
					sizeof(asdu));
			pve = (varevent*) add_header2hmi(sendve);					// Outgoing varevents
			pdm = add_asdu((char*) pdm, (asdu*)(edh + sizeof(ep_data_header)));
			cntdm = 0; cntve = 0; cntdu = 0;

			// Remap and Event cycle
			while (pdu){
				cntdm += add_dataunit(pdu, &pdm, edh);
				cntve += add_varevent(pdu, &pve, edh);
				pdu = get_next_dataunit(pdu, edh);
				cntdu++;
			}

			// Print Statistics and send data to MF
			ts_printf(STDOUT_FILENO, "IEC61850: Statistics:\n");
			ts_printf(STDOUT_FILENO, "IEC61850: receive %d data_units\n", cntdu);
			if (cntdm){
				ts_printf(STDOUT_FILENO, "IEC61850: sent to %d SCADA %d data_unit(s)\n", cntdm, send_asdu2scada(senddm, cntdm));
			}
			if (cntve){
				cntve = send_varevent2hmi(sendve, cntve);
				if (cntve) ts_printf(STDOUT_FILENO, "IEC61850: sent to HMI %d varevent(s)\n", cntve);
				else ts_printf(STDOUT_FILENO, "IEC61850 error: MF channel to HMI closed\n");
			}
			ts_printf(STDOUT_FILENO, "\n");

			// Free buffers
			free(sendve);
			free(senddm);

			break;

		case EP_MSG_TIME_SYNC:
			sync_time( *(time_t*)(buff + offset + sizeof(ep_data_header)) );

			break;

		case EP_MSG_BOOK:

			// Init pointers
			avb = (varbook*) ((int32_t) edh + sizeof(ep_data_header));
			pname = (char*) ((int32_t) avb + sizeof(varbook));

			ts_printf(STDOUT_FILENO, "IEC61850: set subscribe for value %s\n", pname);

			actve = malloc(sizeof(varevent));

			// Detect LOG or ACTUAL variable
			if (strstr(pname, "(")){
				// Get varevent from journal by name
				if (get_logvarevent(pname, avb, &actve)){
					// If log record not found then break subscribe process
					free (actve);
					break;
				}
			}else{
				// Get varevent by name
				actvr = find_varrecbyname(pname);
				get_actvarevent(actvr, avb, &actve);
			}

			// Create send buffer
			sendve = malloc(sizeof(ep_data_header) + sizeof(varevent) + actve->vallen);
			memcpy((char*) sendve + sizeof(ep_data_header), (char*) actve, sizeof(varevent));
			if (actve->vallen) memcpy((char*) sendve + sizeof(ep_data_header) + sizeof(varevent),
									          get_logstring(pname), actve->vallen);

			free(actve);
			send_varevent2hmi((char*) sendve, 1);	// Send 1 varevent to HMI

//
//			adr1 = atoi(pname);
//			actln = fln.next;
//			// Find first LN with equal ldinst
//			while((actln) && (atoi(actln->ln.ldinst) != adr1)) actln = actln->l.next;
//
//			// Find LN with other equal parameters
//			while(actln){
//					sprintf(lnname, "%s/%s.%s.%s", actln->ln.ldinst, actln->ln.prefix, actln->ln.lnclass, actln->ln.lninst);
//					pname[avb->lenname] = 0;
//					if (strstr(pname, lnname)) break;
//					actln = actln->l.next;
//			}
//
//			ts_printf(STDOUT_FILENO, "IEC61850: found LN %s\n", lnname);
//
//			pname = strstr(pname, "(");
//			if (pname == NULL){
//				// Find varrec for this variable
//				actvr = vc_getfirst_varrec();
//				while(actvr){
//					if (actvr->asdu != atoi(actln->ln.ldinst)){
//						actvr = actvr->l.next;
//						continue;
//					}
//					if (actvr->id == avb->id) break;
//					actvr = actvr->l.next;
//				}
//				if (actvr){
//					actvr->uid = avb->uid;
//					actvr->prop |= BOOKING;
//					valtype = actvr->val->idtype;
//					val = *((float*)(actvr->val->val));
//					time = actvr->time;
//					ts_printf(STDOUT_FILENO, "IEC61850: asdu %s of %s has booked to HMI \n", actln->ln.ldinst, actvr->name->fc);
//				}
//			}else{
////				// Call Journal constant and return by multififo
////				recjour = atoi(pname+1);
////				time = atoi(((varbook*) (buff + offset))->time);
////				jour_getvar(actln->ln.ldinst, recjour, time);
//
//				time = 435345386;
//				valtype = avb->type;
//				val = 104;
//			}
//
			// Send variable to HMI
//			len = sizeof(varevent);
//			sendbuff = malloc(len);
//			sedh = (ep_data_header*) sendbuff;
//			ave = (varevent*) sendbuff;
//			sedh->adr = IDHMI;
//			sedh->sys_msg = EP_MSG_BOOKEVENT;
//			sedh->len = len - sizeof(ep_data_header);
//			ave->uid = avb->uid;
//			ave->time = time;
//
//			// By type
//			ave->vallen = sizeof(float);
//			ave->value.f = val;
//
//			mf_toendpoint(sendbuff, len, sedh->adr, DIRUP);
//			free(sendbuff);

			break;

		case EP_MSG_UNBOOK:
			pname = buff + offset + sizeof(varbook);
			ts_printf(STDOUT_FILENO, "IEC61850: set unsubscribe for dataset from value %s\n", pname);
			pname = strstr(pname, "LN:");

			actvr = vc_getfirst_varrec();
			if (actvr) actvr->prop &= ~BOOKING;

			ts_printf(STDOUT_FILENO, "IEC61850: all varrec was unsubscribed\n");

			break;

		}

		// move over the data
		offset += sizeof(ep_data_header) + edh->len;
	}

	free(buff);

	return 0;
}

static int asdu_parser(void){
SCADA_ASDU *actscada;
SCADA_CH *actscadach;
VIRT_ASDU *actasdu;
VIRT_ASDU_TYPE *actasdutype;
ASDU_DATAMAP *actasdudm;

LDEVICE *ald;
LNODE *aln;
LNTYPE *alnt;
DOBJ *adobj;

	ts_printf(STDOUT_FILENO, "ASDU: Start ASDU mapping to parse\n");

	// Create VIRT_ASDU_TYPE list
	actasdutype = (VIRT_ASDU_TYPE*) &fasdutype;
	actasdudm = (ASDU_DATAMAP*) &fdm;
	alnt = (LNTYPE*) flntype.next;
	while(alnt){

		actasdutype = (VIRT_ASDU_TYPE*) create_next_struct_in_list((LIST*) actasdutype, sizeof(VIRT_ASDU_TYPE));

		actasdutype->fdmap = NULL;

		ts_printf(STDOUT_FILENO, "ASDU: new VIRT_ASDU_TYPE for LNTYPE id=%s \n", alnt->lntype.id);

		// Fill VIRT_ASDU_TYPE
		actasdutype->mylntype = alnt;

		// create ASDU_DATAMAP list
		adobj = actasdutype->mylntype->lntype.pfdobj; // (DOBJ*) fdo.next;

		while(adobj){
			// check if DOBJ belongs to _LNODETYPE
			if(adobj->dobj.pmynodetype == &alnt->lntype){
				if (adobj->dobj.options){
						// creating new DATAMAP and filling
						actasdudm = (ASDU_DATAMAP*) create_next_struct_in_list((LIST*) actasdudm, sizeof(ASDU_DATAMAP));

						// if point fdmap to first datamap found
						if(actasdutype->fdmap == NULL) actasdutype->fdmap = actasdudm;

						// Fill ASDU_DATAMAP
						actasdudm->mydobj = adobj;
						actasdudm->scadaid = atoi(adobj->dobj.options);
						if (!vc_get_map_by_name(adobj->dobj.name, &actasdudm->meterid)){
							// find by DOType->DA.name = mag => DOType->DA.btype
							actasdudm->value_type = vc_get_type_by_name("mag", adobj->dobj.type);
							ts_printf(STDOUT_FILENO, "ASDU: new SCADA_ASDU_DO for DOBJ name=%s type=%s: %d =>moveto=> %d by type=%d\n",
									adobj->dobj.name, adobj->dobj.type, actasdudm->meterid, actasdudm->scadaid, actasdudm->value_type);
						}else ts_printf(STDOUT_FILENO, "ASDU: new SCADA_ASDU_DO for DOBJ error: Tag not found into mainmap.cfg\n");
				}else ts_printf(STDOUT_FILENO, "ASDU: new SCADA_ASDU_DO for DOBJ (without mapping) name=%s type=%s\n", adobj->dobj.name, adobj->dobj.type);
			}
			// Next DOBJ
			adobj = adobj->l.next;
		}

		ts_printf(STDOUT_FILENO, "ASDU: ready VIRT_ASDU_TYPE for LNTYPE id=%s \n", alnt->lntype.id);

		alnt = alnt->l.next;
	}

	// Create  SCADA_CH list
	actscadach = (SCADA_CH*) &fscadach;
	ald = (LDEVICE*) fld.next;
	while(ald)
	{
		if(ald->ld.inst){
			actscadach = (SCADA_CH*) create_next_struct_in_list((LIST*) actscadach, sizeof(SCADA_CH));

			// Fill SCADA_CH
			actscadach->myld = ald;
			actscadach->ASDUaddr = atoi(ald->ld.inst);
			memset(actscadach->log_rec, 0, sizeof(actscadach->log_rec)); // initialize array members with zeros

			ts_printf(STDOUT_FILENO, "ASDU: ready SCADA_CH addr=%d for LDEVICE inst=%s \n", actscadach->ASDUaddr, ald->ld.inst);
		}

		ald = ald->l.next;
	}

	// Create VIRT_ASDU and SCADA_ASDU lists
	actasdu = (VIRT_ASDU*) &fasdu;
	actscada = (SCADA_ASDU*) &fscada;
	aln = (LNODE*) fln.next;
	while(aln){
		if (aln->ln.lninst){
			actasdu = (VIRT_ASDU*) create_next_struct_in_list((LIST*) actasdu, sizeof(VIRT_ASDU));

			// Fill VIRT_ASDU
			actasdu->myln = aln;
			actasdu->baseoffset = atoi(aln->ln.lninst) * IEC104_CHLENGHT;

			// If 'scada', create SCADA_ASDU
//			if (strstr(actasdu->myln->ln.iedname, "scada")){
			if (atoi(actasdu->myln->ln.lninst) >= SLAVE_START_INST && strstr(actasdu->myln->ln.lnclass, "MMXU")){
				actscada = (SCADA_ASDU*) create_next_struct_in_list((LIST*) actscada, sizeof(SCADA_ASDU));
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
				ts_printf(STDOUT_FILENO, "ASDU: VIRT_ASDU %s.%s.%s linked to TYPE %s\n",
						actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass, actasdutype->mylntype->lntype.id);
			}
			else ts_printf(STDOUT_FILENO, "ASDU: VIRT_ASDU %s.%s.%s NOT linked to TYPE\n", actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass);

			// Link ASDU_TYPE to DATAMAP


			ts_printf(STDOUT_FILENO, "ASDU: new VIRT_ASDU offset=%d for LN name=%s.%s.%s type=%s ied=%s\n",
					actasdu->baseoffset, aln->ln.ldinst, aln->ln.lninst, aln->ln.lnclass, aln->ln.lntype, aln->ln.iedname);
		}

		aln = aln->l.next;
	}

	ts_printf(STDOUT_FILENO, "ASDU: End ASDU mapping\n");

	return 0;
}

// Load data to low level
static void create_alldo(void){
VIRT_ASDU *sasdu = NULL;
ASDU_DATAMAP *pdm = NULL;
int adr;
frame_dobj fr_do = FRAME_DOBJ_INITIALIZER;

	// Setup of unitlinks for getting DATA OBJECTS
	// get VIRT_ASDU => get LN_TYPE => get DATA_OBJECT list => write list to unitlink
	sasdu = (VIRT_ASDU *) fasdu.next;
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

void create_varctrl(void){
LNODE *actln = (LNODE*) fln.next;
DOBJ *actdo;
char *doname;
int i;
varrec *vr;

	while(actln){
		if (actln->ln.pmytype){
			actdo = actln->ln.pmytype->pfdobj;
			for (i = 0; i < actln->ln.pmytype->maxdobj; i++){
				// Addon '.mag.f' is temporary
				doname = malloc(strlen(actdo->dobj.name)  + 10);
				ts_sprintf(doname, "LN:%s.mag.f", actdo->dobj.name);
				vr = vc_addvarrec(actln, doname, NULL);
				vr->prop &= ~BOOKING;
				actdo = actdo->l.next;
			}
		}
		actln = actln->l.next;
	}
}

int virt_start(char *appname){
int ret = -1;
pid_t chldpid;
char *pchld_app_end;

SCADA_CH *sch = (SCADA_CH *) &fscadach;
char *chld_app;
//
	if (vc_init()) exit(NO_CONFIG_FILE);
	else{
		// Building mapping meter asdu to cid asdu
		if (asdu_parser()) ret = -1;
		else{
			// Run multififo
			chldpid = mf_init(getpath2fifomain(), appname, rcvdata);
			ret = chldpid;
		}
	}

	// Create places for incoming data
	create_varctrl();

	ts_printf(STDOUT_FILENO, "\n--- Configuration ready --- \n\n");

//	//	Execute all low level application for devices by LDevice (SCADA_CH)
	sch = sch->l.next;
	while(sch){

		ts_printf(STDOUT_FILENO, "\n--------------\nIEC Virt: execute for LDevice asdu = %s\n", sch->myld->ld.inst);

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
		mf_newendpoint(sch->ASDUaddr, chld_app, getpath2fifoul(), 0);

		free(chld_app);
		sleep(1);	// Delay for forming next level endpoint

		sch = sch->l.next;
	}

	create_alldo();

	start_collect_data();

	return ret;
}
