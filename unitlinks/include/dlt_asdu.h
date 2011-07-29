/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _DLT_ASDU_H_
#define _DLT_ASDU_H_


#include <time.h>
#include "../../common/asdu.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

/* ASDU constants */
#define DLT_ASDU_LEN_MIN	4		/* ASDU minimum length */

/* Time build flags*/
#define DLT_ASDU_SEC		0x01
#define DLT_ASDU_MIN		0x02
#define DLT_ASDU_HOUR		0x04
#define DLT_ASDU_WDAY		0x08
#define DLT_ASDU_MDAY		0x10
#define DLT_ASDU_MONTH		0x20
#define DLT_ASDU_YEAR		0x40

/*
 *
 * Functions
 *
 */

/* parse input buffer to the ASDU structure */
uint8_t dlt_asdu_buff_parse(unsigned char *buff, uint32_t buff_len, asdu *dlt_asdu);

time_t dlt_asdu_parse_time_tag(unsigned char *buff, uint32_t *offset, uint8_t flags);
void dlt_asdu_build_time_tag(unsigned char *buff, uint32_t *offset, time_t time_tag, uint8_t flags);



#ifdef __cplusplus
}
#endif

#endif //_DLT_ASDU_H_
