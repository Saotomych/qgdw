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
