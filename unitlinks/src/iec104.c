/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 

#include "../include/iec104.h"



#ifdef _DEBUG
	char name[] 		= {"phy_tcp"};
	char unitlink[] 	= {APP_NAME};
	char physlink[] 	= {"phy_tcp"};

	struct config_device cd = {
			name,
			unitlink,
			physlink,
			967
	};
#endif




/*End-point extensions array */
static iec104_ep_ext *ep_exts[MAXEP];

/* Common ASDU parameters */
static uint8_t cot_len = IEC104_COT_LEN;
static uint8_t coa_len = IEC104_COA_LEN;
static uint8_t ioa_len = IEC104_IOA_LEN;

/* Timer parameters */

//static uint8_t t0 = IEC104_T0;
//static uint8_t t1 = IEC104_T1;
//static uint8_t t2 = IEC104_T2;
//static uint8_t t3 = IEC104_T3;


/* Maximum numbers of outstanding */
static uint8_t k = IEC104_K;
static uint8_t w = IEC104_W;

static sigset_t sigmask;

int main(int argc, char *argv[])
{
	pid_t chldpid;
	int exit = 0;

	iec104_init_ep_exts();

	chldpid = mf_init(APP_PATH, APP_NAME, iec104_recv_data, iec104_recv_init);

#ifdef _DEBUG
	iec104_add_ep_ext(967, IEC_HOST_MASTER);
	mf_newendpoint(&cd, "/rw/mx00/phyints", 0);
#endif

	do{
		sigsuspend(&sigmask);
	}while(!exit);

	mf_exit();

	return 0;
}


void iec104_init_ep_exts()
{
	int i;

	for(i=0; i<MAXEP; i++)
	{
		ep_exts[i] = NULL;
	}
}


iec104_ep_ext* iec104_get_ep_ext(uint16_t adr)
{
	int i;
	for(i=0; i<MAXEP; i++)
	{
		if(ep_exts[i] && ep_exts[i]->adr == adr) return ep_exts[i];
	}

	return NULL;
}


uint8_t iec104_add_ep_ext(uint16_t adr, uint8_t host_type)
{
	int i;
	iec104_ep_ext *ep_ext = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(ep_ext) return RES_APDU_SUCCESS;

	ep_ext = (iec104_ep_ext*) calloc(1, sizeof(iec104_ep_ext));

	ep_ext->adr = adr;
	ep_ext->host_type = host_type;

	for(i=0; i<MAXEP; i++)
	{
		if(!ep_exts[i])
		{
			ep_exts[i] = ep_ext;
			return RES_APDU_SUCCESS;;
		}
	}

	return RES_APDU_INCORRECT;
}


int iec104_recv_data(int len)
{
	char *buff;
	int adr, dir;
	ep_data_header ep_header;
	iec104_ep_ext* ep_ext = NULL;

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
#ifdef _DEBUG
			printf("%s: User data in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header.adr, ep_header.len);
#endif
			// asdu received
			iec104_asdu_recv((unsigned char*)(buff + sizeof(ep_data_header)), ep_header.len, ep_header.adr);
		}
		else
		{
#ifdef _DEBUG
			printf("%s: System message in DIRDN received. Address = %d, Length = %d\n", APP_NAME, ep_header.adr, ep_header.len);
#endif
			// system message received
		}
	}
	else
	{
		// direction is up
		if(ep_header.sys_msg == EP_USER_DATA)
		{
#ifdef _DEBUG
			printf("%s: User data in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header.adr, ep_header.len);
#endif

			// apdu frame received
			iec104_frame_recv((unsigned char*)(buff + sizeof(ep_data_header)), ep_header.len, ep_header.adr);
		}
		else
		{
#ifdef _DEBUG
			printf("%s: System message in DIRUP received. Address = %d, Length = %d\n", APP_NAME, ep_header.adr, ep_header.len);
#endif

			// system message received
			switch(ep_header.sys_msg)
			{
			case EP_MSG_CONNECT_ACK:
				ep_ext = iec104_get_ep_ext(ep_header.adr);

				if(ep_ext)
				{
					ep_ext->vs = 0;
					ep_ext->vr = 0;
					ep_ext->as = 0;
					ep_ext->ar = 0;

					ep_ext->link_state = APCI_LINK_OFF;

					if(ep_ext->host_type == IEC_HOST_MASTER)
					{
						iec104_frame_u_send(APCI_U_STARTDT_ACT, ep_ext->adr, DIRDN);
					}

					//TODO Put initialization of a timers here later!!!!
				}

				break;

			case EP_MSG_CONNECT_NACK:
			case EP_MSG_CONNECT_CLOSE:
			case EP_MSG_CONNECT_LOST:
				ep_ext = iec104_get_ep_ext(ep_header.adr);

				if(ep_ext)
				{
					ep_ext->link_state = APCI_LINK_OFF;

					if(ep_ext->host_type == IEC_HOST_MASTER)
					{
						//TODO Put re-connect here!!!
					}
				}

				break;

			default:
				break;
			}
		}
	}




	free(buff);

	return 0;
}


int iec104_recv_init(ep_init_header *ih)
{

#ifdef _DEBUG
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[0]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[1]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[2]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[3]);
	printf("%s: HAS READ INIT DATA: %s\n", APP_NAME, ih->isstr[4]);
#endif

	iec104_add_ep_ext(ih->edc->addr, IEC_HOST_MASTER);

	ih->edc->name = ih->edc->phyname;

	mf_newendpoint(ih->edc, "/rw/mx00/phyints", 0);

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


uint8_t iec104_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr)
{
	uint8_t res;
	uint32_t offset;
	apdu_frame *a_fr = NULL;
	iec104_ep_ext *ep_ext = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(!ep_ext) return RES_APDU_INCORRECT;

	offset = 0;

	while(offset < buff_len)
	{
		a_fr = apdu_frame_create();

		if(!a_fr) return RES_APDU_MEM_ALLOC;

		res = apdu_frame_buff_parse(buff, buff_len, &offset, a_fr);

		if(res == RES_APDU_SUCCESS)
		{
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
				res = RES_APDU_INCORRECT;
				break;
			}
		}

		apdu_frame_destroy(&a_fr);
	}

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

#ifdef _DEBUG
	if(res == RES_APDU_SUCCESS)
	{
		switch(u_cmd)
		{
		case APCI_U_STARTDT_ACT:
			printf("%s: U-Format frame sent STARTDT (act).\n", APP_NAME);
			break;
		case APCI_U_STARTDT_CON:
			printf("%s: U-Format frame sent STARTDT (con).\n", APP_NAME);
			break;
		case APCI_U_STOPDT_ACT:
			printf("%s: U-Format frame sent STOPDT (act).\n", APP_NAME);
			break;
		case APCI_U_STOPDT_CON:
			printf("%s: U-Format frame sent STOPDT (con).\n", APP_NAME);
			break;
		case APCI_U_TESTFR_ACT:
			printf("%s: U-Format frame sent TESTFR (act).\n", APP_NAME);
			break;
		case APCI_U_TESTFR_CON:
			printf("%s: U-Format frame sent TESTFR (con).\n", APP_NAME);
			break;
		}
	}
#endif

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_u_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext)
{
	switch(a_fr->u_cmd)
	{
	case APCI_U_STARTDT_ACT:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STARTDT(act).\n", APP_NAME);
#endif

		if(ep_ext->host_type == IEC_HOST_MASTER) return RES_APDU_SUCCESS;

		ep_ext->link_state = APCI_LINK_ON;

		iec104_frame_u_send(APCI_U_STARTDT_CON, ep_ext->adr, DIRDN);
		break;

	case APCI_U_STARTDT_CON:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STARTDT(con).\n", APP_NAME);
#endif

		if(ep_ext->host_type == IEC_HOST_SLAVE) return RES_APDU_SUCCESS;

		ep_ext->link_state = APCI_LINK_ON;
		break;

	case APCI_U_STOPDT_ACT:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STOPDT(act).\n", APP_NAME);
#endif

		if(ep_ext->host_type == IEC_HOST_MASTER) return RES_APDU_SUCCESS;

		ep_ext->link_state = APCI_LINK_OFF;
		iec104_frame_u_send(APCI_U_STOPDT_CON, ep_ext->adr, DIRDN);
		break;

	case APCI_U_STOPDT_CON:
#ifdef _DEBUG
		printf("%s: U-Format frame received. STOPDT(con).\n", APP_NAME);
#endif

		if(ep_ext->host_type == IEC_HOST_SLAVE) return RES_APDU_SUCCESS;

		ep_ext->link_state = APCI_LINK_OFF;
		break;

	case APCI_U_TESTFR_ACT:
#ifdef _DEBUG
		printf("%s: U-Format frame received. TESTFR(act).\n", APP_NAME);
#endif

		iec104_frame_u_send(APCI_U_TESTFR_CON, ep_ext->adr, DIRDN);
		break;

	case APCI_U_TESTFR_CON:
#ifdef _DEBUG
		printf("%s: U-Format frame received. TESTFR(con).\n", APP_NAME);
#endif

		break;

	default:
#ifdef _DEBUG
		printf("%s: U-Format frame received. ERROR - Unknown command.\n", APP_NAME);
#endif

		return RES_APDU_INCORRECT;
		break;
	}

	return RES_APDU_SUCCESS;
}


uint8_t iec104_frame_s_send(iec104_ep_ext *ep_ext, uint8_t dir)
{
	if(ep_ext->host_type == IEC_HOST_SLAVE && ep_ext->link_state == APCI_LINK_OFF) return RES_APDU_INCORRECT;

	uint8_t res;
	apdu_frame *a_fr = NULL;

	if(ep_ext->ar == ep_ext->vr) return RES_APDU_SUCCESS;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_APDU_MEM_ALLOC;

	a_fr->type = APCI_TYPE_S;
	a_fr->recv_num = ep_ext->vr;

	res = iec104_frame_send(a_fr, ep_ext->adr, dir);

	if(res == RES_APDU_SUCCESS)
	{
		ep_ext->ar = ep_ext->vr;

#ifdef _DEBUG
		printf("%s: S-Format frame sent (N(R) = %d).\n", APP_NAME, ep_ext->vr);
#endif
	}

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_s_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext)
{
#ifdef _DEBUG
		printf("%s: S-Format frame received (N(R) = %d).\n", APP_NAME, a_fr->recv_num);
#endif

	if(iec104_frame_check_recv_num(ep_ext, a_fr->recv_num) == RES_APDU_SUCCESS)
	{
		ep_ext->as = a_fr->recv_num;
		return RES_APDU_SUCCESS;
	}

	return RES_APDU_INCORRECT;
}


uint8_t iec104_frame_i_send(asdu *iec_asdu, iec104_ep_ext *ep_ext, uint8_t dir)
{
	if(ep_ext->host_type == IEC_HOST_SLAVE && ep_ext->link_state == APCI_LINK_OFF) return RES_APDU_INCORRECT;

	uint8_t res;
	apdu_frame *a_fr = NULL;
	uint32_t a_len = 0;
	unsigned char *a_buff = NULL;

	a_fr = apdu_frame_create();

	if(!a_fr) return RES_APDU_MEM_ALLOC;

	res  = iec_asdu_buff_build(&a_buff, &a_len, iec_asdu, cot_len, coa_len, ioa_len);

	//TODO Add check for maximum difference receive sequence number to sent state variable!!!

	if(res == RES_IEC_ASDU_SUCCESS)
	{
		a_fr->type = APCI_TYPE_I;
		a_fr->send_num = ep_ext->vs;
		a_fr->recv_num = ep_ext->vr;
		a_fr->data_len = a_len;
		a_fr->data = a_buff;

		res = iec104_frame_send(a_fr, ep_ext->adr, dir);

		if(res == RES_APDU_SUCCESS)
		{
			ep_ext->vs = (ep_ext->vs + 1) % 32767;

			res = RES_APDU_SUCCESS;

#ifdef _DEBUG
			printf("%s: I-Format frame sent (N(S) = %d, N(R) = %d).\n", APP_NAME, a_fr->send_num, a_fr->recv_num);
#endif
		}
	}
	else
	{
		res = RES_APDU_MEM_ALLOC;
	}

	apdu_frame_destroy(&a_fr);

	return res;
}


uint8_t iec104_frame_i_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext)
{
#ifdef _DEBUG
		printf("%s: I-Format frame received (N(S) = %d, N(R) = %d).\n", APP_NAME, a_fr->send_num, a_fr->recv_num);
#endif

	uint8_t res;
	asdu *iec_asdu = NULL;

	if(iec104_frame_check_recv_num(ep_ext, a_fr->recv_num) != RES_APDU_SUCCESS ||
	   iec104_frame_check_send_num(ep_ext, a_fr->send_num) != RES_APDU_SUCCESS)
	{
		return RES_APDU_INCORRECT;
	}

	ep_ext->as = a_fr->recv_num;

	if( (ep_ext->vr - ep_ext->ar + 32767) % 32767 >= w )
	{
		iec104_frame_s_send(ep_ext, DIRDN);
	}

	iec_asdu = asdu_create();

	if(!iec_asdu) return RES_APDU_MEM_ALLOC;

	res = iec_asdu_buff_parse(a_fr->data, a_fr->data_len, iec_asdu, cot_len, coa_len, ioa_len);

	if(res == RES_IEC_ASDU_SUCCESS)
	{
		res = iec104_asdu_send(iec_asdu, ep_ext->adr, DIRUP);

		if(res == RES_ASDU_SUCCESS)
		{
			ep_ext->vr = (ep_ext->vr + 1) % 32767;
		}
	}

	asdu_destroy(&iec_asdu);

	return res;
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

			res = RES_ASDU_SUCCESS;

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


uint8_t iec104_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr)
{
	uint8_t res;
	asdu *iec_asdu = NULL;
	iec104_ep_ext *ep_ext = NULL;

	ep_ext = iec104_get_ep_ext(adr);

	if(!ep_ext) return RES_ASDU_INCORRECT;

	res = asdu_from_byte(buff, buff_len, &iec_asdu);

	if(res == RES_ASDU_SUCCESS && iec_asdu->adr == ep_ext->adr)
	{
		//TODO Add check for maximum difference receive sequence number to sent state variable!!!

		res = iec104_frame_i_send(iec_asdu, ep_ext, DIRDN);
	}
	else
	{
		res = RES_ASDU_INCORRECT;
	}

	asdu_destroy(&iec_asdu);

	return res;
}


uint8_t iec104_frame_check_send_num(iec104_ep_ext *ep_ext, uint16_t send_num)
{
	if(send_num == ep_ext->vr) return RES_APDU_SUCCESS;

#ifdef _DEBUG
	printf("%s: ERROR - expected N(S) = %d, but received N(S) = %d. Frame lost or reordered.\n", APP_NAME, ep_ext->vr, send_num);
#endif

	return RES_APDU_INCORRECT;
}


uint8_t iec104_frame_check_recv_num(iec104_ep_ext *ep_ext, uint16_t recv_num)
{
	if( (recv_num - ep_ext->as + 32767) % 32767 <= (ep_ext->vs - ep_ext->as + 32767) % 32767 ) return RES_APDU_SUCCESS;

#ifdef _DEBUG
	printf("%s: ERROR - expected %d <= N(S) <= %d, but received N(S) = %d. Frame lost or reordered.\n", APP_NAME, ep_ext->as, ep_ext->vs, recv_num);
#endif

	return RES_APDU_INCORRECT;
}













