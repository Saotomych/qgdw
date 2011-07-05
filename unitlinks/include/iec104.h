/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC104_H_
#define _IEC104_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <signal.h>
#include "p_num.h"
#include "iec_def.h"
#include "apdu_frame.h"
#include "../../common/common.h"
#include "../../common/asdu.h"
#include "iec_asdu.h"
#include "../../common/multififo.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

#define APP_NAME	"unitlink-iec104"
#define APP_PATH 	"/rw/mx00/unitlinks"


/*
 *
 * Structures
 *
 */

typedef struct iec104_ep_ext {
	uint16_t		adr;		/* link (ASDU) address */
	uint8_t			host_type;	/* master/slave */

	uint16_t		send_num;	/* send sequence number */
	uint16_t		recv_num;	/* receive sequence number */

	uint16_t		ack_num;	/* last sent acknowledged ASDU */


} iec104_ep_ext;


/*
 *
 * Functions
 *
 */

int iec104_recv_data(int len);


int iec104_recv_init(char **buf, int len);

iec104_ep_ext* iec104_get_ep_ext(uint16_t adr);

uint8_t iec104_frame_send(apdu_frame *a_fr,  uint16_t adr, uint8_t dir);
uint8_t iec104_frame_recv(unsigned char *buff, uint32_t buff_len);


uint8_t iec104_frame_u_send(uint8_t u_cmd, uint16_t adr, uint8_t dir);
uint8_t iec104_frame_u_recv();

uint8_t iec104_frame_s_send(uint16_t recv_num, uint16_t adr, uint8_t dir);
uint8_t iec104_frame_s_recv();


uint8_t iec104_frame_i_send(asdu *iec_asdu, uint16_t send_num, uint16_t recv_num, uint16_t adr, uint8_t dir);
uint8_t iec104_frame_i_recv();


uint8_t iec104_asdu_send(asdu *iec_asdu, uint16_t adr, uint8_t dir);
uint8_t iec104_asdu_recv(unsigned char* buff, uint32_t buff_len);


void sighandler_sigchld(int arg);
void sighandler_sigquit(int arg);




#ifdef __cplusplus
}
#endif

#endif //_IEC104_H_
