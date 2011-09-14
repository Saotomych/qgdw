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


/*
 *
 * Constants and byte flags/masks
 *
 */

/* M700 constants */
#define M700_START_BYTE			0x68	/* frame start byte */
#define M700_STOP_BYTE_REQ		0x0D	/* request frame stop  byte */
#define M700_STOP_BYTE_RES		0x0A	/* response frame stop  byte */

#define M700_LEN_MIN			6		/* minimum frame length */


uint8_t m700_frame_get_fcs(unsigned char *buff, uint32_t buff_len)
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


m700_frame *m700_frame_create()
{
	m700_frame *frame = (m700_frame*) calloc(1, sizeof(m700_frame));

	return frame;
}


void m700_frame_cleanup(m700_frame *frame)
{
	if(!frame || !frame->data) return;

	frame->data_len = 0;
	free(frame->data);
	frame->data = NULL;
}


void m700_frame_destroy(m700_frame **frame)
{
	if(!*frame) return;

	m700_frame_cleanup(*frame);

	free(*frame);
	*frame = NULL;
}


uint16_t m700_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, m700_frame *frame)
{
	if(!buff || !frame) return RES_INCORRECT;

	uint8_t fcs = 0, start_byte = 0;

	// look for the frame start
	for( ; *offset<buff_len; )
	{
		start_byte = buff_get_le_uint8(buff, *offset);

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
			frame->data = (unsigned char*) malloc(frame->data_len);

			if(!frame->data)
			{
				frame->data_len = 0;

				return RES_MEM_ALLOC;
			}

			memcpy((void*)frame->data, (void*)(buff + *offset), frame->data_len);

			*offset += frame->data_len;
		}
		else
		{
			// something wrong with buffer length
			frame->data_len = 0;
			frame->data = NULL;

			// set offset to the end of the buffer
			*offset = buff_len;

			return RES_LEN_INVALID;
		}
	}

	// calculate frame checksum starting from the first start byte to the current position
	fcs = m700_frame_get_fcs(buff + (*offset - 4 - frame->data_len), 4 + frame->data_len);

	if(fcs != buff_get_le_uint8(buff, *offset)) return RES_FCS_INCORRECT;
	*offset += 1;

	if(buff_get_le_uint8(buff, *offset) != M700_STOP_BYTE_RES) return RES_INCORRECT;
	*offset += 1;

	return RES_SUCCESS;
}


uint16_t m700_frame_buff_build(unsigned char **buff, uint32_t *buff_len, m700_frame *frame)
{
	*buff_len = 0;
	uint32_t offset = 0;

	if(!buff || !frame || (frame->data_len > 0 && frame->data == NULL)) return RES_INCORRECT;

	*buff_len = 4 + frame->data_len + 2;

	*buff = (unsigned char*) malloc(*buff_len);

	if(!*buff)
	{
		*buff_len = 0;

		return RES_MEM_ALLOC;
	}

	// start filling buffer

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
		memcpy((void*)(*buff + offset), (void*)frame->data, frame->data_len);
		offset += frame->data_len;
	}

	buff_put_le_uint8(*buff, offset, m700_frame_get_fcs(*buff, offset));
	offset += 1;

	buff_put_le_uint8(*buff, offset, M700_STOP_BYTE_REQ);
	offset += 1;

	return RES_SUCCESS;
}


m700_asdu_parse_tab m700_asdu_p_tab[][33] = {
		{
				{ 0xB0,	1,	3,	2,	0.01,	1 },
				{ 0xB1,	1,	3,	3,	0.001,	0 },
				{ 0xB2,	1,	6,	4,	0.01,	0 },
				{ 0xB3,	1,	3,	2,	0.01,	0 },
				{ 0xB4,	1,	3,	2,	0.01,	0 },
				{ 0xB5,	1,	1,	2,	1.0,	0 }, // do we even need this???

				{ 0xB9,	1,	3,	2,	0.001,	0 },
				{ 0xBA,	1,	3,	2,	0.01,	0 },
				{ 0xBB,	1,	3,	2,	0.01,	0 },
				{ 0xBC,	1,	3,	4,	0.01,	0 },

				{ 0xCB,	1,	3,	2,	0.01,	0 },
				{ 0xCC,	1,	3,	2,	0.01,	0 },
				{ 0xCD,	1,	20,	2,	0.01,	0 },
				{ 0xCE,	1,	20,	2,	0.01,	0 },
				{ 0xCF,	1,	20,	2,	0.01,	0 },
				{ 0xD0,	1,	20,	2,	0.01,	0 },
				{ 0xD1,	1,	20,	2,	0.01,	0 },
				{ 0xD2,	1,	20,	2,	0.01,	0 },
				{ 0xD3,	1,	3,	1,	1.0,	0 },
				{ 0xD4,	1,	4,	4,	0.01,	0 },
				{ 0xD5,	1,	4,	4,	0.01,	0 },
				{ 0xD6,	1,	4,	4,	0.01,	0 },
				{ 0xD7,	1,	4,	4,	0.01,	0 },
				{ 0xD8,	1,	4,	4,	0.01,	0 },
				{ 0xD9,	1,	4,	4,	0.01,	0 },

				{ 0xDF,	1,	1,	1,	1.0,	0 },

				{ 0xE0,	1,	6,	2,	0.01,	1 },
				{ 0xE1,	1,	24,	4,	0.01,	1 },
				{ 0xE2,	1,	13,	2,	1.0,	0 }, // not sure that it's correct!!!

				{ 0xEA,	1,	1,	2,	0.01,	1 },
				{ 0xEB,	1,	1,	2,	0.01,	1 },
				{ 0xEC,	1,	1,	2,	0.01,	1 },

				// that's all folks
				{ 0x00,	0,	0,	0,	0.0,	0 }
		},

		{
				{ 0xB0,	4,	2,	1,	1.0,	0 },

				{ 0xE0,	7,	3,	3,	0.001,	2 },
				{ 0xE1,	25,	3,	2,	0.01,	0 },

				{ 0xEA,	2,	1,	3,	0.001,	2 },
				{ 0xEB,	2,	1,	3,	0.001,	2 },
				{ 0xEC,	2,	1,	3,	0.001,	2 },

				// that's all folks
				{ 0x00,	0,	0,	0,	0.0,	0 },
		},

		{
				{ 0xE0,	10,	6,	4,	0.01,	3 },

				{ 0xEA,	3,	42,	2,	0.01,	0 },
				{ 0xEB,	3,	42,	2,	0.01,	0 },
				{ 0xEC,	3,	42,	2,	0.01,	0 },

				// that's all folks
				{ 0x00,	0,	0,	0,	0.0,	0 },
		},

		{
				{ 0xE0,	16,	6,	2,	0.01,	4 },

				// that's all folks
				{ 0x00,	0,	0,	0,	0.0,	0 },
		},

		{
				{ 0xE0,	22,	2,	1,	1.0,	5 },

				// that's all folks
				{ 0x00,	0,	0,	0,	0.0,	0 },
		},

		{
				{ 0xE0,	24,	1,	2,	1.0,	0 },

				// that's all folks
				{ 0x00,	0,	0,	0,	0.0,	0 },
		}
};


uint8_t m700_asdu_find_cmd_params(uint8_t cmd, uint8_t *idx, uint8_t *num, uint8_t *type_size, float *mult, uint8_t *sseq)
{
	uint8_t i;

	for(i=0; m700_asdu_p_tab[*sseq][i].cmd != 0 ; i++)
	{
		if(m700_asdu_p_tab[*sseq][i].cmd == cmd)
		{
			if(idx != NULL)			*idx =			m700_asdu_p_tab[*sseq][i].idx;
			if(num != NULL)			*num = 			m700_asdu_p_tab[*sseq][i].num;
			if(type_size != NULL)	*type_size =	m700_asdu_p_tab[*sseq][i].type_size;
			if(mult != NULL)		*mult =			m700_asdu_p_tab[*sseq][i].mult;
			if(sseq != NULL)		*sseq =			m700_asdu_p_tab[*sseq][i].sseq;

			return RES_SUCCESS;
		}
	}

	return RES_NOT_FOUND;
}


uint16_t m700_asdu_buff_parse(m700_frame *m_fr, asdu *m700_asdu, uint32_t *offset, uint8_t *sseq)
{
	if(!m_fr->data || m_fr->data_len == 0) return RES_INCORRECT;

	int i;
	uint8_t idx, num, type_size;
	uint16_t res;
	float mult;

	m700_asdu->proto = PROTO_M700;

	res = m700_asdu_find_cmd_params(m_fr->cmd, &idx, &num, &type_size, &mult, sseq);

	if(res != RES_SUCCESS) return res;

	if(mult < 1.0 )
		m700_asdu->type = 36;
	else
		m700_asdu->type = 35;

	// set to field fnc value cause of transmission - COT_Per_Cyc = 1 (cyclic transmission by IEC101/104 specifications)
	m700_asdu->fnc = 1;

	m700_asdu->size = num;

	// check if buffer length is correct
	if(m_fr->data_len-(*offset) < num*type_size) return RES_INCORRECT;

	m700_asdu->data = (data_unit*) calloc(m700_asdu->size, sizeof(data_unit));

	if(m700_asdu->data == NULL)
	{
		m700_asdu->size = 0;
		return RES_MEM_ALLOC;
	}

	for(i=0; i<m700_asdu->size; i++)
	{
		// build id for each data item
		m700_asdu->data[i].id = (m_fr->cmd << 8) + i + idx;

		m700_asdu->data[i].time_tag = time(NULL);

		if(mult < 1.0)
		{
			m700_asdu->data[i].value.f = (float) buff_get_le_uint(m_fr->data, *offset, type_size) * mult;
			m700_asdu->data[i].value_type = ASDU_VAL_FLT;
		}
		else
		{
			m700_asdu->data[i].value.ui = buff_get_le_uint(m_fr->data, *offset, type_size);
			m700_asdu->data[i].value_type = ASDU_VAL_UINT;
		}

		*offset += type_size;
	}

#ifdef _DEBUG
	printf("%s: ASDU parsed OK. Type = %d, IO num = %d, SQ = %d\n", "unitlink-m700", m700_asdu->type, m700_asdu->size, m700_asdu->attr & 0x01);
#endif

	return RES_SUCCESS;
}


