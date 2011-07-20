/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/dlt645.h"


/*End-point extensions array */
static dlt645_ep_ext *ep_exts[MAXEP] = {0};

/* Timer parameters */
static uint8_t alarm_t = ALARM_PER;

static uint8_t t0 = DLT645_T0;

static sigset_t sigmask;

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

	res = dlt645_config_read(APP_CFG);

	if(res != RES_SUCCESS) exit(1);

	chldpid = mf_init(APP_PATH, APP_NAME, dlt645_recv_data, dlt645_recv_init);

	signal(SIGALRM, dlt645_catch_alarm);

	alarm(alarm_t);

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

			mf_newendpoint(&cd, CHILD_APP_PATH, 1);

			dlt645_sys_msg_send(EP_MSG_CONNECT, cd.addr, DIRDN);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT sent. Address = %d\n", APP_NAME, cd.addr);
#endif
		}

		if(ret == 2) printf("%s: mf_waitevent timeout\n", APP_NAME);

	}while(!appexit);

	mf_exit();

	exit(0);
}


uint16_t dlt645_config_read(const char *file_name)
{
	FILE *cfg_file = NULL;
	char r_buff[256] = {0};
	char *prm;
	uint16_t adr, ep_num;
	uint64_t link_adr;


	cfg_file = fopen(file_name, "r");

	if(cfg_file)
	{
		ep_num = 0;

		while( fgets(r_buff, 255, cfg_file) )
		{
			if(*r_buff == '#') continue;

			prm = strstr(r_buff, "name");

			if(prm && strstr(prm, APP_NAME))
			{
				adr = atoi(r_buff);

				prm = strstr(r_buff, "addr");

				if(prm)
				{
					link_adr = dlt645_config_read_bcd_link_adr(prm+5);
				}
				else
				{
					buff_bcd_put_le_uint((unsigned char*)&link_adr, 0, adr, 6);
				}

				dlt645_add_ep_ext(adr, link_adr);

				ep_num++;
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


uint64_t dlt645_config_read_bcd_link_adr(char *buff)
{
	uint64_t link_adr;
	uint8_t len, bytex;

	len = 0;

	while(*(buff+len) != ' ') len++;

	if(len%2)
	{
		buff -= 1;

		*buff = '0';
	}

	link_adr = 0;

	while(sscanf(buff, "%2x", (unsigned int*)&bytex))
	{
		link_adr = (link_adr << 8) | bytex;
		buff += 2;
	}

	return link_adr;
}


void dlt645_catch_alarm(int sig)
{
#ifdef _DEBUG
	printf("%s: Timers check went off.\n", APP_NAME);
#endif

	time_t cur_time = time(NULL);

	int i;
	for(i=0;i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
#ifdef _DEBUG
			printf("%s: ep_ext found. Address = %d, t0 = %2.0f.\n",
					APP_NAME,
					ep_exts[i]->adr,
					ep_exts[i]->timer_t0 == 0 ? -1 : difftime(cur_time, ep_exts[i]->timer_t0));
#endif

			// Check timer t0
			if(ep_exts[i]->timer_t0 > 0 && difftime(cur_time, ep_exts[i]->timer_t0) >= t0)
			{
//				dlt645_read_data_send(ep_exts[i]->adr, 0x02010100, 0, 0);
				dlt645_read_adr_send(ep_exts[i]->adr);

				// reset t0 timer
				ep_exts[i]->timer_t0 = time(NULL);
			}
		}
	}

	signal(sig, dlt645_catch_alarm);

	alarm(alarm_t);
}


int dlt645_recv_data(int len)
{
	char *buff;
	int adr, dir;
	uint32_t offset;
	ep_data_header ep_header_in;

	buff = malloc(len);

	if(!buff) return -1;

	mf_readbuffer(buff, len, &adr, &dir);

#ifdef _DEBUG
	printf("%s: Data received. Address = %d, Length = %d, Direction = %s.\n", APP_NAME, adr, len, dir == DIRDN? "DIRUP" : "DIRDN");
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

		memcpy((void*)&ep_header_in, (void*)buff, sizeof(ep_data_header));
		offset += sizeof(ep_data_header);

#ifdef _DEBUG
		printf("%s: Received ep_data_header - adr = %d, sys_msg = %d, len = %d.\n", APP_NAME, ep_header_in.adr, ep_header_in.sys_msg, ep_header_in.len);
#endif

		if(len - offset < ep_header_in.len)
		{
#ifdef _DEBUG
			printf("%s: ERROR - Expected data length %d bytes, received %d bytes.\n", APP_NAME, sizeof(ep_data_header) + ep_header_in.len, len);
#endif

			free(buff);
			return -1;
		}

		if(dir == DIRUP)
		{
			// direction is from DIRUP to DIRDN
			if(ep_header_in.sys_msg == EP_USER_DATA)
			{
#ifdef _DEBUG
			printf("%s: User data in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in.adr, ep_header_in.len);
#endif
				// asdu received
				dlt645_asdu_recv((unsigned char*)(buff + offset), ep_header_in.len, ep_header_in.adr);
			}
			else
			{
#ifdef _DEBUG
				printf("%s: System message in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in.adr, ep_header_in.len);
#endif

				// system message received
				dlt645_sys_msg_recv(ep_header_in.sys_msg, ep_header_in.adr, dir);
			}
		}
		else
		{
			// direction is from DIRDN to DIRUP
			if(ep_header_in.sys_msg == EP_USER_DATA)
			{
#ifdef _DEBUG
				printf("%s: User data in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in.adr, ep_header_in.len);
#endif

				// dlt frame received
				dlt645_frame_recv((unsigned char*)(buff + offset), ep_header_in.len, ep_header_in.adr);
			}
			else
			{
#ifdef _DEBUG
				printf("%s: System message in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in.adr, ep_header_in.len);
#endif

				// system message received
				dlt645_sys_msg_recv(ep_header_in.sys_msg, ep_header_in.adr, dir);
			}
		}

		offset += ep_header_in.len;
	}

	free(buff);

	return 0;
}


int dlt645_recv_init(ep_init_header *ih)
{
#ifdef _DEBUG
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[0]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[1]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[2]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[3]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[4]);
#endif

	cd.addr = ih->addr;
	strncpy(cd.name, ih->isstr[2], 100);
	strncpy(cd.protoname, ih->isstr[3], 100);
	strncpy(cd.phyname, ih->isstr[4], 100);

	return 0;
}


dlt645_ep_ext* dlt645_get_ep_ext(uint64_t adr, uint8_t get_by)
{
	int i;
	for(i=0; i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
			if(get_by == DLT645_ASDU_ADR)
			{
				if(ep_exts[i]->adr == adr) return ep_exts[i];
			}
			else
			{
				if(ep_exts[i]->adr_hex == adr) return ep_exts[i];
			}
		}
	}

	return NULL;
}


uint16_t dlt645_add_ep_ext(uint16_t adr, uint64_t link_adr)
{
	int i;
	dlt645_ep_ext *ep_ext = NULL;

	ep_ext = dlt645_get_ep_ext(adr, 0);

	if(ep_ext) return RES_SUCCESS;

	ep_ext = (dlt645_ep_ext*) calloc(1, sizeof(dlt645_ep_ext));

	ep_ext->adr     = adr;
	ep_ext->adr_hex = link_adr;

	for(i=0; i<MAXEP; i++)
	{
		if(!ep_exts[i])
		{
			ep_exts[i] = ep_ext;
#ifdef _DEBUG
			printf("%s: New ep_ext added. Address = %d, Link address (BCD) = %llx\n", APP_NAME, ep_ext->adr, ep_ext->adr_hex);
#endif
			return RES_SUCCESS;
		}
	}

	return RES_INCORRECT;
}


void dlt645_init_ep_ext(dlt645_ep_ext* ep_ext)
{
	if(!ep_ext) return;

	// stop all timers
	ep_ext->timer_t0 = 0;

#ifdef _DEBUG
	printf("%s: All timers stopped.\n", APP_NAME);
#endif
}


uint16_t dlt645_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir)
{
	int res;
	ep_data_header ep_header;

	ep_header.adr     = adr;
	ep_header.sys_msg = sys_msg;
	ep_header.len     = 0;

	res = mf_toendpoint((char*) &ep_header, sizeof(ep_data_header), adr, dir);

	if(res > 0)
		return RES_SUCCESS;
	else
		return RES_INCORRECT;
}


uint16_t dlt645_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir)
{
	// system message received

	dlt645_ep_ext* ep_ext = NULL;

	ep_ext = dlt645_get_ep_ext(adr, DLT645_ASDU_ADR);

	if(!ep_ext) return RES_UNKNOWN;

	if(dir == DIRUP)
	{
		// direction is from DIRUP to DIRDN
		switch(sys_msg)
		{
		case EP_MSG_CONNECT_CLOSE:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_CLOSE received.\n", APP_NAME);
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
			printf("%s: System message EP_MSG_CONNECT_ACK received.\n", APP_NAME);
#endif

			dlt645_init_ep_ext(ep_ext);

			// start t0 timer
			ep_ext->timer_t0 = time(NULL);

#ifdef _DEBUG
			printf("%s: Timer t0 started.\n", APP_NAME);
#endif

			break;

		case EP_MSG_CONNECT_NACK:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_NACK received.\n", APP_NAME);
#endif

			dlt645_init_ep_ext(ep_ext);

			dlt645_sys_msg_send(EP_MSG_CONNECT, ep_ext->adr, DIRDN);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT sent. Address = %d, Link address (BCD) = %llx.\n", APP_NAME, ep_ext->adr, ep_ext->adr_hex);
#endif

			break;

		case EP_MSG_CONNECT_LOST:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_LOST received.\n", APP_NAME);
#endif

			dlt645_init_ep_ext(ep_ext);

			dlt645_sys_msg_send(EP_MSG_CONNECT, ep_ext->adr, DIRDN);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT sent. Address = %d, Link address (BCD) = %llx.\n", APP_NAME, ep_ext->adr, ep_ext->adr_hex);
#endif

			break;

		case EP_MSG_QUIT:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_QUIT received.\n", APP_NAME);
#endif

			dlt645_sys_msg_send(EP_MSG_QUIT, ep_ext->adr, DIRDN);

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


uint16_t dlt645_frame_send(dlt_frame *d_fr, uint16_t adr, uint8_t dir)
{
	uint16_t res;
	uint32_t d_len = 0;
	unsigned char *d_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;
	uint16_t awk_msg = DLT645_AWAKE_MSG;

	dlt645_ep_ext *ep_ext = NULL;

	ep_ext = dlt645_get_ep_ext(adr, DLT645_ASDU_ADR);

	if(!ep_ext) return RES_INCORRECT;

	if(d_fr->adr_hex == 0 && ep_ext->adr_hex > 0)
	{
		d_fr->adr_hex = ep_ext->adr_hex;
	}

	res = dlt_frame_buff_build(&d_buff, &d_len, d_fr);

	if(res == RES_SUCCESS)
	{
		ep_buff = (char*) malloc(sizeof(ep_data_header) + 2 + d_len);

		if(ep_buff)
		{
			ep_header.adr = adr;
			ep_header.sys_msg = EP_USER_DATA;
			ep_header.len = 2 + d_len;

			memcpy((void*)ep_buff, (void*)&ep_header, sizeof(ep_data_header));
			memcpy((void*)(ep_buff+sizeof(ep_data_header)), (void*)&awk_msg, 2);
			memcpy((void*)(ep_buff+sizeof(ep_data_header)+2), (void*)d_buff, d_len);

			mf_toendpoint(ep_buff, sizeof(ep_data_header) + 2 + d_len, adr, dir);

#ifdef _DEBUG
			printf("%s: User data in DIRDN sent. Address = %d, Link address (BCD) = %llx, Length = %d\n", APP_NAME, ep_ext->adr, ep_ext->adr_hex, ep_header.len);

			char c_buff[512] = {0};
			hex2ascii(d_buff, c_buff, d_len);
			printf("%s: %04X%s\n", APP_NAME, awk_msg, c_buff);
#endif

			free(ep_buff);
		}
		else
		{
			res = RES_MEM_ALLOC;
		}

		free(d_buff);
	}

	return res;
}


uint16_t dlt645_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr)
{
#ifdef _DEBUG
	char c_buff[512] = {0};
	hex2ascii(buff, c_buff, buff_len);
	printf("%s: %s\n", APP_NAME, c_buff);
#endif

	uint16_t res;
	uint32_t offset;
	dlt_frame *d_fr = NULL;
	dlt645_ep_ext *ep_ext = NULL;

	offset = 0;

	while(offset < buff_len)
	{
		d_fr = dlt_frame_create();

		if(!d_fr) return RES_MEM_ALLOC;

		res = dlt_frame_buff_parse(buff, buff_len, &offset, d_fr);

		if(res == RES_SUCCESS)
		{
			ep_ext = dlt645_get_ep_ext(d_fr->adr_hex, DLT645_LINK_ADR);

			if(ep_ext)
			{

			}
			else
			{
				res = RES_INCORRECT;
			}
		}

		dlt_frame_destroy(&d_fr);
	}

	return res;
}


uint16_t dlt645_asdu_send(asdu *iec_asdu, uint16_t adr, uint8_t dir)
{
	uint16_t res;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	res = asdu_to_byte(&a_buff, &a_len, iec_asdu);

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
		}
		else
		{
			res = RES_MEM_ALLOC;
		}

		free(a_buff);
	}

	return res;
}


uint16_t dlt645_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr)
{
	uint16_t res;
	asdu *dlt_asdu = NULL;

	dlt645_ep_ext *ep_ext = NULL;

	ep_ext = dlt645_get_ep_ext(adr, DLT645_ASDU_ADR);

	if(!ep_ext) return RES_INCORRECT;

	res = asdu_from_byte(buff, buff_len, &dlt_asdu);

	if(res == RES_SUCCESS && dlt_asdu->adr == ep_ext->adr)
	{
		res = dlt645_read_data_send(ep_ext->adr, 0x02010100, 0, 0);
	}
	else
	{
		res = RES_INCORRECT;
	}

	asdu_destroy(&dlt_asdu);

	return res;
}


uint16_t dlt645_read_data_send(uint16_t adr, uint32_t data_id, uint8_t num, time_t start_time)
{
	uint16_t res;
	dlt_frame *d_fr = NULL;
	uint32_t offset = 0;

	d_fr = dlt_frame_create();

	if(!d_fr)
	{
		res = RES_MEM_ALLOC;
	}
	else
	{
		d_fr->fnc  = FNC_READ_DATA;
		d_fr->dir  = DIR_REQUEST;

		d_fr->adr  = adr;

		if(num > 0 && start_time == 0)
		{
			d_fr->data_len = 4 + 1;
			d_fr->data = malloc(d_fr->data_len);

			if(d_fr->data)
			{
				buff_put_le_uint32(d_fr->data, offset, data_id);
				offset += 4;

				buff_put_le_uint8(d_fr->data, offset, num);
				offset += 1;
			}
		}
		else if(num > 0 && start_time > 0)
		{
			d_fr->data_len = 4 + 6;
			d_fr->data = malloc(d_fr->data_len);

			if(d_fr->data)
			{
				buff_put_le_uint32(d_fr->data, offset, data_id);
				offset += 4;

				buff_put_le_uint8(d_fr->data, offset, num);
				offset += 1;

				dlt_asdu_build_time_tag(d_fr->data, &offset, start_time);
			}
		}
		else
		{
			d_fr->data_len = 4;
			d_fr->data = malloc(d_fr->data_len);

			if(d_fr->data)
			{
				buff_put_le_uint32(d_fr->data, 0, data_id);
			}
		}
		if(d_fr->data)
		{
			res = dlt645_frame_send(d_fr, adr, DIRDN);
		}
		else
		{
			d_fr->data_len = 0;

			res = RES_MEM_ALLOC;
		}

		dlt_frame_destroy(&d_fr);
	}

	return res;
}


uint16_t dlt645_read_data_recv(uint16_t adr, uint32_t data_id)
{
	return RES_SUCCESS;
}


uint16_t dlt645_read_adr_send(uint16_t adr)
{
	uint16_t res;
	dlt_frame *d_fr = NULL;

	d_fr = dlt_frame_create();

	if(!d_fr)
	{
		res = RES_MEM_ALLOC;
	}
	else
	{
		d_fr->fnc = FNC_READ_ADDRESS;
		d_fr->dir = DIR_REQUEST;

		d_fr->adr_hex = 0xAAAAAAAAAAAA;

		res = dlt645_frame_send(d_fr, adr, DIRDN);

		dlt_frame_destroy(&d_fr);
	}

	return res;
}


uint16_t dlt645_read_adr_recv(uint16_t adr)
{
	return RES_SUCCESS;
}








int hex2ascii(unsigned char *h_buff, char *c_buff, int len)
{
	char tmp[4];
	int i;

	c_buff[0]=0;

	for(i=0; i<len; i++)
	{
		sprintf(tmp, "%02X", h_buff[i]);
		strcat(c_buff, tmp);
	}

	return len;
}






