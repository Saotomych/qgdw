/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _DLT645_H_
#define _DLT645_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <time.h>
#include <signal.h>
#include "p_num.h"
#include "dlt_frame.h"
#include "../../common/resp_codes.h"
#include "../../common/common.h"
#include "../../common/asdu.h"
#include "dlt_asdu.h"
#include "../../common/multififo.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

#define APP_NAME				"unitlink-dlt645"
#define APP_PATH 				"/rw/mx00/unitlinks"
#define CHILD_APP_NAME			"phy_tty"
#define CHILD_APP_PATH 			"/rw/mx00/phyints"
#define APP_CFG					"/rw/mx00/configs/lowlevel.cfg"
#define APP_MAP					"/rw/mx00/configs/dlt645map.cfg"

#define ALARM_PER				1		/* timers check period */

#define DLT645_T_RC				10 		/* default re-connect period */
#define DLT645_T_SYNC			300 /* default time sync period */


#define RECV_TIMEOUT			3		/* default waiting timeout for full response from device */
#define RECV_BUFF_SIZE			512		/* frame receive buffer size */


#define DCOLL_PER				5		/* default data collection period */


#define DLT645_ASDU_ADR			0
#define DLT645_LINK_ADR			1

#define DLT645_ID				0
#define BASE_ID					1

#define DLT645_AWAKE_MSG		0xFEFE	/* device awake message */



/*
 *
 * Structures
 *
 */

/* Structure for extend end-point with protocol specific data */
typedef struct dlt645_ep_ext {
	uint64_t		adr_hex;		/* link address in BCD format */
	uint32_t		adr;			/* ASDU address */

	uint32_t		*data_ids;		/* data identifiers array */
	uint32_t		data_ids_size;	/* size of data identifiers array */

	uint16_t		tx_ready;		/* data transfer state */

	uint16_t 		t_sync;			/* time sync period */

	time_t			timer_rc;		/* re-connect timer */
	time_t			timer_sync;		/* time sync timer */
} dlt645_ep_ext;


/*
 *
 * Functions
 *
 */

uint16_t dlt645_config_read(const char *file_name);

void dlt645_catch_alarm(int sig);


int dlt645_recv_data(int len);


dlt645_ep_ext* dlt645_get_ep_ext(uint64_t adr, uint8_t get_by);

dlt645_ep_ext* dlt645_add_ep_ext(uint16_t adr);
void dlt645_init_ep_ext(dlt645_ep_ext* ep_ext);

uint16_t dlt645_add_dobj_item(dlt645_ep_ext* ep_ext, uint32_t dobj_id, unsigned char *dobj_name);
uint16_t dlt645_get_dobj_item(dlt645_ep_ext* ep_ext, uint32_t dlt645_id);


uint16_t dlt645_collect_data();

uint16_t dlt645_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len);
uint16_t dlt645_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len);


uint16_t dlt645_read_data_send(uint16_t adr, uint32_t data_id, uint8_t num, time_t start_time);
uint16_t dlt645_read_data_recv(dlt_frame *d_fr, dlt645_ep_ext *ep_ext);

uint16_t dlt645_read_adr_send(uint16_t adr);
uint16_t dlt645_read_adr_recv(dlt_frame *d_fr, dlt645_ep_ext *ep_ext);

uint16_t dlt645_set_baudrate_send(uint16_t adr, uint8_t br);
uint16_t dlt645_set_baudrate_recv(dlt_frame *d_fr, dlt645_ep_ext *ep_ext);

uint16_t dlt645_time_sync_send(dlt645_ep_ext *ep_ext);

uint16_t dlt645_frame_send(dlt_frame *d_fr, uint16_t adr, uint8_t dir);
uint16_t dlt645_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr);


uint16_t dlt645_asdu_send(asdu *dlt_asdu, uint16_t adr, uint8_t dir);
uint16_t dlt645_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);


#ifdef __cplusplus
}
#endif

#endif //_DLT645_H_
