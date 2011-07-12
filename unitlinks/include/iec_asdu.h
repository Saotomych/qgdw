/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC_ASDU_H_
#define _IEC_ASDU_H_


#include <time.h>
#include "iec_def.h"
#include "iec_types.h"
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
#define IEC_ASDU_LEN_MIN			4		/* ASDU minimum length */


/*
 *
 * Functions
 *
 */

/* parse input buffer to the ASDU structure */
uint8_t iec_asdu_buff_parse(unsigned char *buff, uint32_t buff_len, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len, uint8_t ioa_len);

/* build buffer from given ASDU structure */
uint8_t iec_asdu_buff_build(unsigned char **buff, uint32_t *buff_len, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len, uint8_t ioa_len);


#ifdef __cplusplus
}
#endif

#endif //_IEC_ASDU_H_
