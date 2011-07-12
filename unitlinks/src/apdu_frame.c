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


/* Constants and byte flags/masks */
/* APDU constants */
#define APDU_START_BYTE			0x68	/* start byte (frame with variable and fixed length) */


apdu_frame *apdu_frame_create()
{
	// try to allocate memory for the structure
	apdu_frame *frame = (apdu_frame*) calloc(1, sizeof(apdu_frame));

	return frame;
}


void apdu_frame_cleanup(apdu_frame *frame)
{
	// fast check input data
	if(!frame || !frame->data) return;

	// free memory allocated for the data
	frame->data_len = 0;
	free(frame->data);
	frame->data = NULL;
}


void apdu_frame_destroy(apdu_frame **frame)
{
	// fast check input data
	if(!*frame) return;

	// cleanup insides
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
		// get send sequence number
		frame->send_num = buff_get_le_uint16(buff, *offset) >> 1;

		// get receive sequence number
		frame->recv_num = buff_get_le_uint16(buff, *offset + 2) >> 1;
		break;

	case APCI_TYPE_S:
		// get receive sequence number
		frame->recv_num = buff_get_le_uint16(buff, *offset + 2) >> 1;
		break;

	case APCI_TYPE_U:
		// get U-Format command code
		frame->u_cmd    = buff_get_le_uint8(buff, *offset) & ~0x03;
		break;
	}

	// move over 4 bytes of control fields
	*offset += 4;
}


void apdu_frame_buff_build_ctrl_field(unsigned char *buff, uint32_t *offset, apdu_frame *frame)
{
	switch(frame->type)
	{
	case APCI_TYPE_I:
		// put send sequence number & frame type
		buff_put_le_uint16(buff, *offset, (frame->send_num << 1) | frame->type);

		// put receive sequence number
		buff_put_le_uint16(buff, *offset + 2, frame->recv_num << 1);
		break;

	case APCI_TYPE_S:
		// put frame type
		buff_put_le_uint16(buff, *offset, frame->type);

		// put receive sequence number
		buff_put_le_uint16(buff, *offset + 2, frame->recv_num << 1);
		break;

	case APCI_TYPE_U:
		// put U-Format command code
		buff_put_le_uint8(buff, *offset, frame->u_cmd | frame->type);

		// set other 3 bytes to zero
		buff_put_le_uint24(buff, *offset + 1, 0);
		break;

	default:
		break;
	}

	// move over 4 bytes control field
	*offset += 4;
}


uint8_t apdu_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, apdu_frame *frame)
{
	// fast check input data
	if(!buff || !frame) return RES_INCORRECT;

	uint8_t res;
	uint8_t start_byte = 0;

	// look for the frame start
	for( ; *offset<buff_len; )
	{
		start_byte = buff_get_le_uint8(buff, *offset);

		if(start_byte == APDU_START_BYTE) break;

		// move to the next byte
		*offset += 1;
	}

	// check if rest of the buffer long enough to contain the frame
	if(buff_len - *offset < APDU_LEN_MIN) return RES_LEN_INVALID;

	// check if start byte found, otherwise return error
	if(start_byte != APDU_START_BYTE) return RES_INCORRECT;

	// skip start byte
	*offset += 1;

	// get data length
	frame->data_len = buff_get_le_uint8(buff, *offset) - 4;
	*offset += 1;

	apdu_frame_buff_parse_ctrl_field(buff, offset, frame);

	switch(frame->type)
	{
	case APCI_TYPE_I:
		// check if data length is correct
		if(frame->data_len > 0 && buff_len - *offset >= frame->data_len)
		{
			// allocate memory for the data
			frame->data = (unsigned char*) malloc(frame->data_len);

			// check if memory allocated OK, otherwise return error
			if(!frame->data)
			{
				// set data length to zero
				frame->data_len = 0;

				return RES_MEM_ALLOC;
			}

			// copy data from the buffer
			memcpy((void*)frame->data, (void*)(buff + *offset), frame->data_len);

			// move over data in the buffer
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

			// return error
			res = RES_LEN_INVALID;
		}
		break;

	case APCI_TYPE_S:
	case APCI_TYPE_U:
		// no data with this type of frame therefore set data pointer and length to zero
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


uint8_t apdu_frame_buff_build(unsigned char **buff, uint32_t *buff_len, apdu_frame *frame)
{
	// set buffer length to zero and start building it
	*buff_len = 0;
	uint32_t offset = 0;

	// fast check input data
	if(!buff || !frame || (frame->type == APCI_TYPE_I && (frame->data_len == 0 || frame->data == NULL))) return RES_INCORRECT;

	// calculate buffer length
	*buff_len = 6 + frame->data_len;

	// allocate memory for the buffer
	*buff = (unsigned char*) malloc(*buff_len);

	// check if memory allocated OK, otherwise return error
	if(!*buff)
	{
		// set buffer length to zero
		*buff_len = 0;

		return RES_MEM_ALLOC;
	}

	// start filling buffer

	buff_put_le_uint8(*buff, offset, APDU_START_BYTE);
	offset += 1;

	// put data length in the buffer
	buff_put_le_uint8(*buff, offset, *buff_len - 2);
	offset += 1;

	apdu_frame_buff_build_ctrl_field(*buff, &offset, frame);

	if(frame->type == APCI_TYPE_I)
	{
		// copy data to buffer
		memcpy((void*)(*buff + offset), (void*)frame->data, frame->data_len);
		offset += frame->data_len;
	}

	return RES_SUCCESS;
}











