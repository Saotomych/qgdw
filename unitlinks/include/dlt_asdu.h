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

/* Response codes */
#define RES_DLT_ASDU_SUCCESS		0x00	/* no error(s) */
#define RES_DLT_ASDU_INCORRECT		0x01	/* ASDU or/and buffer are incorrect */
#define RES_DLT_ASDU_UNKNOWN		0x02	/* unknown information object type */
#define RES_DLT_ASDU_MEM_ALLOC		0x04	/* memory allocation error */
#define RES_DLT_ASDU_LEN_INVALID	0x08	/* frame length invalid */

/* ASDU constants */
#define DLT_ASDU_LEN_MIN			4		/* ASDU minimum length */


/*
 *
 * Functions
 *
 */

/* parse input buffer to the ASDU structure */
uint8_t dlt_asdu_buff_parse(unsigned char *buff, uint32_t buff_len, asdu *dlt_asdu);

/* build buffer from given ASDU structure */
uint8_t dlt_asdu_buff_build(unsigned char **buff, uint32_t *buff_len, asdu *dlt_asdu);


#ifdef __cplusplus
}
#endif

#endif //_DLT_ASDU_H_
