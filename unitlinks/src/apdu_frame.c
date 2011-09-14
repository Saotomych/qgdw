/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "../include/apdu_frame.h"
#include "../../common/resp_codes.h"
#include "../include/p_num.h"


/*
 *
 * Constants and byte flags/masks
 *
 */

/* APDU constants */
#define APDU_START_BYTE			0x68	/* start byte (frame with variable and fixed length) */


apdu_frame *apdu_frame_create()
{
	apdu_frame *frame = (apdu_frame*) calloc(1, sizeof(apdu_frame));

	return frame;
}


void apdu_frame_cleanup(apdu_frame *frame)
{
	if(!frame || !frame->data) return;

	frame->data_len = 0;
	free(frame->data);
	frame->data = NULL;
}


void apdu_frame_destroy(apdu_frame **frame)
{
	if(!*frame) return;

	apdu_frame_cleanup(*frame);

	free(*frame);
	*frame = NULL;
}


void apdu_frame_buff_parse_ctrl_field(unsigned char *buff, uint32_t *offset, apdu_frame *frame)
{
	frame->type = buff_get_le_uint8(buff, *offset) & 0x03;

	// fix for 0x02 value to avoid parsing 2 masks separately for APCI_TYPE_I (1-bit mask) and APCI_TYPE_S(U) (2-bit mask)
	if(frame->type == 0x02)
		frame->type = APCI_TYPE_I;

	switch(frame->type)
	{
	case APCI_TYPE_I:
		frame->send_num = buff_get_le_uint16(buff, *offset) >> 1;

		frame->recv_num = buff_get_le_uint16(buff, *offset + 2) >> 1;
		break;

	case APCI_TYPE_S:
		frame->recv_num = buff_get_le_uint16(buff, *offset + 2) >> 1;
		break;

	case APCI_TYPE_U:
		frame->u_cmd    = buff_get_le_uint8(buff, *offset) & ~0x03;
		break;
	}

	*offset += 4;
}


void apdu_frame_buff_build_ctrl_field(unsigned char *buff, uint32_t *offset, apdu_frame *frame)
{
	switch(frame->type)
	{
	case APCI_TYPE_I:
		buff_put_le_uint16(buff, *offset, (frame->send_num << 1) | frame->type);

		buff_put_le_uint16(buff, *offset + 2, frame->recv_num << 1);
		break;

	case APCI_TYPE_S:
		buff_put_le_uint16(buff, *offset, frame->type);

		buff_put_le_uint16(buff, *offset + 2, frame->recv_num << 1);
		break;

	case APCI_TYPE_U:
		buff_put_le_uint8(buff, *offset, frame->u_cmd | frame->type);

		// set other 3 bytes to zero
		buff_put_le_uint24(buff, *offset + 1, 0);
		break;

	default:
		break;
	}

	*offset += 4;
}


uint16_t apdu_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, apdu_frame *frame)
{
	if(!buff || !frame) return RES_INCORRECT;

	uint16_t res;
	uint8_t start_byte = 0;

	// look for the frame start
	for( ; *offset<buff_len; )
	{
		start_byte = buff_get_le_uint8(buff, *offset);

		*offset += 1;

		if(start_byte == APDU_START_BYTE) break;
	}

	// check if rest of the buffer long enough to contain the frame
	if(buff_len - *offset + 1 < APDU_LEN_MIN) return RES_LEN_INVALID;

	if(start_byte != APDU_START_BYTE) return RES_INCORRECT;

	frame->data_len = buff_get_le_uint8(buff, *offset) - 4;
	*offset += 1;

	apdu_frame_buff_parse_ctrl_field(buff, offset, frame);

	switch(frame->type)
	{
	case APCI_TYPE_I:
		// check if data length is correct
		if(frame->data_len > 0 && buff_len - *offset >= frame->data_len)
		{
			frame->data = (unsigned char*) malloc(frame->data_len);

			if(!frame->data)
			{
				frame->data_len = 0;

				return RES_MEM_ALLOC;
			}

			memcpy((void*)frame->data, (void*)(buff + *offset), frame->data_len);

			*offset += frame->data_len;

			res = RES_SUCCESS;
		}
		else
		{
			// something wrong with buffer length
			// set data pointer and length to zero
			frame->data_len = 0;
			frame->data = NULL;

			// set offset to the end of the buffer
			*offset = buff_len;

			res = RES_LEN_INVALID;
		}
		break;

	case APCI_TYPE_S:
	case APCI_TYPE_U:
		frame->data_len = 0;
		frame->data = NULL;

		res = RES_SUCCESS;
		break;

	default:
		res = RES_INCORRECT;
		break;
	}

	return res;
}


uint16_t apdu_frame_buff_build(unsigned char **buff, uint32_t *buff_len, apdu_frame *frame)
{
	*buff_len = 0;
	uint32_t offset = 0;

	if(!buff || !frame || (frame->type == APCI_TYPE_I && (frame->data_len == 0 || frame->data == NULL))) return RES_INCORRECT;

	*buff_len = 6 + frame->data_len;

	*buff = (unsigned char*) malloc(*buff_len);

	if(!*buff)
	{
		*buff_len = 0;

		return RES_MEM_ALLOC;
	}

	// start filling buffer

	buff_put_le_uint8(*buff, offset, APDU_START_BYTE);
	offset += 1;

	buff_put_le_uint8(*buff, offset, *buff_len - 2);
	offset += 1;

	apdu_frame_buff_build_ctrl_field(*buff, &offset, frame);

	if(frame->type == APCI_TYPE_I)
	{
		memcpy((void*)(*buff + offset), (void*)frame->data, frame->data_len);
		offset += frame->data_len;
	}

	return RES_SUCCESS;
}











