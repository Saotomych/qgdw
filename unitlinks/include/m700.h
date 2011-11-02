/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _M700_H_
#define _M700_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <time.h>
#include <signal.h>
#include "p_num.h"
#include "m700_frame.h"
#include "../../common/resp_codes.h"
#include "../../common/common.h"
#include "../../common/asdu.h"
#include "../../common/multififo.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

#define APP_NAME				"unitlink-m700"
#define APP_PATH 				"/rw/mx00/unitlinks"
#define CHILD_APP_NAME			"phy_tty"
#define CHILD_APP_PATH 			"/rw/mx00/phyints"
#define APP_CFG					"/rw/mx00/configs/lowlevel.cfg"
#define APP_MAP					"/rw/mx00/configs/m700map.cfg"

#define ALARM_PER				1		/* timers check period */

#define M700_T_RC				10 		/* default re-connect period */


#define RECV_TIMEOUT			3		/* default waiting timeout for full response from device */
#define RECV_BUFF_SIZE			512		/* frame receive buffer size */


#define DCOLL_PER				5		/* default data collection period */


#define M700_ASDU_ADR			0
#define M700_LINK_ADR			1

#define M700_ID					0
#define BASE_ID					1


/*
 *
 * Structures
 *
 */

/* Structure for extend end-point with protocol specific data */
typedef struct m700_ep_ext {
	uint16_t		adr;			/* ASDU address */
	uint8_t			adr_link;		/* link address */

	uint8_t			tx_ready;		/* data transfer state */

	uint32_t		*data_ids;		/* data identifiers array */
	uint32_t		data_ids_size;	/* size of data identifiers array */

	time_t			timer_rc;	/* re-connect timer */
} m700_ep_ext;


/*
 *
 * Functions
 *
 */

uint16_t m700_config_read(const char *file_name);

void m700_catch_alarm(int sig);


int m700_recv_data(int len);


m700_ep_ext* m700_get_ep_ext(uint16_t adr, uint8_t get_by);

m700_ep_ext* m700_add_ep_ext(uint16_t adr);
void m700_init_ep_ext(m700_ep_ext* ep_ext);

uint16_t m700_add_dobj_item(m700_ep_ext* ep_ext, uint32_t dobj_id, unsigned char *dobj_name);
uint16_t m700_get_dobj_item(m700_ep_ext* ep_ext, uint32_t m700_id);


uint16_t m700_collect_data();

uint16_t m700_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len);
uint16_t m700_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir, unsigned char *buff, uint32_t buff_len);


uint16_t m700_read_data_send(uint16_t adr, uint32_t data_id, uint8_t num, time_t start_time);
uint16_t m700_read_data_recv(m700_frame *m_fr, m700_ep_ext *ep_ext);

uint16_t m700_read_adr_send(uint16_t adr);
uint16_t m700_read_adr_recv(m700_frame *m_fr, m700_ep_ext *ep_ext);

uint16_t m700_frame_send(m700_frame *m_fr, uint16_t adr, uint8_t dir);
uint16_t m700_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr);


uint16_t m700_asdu_send(asdu *dlt_asdu, uint16_t adr, uint8_t dir);
uint16_t m700_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);


#ifdef __cplusplus
}
#endif

#endif //_M700_H_
