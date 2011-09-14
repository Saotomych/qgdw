/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/m700.h"


/* End-point extensions array */
static m700_ep_ext *ep_exts[MAXEP] = {0};

static m700_map *map_list = NULL;

/* Request-Response frame buffer variables */
static time_t			timer_recv = 0;				/* timer for full response from device */
static uint16_t			t_recv = RECV_TIMEOUT;		/* timeout for full response from device */
static unsigned char	recv_buff[RECV_BUFF_SIZE];	/* buffer for receiving frame chunks */
static uint32_t		 	recv_buff_len = 0;			/* used length of receive frame buffer */

/* Data collection variables */
static time_t			timer_dcoll = 0;			/* data collection timer */
static uint16_t			t_dcoll_p = DCOLL_PER;		/* period for data collection */
static uint16_t			t_dcoll_d = DCOLL_DELAY;	/* delay for data collection start */
static uint8_t			dcoll_stopped = 1;			/* data collection state sign */
static int32_t			dcoll_ep_idx = -1;			/* current ep_ext index collector working with */
static int32_t			dcoll_data_idx = -1;		/* current data identifier array's index collector working with */

/* Timer parameters */
static uint8_t alarm_t	= ALARM_PER;

static uint8_t t_rc		= M700_T_RC;

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

char forname[100];
char forprotoname[100];
char forphyname[100];
struct config_device cd ={
		forname,
		forprotoname,
		forphyname,
		0
};


int main(int argc, char *argv[])
{
	pid_t chldpid;
	uint16_t res;

	int ret;
	struct ep_init_header *eih = 0;

	res = m700_config_read(APP_CFG);

	if(res != RES_SUCCESS) exit(1);

	res = m700_map_read(APP_MAP);

	chldpid = mf_init(APP_PATH, APP_NAME, m700_recv_data);

	signal(SIGALRM, m700_catch_alarm);
	alarm(alarm_t);

	// start data collecting timer with delay for devices initialization
	timer_dcoll = time(NULL) + t_dcoll_d;
	dcoll_stopped = 0;

#ifdef _DEBUG
	printf("%s: Waiting for end-point initialization end event...\n", APP_NAME);
#endif

	do
	{
		ret = mf_waitevent((char*) &eih, sizeof(eih), 0);

		if(!ret)
		{
			mf_exit();
			exit(0);
		}

		if(ret == 1)
		{
			// start forward endpoint
#ifdef _DEBUG
			printf("%s: Forward endpoint DIRDN\n", APP_NAME);
#endif

			cd.addr = eih->addr;
			strncpy(cd.name, eih->isstr[2], 100);
			strncpy(cd.protoname, eih->isstr[3], 100);
			strncpy(cd.phyname, eih->isstr[4], 100);

			mf_newendpoint(&cd, CHILD_APP_PATH, 1);

			m700_sys_msg_send(EP_MSG_CONNECT, cd.addr, DIRDN, NULL, 0);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT sent. Address = %d\n", APP_NAME, cd.addr);
#endif
		}

#ifdef _DEBUG
		if(ret == 2) printf("%s: mf_waitevent timeout\n", APP_NAME);
#endif
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
			if(*r_buff == '#') continue;

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
				printf("%s: New ep_ext added. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
			}
		}
	}
	else
	{
		return RES_UNKNOWN;
	}

	if(ep_num)
		return RES_SUCCESS;
	else
		return RES_NOT_FOUND;
}


uint16_t m700_map_read(const char *file_name)
{
	FILE *map_file = NULL;
	char r_buff[256] = {0};
	uint32_t map_num;
	uint32_t m700_id, base_id;

	map_file = fopen(file_name, "r");

	if(map_file)
	{
		map_num = 0;

		while(fgets(r_buff, 255, map_file))
		{
			if(*r_buff == '#') continue;

			sscanf(r_buff,"%x %d", &m700_id, &base_id);

			m700_add_map_item(m700_id, base_id);

			map_num++;
		}
	}
	else
	{
		return RES_UNKNOWN;
	}

	if(map_num)
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
		printf("%s: Timer req went off.\n", APP_NAME);
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
		printf("%s: Data Collection started.\n", APP_NAME);
#endif

		m700_collect_data();
	}

	// check devices timers
	for(i=0;i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
			// check timer rc
			if(ep_exts[i]->timer_rc > 0 && difftime(cur_time, ep_exts[i]->timer_rc) >= t_rc)
			{
				m700_init_ep_ext(ep_exts[i]);

				m700_sys_msg_send(EP_MSG_RECONNECT, ep_exts[i]->adr, DIRDN, NULL, 0);

	#ifdef _DEBUG
				printf("%s: Timer rc went off. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
				printf("%s: System message EP_MSG_RECONNECT sent. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
	#endif
			}
		}

	}

	signal(sig, m700_catch_alarm);

	alarm(alarm_t);
}


int m700_recv_data(int len)
{
	char *buff;
	int adr, dir;
	uint32_t offset;
	ep_data_header *ep_header_in;

	buff = malloc(len);

	if(!buff) return -1;

	len = mf_readbuffer(buff, len, &adr, &dir);

#ifdef _DEBUG
	printf("%s: Data received. Address = %d, Length = %d, Direction = %s.\n", APP_NAME, adr, len, dir == DIRDN ? "DIRUP" : "DIRDN");
#endif

	offset = 0;

	while(offset < len)
	{
		if(len - offset < sizeof(ep_data_header))
		{
#ifdef _DEBUG
			printf("%s: ERROR - Looks like ep_data_header missed.\n", APP_NAME);
#endif

			free(buff);
			return -1;
		}

		ep_header_in = (ep_data_header*)(buff + offset);
		offset += sizeof(ep_data_header);

#ifdef _DEBUG
		printf("%s: Received ep_data_header - adr = %d, sys_msg = %d, len = %d.\n", APP_NAME, ep_header_in->adr, ep_header_in->sys_msg, ep_header_in->len);
#endif

		if(len - offset < ep_header_in->len)
		{
#ifdef _DEBUG
			printf("%s: ERROR - Expected data length %d bytes, received %d bytes.\n", APP_NAME, sizeof(ep_data_header) + ep_header_in->len, len - offset);
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
				printf("%s: User data in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_asdu_recv((unsigned char*)(buff + offset), ep_header_in->len, ep_header_in->adr);
			}
			else
			{
#ifdef _DEBUG
				printf("%s: System message in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
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
				printf("%s: User data in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_frame_recv((unsigned char*)(buff + offset), ep_header_in->len, ep_header_in->adr);

			}
			else
			{
#ifdef _DEBUG
				printf("%s: System message in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				m700_sys_msg_recv(ep_header_in->sys_msg, ep_header_in->adr, dir, (unsigned char*)(buff + offset), ep_header_in->len);
			}
		}

		offset += ep_header_in->len;
	}

	free(buff);

	return 0;
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
	printf("%s: ep_ext (re)initialized. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
}


uint16_t m700_add_map_item(uint32_t m700_id, uint32_t base_id)
{
	m700_map *last_map, *new_map;

	// try to allocate memory for new map
	new_map = malloc(sizeof(m700_map));

	if(!new_map) return RES_MEM_ALLOC;

	new_map->m700_id = m700_id;
	new_map->base_id = base_id;

	last_map = map_list;

	while(last_map && last_map->next)
	{
		last_map = last_map->next;
	}

	new_map->prev = last_map;
	new_map->next = NULL;

	if(!map_list)
		map_list = new_map;
	else
		last_map->next = new_map;

#ifdef _DEBUG
	printf("%s: New m700_map added. m700_id = 0x%02X, base_id = %d\n", APP_NAME, new_map->m700_id, new_map->base_id);
#endif

	return RES_SUCCESS;
}


m700_map *m700_get_map_item(uint32_t id, uint8_t get_by)
{
	m700_map *res_map;

	res_map = map_list;

	while(res_map)
	{
		if(get_by == M700_ID && res_map->m700_id == id) break;
		if(get_by == BASE_ID && res_map->base_id   == id) break;

		res_map = res_map->next;
	}

	return res_map;
}


void m700_asdu_map_ids(asdu *m700_asdu)
{
	// fast check input data
	if(!m700_asdu) return;

	int i;
	m700_map *res_map;

	for(i=0; i<m700_asdu->size; i++)
	{
		res_map = m700_get_map_item(m700_asdu->data[i].id, M700_ID);

		if(res_map)
			m700_asdu->data[i].id = res_map->base_id;
		else
			m700_asdu->data[i].id = 0xFFFFFFFF;

#ifdef _DEBUG
		if(res_map) printf("%s: Identifier mapped. Address = %d, m700_id = 0x%04X, base_id = %d\n", APP_NAME, m700_asdu->adr, res_map->m700_id, res_map->base_id);
#endif
	}
}


uint16_t m700_add_dobj_item(m700_ep_ext* ep_ext, uint32_t dobj_id, unsigned char *dobj_name)
{
	uint32_t *data_ids_new = NULL;
	uint32_t m700_id;

	if(!ep_ext) return RES_INCORRECT;

	m700_map *res_map = m700_get_map_item(dobj_id, BASE_ID);

	if(!res_map)
	{
#ifdef _DEBUG
		printf("%s: ERROR - Received DOBJ was ignored - no map found. Address = %d, dobj_id = %d, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, dobj_id, dobj_name);
#endif

		return RES_INCORRECT;
	}

	m700_id = (res_map->m700_id >> 8) - 0x80;

	if(m700_get_dobj_item(ep_ext, m700_id) == RES_SUCCESS)
	{
#ifdef _DEBUG
		printf("%s: New DOBJ already exists. Address = %d, m700_id = 0x%04x, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, res_map->m700_id, dobj_name);
#endif
		return RES_SUCCESS;
	}

	data_ids_new = (uint32_t*) realloc((void*)ep_ext->data_ids, sizeof(uint32_t) * (ep_ext->data_ids_size + 1));

	if(!data_ids_new) return RES_MEM_ALLOC;

	ep_ext->data_ids = data_ids_new;

	ep_ext->data_ids[ep_ext->data_ids_size] = m700_id;
	ep_ext->data_ids_size++;

#ifdef _DEBUG
	printf("%s: New DOBJ was added. Address = %d, m700_id = 0x%04x, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, res_map->m700_id, dobj_name);
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
	// check state
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
				printf("%s: m700_collect_data - reached end of ep_exts array\n", APP_NAME);
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
			printf("%s: Collecting data from - Address = %d, ep_idx = %d\n", APP_NAME, ep_ext->adr, dcoll_ep_idx);
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
		printf("%s: m700_collect_data - data_idx = %d. Address = %d\n", APP_NAME, dcoll_data_idx, ep_ext->adr);
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


uint16_t m700_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len)
{
	m700_ep_ext* ep_ext = NULL;
	frame_dobj *fr_do = NULL;

	ep_ext = m700_get_ep_ext(adr, M700_ASDU_ADR);

	if(!ep_ext) return RES_UNKNOWN;

	if(dir == DIRUP)
	{
		// direction is from DIRUP to DIRDN
		switch(sys_msg)
		{
		case EP_MSG_NEWDOBJ:
			fr_do = (frame_dobj *) (buff - sizeof(ep_data_header));

#ifdef _DEBUG
			printf("%s: System message EP_MSG_NEWDOBJ (%d, %s) received. Address = %d\n", APP_NAME, fr_do->id, fr_do->name, ep_ext->adr);
#endif

			m700_add_dobj_item(ep_ext, fr_do->id, (unsigned char*)fr_do->name);

			break;

		case EP_MSG_CONNECT_CLOSE:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_CLOSE received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			m700_init_ep_ext(ep_ext);

			break;

		case EP_MSG_QUIT:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_QUIT received.\n", APP_NAME);
#endif
			appexit = 1;

			break;
		}
	}
	else
	{
		switch(sys_msg)
		{

		case EP_MSG_CONNECT_ACK:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_ACK received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			m700_init_ep_ext(ep_ext);

			// stop timer rc
			ep_ext->timer_rc = 0;

			ep_ext->tx_ready = 1;

			break;

		case EP_MSG_CONNECT_NACK:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_NACK received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			m700_init_ep_ext(ep_ext);

			// start rc timer
			ep_ext->timer_rc = time(NULL);

			break;

		case EP_MSG_CONNECT_LOST:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_LOST received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			m700_init_ep_ext(ep_ext);

			// start rc timer
			ep_ext->timer_rc = time(NULL);

			break;

		case EP_MSG_QUIT:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_QUIT received.\n", APP_NAME);
#endif

			m700_sys_msg_send(EP_MSG_QUIT, ep_ext->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_QUIT sent. Address = all.\n", APP_NAME);
#endif

			appexit = 1;

			break;

		default:
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
			printf("%s: User data in DIRDN sent. Address = %d, Length = %d\n", APP_NAME, ep_ext->adr, ep_header.len);

			char c_buff[512] = {0};
			hex2ascii((unsigned char *)ep_buff+sizeof(ep_data_header), c_buff, + d_len);
			printf("%s: %s\n", APP_NAME, c_buff);
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

	// check if collection started and frame from correct address
	if(!dcoll_stopped && timer_dcoll == 0 && adr != ep_exts[dcoll_ep_idx]->adr)
	{
#ifdef _DEBUG
		printf("%s: ERROR - Frame in DIRUP ignored. Expected adr = %d , received adr = %d.\n", APP_NAME, ep_exts[dcoll_ep_idx]->adr, adr);
#endif
		return RES_INCORRECT;
	}

	// copy next received chunk to the end of the recv_buffer
	memcpy((void*)(recv_buff+recv_buff_len), (void*)buff, buff_len);
	recv_buff_len += buff_len;

#ifdef _DEBUG
	printf("%s: Frame chunk in DIRUP received. Address = %d, Length = %d\n", APP_NAME, adr, recv_buff_len);

	char c_buff[512] = {0};
	hex2ascii(buff, c_buff, buff_len);
	printf("%s: %s\n", APP_NAME, c_buff);
#endif

	offset = 0;

	m_fr = m700_frame_create();

	if(!m_fr) return RES_MEM_ALLOC;

	res = m700_frame_buff_parse(recv_buff, recv_buff_len, &offset, m_fr);

	if(res == RES_SUCCESS)
	{
#ifdef _DEBUG
		printf("%s: Frame in DIRUP received. Length = %d\n", APP_NAME, recv_buff_len);

		char c_buff[512] = {0};
		hex2ascii(recv_buff, c_buff, recv_buff_len);
		printf("%s: %s\n", APP_NAME, c_buff);
#endif

		// reset request-response variables
		timer_recv = 0;
		recv_buff_len = 0;

		ep_ext = m700_get_ep_ext(m_fr->adr, M700_LINK_ADR);

		if(ep_ext)
		{
			// check if collection started and frame from correct address
			if(!dcoll_stopped && timer_dcoll == 0 && ep_ext->adr != ep_exts[dcoll_ep_idx]->adr)
			{
#ifdef _DEBUG
				printf("%s: ERROR - Frame in DIRUP ignored. Expected adr = %d , received adr = %d.\n", APP_NAME, ep_exts[dcoll_ep_idx]->adr, ep_ext->adr);
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

	m700_asdu_map_ids(m700_asdu);

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
		printf("%s: ASDU sent in DIRUP. Address = %d, Length = %d\n", APP_NAME, adr, a_len);
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
			printf("%s: Read Data command sent. Address = %d.\n", APP_NAME, adr);
#endif
		}

		m700_frame_destroy(&m_fr);
	}

	return res;
}


uint16_t m700_read_data_recv(m700_frame *m_fr, m700_ep_ext *ep_ext)
{
#ifdef _DEBUG
	printf("%s: Read Data frame received. Address = %d, Length = %d\n", APP_NAME, ep_ext->adr, m_fr->data_len);
#endif

	uint8_t res, sseq;
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
			printf("%s: Read address command sent. Address = %d.\n", APP_NAME, adr);
#endif
		}

		m700_frame_destroy(&m_fr);
	}

	return res;
}


uint16_t m700_read_adr_recv(m700_frame *m_fr, m700_ep_ext *ep_ext)
{
#ifdef _DEBUG
	printf("%s: Read Address frame received. Address = %d, Length = %d\n", APP_NAME, ep_ext->adr, m_fr->data_len);
#endif

	// TODO finish function m700_read_adr_recv()
	return RES_SUCCESS;
}


