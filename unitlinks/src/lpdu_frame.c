/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "../include/lpdu_frame.h"
#include "../../common/resp_codes.h"
#include "../../common/p_num.h"


/*
 *
 * Constants and byte flags/masks
 *
 */

/* LPDU constants */
#define LPDU_FT_12_START_VAR	0x68	/* FT 1.2 start byte (frame with variable length) */
#define LPDU_FT_12_START_FIX	0x10	/* FT 1.2 start byte (frame with fixed length) */
#define LPDU_FT_12_STOP			0x16	/* FT 1.2 stop  byte (frame with fixed and variable length) */
#define LPDU_FT_12_SC_ACK		0xE5	/* FT 1.2 positive acknowledgment (single char frame) */
#define LPDU_FT_12_SC_NACK		0xA2	/* FT 1.2 negative acknowledgment (single char frame) */

#define LPDU_FT_2_START			0x27	/* FT 2 start byte (frame with fixed and variable length) */
#define LPDU_FT_2_SC			0x14	/* FT 2 start byte (single char frame) */

#define LPDU_FT_3_START_1		0x05	/* FT 3 first start byte (frame with fixed and variable length) */
#define LPDU_FT_3_START_2		0x64	/* FT 3 second start byte (frame with fixed and variable length) */
#define LPDU_FT_3_SC_1			0x12	/* FT 3 first start byte (single char frame) */
#define LPDU_FT_3_SC_2			0x30	/* FT 3 second start byte (single char frame) */

/* LPCI constants */
/* Control field masks */
#define LPCI_FNC				0x0F	/* function code */
#define LPCI_FCV				0x10	/* FCB valid(1)/invalid (0) */
#define LPCI_FCB				0x20	/* frame count bit */
#define LPCI_DFC				0x10	/* data flow control - overflow(1)/accept(0) */
#define LPCI_ACD				0x20	/* access demand(1)/no access demand(0) */
#define LPCI_PRM				0x40	/* from primary(1)/from secondary(0) */
#define LPCI_DIR				0x80	/* from controlling station(1)/from controlled station(0) */


uint8_t lpdu_frame_get_fcs(unsigned char *buff, uint32_t buff_len)
{
	if(!buff || buff_len == 0) return 0;

	uint8_t i, fcs;

	fcs = 0;

	for(i=0; i < buff_len; i++)
	{
		fcs = (fcs + buff[i]) % 256;
	}

	return fcs;
}


lpdu_frame *lpdu_frame_create()
{
	lpdu_frame *frame = (lpdu_frame*) calloc(1, sizeof(lpdu_frame));

	return frame;
}


void lpdu_frame_cleanup(lpdu_frame *frame)
{
	if(!frame || !frame->data) return;

	frame->data_len = 0;
	free(frame->data);
	frame->data = NULL;
}


void lpdu_frame_destroy(lpdu_frame **frame)
{
	if(!*frame) return;

	lpdu_frame_cleanup(*frame);

	free(*frame);
	*frame = NULL;
}


void lpdu_frame_buff_parse_ctrl_field(unsigned char *buff, uint32_t *offset, lpdu_frame *frame)
{
	frame->fnc = buff_get_le_uint8(buff, *offset) & LPCI_FNC;
	frame->prm = (buff_get_le_uint8(buff, *offset) & LPCI_PRM) >> 6;

	if(frame->prm == 1)
	{
		// parse fields
		frame->fcv = (buff_get_le_uint8(buff, *offset) & LPCI_FCV) >> 4;
		frame->fcb = (buff_get_le_uint8(buff, *offset) & LPCI_FCB) >> 5;

		// initialize with zero
		frame->dfc = 0;
		frame->acd = 0;
	}
	else
	{
		// initialize with zero
		frame->fcv = 0;
		frame->fcb = 0;

		// parse fields
		frame->dfc = (buff_get_le_uint8(buff, *offset) & LPCI_DFC) >> 4;
		frame->acd = (buff_get_le_uint8(buff, *offset) & LPCI_ACD) >> 5;
	}

	frame->dir = (buff_get_le_uint8(buff, *offset) & LPCI_DIR) >> 7;

	*offset += 1;
}


void lpdu_frame_buff_build_ctrl_field(unsigned char *buff, uint32_t *offset, lpdu_frame *frame)
{
	uint8_t bytex;

	bytex = frame->fnc | (frame->prm << 6);

	if(frame->prm == 1)
	{
		bytex |= (frame->fcv << 4) | (frame->fcb << 5);
	}
	else
	{
		bytex |= (frame->dfc << 4) | (frame->acd << 5);
	}

	bytex |= (frame->dir << 7);

	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;
}


uint16_t lpdu_frame_buff_parse_link_address(unsigned char *buff, uint32_t *offset, uint8_t address_len)
{
	uint16_t address;

	switch(address_len)
	{

	case 1:
		address  = buff_get_le_uint8(buff, *offset);
		*offset += 1;
		break;

	case 2:
		address  = buff_get_le_uint16(buff, *offset);
		*offset += 2;
		break;

	default:
		address = 0;
		break;
	}

	return address;
}


void lpdu_frame_buff_build_link_address(unsigned char *buff, uint32_t *offset, uint16_t address, uint8_t address_len)
{
	switch(address_len)
	{
	case 1:
		buff_put_le_uint8(buff, *offset, address);
		*offset += 1;
		break;

	case 2:
		buff_put_le_uint16(buff, *offset, address);
		*offset += 2;
		break;

	default:
		break;
	}
}


uint16_t lpdu_frame_buff_parse_ft12(unsigned char *buff, uint32_t buff_len, uint32_t *offset, lpdu_frame *frame, uint8_t adr_len)
{
	if(!buff || buff_len - *offset < LPDU_LEN_MIN) return RES_INCORRECT;

	uint16_t res, user_data_len = 0;
	uint8_t fcs, start_byte = 0;

	// get start byte and check type of the frame

	start_byte = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	switch(start_byte)
	{
	case LPDU_FT_12_START_VAR:
		frame->type = LPDU_FT_VAR_LEN;

		user_data_len = buff_get_le_uint8(buff, *offset);
		*offset += 1;

		// compare first user data length with received copy, if not the same return error
		if(user_data_len != buff_get_le_uint8(buff, *offset)) return RES_LEN_INVALID;
		*offset += 1;

		if( buff_get_le_uint8(buff, *offset) != LPDU_FT_12_START_VAR) return RES_INCORRECT;
		*offset += 1;

		// check if buffer long enough to contain frame
		if(buff_len - *offset < user_data_len + 2)
		{
			// something wrong with buffer length
			// set data pointer and length to zero
			frame->data_len = 0;
			frame->data = NULL;

			*offset = buff_len;

			return RES_LEN_INVALID;
		}

		lpdu_frame_buff_parse_ctrl_field(buff, offset, frame);

		frame->adr = lpdu_frame_buff_parse_link_address(buff, offset, adr_len);

		frame->data_len = user_data_len - 1 - adr_len;

		frame->data = (unsigned char*) malloc(frame->data_len);

		if(!frame->data)
		{
			frame->data_len = 0;

			return RES_MEM_ALLOC;
		}

		memcpy((void*)frame->data, (void*)(buff + *offset), frame->data_len);

		*offset += frame->data_len;

		// calculate frame checksum from control field position to the current position
		fcs = lpdu_frame_get_fcs(buff + (*offset - 1 - adr_len - frame->data_len), 1 + adr_len + frame->data_len);

		// compare calculated and received frame checksums, if not the same return error
		if(fcs != buff_get_le_uint8(buff, *offset)) return RES_FCS_INCORRECT;
		*offset += 1;

		if(buff_get_le_uint8(buff, *offset) != LPDU_FT_12_STOP) return RES_INCORRECT;
		*offset += 1;

		res = RES_SUCCESS;

		break;

	case LPDU_FT_12_START_FIX:
		// check if buffer long enough to contain frame
		if(buff_len - *offset < 1 + adr_len + 2) return RES_LEN_INVALID;

		frame->type = LPDU_FT_FIX_LEN;

		lpdu_frame_buff_parse_ctrl_field(buff, offset, frame);

		frame->adr = lpdu_frame_buff_parse_link_address(buff, offset, adr_len);

		frame->data_len = 0;
		frame->data = NULL;

		// calculate frame checksum from control field position to the current position
		fcs = lpdu_frame_get_fcs(buff + (*offset - 1 - adr_len), 1 + adr_len);

		// compare calculated and received frame checksums, if not the same return error
		if(fcs != buff_get_le_uint8(buff, *offset)) return RES_FCS_INCORRECT;
		*offset += 1;

		if(buff_get_le_uint8(buff, *offset) != LPDU_FT_12_STOP) return RES_INCORRECT;
		*offset += 1;

		res = RES_SUCCESS;

		break;

	case LPDU_FT_12_SC_ACK:
	case LPDU_FT_12_SC_NACK:
		frame->type = LPDU_FT_SC_LEN;

		frame->sc = start_byte;

		frame->data_len = 0;
		frame->data = NULL;

		res = RES_SUCCESS;

		break;

	default:
		res = RES_UNKNOWN;

		break;
	}

	return res;
}


uint16_t lpdu_frame_buff_build_ft12(unsigned char **buff, uint32_t *buff_len, lpdu_frame *frame, uint8_t adr_len)
{
	uint32_t offset = 0;
	uint16_t res;

	switch(frame->type)
	{
	case LPDU_FT_VAR_LEN:
		if(frame->data_len == 0 || frame->data == NULL) return RES_INCORRECT;

		*buff_len = 5 + adr_len + frame->data_len + 2;

		*buff = (unsigned char*) malloc(*buff_len);

		if(!*buff)
		{
			*buff_len = 0;

			return RES_MEM_ALLOC;
		}

		// start filling buffer

		buff_put_le_uint8(*buff, offset, LPDU_FT_12_START_VAR);
		offset += 1;

		// put first copy of user data length
		buff_put_le_uint8(*buff, offset, 1 + adr_len + frame->data_len);
		offset += 1;

		// put second copy of user data length
		buff_put_le_uint8(*buff, offset, 1 + adr_len + frame->data_len);
		offset += 1;

		buff_put_le_uint8(*buff, offset, LPDU_FT_12_START_VAR);
		offset += 1;

		lpdu_frame_buff_build_ctrl_field(*buff, &offset, frame);

		lpdu_frame_buff_build_link_address(*buff, &offset, frame->adr, adr_len);

		memcpy((void*)(*buff + offset), (void*)frame->data, frame->data_len);
		offset += frame->data_len;

		// calculate frame checksum starting from the fifth byte to the current position
		// and put frame checksum in the buffer
		buff_put_le_uint8(*buff, offset, lpdu_frame_get_fcs(*buff + 4, offset - 4));
		offset += 1;

		buff_put_le_uint8(*buff, offset, LPDU_FT_12_STOP);
		offset += 1;

		res =  RES_SUCCESS;

		break;

	case LPDU_FT_FIX_LEN:
		*buff_len = 2 + adr_len + 2;

		*buff = (unsigned char*) malloc(*buff_len);

		if(!*buff)
		{
			*buff_len = 0;

			return RES_MEM_ALLOC;
		}

		// start filling buffer

		buff_put_le_uint8(*buff, offset, LPDU_FT_12_START_FIX);
		offset += 1;

		lpdu_frame_buff_build_ctrl_field(*buff, &offset, frame);

		lpdu_frame_buff_build_link_address(*buff, &offset, frame->adr, adr_len);

		// calculate frame checksum starting from the second byte to the current position
		// and put frame checksum in the buffer
		buff_put_le_uint8(*buff, offset, lpdu_frame_get_fcs(*buff + 1, offset - 1));
		offset += 1;

		buff_put_le_uint8(*buff, offset, LPDU_FT_12_STOP);
		offset += 1;

		res = RES_SUCCESS;

		break;

	case LPDU_FT_SC_LEN:
		if(frame->data_len != 1 || frame->data == NULL) return RES_INCORRECT;

		*buff_len = 1;

		*buff = (unsigned char*) malloc(*buff_len);

		if(!*buff)
		{
			*buff_len = 0;

			return RES_MEM_ALLOC;
		}

		// start filling buffer

		buff_put_le_uint8(*buff, offset, frame->sc);
		offset += 1;

		res = RES_SUCCESS;

		break;

	default:

		res = RES_UNKNOWN;

		break;
	}

	return res;
}


uint16_t lpdu_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, lpdu_frame *frame, uint8_t format, uint8_t adr_len)
{
	// fast check input data
	if(!buff || !frame) return RES_INCORRECT;

	uint16_t res;
	uint8_t start_byte = 0;

	// look for the frame start
	for( ; *offset<buff_len; )
	{
		start_byte = buff_get_le_uint8(buff, *offset);

		switch(start_byte)
		{
		case LPDU_FT_12_START_VAR:
		case LPDU_FT_12_START_FIX:
		case LPDU_FT_12_SC_ACK:
		case LPDU_FT_12_SC_NACK:
			res = lpdu_frame_buff_parse_ft12(buff, buff_len, offset, frame, adr_len);
			return res;

		case LPDU_FT_2_START:
		case LPDU_FT_2_SC:
		case LPDU_FT_3_START_1:
		case LPDU_FT_3_START_2:
		case LPDU_FT_3_SC_1:
		case LPDU_FT_3_SC_2:
			res = RES_UNSUPPORTED;
			break;

		default:
			res = RES_UNKNOWN;
			break;
		}

		*offset += 1;
	}

	return res;
}


uint16_t lpdu_frame_buff_build(unsigned char **buff, uint32_t *buff_len, lpdu_frame *frame, uint8_t format, uint8_t adr_len)
{
	*buff_len = 0;

	if(!buff || !frame) return RES_INCORRECT;

	uint16_t res;

	switch(format)
	{
	case LPDU_FT_12:
		res = lpdu_frame_buff_build_ft12(buff, buff_len, frame, adr_len);
		break;

	case LPDU_FT_11:
	case LPDU_FT_2:
	case LPDU_FT_3:
		res = RES_UNSUPPORTED;
		break;

	default:
		res = RES_UNKNOWN;
		break;
	}

	return res;
}











