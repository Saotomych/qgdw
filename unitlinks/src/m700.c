/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <linux/input.h>
#include "../include/m700.h"
#include "../../common/ts_print.h"


/* End-point extensions array */
static m700_ep_ext *ep_exts[MAXEP] = {0};

/* list for mapping data identifiers */
static asdu_map *map_list = NULL;

/* Request-Response frame buffer variables */
static time_t			timer_recv = 0;				/* timer for full response from device */
static uint16_t			t_recv = RECV_TIMEOUT;		/* timeout for full response from device */
static unsigned char	recv_buff[RECV_BUFF_SIZE];	/* buffer for receiving frame chunks */
static uint32_t		 	recv_buff_len = 0;			/* used length of receive frame buffer */

/* Data collection variables */
static time_t			timer_dcoll = 0;			/* data collection timer */
static uint16_t			t_dcoll_p = DCOLL_PER;		/* period for data collection */
static uint8_t			dcoll_stopped = 1;			/* data collection state flag */
static int32_t			dcoll_ep_idx = -1;			/* current ep_ext index collector working with */
static int32_t			dcoll_data_idx = -1;		/* current data identifier array's index collector working with */

/* Timer parameters */
static uint8_t alarm_t	= ALARM_PER;

static uint8_t t_rc		= M700_T_RC;

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

static int fev0 = 0;               // ITMI event file open

static char mf_buffer[sizeof(struct ep_init_header)];

int main(int argc, char *argv[])
{
	pid_t chldpid;
	uint16_t res;
	char *fname;
	int ret, evlen;
	ep_data_header *edh;
	static struct ep_init_header *eih;	// Init header from multififo
	static struct input_event *iets;	// Input event from multififo

	init_allpaths();
	mf_semadelete(getpath2fifomain(), APP_NAME);

	fname = malloc(strlen(getpath2configs()) + strlen(APP_CFG) + 1);
	strcpy(fname, getpath2configs());
	strcat(fname, APP_CFG);
	res = m700_config_read(fname);
	free(fname);

	if(res != RES_SUCCESS){
		ts_printf(STDOUT_FILENO, TS_INFO, "%s: Configuration not found...\n", APP_NAME);
		exit(1);
	}

	fname = malloc(strlen(getpath2configs()) + strlen(APP_MAP) + 1);
	strcpy(fname, getpath2configs());
	strcat(fname, APP_MAP);
	res = asdu_map_read(&map_list, fname, APP_NAME, HEX_BASE);
	free(fname);

	if(res != RES_SUCCESS){
		ts_printf(STDOUT_FILENO, TS_INFO, "%s: Variable map not found...\n", APP_NAME);
		exit(1);
	}

	fev0 = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
	evlen = 1;
	if (fev0 == -1){
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_INFO, "%s: Event driver hasn't open\n", APP_NAME);
#endif
		fev0 = 0;
		evlen = 0;
	}

	chldpid = mf_init(getpath2fifoul(), APP_NAME, m700_recv_data);

	signal(SIGALRM, m700_catch_alarm);
	alarm(alarm_t);

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Waiting for end-point initialization end event...\n", APP_NAME);
#endif

	do
	{
		ret = mf_waitevent(mf_buffer, sizeof(mf_buffer), 0, &fev0, evlen);

		if(!ret)
		{
			mf_exit();
			exit(0);
		}

		if (ret == 1)
		{
			// start forward endpoint
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Forward endpoint DIRDN\n", APP_NAME);
#endif
			eih = (ep_init_header*) mf_buffer;
			mf_newendpoint(eih->addr, CHILD_APP_NAME, getpath2fifophy(), 1);

			m700_sys_msg_send(EP_MSG_CONNECT, eih->addr, DIRDN, NULL, 0);

#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_CONNECT sent. Address = %d\n", APP_NAME, eih->addr);
#endif
		}

#ifdef _DEBUG
		if(ret == 2) ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: mf_waitevent timeout\n", APP_NAME);
#endif

		if (ret == FDSETPOS)
		{
			iets = (struct input_event *) mf_buffer;
			if (iets->code >= BTN_TRIGGER_HAPPY)
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "M700: ITMI event: %X, %X, %X\n", iets->value, iets->type, iets->code);
#endif
				if(ep_exts[0])
				{
					// Create asdu frame for LocX
					edh = (ep_data_header*) make_tsasdu(iets, ep_exts[0]->adr);

					// send ITMI event to startiec
					mf_toendpoint((char*) edh, sizeof(ep_data_header) + edh->len, edh->adr, DIRUP);
				}
			}
		}

	}while(!appexit);

	mf_exit();

	exit(0);
}


uint16_t m700_config_read(const char *file_name)
{
	FILE *cfg_file = NULL;
	char r_buff[256] = {0};
	char *prm;
	uint16_t adr, ep_num = 0;
	m700_ep_ext *ep_ext = NULL;

	cfg_file = fopen(file_name, "r");

	if(cfg_file)
	{
		while( fgets(r_buff, 255, cfg_file) )
		{
			if(strstr(r_buff, "#")) continue;

			prm = strstr(r_buff, "name");

			if(prm && strstr(prm, APP_NAME))
			{
				adr = atoi(r_buff);

				ep_ext = m700_add_ep_ext(adr);

				if(!ep_ext) continue;

				prm = strstr(r_buff, "addr");

				if(prm) ep_ext->adr_link =	atoi(prm+5);

				ep_num++;
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: New ep_ext added. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			}
		}
	}
	else
	{
		return RES_UNKNOWN;
	}

	fclose(cfg_file);

	if(ep_num)
		return RES_SUCCESS;
	else
		return RES_NOT_FOUND;
}


void m700_catch_alarm(int sig)
{
	time_t cur_time = time(NULL);
	int i;

	// check request-response timer
	if(timer_recv > 0 && difftime(cur_time, timer_recv) >= t_recv)
	{
		// tired of waiting for response from device

		// stop request timer
		timer_recv = 0;
		recv_buff_len = 0;

#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Timer req went off.\n", APP_NAME);
#endif

		// if collection is in progress force to collect data from the next device
		if(!dcoll_stopped && timer_dcoll == 0)
		{
			dcoll_data_idx = -1;

			m700_collect_data();
		}
	}

	// check data collection timer
	if(timer_dcoll > 0 && difftime(cur_time, timer_dcoll) >= t_dcoll_p)
	{
		// stop data collection timer and wait for collection to finish
		timer_dcoll = 0;

#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Data Collection started.\n", APP_NAME);
#endif

		m700_collect_data();
	}

	// check devices timers
	for(i=0;i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
			if(ep_exts[i]->timer_rc > 0 && difftime(cur_time, ep_exts[i]->timer_rc) >= t_rc)
			{
				m700_init_ep_ext(ep_exts[i]);

				m700_sys_msg_send(EP_MSG_RECONNECT, ep_exts[i]->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Timer rc went off. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_RECONNECT sent. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif
			}
		}

	}

	signal(sig, m700_catch_alarm);

	alarm(alarm_t);
}


int m700_recv_data(int len)
{
	char *buff = NULL;
	int adr, dir;
	uint32_t offset;
	ep_data_header *ep_header_in = NULL;

	buff = (char*) malloc(len);

	if(!buff) return -1;

	len = mf_readbuffer(buff, len, &adr, &dir);

	if(len <= 0)
	{
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_INFO, "%s: ERROR - Data received with Length = %d! Something went wrong!\n", APP_NAME, len);
#endif

		free(buff);
		return -1;
	}

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Data received. Address = %d, Length = %d, Direction = %s.\n", APP_NAME, adr, len, dir == DIRDN ? "DIRUP" : "DIRDN");
#endif

	offset = 0;

	while(offset < len)
	{
		if(len - offset < sizeof(ep_data_header))
		{
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_INFO, "%s: ERROR - Looks like ep_data_header missed.\n", APP_NAME);
#endif

			free(buff);
			return -1;
		}

		ep_header_in = (ep_data_header*)(buff + offset);
		offset += sizeof(ep_data_header);

#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Received ep_data_header - adr = %d, sys_msg = %d, len = %d.\n", APP_NAME, ep_header_in->adr, ep_header_in->sys_msg, ep_header_in->len);
#endif

		if(ep_header_in->len < 0 || len - offset < ep_header_in->len)
		{
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_INFO, "%s: ERROR - Expected data length %d bytes, received %d bytes.\n", APP_NAME, ep_header_in->len, len - offset);
#endif

			free(buff);
			return -1;
		}

		if(dir == DIRUP)
		{
			// direction is from DIRUP to DIRDN
			if(ep_header_in->sys_msg == EP_USER_DATA)
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: User data in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_asdu_recv((unsigned char*)(buff + offset), ep_header_in->len, ep_header_in->adr);
			}
			else
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_sys_msg_recv(ep_header_in->sys_msg, ep_header_in->adr, dir, (unsigned char*)(buff + offset), ep_header_in->len);
			}
		}
		else
		{
			// direction is from DIRDN to DIRUP
			if(ep_header_in->sys_msg == EP_USER_DATA)
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: User data in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_frame_recv((unsigned char*)(buff + offset), ep_header_in->len, ep_header_in->adr);

			}
			else
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_sys_msg_recv(ep_header_in->sys_msg, ep_header_in->adr, dir, (unsigned char*)(buff + offset), ep_header_in->len);
			}
		}

		offset += ep_header_in->len;
	}

	free(buff);

	return 0;
}


void m700_test_connect(uint16_t adr)
{
	int i;
	uint32_t *data_ids_new = NULL;

	for(i=0; i<MAXEP; i++)
	{
		if(!ep_exts[i]) continue;

		data_ids_new = (uint32_t*) realloc((void*)ep_exts[i]->data_ids, sizeof(uint32_t) * (ep_exts[i]->data_ids_size + 1));

		if(!data_ids_new) return;

		ep_exts[i]->data_ids = data_ids_new;

		ep_exts[i]->data_ids[ep_exts[i]->data_ids_size] = 0x37;
		ep_exts[i]->data_ids_size++;

	#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: New DOBJ was added. Address = %d, m700_id = 0x%02x\n", APP_NAME, ep_exts[i]->adr, ep_exts[i]->data_ids[ep_exts[i]->data_ids_size-1]);
	#endif
	}

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Connection test started. Address = all\n", APP_NAME);
#endif

	// allow data collection
	dcoll_stopped = 0;
	m700_collect_data();
}


m700_ep_ext* m700_get_ep_ext(uint16_t adr, uint8_t get_by)
{
	int i;

	for(i=0; i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
			if(get_by == M700_ASDU_ADR)
			{
				if(ep_exts[i]->adr == adr) return ep_exts[i];
			}
			else
			{
				if(ep_exts[i]->adr_link == adr) return ep_exts[i];
			}
		}
	}

	return NULL;
}


int m700_get_ep_ext_idx(uint16_t adr, uint8_t get_by)
{
	int i;

	for(i=0; i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
			if(get_by == M700_ASDU_ADR)
			{
				if(ep_exts[i]->adr == adr) return i;
			}
			else
			{
				if(ep_exts[i]->adr_link == adr) return i;
			}
		}
	}

	return -1;
}


m700_ep_ext* m700_add_ep_ext(uint16_t adr)
{
	int i;
	m700_ep_ext *ep_ext = NULL;

	ep_ext = m700_get_ep_ext(adr, M700_ASDU_ADR);

	if(ep_ext) return ep_ext;

	ep_ext = (m700_ep_ext*) calloc(1, sizeof(m700_ep_ext));

	if(!ep_ext) return NULL;

	ep_ext->adr           = adr;

	ep_ext->data_ids      = NULL;
	ep_ext->data_ids_size = 0;

	for(i=0; i<MAXEP; i++)
	{
		if(!ep_exts[i])
		{
			ep_exts[i] = ep_ext;

			return ep_ext;
		}
	}

	return NULL;
}


void m700_init_ep_ext(m700_ep_ext* ep_ext)
{
	if(!ep_ext) return;

	// set data transfer state
	ep_ext->tx_ready = 0;

	// stop all timers
	ep_ext->timer_rc = 0;

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: ep_ext (re)initialized. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
}


uint16_t m700_add_dobj_item(m700_ep_ext* ep_ext, uint32_t dobj_id, unsigned char *dobj_name)
{
	if(!ep_ext) return RES_INCORRECT;

	uint32_t *data_ids_new = NULL;
	uint32_t m700_id;

	asdu_map *res_map = asdu_get_map_item(&map_list, dobj_id, BASE_ID);

	if(!res_map)
	{
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_VERBOSE, "%s: ERROR - Received DOBJ was ignored - no map found. Address = %d, dobj_id = %d, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, dobj_id, dobj_name);
#endif

		return RES_INCORRECT;
	}

	m700_id = (res_map->proto_id >> 8) - 0x80;

	if(m700_get_dobj_item(ep_ext, m700_id) == RES_SUCCESS)
	{
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_VERBOSE, "%s: DOBJ already exists. Address = %d, m700_id = 0x%04x, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, res_map->proto_id, dobj_name);
#endif
		return RES_SUCCESS;
	}

	data_ids_new = (uint32_t*) realloc((void*)ep_ext->data_ids, sizeof(uint32_t) * (ep_ext->data_ids_size + 1));

	if(!data_ids_new) return RES_MEM_ALLOC;

	ep_ext->data_ids = data_ids_new;

	ep_ext->data_ids[ep_ext->data_ids_size] = m700_id;
	ep_ext->data_ids_size++;

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: New DOBJ was added. Address = %d, m700_id = 0x%04x, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, res_map->proto_id, dobj_name);
#endif

	return RES_SUCCESS;
}


uint16_t m700_get_dobj_item(m700_ep_ext* ep_ext, uint32_t m700_id)
{
	int i;

	for(i=0; i<ep_ext->data_ids_size; i++)
	{
		if(ep_ext->data_ids[i] == m700_id) return RES_SUCCESS;
	}

	return RES_NOT_FOUND;
}


uint16_t m700_collect_data()
{
	// check data collection state
	if(dcoll_stopped || timer_dcoll > 0) return RES_INCORRECT;

	uint16_t res;

	m700_ep_ext* ep_ext = NULL;

	if(dcoll_data_idx == -1)
	{
		// look for the next ep_ext
		while(!ep_ext)
		{
			dcoll_ep_idx++;

			if(dcoll_ep_idx >= MAXEP)
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: m700_collect_data - reached end of ep_exts array\n", APP_NAME);
#endif

				// reached end of array, time to wait for next data collection
				dcoll_ep_idx = -1;

				ep_ext = NULL;

				// start data collection timer
				timer_dcoll = time(NULL);

				break;
			}

			ep_ext = ep_exts[dcoll_ep_idx];
		}

#ifdef _DEBUG
		if(ep_ext && ep_ext->tx_ready)
		{
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Collecting data from - Address = %d, ep_idx = %d\n", APP_NAME, ep_ext->adr, dcoll_ep_idx);
		}
#endif
	}
	else
	{
		// continue collect data from current ep_ext
		ep_ext = ep_exts[dcoll_ep_idx];
	}

	if(!ep_ext) return RES_INCORRECT;

	dcoll_data_idx++;

	if(!ep_ext->tx_ready || dcoll_data_idx >= ep_ext->data_ids_size)
	{
		// data transfer is not ready or reached end of array - collect data from the next device
		dcoll_data_idx = -1;

		// forced to use recursion, is it bad?
		m700_collect_data();
	}
	else
	{
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: m700_collect_data - data_idx = %d. Address = %d\n", APP_NAME, dcoll_data_idx, ep_ext->adr);
#endif

		// device and data identifier were found! device is ready for data transfer! let's get the data from it!
		res = m700_read_data_send(ep_ext->adr, ep_ext->data_ids[dcoll_data_idx], 0, 0);
	}

	return res;
}


uint16_t m700_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len)
{
	int res;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	ep_header.adr     = adr;
	ep_header.sys_msg = sys_msg;
	ep_header.len     = buff_len;

	if(buff_len == 0)
	{
		res = mf_toendpoint((char*) &ep_header, sizeof(ep_data_header), adr, dir);
	}
	else
	{
		ep_buff = (char*) malloc(sizeof(ep_data_header) + buff_len);

		if(!ep_buff) return RES_MEM_ALLOC;

		memcpy((void*)ep_buff, (void*)&ep_header, sizeof(ep_data_header));
		memcpy((void*)(ep_buff+sizeof(ep_data_header)), (void*)buff, buff_len);

		res = mf_toendpoint(ep_buff, sizeof(ep_data_header) + buff_len, adr, dir);

		free(ep_buff);
	}

	if(res > 0)
		return RES_SUCCESS;
	else
		return RES_INCORRECT;
}


uint16_t m700_log_msg_send(uint16_t adr, char *msg)
{
	uint16_t res;
	int buf_len;
	char *buf = NULL;

	buf = malloc(strlen(APP_NAME) + strlen(msg) + 5);

	if(!buf) return RES_MEM_ALLOC;

	if(adr > 0)
	{
		buf_len = ts_sprintf(buf, "%s", msg) + 1;
		res = m700_sys_msg_send(EP_MSG_LOG_DEV_EVENT, adr, DIRUP, (unsigned char *)buf, buf_len);
	}
	else if(ep_exts[0])
	{
		buf_len = ts_sprintf(buf, "%s: %s", APP_NAME, msg) + 1;
		res = m700_sys_msg_send(EP_MSG_LOG_APP_EVENT, ep_exts[0]->adr, DIRUP, (unsigned char *)buf, buf_len);
	}

	free(buf);

	return res;
}


uint16_t m700_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len)
{
	m700_ep_ext* ep_ext = NULL;
	frame_dobj *fr_do = NULL;
	int ret;

	ep_ext = m700_get_ep_ext(adr, M700_ASDU_ADR);

	if(!ep_ext) return RES_UNKNOWN;

	if(dir == DIRUP)
	{
		// direction is from DIRUP to DIRDN
		switch(sys_msg)
		{
		case EP_MSG_TEST_CONN:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_TEST_CONN received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			m700_test_connect(ep_ext->adr);

			break;

		case EP_MSG_NEWDOBJ:
			fr_do = (frame_dobj *) (buff - sizeof(ep_data_header));

#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_NEWDOBJ (%d, %s) received. Address = %d\n", APP_NAME, fr_do->id, fr_do->name, ep_ext->adr);
#endif

			m700_add_dobj_item(ep_ext, fr_do->id, (unsigned char*)fr_do->name);

			break;

		case EP_MSG_DCOLL_START:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_DCOLL_START received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			if(dcoll_stopped) // do it only if data collection is stopped
			{
				// allow data collection
				dcoll_stopped = 0;

				// start data collection
				m700_collect_data();
			}

			break;

		case EP_MSG_DCOLL_STOP:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_DCOLL_STOP received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			// disable data collection
			dcoll_stopped = 1;
			timer_dcoll = 0;

			// reset indexes
			dcoll_ep_idx = -1;
			dcoll_data_idx = -1;

			break;

		case EP_MSG_CONNECT_CLOSE:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_CONNECT_CLOSE received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			m700_init_ep_ext(ep_ext);

			break;

		case EP_MSG_QUIT:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_QUIT received.\n", APP_NAME);
#endif

			m700_sys_msg_send(EP_MSG_QUIT, ep_ext->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_QUIT sent. Address = all.\n", APP_NAME);
#endif

			// wait until child app is quit
			for(;;)
			{
				ret = mf_testrunningapp(CHILD_APP_NAME);
				if( ret == 0 || ret == -1 ) break;
				usleep(100000);
			}

			appexit = 1;

			break;

		default:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_VERBOSE, "%s: Warning - unsupported System message (%d) received. Address = %d\n", APP_NAME, sys_msg, ep_ext->adr);
#endif

			break;
		}
	}
	else
	{
		switch(sys_msg)
		{

		case EP_MSG_CONNECT_ACK:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_CONNECT_ACK received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			m700_log_msg_send(ep_ext->adr, "Connection established");

			m700_init_ep_ext(ep_ext);

			// stop timer rc
			ep_ext->timer_rc = 0;

			ep_ext->tx_ready = 1;

			break;

		case EP_MSG_CONNECT_NACK:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_VERBOSE, "%s: System message EP_MSG_CONNECT_NACK received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			m700_log_msg_send(ep_ext->adr, "Connection failed");

			m700_init_ep_ext(ep_ext);

			// start rc timer
			ep_ext->timer_rc = time(NULL);

			break;

		case EP_MSG_CONNECT_LOST:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_CONNECT_LOST received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			m700_log_msg_send(ep_ext->adr, "Connection lost");

			m700_init_ep_ext(ep_ext);

			// start rc timer
			ep_ext->timer_rc = time(NULL);

			break;

		default:
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_VERBOSE, "%s: Warning - unsupported System message (%d) received. Address = %d\n", APP_NAME, sys_msg, ep_ext->adr);
#endif

			break;
		}
	}

	return RES_SUCCESS;
}


uint16_t m700_frame_send(m700_frame *m_fr, uint16_t adr, uint8_t dir)
{
	uint16_t res;
	uint32_t d_len = 0;
	unsigned char *m_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;
	m700_ep_ext *ep_ext = NULL;

	ep_ext = m700_get_ep_ext(adr, M700_ASDU_ADR);

	if(!ep_ext || !ep_ext->tx_ready) return RES_INCORRECT;

	if(m_fr->adr == 0 && ep_ext->adr_link > 0)
	{
		m_fr->adr = ep_ext->adr_link;
	}

	// address request fix
	if(m_fr->cmd == 0x37) m_fr->adr = 0xFF;

	res = m700_frame_buff_build(&m_buff, &d_len, m_fr);

	if(res == RES_SUCCESS)
	{
		ep_buff = (char*) malloc(sizeof(ep_data_header) + d_len);

		if(ep_buff)
		{
			ep_header.adr = adr;
			ep_header.sys_msg = EP_USER_DATA;
			ep_header.len = d_len;

			memcpy((void*)ep_buff, (void*)&ep_header, sizeof(ep_data_header));
			memcpy((void*)(ep_buff+sizeof(ep_data_header)), (void*)m_buff, d_len);

			if(mf_toendpoint(ep_buff, sizeof(ep_data_header)+ d_len, adr, dir) <= 0) res = RES_INCORRECT;

#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: User data in DIRDN sent. Address = %d, Length = %d\n", APP_NAME, ep_ext->adr, ep_header.len);

			char c_buff[512] = {0};
			hex2ascii((unsigned char *)ep_buff+sizeof(ep_data_header), c_buff, + d_len);
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: %s\n", APP_NAME, c_buff);
#endif

			free(ep_buff);
		}
		else
		{
			res = RES_MEM_ALLOC;
		}

		free(m_buff);
	}

	return res;
}


uint16_t m700_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr)
{
	uint16_t res;
	uint32_t offset;
	m700_frame *m_fr = NULL;
	m700_ep_ext *ep_ext = NULL;
	time_t cur_time = time(NULL);

	// check request-response state
	if(difftime(cur_time, timer_recv) >= t_recv || recv_buff_len+buff_len > RECV_BUFF_SIZE)
	{
		timer_recv = 0;
		recv_buff_len = 0;

		return RES_INCORRECT;
	}

	// check if frame chunk from correct address
	if(adr != ep_exts[dcoll_ep_idx]->adr)
	{
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_INFO, "%s: ERROR - Frame chunk in DIRUP ignored. Expected adr = %d , received adr = %d.\n", APP_NAME, ep_exts[dcoll_ep_idx]->adr, adr);
#endif
		return RES_INCORRECT;
	}

	// copy next received chunk to the end of the recv_buffer
	memcpy((void*)(recv_buff+recv_buff_len), (void*)buff, buff_len);
	recv_buff_len += buff_len;
/*
#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, "%s: Frame chunk in DIRUP received. Address = %d, Length = %d\n", APP_NAME, adr, recv_buff_len);

	char c_buff[512] = {0};
	hex2ascii(buff, c_buff, buff_len);
	ts_printf(STDOUT_FILENO, "%s: %s\n", APP_NAME, c_buff);
#endif
*/
	offset = 0;

	m_fr = m700_frame_create();

	if(!m_fr) return RES_MEM_ALLOC;

	res = m700_frame_buff_parse(recv_buff, recv_buff_len, &offset, m_fr);

	if(res == RES_SUCCESS)
	{
#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Frame in DIRUP received. Length = %d\n", APP_NAME, recv_buff_len);

		char c_buff[512] = {0};
		hex2ascii(recv_buff, c_buff, recv_buff_len);
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: %s\n", APP_NAME, c_buff);
#endif

		// reset request-response variables
		timer_recv = 0;
		recv_buff_len = 0;

		ep_ext = m700_get_ep_ext(m_fr->adr, M700_LINK_ADR);

		if(ep_ext)
		{
			// check if frame from correct address
			if(ep_ext->adr != ep_exts[dcoll_ep_idx]->adr)
			{
#ifdef _DEBUG
				ts_printf(STDOUT_FILENO, TS_INFO, "%s: ERROR - Frame in DIRUP ignored. Expected adr = %d , received adr = %d.\n", APP_NAME, ep_exts[dcoll_ep_idx]->adr, ep_ext->adr);
#endif

				res = RES_INCORRECT;
			}
			else
			{
				m700_read_data_recv(m_fr, ep_ext);
			}
		}
		else
		{
			res = RES_INCORRECT;
		}

		if(!dcoll_stopped) m700_collect_data();
	}

	m700_frame_destroy(&m_fr);

	return res;
}


uint16_t m700_asdu_send(asdu *m700_asdu, uint16_t adr, uint8_t dir)
{
	uint16_t res;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	asdu_map_ids(&map_list, m700_asdu, APP_NAME, HEX_BASE);

	res = asdu_to_byte(&a_buff, &a_len, m700_asdu);

	if(res == RES_SUCCESS)
	{
		ep_buff = (char*) malloc(sizeof(ep_data_header) + a_len);

		if(ep_buff)
		{
			ep_header.adr = adr;
			ep_header.sys_msg = EP_USER_DATA;
			ep_header.len = a_len;

			memcpy((void*)ep_buff, (void*)&ep_header, sizeof(ep_data_header));

			memcpy((void*)(ep_buff + sizeof(ep_data_header)), (void*)a_buff, a_len);

			mf_toendpoint(ep_buff, sizeof(ep_data_header) + a_len, adr, dir);

			res = RES_SUCCESS;

			free(ep_buff);

#ifdef _DEBUG
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: ASDU sent in DIRUP. Address = %d, Length = %d\n", APP_NAME, adr, a_len);
#endif
		}
		else
		{
			res = RES_MEM_ALLOC;
		}

		free(a_buff);
	}

	return res;
}


uint16_t m700_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr)
{
	uint16_t res;
	asdu *m700_asdu = NULL;

	m700_ep_ext *ep_ext = NULL;

	ep_ext = m700_get_ep_ext(adr, M700_ASDU_ADR);

	if(!ep_ext) return RES_INCORRECT;

	res = asdu_from_byte(buff, buff_len, &m700_asdu);

	if(res == RES_SUCCESS)
	{
		// TODO finish m700_asdu_recv function
	}
	else
	{
		res = RES_INCORRECT;
	}

	asdu_destroy(&m700_asdu);

	return res;
}


uint16_t m700_read_data_send(uint16_t adr, uint32_t data_id, uint8_t num, time_t start_time)
{
	uint16_t res;
	m700_frame *m_fr = NULL;
	time_t cur_time = time(NULL);

	// check request-response variables
	if(timer_recv > 0 && difftime(cur_time, timer_recv) < t_recv)
	{
		return RES_INCORRECT;
	}
	else
	{
		timer_recv = 0;
		recv_buff_len = 0;
	}

	m_fr = m700_frame_create();

	if(!m_fr)
	{
		res = RES_MEM_ALLOC;
	}
	else
	{
		m_fr->cmd = data_id;

		res = m700_frame_send(m_fr, adr, DIRDN);

		if(res == RES_SUCCESS)
		{
			// set request-response variables
			timer_recv = time(NULL);
			recv_buff_len = 0;
#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Read Data command sent. Address = %d.\n", APP_NAME, adr);
#endif
		}

		m700_frame_destroy(&m_fr);
	}

	return res;
}


uint16_t m700_read_data_recv(m700_frame *m_fr, m700_ep_ext *ep_ext)
{
#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Read Data frame received. Address = %d, Length = %d\n", APP_NAME, ep_ext->adr, m_fr->data_len);
#endif

	uint16_t res;
	uint8_t sseq;
	uint32_t offset;
	asdu *m700_asdu = NULL;

	sseq = offset = 0;

	while(offset < m_fr->data_len)
	{
		m700_asdu = asdu_create();

		if(!m700_asdu) return RES_MEM_ALLOC;

		res = m700_asdu_buff_parse(m_fr, m700_asdu, &offset, &sseq);

		if(res == RES_SUCCESS)
		{
			m700_asdu->adr = ep_ext->adr;

			if(m700_asdu->data->id == 0xB701)
			{
				// send system message if it address info
				res = m700_sys_msg_send(EP_MSG_DEV_ONLINE, ep_ext->adr, DIRUP, NULL, 0);
#ifdef _DEBUG
				if(res == RES_SUCCESS) ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: System message EP_MSG_DEV_ONLINE sent. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			}

			res = m700_asdu_send(m700_asdu, ep_ext->adr, DIRUP);
		}

		asdu_destroy(&m700_asdu);

		if(res == RES_SUCCESS && sseq)
			continue;
		else
			break;
	}

	return RES_SUCCESS;
}


uint16_t m700_read_adr_send(uint16_t adr)
{
	uint16_t res;
	m700_frame *m_fr = NULL;
	time_t cur_time = time(NULL);

	// check request-response variables
	if(timer_recv > 0 && difftime(cur_time, timer_recv) < t_recv)
	{
		return RES_INCORRECT;
	}
	else
	{
		timer_recv = 0;
		recv_buff_len = 0;
	}

	m_fr = m700_frame_create();

	if(!m_fr)
	{
		res = RES_MEM_ALLOC;
	}
	else
	{
		m_fr->cmd = 0x37;

		res = m700_frame_send(m_fr, adr, DIRDN);

		if(res == RES_SUCCESS)
		{
			// set request-response variables
			timer_recv = time(NULL);
			recv_buff_len = 0;

#ifdef _DEBUG
			ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Read address command sent. Address = %d.\n", APP_NAME, adr);
#endif
		}

		m700_frame_destroy(&m_fr);
	}

	return res;
}


uint16_t m700_read_adr_recv(m700_frame *m_fr, m700_ep_ext *ep_ext)
{
#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Read Address frame received. Address = %d, Length = %d\n", APP_NAME, ep_ext->adr, m_fr->data_len);
#endif

	// TODO finish function m700_read_adr_recv()
	return RES_SUCCESS;
}

// Make frame for telesignal data
uint8_t tsframe[sizeof(ep_data_header) + sizeof(asdu) + sizeof(data_unit)];

uint8_t *make_tsasdu(struct input_event *ev, uint16_t adr){
ep_data_header *edh = (ep_data_header*) tsframe;
asdu *pasdu = (asdu*)(tsframe + sizeof(ep_data_header));
data_unit *pdu = (data_unit*) ((char*)pasdu + sizeof(asdu));

	edh->sys_msg = EP_USER_DATA;
	edh->len = sizeof(asdu) + sizeof(data_unit);
	edh->adr = adr;

	pasdu->data = pdu;

	pasdu->adr = adr;	// it's zero, set next
	pasdu->attr = 0;
	pasdu->fnc = COT_Spont; // set to field fnc value cause of transmission - COT_Spont = 3 (spontaneous transmission by IEC101/104 specifications)
	pasdu->proto = PROTO_M700;
	pasdu->size = sizeof(data_unit);
	pasdu->type = M_SP_TB_1;

	ts_sprintf(pdu->name, "TMLoc%d", ev->code - BTN_TRIGGER_HAPPY);
	pdu->attr = 0;
	// FIXME set some real id!!!
	pdu->id = ev->code;
	pdu->time_iv = 0;
	pdu->time_tag = ev->time.tv_sec;
	pdu->value_type = ASDU_VAL_BOOL;

	pdu->value.ui = ev->value ? 1 : 0;

	return tsframe;
}
