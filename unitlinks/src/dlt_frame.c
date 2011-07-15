/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "../include/dlt_frame.h"
#include "../../common/resp_codes.h"
#include "../include/p_num.h"


/* Constants and byte flags/masks */
/* DLT constants */
#define DLT_START_BYTE			0x68	/* frame start byte */
#define DLT_STOP_BYTE			0x16	/* frame stop  byte */

#define DLT_LEN_MIN				12		/* minimum frame length */


/* Frame header constants */
/* Control field masks */
#define DLT_FNC					0x1F	/* function */
#define DLT_SSEQ				0x20	/* subsequent data */
#define DLT_ASYN				0x40	/* asynchronous/synchronous */
#define DLT_DIR					0x80	/* direction bit */


uint8_t dlt_frame_get_fcs(unsigned char *buff, uint32_t buff_len)
{
	// fast check input data
	if(!buff || buff_len == 0) return 0;

	uint8_t i, fcs;

	// initialize FCS value
	fcs = 0;

	// proceed through the buffer
	for(i=0; i < buff_len; i++)
	{
		fcs = (fcs + buff[i]) % 256;
	}

	return fcs;
}


dlt_frame *dlt_frame_create()
{
	// try to allocate memory for the structure
	dlt_frame *frame = (dlt_frame*) calloc(1, sizeof(dlt_frame));

	return frame;
}


void dlt_frame_cleanup(dlt_frame *frame)
{
	// fast check input data
	if(!frame || !frame->data) return;

	// free memory allocated for the data
	frame->data_len = 0;
	free(frame->data);
	frame->data = NULL;
}


void dlt_frame_destroy(dlt_frame **frame)
{
	// fast check input data
	if(!*frame) return;

	// cleanup insides
	dlt_frame_cleanup(*frame);

	free(*frame);
	*frame = NULL;
}


void dlt_frame_buff_parse_ctrl_field(unsigned char *buff, uint32_t *offset, dlt_frame *frame)
{
	frame->fnc =  buff_get_le_uint8(buff, *offset) & DLT_FNC;

	frame->sseq = (buff_get_le_uint8(buff, *offset) & DLT_SSEQ) >> 5;

	frame->asyn = (buff_get_le_uint8(buff, *offset) & DLT_ASYN) >> 6;

	frame->dir = (buff_get_le_uint8(buff, *offset) & DLT_DIR) >> 7;

	*offset += 1;
}


void dlt_frame_buff_build_ctrl_field(unsigned char *buff, uint32_t *offset, dlt_frame *frame)
{
	uint8_t bytex;

	bytex = frame->fnc | (frame->sseq << 5) | (frame->asyn << 6) | (frame->dir << 7);

	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;
}


uint16_t dlt_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, dlt_frame *frame)
{
	// fast check input data
	if(!buff || !frame) return RES_INCORRECT;

	uint8_t fcs = 0;
	uint8_t start_byte = 0;

	// look for the frame start
	for( ; *offset<buff_len; )
	{
		start_byte = buff_get_le_uint8(buff, *offset);

		if(start_byte == DLT_START_BYTE) break;

		// move to the next byte
		*offset += 1;
	}

	// check if rest of the buffer long enough to contain the frame
	if(buff_len - *offset < DLT_LEN_MIN) return RES_LEN_INVALID;

	// check if both of start bytes were found, otherwise return error
	if(start_byte != DLT_START_BYTE || buff_get_le_uint8(buff, *offset + 7) != DLT_START_BYTE) return RES_INCORRECT;

	// skip first start byte
	*offset += 1;

	frame->adr     = buff_bcd_get_le_uint(buff, *offset, 6);
	frame->adr_hex = buff_get_le_uint48(buff, *offset);
	*offset += 6;

	// skip second start byte
	*offset += 1;

	dlt_frame_buff_parse_ctrl_field(buff, offset, frame);

	frame->data_len = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	if(frame->data_len == 0)
	{
		frame->data = NULL;
	}
	else
	{
		// check if buffer length is correct
		if(buff_len - *offset >= frame->data_len)
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

			uint8_t i;

			// copy data from the buffer subtracting 0x33 from each byte
			for(i=0; i< frame->data_len; i++)
			{
				frame->data[i] = *(buff + *offset + i) - 0x33;
			}

			// move over data in the buffer
			*offset += frame->data_len;
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
			return RES_LEN_INVALID;
		}
	}

	// calculate frame checksum starting from the first start byte to the current position
	fcs = dlt_frame_get_fcs(buff + (*offset - 10 - frame->data_len), 10 + frame->data_len);

	// compare calculated and received frame checksums, if not the same return error
	if(fcs != buff_get_le_uint8(buff, *offset)) return RES_FCS_INCORRECT;
	*offset += 1;

	// check if stop byte is correct, otherwise return error
	if(buff_get_le_uint8(buff, *offset) != DLT_STOP_BYTE) return RES_INCORRECT;
	*offset += 1;

	return RES_SUCCESS;
}


uint16_t dlt_frame_buff_build(unsigned char **buff, uint32_t *buff_len, dlt_frame *frame)
{
	// set buffer length to zero and start building it
	*buff_len = 0;
	uint32_t offset = 0;

	// fast check input data
	if(!buff || !frame || (frame->data_len > 0 && frame->data == NULL)) return RES_INCORRECT;

	// calculate buffer length
	*buff_len = 10 + frame->data_len + 2;

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

	// put start byte
	buff_put_le_uint8(*buff, offset, DLT_START_BYTE);
	offset += 1;

	if(frame->adr > 0)
		buff_bcd_put_le_uint(*buff, offset, frame->adr, 6);
	else
		buff_put_le_uint48(*buff, offset, frame->adr_hex);
	offset += 6;

	// put second start byte
	buff_put_le_uint8(*buff, offset, DLT_START_BYTE);
	offset += 1;

	dlt_frame_buff_build_ctrl_field(*buff, &offset, frame);

	buff_put_le_uint8(*buff, offset, frame->data_len);
	offset += 1;

	if(frame->data_len > 0)
	{
		uint8_t i;

		// copy data to the buffer adding 0x33 to each byte
		for(i=0; i< frame->data_len; i++)
		{
			*(*buff + offset + i) = frame->data[i] + 0x33;
		}

		// move over data in the buffer
		offset += frame->data_len;
	}

	// calculate frame checksum starting from the first start byte to the current position
	// and put frame checksum in the buffer
	buff_put_le_uint8(*buff, offset, dlt_frame_get_fcs(*buff, *buff_len - 2));
	offset += 1;

	buff_put_le_uint8(*buff, offset, DLT_STOP_BYTE);
	offset += 1;

	return RES_SUCCESS;
}












