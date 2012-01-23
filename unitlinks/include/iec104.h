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
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "p_num.h"
#include "iec_def.h"
#include "apdu_frame.h"
#include "../../common/resp_codes.h"
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

#define APP_NAME		"unitlink-iec104"
#define APP_PATH 		"/rw/mx00/unitlinks"
#define CHILD_APP_NAME	"phy_tcp"
#define CHILD_APP_PATH 	"/rw/mx00/phyints"
#define APP_CFG			"/rw/mx00/configs/lowlevel.cfg"
#define APP_MAP			"/rw/mx00/configs/iec10Xmap.cfg"

#define ALARM_PER		1		/* timers check period (sec) */

#define IEC104_T_RC		10 		/* default re-connect period (sec) */
#define IEC104_T_SYNC	300 	/* default time sync period (sec) */

/*
 *
 * Structures
 *
 */

/* Structure for extend end-point with protocol specific data */
typedef struct iec104_ep_ext {
	uint16_t		adr;		/* link (ASDU) address */
	uint16_t		host_type;	/* master/slave */

	uint16_t		vs;			/* send sequence number counter */
	uint16_t		vr;			/* receive sequence number counter*/

	uint16_t		as;			/* last sent ASDU with acknowledge received from remote peer */
	uint16_t		ar;			/* last received ASDU with acknowledge sent to remote peer */

	uint16_t		rc_cnt;		/* re-connect counter */

	uint16_t		u_cmd;		/* data transfer state */

	uint16_t		k_ack;		/* latest acknowledgment after sending I-Format frame */
	uint16_t		w_ack;		/* latest acknowledgment after receiving I-Format frame */

	uint32_t		*data_ids;		/* data identifiers array */
	uint32_t		data_ids_size;	/* size of data identifiers array */

	uint32_t 		t_sync;		/* time sync period */

	time_t			timer_t0;
	time_t			timer_t1;
	time_t			timer_t2;
	time_t			timer_t3;

	time_t			timer_rc;	/* re-connect timer */
	time_t			timer_sync;	/* time sync timer */
} iec104_ep_ext;


/*
 *
 * Functions
 *
 */

uint16_t iec104_config_read(const char *file_name);

void iec104_catch_alarm(int sig);

uint16_t iec104_reconnect(iec104_ep_ext *ep_ext);


int iec104_recv_data(int len);


iec104_ep_ext* iec104_get_ep_ext(uint16_t adr);

iec104_ep_ext* iec104_add_ep_ext(uint16_t adr);
void iec104_init_ep_ext(iec104_ep_ext* ep_ext);

uint16_t iec104_add_dobj_item(iec104_ep_ext* ep_ext, uint32_t dobj_id, unsigned char *dobj_name);
uint16_t iec104_get_dobj_item(iec104_ep_ext* ep_ext, uint32_t iec104_id);


uint16_t iec104_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len);
uint16_t iec104_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len);


uint16_t iec104_time_sync_send(iec104_ep_ext *ep_ext);
uint16_t iec104_time_sync_recv(asdu *iec_asdu, iec104_ep_ext* ep_ext);

uint16_t iec104_comm_inter_send(iec104_ep_ext *ep_ext);


uint16_t iec104_frame_send(apdu_frame *a_fr,  uint16_t adr, uint8_t dir);
uint16_t iec104_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr);


uint16_t iec104_frame_u_send(uint8_t u_cmd, iec104_ep_ext *ep_ext, uint8_t dir);
uint16_t iec104_frame_u_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext);


uint16_t iec104_frame_s_send(iec104_ep_ext *ep_ext, uint8_t dir);
uint16_t iec104_frame_s_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext);


uint16_t iec104_frame_i_send(asdu *iec_asdu, iec104_ep_ext *ep_ext, uint8_t dir);
uint16_t iec104_frame_i_recv(apdu_frame *a_fr, iec104_ep_ext *ep_ext);


uint16_t iec104_asdu_send(asdu *iec_asdu, uint16_t adr, uint8_t dir);
uint16_t iec104_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr);

uint16_t iec104_frame_check_send_num(iec104_ep_ext *ep_ext, uint16_t send_num);
uint16_t iec104_frame_check_recv_num(iec104_ep_ext *ep_ext, uint16_t recv_num);


void sighandler_sigchld(int arg);
void sighandler_sigquit(int arg);




#ifdef __cplusplus
}
#endif

#endif //_IEC104_H_
