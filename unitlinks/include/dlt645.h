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


/* Constants and byte flags/masks */
#define APP_NAME				"unitlink-dlt645"
#define APP_PATH 				"/rw/mx00/unitlinks"
#define CHILD_APP_PATH 			"/rw/mx00/phyints"
#define ALARM_PER	1

#define DLT645_ASDU_ADR			0
#define DLT645_LINK_ADR			1

#define DLT645_AWAKE_MSG		0xFEFE	/* device awake message */

#define DLT645_T0				5		/* timeout */


/*
 *
 * Structures
 *
 */
/* Structure for extend end-point with protocol specific data */
typedef struct dlt645_ep_ext {
	uint16_t		adr;			/* ASDU address */
	uint64_t		adr_hex;		/* link address in BCD format */

	time_t			timer_t0;
} dlt645_ep_ext;




/*
 *
 * Functions
 *
 */

uint16_t dlt645_config_read(const char *file_name);
uint64_t dlt645_config_read_bcd_link_adr(char *buff);



void dlt645_catch_alarm(int sig);


int dlt645_recv_data(int len);

int dlt645_recv_init(ep_init_header *ih);


dlt645_ep_ext* dlt645_get_ep_ext(uint64_t adr, uint8_t get_by);

uint16_t dlt645_add_ep_ext(uint16_t adr, uint64_t link_adr);
void dlt645_init_ep_ext(dlt645_ep_ext* ep_ext);

uint16_t dlt645_sys_msg_send(uint32_t sys_msg, uint16_t adr, uint8_t dir);
uint16_t dlt645_sys_msg_recv(uint32_t sys_msg, uint16_t adr, uint8_t dir);


uint16_t dlt645_read_data_send(uint16_t adr, uint32_t data_id, uint8_t num, time_t start_time);
uint16_t dlt645_read_data_recv(uint16_t adr, uint32_t data_id);

uint16_t dlt645_read_adr_send(uint16_t adr);
uint16_t dlt645_read_adr_recv(uint16_t adr);

uint16_t dlt645_frame_send(dlt_frame *d_fr, uint16_t adr, uint8_t dir);
uint16_t dlt645_frame_recv(unsigned char *buff, uint32_t buff_len, uint16_t adr);


uint16_t dlt645_asdu_send(asdu *dlt_asdu, uint16_t adr, uint8_t dir);
uint16_t dlt645_asdu_recv(unsigned char* buff, uint32_t buff_len, uint16_t adr);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);


int hex2ascii(unsigned char *h_buff, char *c_buff, int len);


#ifdef __cplusplus
}
#endif

#endif //_DLT645_H_
