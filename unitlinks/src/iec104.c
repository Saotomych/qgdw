/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/iec104.h"


static iec104_ep_ext *ep_exts[MAXEP];

static uint8_t cot_len = IEC104_COT_LEN;
static uint8_t coa_len = IEC104_COA_LEN;
static uint8_t ioa_len = IEC104_IOA_LEN;

static sigset_t sigmask;

int main(int argc, char *argv[])
{
	pid_t chldpid;
	int exit = 0;

	chldpid = mf_init(APP_PATH, APP_NAME, iec104_recv_data, iec104_recv_init);

	do{
		sigsuspend(&sigmask);
	}while(!exit);

	mf_exit();

	return 0;
}


iec104_ep_ext* iec104_get_ep_ext(uint16_t adr)
{
	int i;
	for(i=0; i<MAXEP; i++)
	{
		if(ep_exts[i] || ep_exts[i]->adr == adr) return ep_exts[i];
	}

	return NULL;
}



int iec104_recv_data(int len)
{
	char *buff;
	int adr, dir;
	ep_data_header ep_header;

	buff = malloc(len);

	if(!buff) return -1;

	mf_readbuffer(buff, len, &adr, &dir);

	if(len < sizeof(ep_data_header))
	{
		free(buff);
		return -1;
	}

	memcpy((void*)&ep_header, (void*)buff, sizeof(ep_data_header));

	if(len < sizeof(ep_data_header) + ep_header.len)
	{
		free(buff);
		return -1;
	}

	if(dir == DIRDN)
	{
		// direction is down
		if(ep_header.sys_msg == EP_USER_DATA)
		{
			// asdu received
			iec104_asdu_recv((unsigned char*)(buff + sizeof(ep_data_header)), ep_header.len);
		}
		else
		{
			// system message received
		}
	}
	else
	{
		// direction is up
		if(ep_header.sys_msg == EP_USER_DATA)
		{
			// apdu frame received
			iec104_frame_recv((unsigned char*)(buff + sizeof(ep_data_header)), ep_header.len);
		}
		else
		{
			// system message received
		}
	}




#ifdef _DEBUG
	printf("unitlink-iec104: HAS READ DATA: %s\n", buff);
#endif

	return 0;
}

int iec104_recv_init(char **buf, int len)
{



#ifdef _DEBUG
	printf("unitlink-iec104: HAS READ INIT DATA: %s\n", buf[0]);
	printf("unitlink-iec104: HAS READ INIT DATA: %s\n", buf[1]);
	printf("unitlink-iec104: HAS READ INIT DATA: %s\n", buf[2]);
	printf("unitlink-iec104: HAS READ INIT DATA: %s\n", buf[3]);
	printf("unitlink-iec104: HAS READ INIT DATA: %s\n", buf[4]);
#endif

	return 0;
}


uint8_t iec104_frame_send(apdu_frame *a_fr,  uint16_t adr, uint8_t dir)
{
	uint8_t res;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	res = apdu_frame_buff_build(&a_buff, &a_len, a_fr);

	if(res == RES_APDU_SUCCESS)
	{
		ep_buff = (char*) malloc(sizeof(ep_data_header) + a_len);

		if(ep_buff)
		{
			ep_header.adr = adr;
			ep_header.sys_msg = EP_USER_DATA;
			ep_header.len = a_len;

			memcpy((void*)ep_buff, (void*)&ep_header, sizeof(ep_data_header));
			memcpy((void*)(ep_buff+sizeof(ep_data_header)), (void*)a_buff, a_len);

			mf_toendpoint(ep_buff, sizeof(ep_data_header) + a_len, adr, dir);

			free(ep_buff);
		}
		else
		{
			res = RES_APDU_MEM_ALLOC;
		}

		free(a_buff);
	}

	return res;
}


uint8_t iec104_frame_recv(unsigned char *buff, uint32_t buff_len)
{
	uint8_t res;
	uint32_t offset;
	apdu_frame *a_fr = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_APDU_MEM_ALLOC;

	offset = 0;

	res = apdu_frame_buff_parse(buff, buff_len, &offset, a_fr);

	if(res == RES_APDU_SUCCESS)
	{
		switch(a_fr->type)
		{
		case APCI_TYPE_U:
			break;

		case APCI_TYPE_S:
			break;

		case APCI_TYPE_I:
			break;

		default:
			res = RES_APDU_INCORRECT;
			break;
		}
	}

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_u_send(uint8_t u_cmd, uint16_t adr, uint8_t dir)
{
	uint8_t res;
	apdu_frame *a_fr = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_APDU_MEM_ALLOC;

	a_fr->type = APCI_TYPE_U;
	a_fr->u_cmd = u_cmd;

	res = iec104_frame_send(a_fr, adr, dir);

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_u_recv()
{
	return RES_APDU_SUCCESS;
}


uint8_t iec104_frame_s_send(uint16_t recv_num, uint16_t adr, uint8_t dir)
{
	uint8_t res;
	apdu_frame *a_fr = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_APDU_MEM_ALLOC;

	a_fr->type = APCI_TYPE_S;
	a_fr->recv_num = recv_num;

	res = iec104_frame_send(a_fr, adr, dir);

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_s_recv()
{
	return RES_APDU_SUCCESS;
}


uint8_t iec104_frame_i_send(asdu *iec_asdu, uint16_t send_num, uint16_t recv_num, uint16_t adr, uint8_t dir)
{
	uint8_t res;
	apdu_frame *a_fr = NULL;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_APDU_MEM_ALLOC;

	res  = iec_asdu_buff_build(&a_buff, &a_len, iec_asdu, cot_len, coa_len, ioa_len);

	if(res == RES_IEC_ASDU_SUCCESS)
	{
		a_fr->type = APCI_TYPE_I;
		a_fr->send_num = send_num;
		a_fr->recv_num = recv_num;
		a_fr->data_len = a_len;
		a_fr->data = a_buff;

		res = iec104_frame_send(a_fr, adr, dir);
	}
	else
	{
		res = RES_APDU_MEM_ALLOC;
	}

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_i_recv()
{
	return RES_APDU_SUCCESS;
}


uint8_t iec104_asdu_send(asdu *iec_asdu, uint16_t adr, uint8_t dir)
{
	uint8_t res;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;
	char *ep_buff = NULL;
	ep_data_header ep_header;

	res = asdu_to_byte(&a_buff, &a_len, iec_asdu);

	if(res == RES_ASDU_SUCCESS)
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

			free(ep_buff);
		}
		else
		{
			res = RES_ASDU_MEM_ALLOC;
		}

		free(a_buff);
	}

	return res;
}


uint8_t iec104_asdu_recv(unsigned char* buff, uint32_t buff_len)
{
	uint8_t res;
	asdu *iec_asdu = NULL;
	iec104_ep_ext *ep_ext;

	res = asdu_from_byte(buff, buff_len, &iec_asdu);

	if(res == RES_ASDU_SUCCESS)
	{
		ep_ext = iec104_get_ep_ext(iec_asdu->adr);

		if(ep_ext)
		{
			res = iec104_frame_i_send(iec_asdu, ep_ext->send_num, ep_ext->recv_num, ep_ext->adr, DIRDN);

			if(res == RES_APDU_SUCCESS) ep_ext->send_num += 1;
		}
		else
		{
			res = RES_ASDU_INCORRECT;
		}
	}

	asdu_destroy(&iec_asdu);

	return res;
}













