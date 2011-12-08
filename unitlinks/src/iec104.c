/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/iec104.h"


/*End-point extensions array */
static iec104_ep_ext *ep_exts[MAXEP] = {0};

/* list for mapping data identifiers */
static asdu_map *map_list = NULL;

/* Common ASDU parameters */
static uint8_t cot_len = IEC104_COT_LEN;
static uint8_t coa_len = IEC104_COA_LEN;
static uint8_t ioa_len = IEC104_IOA_LEN;

/* Timer parameters */
static uint8_t alarm_t = ALARM_PER;

static uint8_t t_t0 = IEC104_T_T0;
static uint8_t t_t1 = IEC104_T_T1;
static uint8_t t_t2 = IEC104_T_T2;
static uint8_t t_t3 = IEC104_T_T3;

static uint16_t t_rc = IEC104_T_RC;

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo


int main(int argc, char *argv[])
{
	pid_t chldpid;
	uint16_t res;

	int ret;
	struct ep_init_header *eih = 0;

	res = iec104_config_read(APP_CFG);

	if(res != RES_SUCCESS) exit(1);

	res = asdu_map_read(&map_list, APP_MAP, APP_NAME, DEC_BASE);

	chldpid = mf_init(APP_PATH, APP_NAME, iec104_recv_data);

	signal(SIGALRM, iec104_catch_alarm);
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

			mf_newendpoint(eih->addr, CHILD_APP_NAME, CHILD_APP_PATH, 1);

			iec104_sys_msg_send(EP_MSG_CONNECT, eih->addr, DIRDN, NULL, 0);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT sent. Address = %d\n", APP_NAME, eih->addr);
#endif
		}

#ifdef _DEBUG
		if(ret == 2) printf("%s: mf_waitevent timeout\n", APP_NAME);
#endif
	}while(!appexit);

	mf_exit();

	exit(0);
}


uint16_t iec104_config_read(const char *file_name)
{
	FILE *cfg_file = NULL;
	char r_buff[256] = {0};
	char *prm;
	uint16_t adr, ep_num;
	iec104_ep_ext *ep_ext = NULL;

	cfg_file = fopen(file_name, "r");

	if(cfg_file)
	{
		ep_num = 0;

		while(fgets(r_buff, 255, cfg_file))
		{
			if(*r_buff == '#') continue;

			prm = strstr(r_buff, "name");

			if(prm && strstr(prm, APP_NAME))
			{
				adr = atoi(r_buff);

				ep_ext = iec104_add_ep_ext(adr);

				if(!ep_ext) continue;

				prm = strstr(r_buff, "port");

				if(prm && strstr(prm, "LISTEN"))
					ep_ext->host_type = IEC_HOST_SLAVE;
				else
					ep_ext->host_type = IEC_HOST_MASTER;

				if(prm && ep_ext->host_type == IEC_HOST_MASTER)
				{
					prm = strstr(r_buff, "sync");

					if(prm)
						ep_ext->t_sync = atoi(prm+5);
					else
						ep_ext->t_sync = IEC104_T_SYNC;
				}

				prm = strstr(r_buff, "kack");

				if(prm)
					ep_ext->k_ack = atoi(prm+5);
				else
					ep_ext->k_ack = IEC104_K;

				prm = strstr(r_buff, "wack");

				if(prm)
					ep_ext->w_ack = atoi(prm+5);
				else
					ep_ext->w_ack = IEC104_W;

				ep_num++;
#ifdef _DEBUG
				printf("%s: New ep_ext added. Address = %d.\n", APP_NAME, ep_ext->adr);
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


void iec104_catch_alarm(int sig)
{
	time_t cur_time = time(NULL);

	int i;

	// check devices timers
	for(i=0;i<MAXEP; i++)
	{
		if(ep_exts[i])
		{
			if(ep_exts[i]->timer_t0 > 0 && difftime(cur_time, ep_exts[i]->timer_t0) >= t_t0)
			{
#ifdef _DEBUG
				printf("%s: Timer t0 went off. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif

				iec104_init_ep_ext(ep_exts[i]);

				if(ep_exts[i]->host_type == IEC_HOST_MASTER)
				{
					iec104_sys_msg_send(EP_MSG_RECONNECT, ep_exts[i]->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
					printf("%s: System message EP_MSG_RECONNECT sent. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif
				}
			}

			if(ep_exts[i]->timer_t1 > 0 && difftime(cur_time, ep_exts[i]->timer_t1) >= t_t1)
			{
#ifdef _DEBUG
				printf("%s: Timer t1 went off. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif

				iec104_init_ep_ext(ep_exts[i]);

				if(ep_exts[i]->host_type == IEC_HOST_MASTER)
				{
					iec104_sys_msg_send(EP_MSG_RECONNECT, ep_exts[i]->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
					printf("%s: System message EP_MSG_RECONNECT sent. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif
				}
			}

			if(ep_exts[i]->timer_t2 > 0 && difftime(cur_time, ep_exts[i]->timer_t2) >= t_t2)
			{
				iec104_frame_s_send(ep_exts[i], DIRDN);
			}

			if(ep_exts[i]->timer_t3 > 0 && difftime(cur_time, ep_exts[i]->timer_t3) >= t_t3)
			{
				iec104_frame_u_send(APCI_U_TESTFR_ACT, ep_exts[i], DIRDN);
			}

			if(ep_exts[i]->timer_sync > 0 && difftime(cur_time, ep_exts[i]->timer_sync) >= ep_exts[i]->t_sync)
			{
				iec104_time_sync_send(ep_exts[i]);
			}

			if(ep_exts[i]->timer_rc > 0 && difftime(cur_time, ep_exts[i]->timer_rc) >= t_rc)
			{
#ifdef _DEBUG
				printf("%s: Timer rc went off. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif

				iec104_init_ep_ext(ep_exts[i]);

				if(ep_exts[i]->host_type == IEC_HOST_MASTER)
				{
					iec104_sys_msg_send(EP_MSG_RECONNECT, ep_exts[i]->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
					printf("%s: System message EP_MSG_RECONNECT sent. Address = %d.\n", APP_NAME, ep_exts[i]->adr);
#endif
				}
			}
		}
	}

	signal(sig, iec104_catch_alarm);

	alarm(alarm_t);
}


iec104_ep_ext* iec104_get_ep_ext(uint16_t adr)
{
	int i;

	for(i=0; i < MAXEP; i++)
	{
		if(ep_exts[i] && ep_exts[i]->adr == adr) return ep_exts[i];
	}

	return NULL;
}


iec104_ep_ext* iec104_add_ep_ext(uint16_t adr)
{
	int i;
	iec104_ep_ext *ep_ext = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(ep_ext) return ep_ext;

	ep_ext = (iec104_ep_ext*) calloc(1, sizeof(iec104_ep_ext));

	if(!ep_ext) return NULL;

	ep_ext->adr       = adr;
	ep_ext->u_cmd     = APCI_U_STOPDT_CON;

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


void iec104_init_ep_ext(iec104_ep_ext* ep_ext)
{
	if(!ep_ext) return;

	ep_ext->u_cmd = APCI_U_STOPDT_CON;

	// reset all counters
	ep_ext->vs = ep_ext->vr = ep_ext->as = ep_ext->ar = 0;

	// stop all timers
	ep_ext->timer_t0 = ep_ext->timer_t1 = ep_ext->timer_t2 = ep_ext->timer_t3 = ep_ext->timer_sync = ep_ext->timer_rc = 0;

#ifdef _DEBUG
	printf("%s: ep_ext (re)initialized. Address = %d.\n", APP_NAME, ep_ext->adr);
#endif
}


uint16_t iec104_add_dobj_item(iec104_ep_ext* ep_ext, uint32_t dobj_id, unsigned char *dobj_name)
{
	if(!ep_ext) return RES_INCORRECT;

	uint32_t *data_ids_new = NULL;

	asdu_map *res_map = asdu_get_map_item(&map_list, dobj_id, BASE_ID);

	if(!res_map)
	{
#ifdef _DEBUG
		printf("%s: ERROR - Received DOBJ was ignored - no map found. Address = %d, dobj_id = %d, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, dobj_id, dobj_name);
#endif

		return RES_INCORRECT;
	}

	if(iec104_get_dobj_item(ep_ext, res_map->proto_id) == RES_SUCCESS)
	{
#ifdef _DEBUG
		printf("%s: DOBJ already exists. Address = %d, iec104_id = %d, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, ep_ext->data_ids[ep_ext->data_ids_size-1], dobj_name);
#endif
		return RES_SUCCESS;
	}

	data_ids_new = (uint32_t*) realloc((void*)ep_ext->data_ids, sizeof(uint32_t) * (ep_ext->data_ids_size + 1));

	if(!data_ids_new) return RES_MEM_ALLOC;

	ep_ext->data_ids = data_ids_new;

	ep_ext->data_ids[ep_ext->data_ids_size] = res_map->proto_id;
	ep_ext->data_ids_size++;

#ifdef _DEBUG
	printf("%s: New DOBJ was added. Address = %d, iec104_id = %d, dobj_name = \"%s\"\n", APP_NAME, ep_ext->adr, ep_ext->data_ids[ep_ext->data_ids_size-1], dobj_name);
#endif

	return RES_SUCCESS;
}


uint16_t iec104_get_dobj_item(iec104_ep_ext* ep_ext, uint32_t iec104_id)
{
	int i;

	for(i=0; i<ep_ext->data_ids_size; i++)
	{
		if(ep_ext->data_ids[i] == iec104_id) return RES_SUCCESS;
	}

	return RES_NOT_FOUND;
}


int iec104_recv_data(int len)
{
	char *buff;
	int adr, dir;
	uint32_t offset;
	ep_data_header *ep_header_in;

	buff = (char*) malloc(len);

	if(!buff) return -1;

	len = mf_readbuffer(buff, len, &adr, &dir);

	if(len <= 0)
	{
#ifdef _DEBUG
		printf("%s: ERROR - Data received with Length = %d! Something went wrong!\n", APP_NAME, len);
#endif

		free(buff);
		return -1;
	}

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

		ep_header_in = (ep_data_header*)(buff + offset);
		offset += sizeof(ep_data_header);

#ifdef _DEBUG
		printf("%s: Received ep_data_header - adr = %d, sys_msg = %d, len = %d.\n", APP_NAME, ep_header_in->adr, ep_header_in->sys_msg, ep_header_in->len);
#endif

		if(ep_header_in->len < 0 || len - offset < ep_header_in->len)
		{
#ifdef _DEBUG
			printf("%s: ERROR - Expected data length %d bytes, received %d bytes.\n", APP_NAME, ep_header_in->len, len - offset);
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
				iec104_asdu_recv((unsigned char*)(buff + offset), ep_header_in->len, ep_header_in->adr);
			}
			else
			{
#ifdef _DEBUG
				printf("%s: System message in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				iec104_sys_msg_recv(ep_header_in->sys_msg, ep_header_in->adr, dir, (unsigned char*)(buff + offset), ep_header_in->len);
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

				iec104_frame_recv((unsigned char*)(buff + offset), ep_header_in->len, ep_header_in->adr);
			}
			else
			{
#ifdef _DEBUG
				printf("%s: System message in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header_in->adr, ep_header_in->len);
#endif

				iec104_sys_msg_recv(ep_header_in->sys_msg, ep_header_in->adr, dir, (unsigned char*)(buff + offset), ep_header_in->len);
			}
		}

		offset += ep_header_in->len;
	}

	free(buff);

	return 0;
}


uint16_t iec104_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len)
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


uint16_t iec104_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len)
{
	int i, ret;
	iec104_ep_ext* ep_ext = NULL;
	frame_dobj *fr_do = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(!ep_ext) return RES_UNKNOWN;

	if(dir == DIRUP)
	{
		// direction is from DIRUP to DIRDN
		switch(sys_msg)
		{
		case EP_MSG_TEST_CONN:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_TEST_CONN received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			break;

		case EP_MSG_NEWDOBJ:

			fr_do = (frame_dobj *) (buff - sizeof(ep_data_header));

#ifdef _DEBUG
			printf("%s: System message EP_MSG_NEWDOBJ (%d, %s) received. Address = %d.\n", APP_NAME, fr_do->id, fr_do->name, ep_ext->adr);
#endif

			iec104_add_dobj_item(ep_ext, fr_do->id, (unsigned char*)fr_do->name);

			break;

		case EP_MSG_DCOLL_START:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_DCOLL_START received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			iec104_comm_inter_send(ep_ext);

			break;

		case EP_MSG_CONNECT_CLOSE:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_CLOSE received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			if(ep_ext->host_type == IEC_HOST_MASTER)
			{
				iec104_frame_u_send(APCI_U_STOPDT_ACT, ep_ext, DIRDN);

				ep_ext->u_cmd = APCI_U_STOPDT_ACT;
			}

			break;

		case EP_MSG_QUIT:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_QUIT received.\n", APP_NAME);
#endif

			// initiate data transfer stop from all connected devices
			for(i=0; i<MAXEP; i++)
			{
				if(ep_exts[i] && ep_ext->host_type == IEC_HOST_MASTER)
				{
					iec104_frame_u_send(APCI_U_STOPDT_ACT, ep_exts[i], DIRDN);

					ep_exts[i]->u_cmd = APCI_U_STOPDT_ACT;
				}
			}

			iec104_sys_msg_send(EP_MSG_QUIT, ep_ext->adr, DIRDN, NULL, 0);

#ifdef _DEBUG
			printf("%s: System message EP_MSG_QUIT sent. Address = all.\n", APP_NAME);
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
			printf("%s: Warning - unsupported System message (%d) received. Address = %d\n", APP_NAME, sys_msg, ep_ext->adr);
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
			printf("%s: System message EP_MSG_CONNECT_ACK received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			iec104_init_ep_ext(ep_ext);

			if(ep_ext->host_type == IEC_HOST_MASTER)
			{
				// set re-connect counter to zero
				ep_ext->rc_cnt = 0;

				// stop timer rc
				ep_ext->timer_rc = 0;

				iec104_frame_u_send(APCI_U_STARTDT_ACT, ep_ext, DIRDN);

				ep_ext->u_cmd = APCI_U_STARTDT_ACT;

				// start t0 timer
				ep_ext->timer_t0 = time(NULL);
			}

			break;

		case EP_MSG_CONNECT_NACK:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_NACK received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			iec104_init_ep_ext(ep_ext);

			if(ep_ext->host_type == IEC_HOST_MASTER)
			{
				// increase re-connect counter
				ep_ext->rc_cnt++;

				// start rc timer
				ep_ext->timer_rc = time(NULL);
			}

			break;

		case EP_MSG_CONNECT_LOST:
#ifdef _DEBUG
			printf("%s: System message EP_MSG_CONNECT_LOST received. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

			iec104_init_ep_ext(ep_ext);

			if(ep_ext->host_type == IEC_HOST_MASTER)
			{
				// increase re-connect counter
				ep_ext->rc_cnt++;

				// start rc timer
				ep_ext->timer_rc = time(NULL);
			}

			break;

		default:
#ifdef _DEBUG
			printf("%s: Warning - unsupported System message (%d) received. Address = %d\n", APP_NAME, sys_msg, ep_ext->adr);
#endif

			break;
		}
	}

	return RES_SUCCESS;
}


uint16_t iec104_time_sync_send(iec104_ep_ext *ep_ext)
{
	uint16_t res;
	asdu *iec_asdu = NULL;
	time_t cur_time = time(NULL);

	if(!ep_ext) return RES_INCORRECT;

	if(ep_ext->host_type == IEC_HOST_MASTER)
	{
		// start/reset sync timer
		ep_ext->timer_sync = time(NULL);
	}

	iec_asdu = asdu_create();

	if(!iec_asdu)
	{
		res = RES_MEM_ALLOC;
	}
	else
	{
		iec_asdu->adr = ep_ext->adr;

		iec_asdu->type = C_CS_NA_1;

		if(ep_ext->host_type == IEC_HOST_MASTER)
			iec_asdu->fnc = COT_Act;
		else
			iec_asdu->fnc = COT_ActCon;

		iec_asdu->size = 1;
		iec_asdu->data = (data_unit*) calloc(1, sizeof(data_unit));

		if(iec_asdu->data)
		{
			iec_asdu->data->time_tag = cur_time;
			iec_asdu->data->value_type = ASDU_VAL_TIME;

			res = iec104_frame_i_send(iec_asdu, ep_ext, DIRDN);

#ifdef _DEBUG
			if(res == RES_SUCCESS) printf("%s: Time synchronization command sent. Address = %d, COT = %s.\n", APP_NAME, ep_ext->adr, iec_asdu->fnc == COT_Act?"COT_Act":"COT_ActCon");
#endif
		}
		else
		{
			res = RES_MEM_ALLOC;
		}

		asdu_destroy(&iec_asdu);
	}

	return res;
}


uint16_t iec104_time_sync_recv(asdu *iec_asdu, iec104_ep_ext* ep_ext)
{
#ifdef _DEBUG
	printf("%s: Time synchronization command received. Address = %d, COT = %s.\n", APP_NAME, ep_ext->adr, iec_asdu->fnc == COT_Act?"COT_Act":"COT_ActCon");
#endif

	int res;

	struct timeval tv = {iec_asdu->data->time_tag, 0};

	res = settimeofday(&tv, NULL);

#ifdef _DEBUG
	printf("%s: Time synchronization with IEC_HOST_MASTER %s. Address = %d.\n", APP_NAME, res?"failed":"succeeded", ep_ext->adr);
#endif

	if(res == 0)
		return RES_SUCCESS;
	else
		return RES_ACCESS_DND;
}


uint16_t iec104_comm_inter_send(iec104_ep_ext *ep_ext)
{
	uint16_t res;
	asdu *iec_asdu = NULL;

	if(!ep_ext) return RES_INCORRECT;

	iec_asdu = asdu_create();

	if(!iec_asdu)
	{
		res = RES_MEM_ALLOC;
	}
	else
	{
		iec_asdu->adr = ep_ext->adr;

		iec_asdu->type = C_IC_NA_1;

		if(ep_ext->host_type == IEC_HOST_MASTER)
			iec_asdu->fnc = COT_Act;
		else
			iec_asdu->fnc = COT_ActCon;


		iec_asdu->size = 1;
		iec_asdu->data = (data_unit*) calloc(1, sizeof(data_unit));

		if(iec_asdu->data)
		{
			iec_asdu->data->value.ui = COT_InroGen;
			iec_asdu->data->value_type = ASDU_VAL_UINT;

			res = iec104_frame_i_send(iec_asdu, ep_ext, DIRDN);

#ifdef _DEBUG
			if(res == RES_SUCCESS) printf("%s: Common interrogation command sent. Address = %d, COT = %s.\n", APP_NAME, ep_ext->adr, iec_asdu->fnc == COT_Act?"COT_Act":"COT_ActCon");
#endif
		}
		else
		{
			res = RES_MEM_ALLOC;
		}

		asdu_destroy(&iec_asdu);
	}

	return res;
}


uint16_t iec104_frame_send(apdu_frame *a_fr,  uint16_t adr, uint8_t dir)
{
	uint16_t res;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	res = apdu_frame_buff_build(&a_buff, &a_len, a_fr);

	if(res == RES_SUCCESS)
	{
		ep_buff = (char*) malloc(sizeof(ep_data_header) + a_len);

		if(ep_buff)
		{
			ep_header.adr = adr;
			ep_header.sys_msg = EP_USER_DATA;
			ep_header.len = a_len;

			memcpy((void*)ep_buff, (void*)&ep_header, sizeof(ep_data_header));
			memcpy((void*)(ep_buff+sizeof(ep_data_header)), (void*)a_buff, a_len);

			if(mf_toendpoint(ep_buff, sizeof(ep_data_header) + a_len, adr, dir) <= 0) res = RES_INCORRECT;

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


uint16_t iec104_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr)
{
	uint16_t res;
	uint32_t offset;
	apdu_frame *a_fr = NULL;
	iec104_ep_ext *ep_ext = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(!ep_ext) return RES_INCORRECT;

	offset = 0;

	while(offset < buff_len)
	{
		a_fr = apdu_frame_create();

		if(!a_fr) return RES_MEM_ALLOC;

		res = apdu_frame_buff_parse(buff, buff_len, &offset, a_fr);

		if(res == RES_SUCCESS)
		{
			if(ep_ext->timer_t3 > 0)
			{
				// reset t3 timer if it started
				ep_ext->timer_t3 = time(NULL);
			}

			switch(a_fr->type)
			{
			case APCI_TYPE_U:
				iec104_frame_u_recv(a_fr, ep_ext);
				break;

			case APCI_TYPE_S:
				iec104_frame_s_recv(a_fr, ep_ext);
				break;

			case APCI_TYPE_I:
				iec104_frame_i_recv(a_fr, ep_ext);
				break;

			default:
				res = RES_UNKNOWN;
				break;
			}
		}

		apdu_frame_destroy(&a_fr);
	}

	return res;
}


uint16_t iec104_frame_u_send(uint8_t u_cmd, iec104_ep_ext *ep_ext, uint8_t dir)
{
	uint16_t res;
	apdu_frame *a_fr = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_MEM_ALLOC;

	switch(u_cmd)
	{
	case APCI_U_STOPDT_ACT:
	case APCI_U_STOPDT_CON:
		iec104_frame_s_send(ep_ext, DIRDN);
		break;

	case APCI_U_TESTFR_ACT:
		// start/reset timers t1, t3
		ep_ext->timer_t1 = ep_ext->timer_t3 = time(NULL);
		break;

	case APCI_U_TESTFR_CON:
		// stop t1 timer
		ep_ext->timer_t1 = 0;
		break;
	}

	a_fr->type  = APCI_TYPE_U;
	a_fr->u_cmd = u_cmd;

	res = iec104_frame_send(a_fr, ep_ext->adr, dir);

#ifdef _DEBUG
	if(res == RES_SUCCESS)
	{
		switch(u_cmd)
		{
		case APCI_U_STARTDT_ACT:
			printf("%s: U-Format frame sent STARTDT (act). Address = %d\n", APP_NAME, ep_ext->adr);
			break;
		case APCI_U_STARTDT_CON:
			printf("%s: U-Format frame sent STARTDT (con). Address = %d\n", APP_NAME, ep_ext->adr);
			break;
		case APCI_U_STOPDT_ACT:
			printf("%s: U-Format frame sent STOPDT (act). Address = %d\n", APP_NAME, ep_ext->adr);
			break;
		case APCI_U_STOPDT_CON:
			printf("%s: U-Format frame sent STOPDT (con). Address = %d\n", APP_NAME, ep_ext->adr);
			break;
		case APCI_U_TESTFR_ACT:
			printf("%s: U-Format frame sent TESTFR (act). Address = %d\n", APP_NAME, ep_ext->adr);
			break;
		case APCI_U_TESTFR_CON:
			printf("%s: U-Format frame sent TESTFR (con). Address = %d\n", APP_NAME, ep_ext->adr);
			break;
		}
	}
#endif

	apdu_frame_destroy(&a_fr);

	return res;
}


uint16_t iec104_frame_u_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext)
{
	uint16_t res;

	switch(a_fr->u_cmd)
	{
	case APCI_U_STARTDT_ACT:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STARTDT (act). Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		if(ep_ext->host_type == IEC_HOST_MASTER) return RES_SUCCESS;

		iec104_sys_msg_send(EP_MSG_DEV_ONLINE, ep_ext->adr, DIRUP, NULL, 0);

		ep_ext->u_cmd = APCI_U_STARTDT_ACT;

		res = iec104_frame_u_send(APCI_U_STARTDT_CON, ep_ext, DIRDN);

		if(res == RES_SUCCESS)
		{
			ep_ext->u_cmd = APCI_U_STARTDT_CON;

			// start t2-t3 timers
			ep_ext->timer_t2 = time(NULL);
			ep_ext->timer_t3 = time(NULL);
		}

		break;

	case APCI_U_STARTDT_CON:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STARTDT (con). Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		if(ep_ext->host_type == IEC_HOST_SLAVE) return RES_SUCCESS;

		iec104_sys_msg_send(EP_MSG_DEV_ONLINE, ep_ext->adr, DIRUP, NULL, 0);

		ep_ext->u_cmd = APCI_U_STARTDT_CON;

		// stop t0 timer
		ep_ext->timer_t0 = 0;

		iec104_time_sync_send(ep_ext);

		// start t2-t3 timers
		ep_ext->timer_t2 = time(NULL);
		ep_ext->timer_t3 = time(NULL);

		break;

	case APCI_U_STOPDT_ACT:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STOPDT (act). Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		if(ep_ext->host_type == IEC_HOST_MASTER) return RES_SUCCESS;

		ep_ext->u_cmd = APCI_U_STOPDT_ACT;

		res = iec104_frame_u_send(APCI_U_STOPDT_CON, ep_ext, DIRDN);

		if(res == RES_SUCCESS) ep_ext->u_cmd = APCI_U_STOPDT_CON;

		break;

	case APCI_U_STOPDT_CON:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STOPDT (con). Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		if(ep_ext->host_type == IEC_HOST_SLAVE) return RES_SUCCESS;

		ep_ext->u_cmd = APCI_U_STOPDT_CON;
		break;

	case APCI_U_TESTFR_ACT:
#ifdef _DEBUG
		printf("%s: U-Format frame received. TESTFR (act). Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		iec104_frame_u_send(APCI_U_TESTFR_CON, ep_ext, DIRDN);

		break;

	case APCI_U_TESTFR_CON:
		// stop t1 timer
		ep_ext->timer_t1 = 0;

#ifdef _DEBUG
		printf("%s: U-Format frame received. TESTFR (con). Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		break;

	default:
#ifdef _DEBUG
		printf("%s: U-Format frame received. ERROR - Unknown command. Address = %d\n", APP_NAME, ep_ext->adr);
#endif

		return RES_INCORRECT;
		break;
	}

	return RES_SUCCESS;
}


uint16_t iec104_frame_s_send(iec104_ep_ext *ep_ext, uint8_t dir)
{
	if(ep_ext->host_type == IEC_HOST_SLAVE && ep_ext->u_cmd != APCI_U_STARTDT_CON)
	{
#ifdef _DEBUG
		printf("%s: S-Format frame send rejected, TX is not ready. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
		return RES_INCORRECT;
	}

	if(ep_ext->ar == ep_ext->vr) return RES_INCORRECT;

	uint16_t res;
	apdu_frame *a_fr = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_MEM_ALLOC;

	a_fr->type     = APCI_TYPE_S;
	a_fr->recv_num = ep_ext->vr;

	res = iec104_frame_send(a_fr, ep_ext->adr, dir);

	if(res == RES_SUCCESS)
	{
		ep_ext->ar = ep_ext->vr;

		// reset t2 timer
		ep_ext->timer_t2 = time(NULL);

#ifdef _DEBUG
		printf("%s: S-Format frame sent (N(R) = %d). Address = %d\n", APP_NAME, ep_ext->vr, ep_ext->adr);
#endif
	}

	apdu_frame_destroy(&a_fr);

	return res;
}


uint16_t iec104_frame_s_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext)
{
#ifdef _DEBUG
	printf("%s: S-Format frame received (N(R) = %d). Address = %d\n", APP_NAME, a_fr->recv_num, ep_ext->adr);
#endif

	if(iec104_frame_check_recv_num(ep_ext, a_fr->recv_num) == RES_SUCCESS)
	{
		ep_ext->as = a_fr->recv_num;

		if(ep_ext->timer_t1 > 0 && a_fr->recv_num == ep_ext->vs)
		{
			// stop t1 timer
			ep_ext->timer_t1 = 0;
		}

		return RES_SUCCESS;
	}

	return RES_INCORRECT;
}


uint16_t iec104_frame_i_send(asdu *iec_asdu, iec104_ep_ext *ep_ext, uint8_t dir)
{
	if(ep_ext->host_type == IEC_HOST_SLAVE && ep_ext->u_cmd != APCI_U_STARTDT_CON)
	{
#ifdef _DEBUG
		printf("%s: I-Format frame send rejected, TX is not ready. Address = %d\n", APP_NAME, ep_ext->adr);
#endif
		return RES_INCORRECT;
	}

	uint16_t res;
	apdu_frame *a_fr = NULL;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_MEM_ALLOC;

	res  = iec_asdu_buff_build(&a_buff, &a_len, iec_asdu, cot_len, coa_len, ioa_len);


	if(res == RES_SUCCESS)
	{
		a_fr->type     = APCI_TYPE_I;
		a_fr->send_num = ep_ext->vs;
		a_fr->recv_num = ep_ext->vr;
		a_fr->data_len = a_len;
		a_fr->data     = a_buff;

		if((ep_ext->vs - ep_ext->as + 32768) % 32768 <= ep_ext->k_ack)
		{
			res = iec104_frame_send(a_fr, ep_ext->adr, dir);
		}
		else
		{
#ifdef _DEBUG
			printf("%s: I-Format frame send rejected, limit of unconfirmed sent frames is reached. Address = %d, k = %d\n", APP_NAME, ep_ext->adr, (ep_ext->vs - ep_ext->as + 32768) % 32768);
#endif
			res = RES_INCORRECT;
		}

		if(res == RES_SUCCESS)
		{
			ep_ext->vs = (ep_ext->vs + 1) % 32768;

			// start/reset t1 timer
			ep_ext->timer_t1 = time(NULL);

			// reset t2 timer
			ep_ext->timer_t2 = time(NULL);

#ifdef _DEBUG
			printf("%s: I-Format frame sent (N(S) = %d, N(R) = %d). Address = %d\n", APP_NAME, a_fr->send_num, a_fr->recv_num, ep_ext->adr);
#endif
		}
	}
	else
	{
		res = RES_MEM_ALLOC;
	}

	apdu_frame_destroy(&a_fr);

	return res;
}


uint16_t iec104_frame_i_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext)
{
#ifdef _DEBUG
	printf("%s: I-Format frame received (N(S) = %d, N(R) = %d). Address = %d\n", APP_NAME, a_fr->send_num, a_fr->recv_num, ep_ext->adr);
#endif

	uint16_t res;
	asdu *iec_asdu = NULL;

	if(iec104_frame_check_recv_num(ep_ext, a_fr->recv_num) != RES_SUCCESS)
	{
		iec104_frame_s_send(ep_ext, DIRDN);

		iec104_sys_msg_send(EP_MSG_RECONNECT, ep_ext->adr, DIRDN, NULL, 0);

		return RES_INCORRECT;
	}

	ep_ext->as = a_fr->recv_num;

	if(ep_ext->timer_t1 > 0 && a_fr->recv_num == ep_ext->vs)
	{
		// stop t1 timer
		ep_ext->timer_t1 = 0;
	}

	if(iec104_frame_check_send_num(ep_ext, a_fr->send_num) != RES_SUCCESS)
	{
		iec104_frame_s_send(ep_ext, DIRDN);

		iec104_sys_msg_send(EP_MSG_RECONNECT, ep_ext->adr, DIRDN, NULL, 0);

		return RES_INCORRECT;
	}

	iec_asdu = asdu_create();

	if(!iec_asdu) return RES_MEM_ALLOC;

	res = iec_asdu_buff_parse(a_fr->data, a_fr->data_len, iec_asdu, cot_len, coa_len, ioa_len);

	if(res == RES_SUCCESS)
	{
		ep_ext->vr = (ep_ext->vr + 1) % 32768;

		switch(iec_asdu->type)
		{
		case C_IC_NA_1:
			if(ep_ext->host_type == IEC_HOST_SLAVE)
			{
				iec104_sys_msg_send(EP_MSG_COMM_INTER, ep_ext->adr, DIRUP, NULL, 0);

				iec104_comm_inter_send(ep_ext);
			}
			break;

		case C_CS_NA_1:
			if(ep_ext->host_type == IEC_HOST_SLAVE)
			{
				iec104_sys_msg_send(EP_MSG_TIME_SYNC, ep_ext->adr, DIRUP, (unsigned char*) &iec_asdu->data->time_tag, sizeof(int32_t));

				iec104_time_sync_send(ep_ext);
			}
			break;

		default:
			res = iec104_asdu_send(iec_asdu, ep_ext->adr, DIRUP);
			break;
		}
	}

	if((ep_ext->vr - ep_ext->ar + 32768) % 32768 >= ep_ext->w_ack)
	{
		iec104_frame_s_send(ep_ext, DIRDN);
	}

	asdu_destroy(&iec_asdu);

	return res;
}


uint16_t iec104_asdu_send(asdu *iec_asdu, uint16_t adr, uint8_t dir)
{
	uint16_t res;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	// FIXME id mapping!!!
	//asdu_map_ids(&map_list, iec_asdu, APP_NAME, DEC_BASE);

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


uint16_t iec104_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr)
{
	uint16_t res;
	asdu *iec_asdu = NULL;
	iec104_ep_ext *ep_ext = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(!ep_ext) return RES_INCORRECT;

	res = asdu_from_byte(buff, buff_len, &iec_asdu);

	if(res == RES_SUCCESS)
	{
#ifdef _DEBUG
		printf("%s: ASDU received in DIRDN. Address = %d, Length = %d\n", APP_NAME, adr, buff_len);
#endif
		res = iec104_frame_i_send(iec_asdu, ep_ext, DIRDN);
	}
	else
	{
		res = RES_INCORRECT;
	}

	asdu_destroy(&iec_asdu);

	return res;
}


uint16_t iec104_frame_check_send_num(iec104_ep_ext *ep_ext, uint16_t send_num)
{
	if(send_num == ep_ext->vr) return RES_SUCCESS;

#ifdef _DEBUG
	printf("%s: ERROR - expected N(S) = %d, but received N(S) = %d. Frame lost or reordered. Address = %d\n", APP_NAME, ep_ext->vr, send_num, ep_ext->adr);
#endif

	return RES_INCORRECT;
}


uint16_t iec104_frame_check_recv_num(iec104_ep_ext *ep_ext, uint16_t recv_num)
{
	if( (recv_num - ep_ext->as + 32768) % 32768 <= (ep_ext->vs - ep_ext->as + 32768) % 32768 ) return RES_SUCCESS;

#ifdef _DEBUG
	printf("%s: ERROR - expected %d <= N(S) <= %d, but received N(S) = %d. Frame lost or reordered. Address = %d\n", APP_NAME, ep_ext->as, ep_ext->vs, recv_num, ep_ext->adr);
#endif

	return RES_INCORRECT;
}













