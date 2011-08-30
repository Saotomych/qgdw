/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "../include/m700_frame.h"
#include "../../common/resp_codes.h"
#include "../include/p_num.h"


/* Constants and byte flags/masks */
/* M700 constants */
#define M700_START_BYTE			0x68	/* frame start byte */
#define M700_STOP_BYTE_REQ		0x0D	/* request frame stop  byte */
#define M700_STOP_BYTE_RES		0x0A	/* response frame stop  byte */

#define M700_LEN_MIN			6		/* minimum frame length */


uint8_t m700_frame_get_fcs(unsigned char *buff, uint32_t buff_len)
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


m700_frame *m700_frame_create()
{
	// try to allocate memory for the structure
	m700_frame *frame = (m700_frame*) calloc(1, sizeof(m700_frame));

	return frame;
}


void m700_frame_cleanup(m700_frame *frame)
{
	// fast check input data
	if(!frame || !frame->data) return;

	// free memory allocated for the data
	frame->data_len = 0;
	free(frame->data);
	frame->data = NULL;
}


void m700_frame_destroy(m700_frame **frame)
{
	// fast check input data
	if(!*frame) return;

	// cleanup insides
	m700_frame_cleanup(*frame);

	free(*frame);
	*frame = NULL;
}


uint16_t m700_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, m700_frame *frame)
{
	// fast check input data
	if(!buff || !frame) return RES_INCORRECT;

	uint8_t fcs = 0;
	uint8_t start_byte = 0;

	// look for the frame start
	for( ; *offset<buff_len; )
	{
		start_byte = buff_get_le_uint8(buff, *offset);

		// move to the next byte
		*offset += 1;

		if(start_byte == M700_START_BYTE) break;
	}

	// check if rest of the buffer long enough to contain the frame
	if(buff_len - *offset + 1 < M700_LEN_MIN) return RES_LEN_INVALID;

	frame->adr = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	frame->cmd = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	frame->data_len = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	if(frame->data_len == 0)
	{
		frame->data = NULL;
	}
	else
	{
		// check if buffer length is correct
		if(buff_len - *offset >= frame->data_len + 2)
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
	fcs = m700_frame_get_fcs(buff + (*offset - 4 - frame->data_len), 4 + frame->data_len);

	// compare calculated and received frame checksums, if not the same return error
	if(fcs != buff_get_le_uint8(buff, *offset)) return RES_FCS_INCORRECT;
	*offset += 1;

	// check if stop byte is correct, otherwise return error
	if(buff_get_le_uint8(buff, *offset) != M700_STOP_BYTE_RES) return RES_INCORRECT;
	*offset += 1;

	return RES_SUCCESS;
}


uint16_t m700_frame_buff_build(unsigned char **buff, uint32_t *buff_len, m700_frame *frame)
{
	// set buffer length to zero and start building it
	*buff_len = 0;
	uint32_t offset = 0;

	// fast check input data
	if(!buff || !frame || (frame->data_len > 0 && frame->data == NULL)) return RES_INCORRECT;

	// calculate buffer length
	*buff_len = 4 + frame->data_len + 2;

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
	buff_put_le_uint8(*buff, offset, M700_START_BYTE);
	offset += 1;

	buff_put_le_uint8(*buff, offset, frame->adr);
	offset += 1;

	buff_put_le_uint8(*buff, offset, frame->cmd);
	offset += 1;

	buff_put_le_uint8(*buff, offset, frame->data_len);
	offset += 1;

	if(frame->data_len > 0)
	{
		// copy data to buffer
		memcpy((void*)(*buff + offset), (void*)frame->data, frame->data_len);
		offset += frame->data_len;
	}

	// calculate frame checksum starting from the first start byte to the current position
	// and put frame checksum in the buffer
	buff_put_le_uint8(*buff, offset, m700_frame_get_fcs(*buff, offset));
	offset += 1;

	buff_put_le_uint8(*buff, offset, M700_STOP_BYTE_REQ);
	offset += 1;

	return RES_SUCCESS;
}


struct m700_asdu_parse_tab{
	uint8_t		cmd;
	uint8_t		num;
	uint8_t		type_size;
	float		mult;
	uint8_t		extra;
}
m700_asdu_p_tab[] = {
	{ 0xB0,	3,	2,	0.01,	1 },
	{ 0xB1,	3,	3,	0.001,	0 },
	{ 0xB2,	6,	4,	0.01,	0 },
	{ 0xB3,	3,	2,	0.01,	0 },
	{ 0xB4,	3,	2,	0.01,	0 },

	{ 0xB9,	3,	2,	0.001,	0 },
	{ 0xBA,	3,	2,	0.01,	0 },
	{ 0xBB,	3,	2,	0.01,	0 },
	{ 0xBC,	3,	4,	0.01,	0 },

	{ 0xD4,	4,	4,	0.01,	0 },
	{ 0xD5,	4,	4,	0.01,	0 },
	{ 0xD6,	4,	4,	0.01,	0 },
	{ 0xD7,	4,	4,	0.01,	0 },
	{ 0xD8,	4,	4,	0.01,	0 },
	{ 0xD9,	4,	4,	0.01,	0 },

	{ 0xDF,	1,	1,	1.0,	0 },

	{ 0xE1,	24,	4,	0.01,	1 },

	// that's all folks
	{ 0x00,	0,	0,	0.0,	0 }
};


uint8_t m700_asdu_find_cmd_params(uint8_t cmd, uint8_t *num, uint8_t *type_size, float *mult, uint8_t *extra)
{
	uint8_t res, i;

	// presume that cmd not found (by default)
	res = RES_NOT_FOUND;

	// look for specific cmd
	for(i=0; m700_asdu_p_tab[i].cmd != 0 ; i++)
	{
		if(m700_asdu_p_tab[i].cmd == cmd)
		{
			res = RES_SUCCESS;

			if(num != NULL) *num = m700_asdu_p_tab[i].num;
			if(type_size != NULL) *type_size   = m700_asdu_p_tab[i].type_size;
			if(mult != NULL) *mult = m700_asdu_p_tab[i].mult;
			if(extra != NULL) *extra  = m700_asdu_p_tab[i].extra;

			return res;
		}
	}

	return res;
}


uint16_t m700_asdu_buff_parse(m700_frame *m_fr, asdu *m700_asdu)
{
	// fast check input data
	if(!m_fr->data || m_fr->data_len == 0) return RES_INCORRECT;

	int i;
	uint32_t offset;
	uint8_t res, num, type_size, extra;
	float mult;


	// set protocol type
	m700_asdu->proto = PROTO_M700;


	res = m700_asdu_find_cmd_params(m_fr->cmd, &num, &type_size, &mult, &extra);

	if(res != RES_SUCCESS) return res;

	if(mult < 1.0 )
		m700_asdu->type = 36;
	else
		m700_asdu->type = 35;

	// set to field fnc value cause of transmission - COT_Per_Cyc = 1 (cyclic transmission by IEC101/104 specifications)
	m700_asdu->fnc = 1;

	// number of items in block
	m700_asdu->size = num;

	// check if buffer length is correct
	if(m_fr->data_len < num*type_size) return RES_INCORRECT;

	//try to allocate memory for the data unit array
	m700_asdu->data = (data_unit*) calloc(m700_asdu->size, sizeof(data_unit));

	// check if memory was allocated if not - return error
	if(m700_asdu->data == NULL)
	{
		m700_asdu->size = 0;
		return RES_MEM_ALLOC;
	}

	offset = 0;

	for(i=0; i<m700_asdu->size; i++)
	{
		// build id for each data item
		m700_asdu->data[i].id = (m_fr->cmd << 8) + i + 1;

		m700_asdu->data[i].time_tag = time(NULL);

		if(mult < 1.0)
		{
			m700_asdu->data[i].value.f = (float) buff_get_le_uint(m_fr->data, offset, type_size) * mult;
			m700_asdu->data[i].value_type = ASDU_VAL_FLT;
		}
		else
		{
			m700_asdu->data[i].value.ui = buff_get_le_uint(m_fr->data, offset, type_size);
			m700_asdu->data[i].value_type = ASDU_VAL_UINT;
		}

		offset += type_size;
	}

#ifdef _DEBUG
	printf("%s: ASDU parsed OK. Type = %d, IO num = %d, SQ = %d\n", "unitlink-m700", m700_asdu->type, m700_asdu->size, m700_asdu->attr & 0x01);
#endif

	return RES_SUCCESS;
}













