/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "../include/dlt_asdu.h"
#include "../../common/resp_codes.h"
#include "../include/p_num.h"


uint8_t dlt_asdu_parse_header(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu)
{
	return RES_SUCCESS;
}


uint8_t dlt_asdu_build_header(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu)
{
	uint8_t bytex;

	// start building header field by field

	return RES_SUCCESS;
}


uint8_t dlt_asdu_buff_parse(unsigned char *buff, uint32_t buff_len, asdu *dlt_asdu)
{
	// fast check input data
	if(!buff || buff_len < DLT_ASDU_LEN_MIN) return RES_INCORRECT;

	return RES_SUCCESS;
}


uint8_t dlt_asdu_buff_build(unsigned char **buff, uint32_t *buff_len, asdu *dlt_asdu)
{
	// set buffer length to zero and start building it
	*buff_len = 0;
	uint32_t offset = 0;

	// fast check input data
	if(!buff || !dlt_asdu || !dlt_asdu->data) return RES_INCORRECT;

	return RES_SUCCESS;
}

