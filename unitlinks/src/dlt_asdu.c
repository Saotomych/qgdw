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


typedef void (*asdu_pb_funcp)(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu);


struct dlt_asdu_parse_build_tab{
	uint32_t		type_mask;
	uint32_t		type_value;
	uint8_t			type_size;
	uint8_t			frc_size;
	uint8_t			start_id;
}
dlt_asdu_pb_tab[] = {
	// D3 = 00 - energy
	{ 0xFF000000,	0x00000000,	4,	2,	0 },

	// D3 = 02 - instantaneous values
	{ 0xFFFF0000,	0x02010000,	2,	1,	1 },
	{ 0xFFFF0000,	0x02020000,	3,	3,	1 },
	{ 0xFFFF0000,	0x02030000,	3,	4,	0 },
	{ 0xFFFF0000,	0x02040000,	3,	4,	0 },
	{ 0xFFFF0000,	0x02050000,	3,	4,	0 },
	{ 0xFFFF0000,	0x02060000,	2,	3,	0 },
	{ 0xFFFF0000,	0x02070000,	2,	1,	1 },
	{ 0xFFFF0000,	0x02080000,	2,	2,	1 },
	{ 0xFFFF0000,	0x02090000,	2,	2,	1 },
	{ 0xFFFF0000,	0x020A0000,	2,	2,	1 },
	{ 0xFFFF0000,	0x020B0000,	2,	2,	1 },
	{ 0xFFFF00FF,	0x02800001,	3,	3,	0 },
	{ 0xFFFF00FF,	0x02800002,	2,	2,	0 },
	{ 0xFFFF00FF,	0x02800003,	3,	4,	0 },
	{ 0xFFFF00FF,	0x02800004,	3,	4,	0 },
	{ 0xFFFF00FF,	0x02800005,	3,	4,	0 },
	{ 0xFFFF00FF,	0x02800006,	3,	4,	0 },
	{ 0xFFFF00FF,	0x02800007,	2,	1,	0 },
	{ 0xFFFF00FF,	0x02800008,	2,	2,	0 },
	{ 0xFFFF00FF,	0x02800009,	2,	2,	0 },
	{ 0xFFFF00FF,	0x0280000A,	4,	0,	0 },

	// D3 = 03 - events log
	{ 0xFFFFFFFF,	0x03300D00,	3,	0,	0 },
	{ 0xFFFFFFFF,	0x03300E00,	3,	0,	0 },

	// D3 = 04 - meter parameters
	{ 0xFFFFFFFF,	0x04000401,	6,	0,	0 },

	// that's all folks
	{ 0x00000000,	0x00000000,	0,	0 }
};


uint16_t dlt_asdu_find_type_params(uint32_t ib_id, uint8_t *type_size, uint8_t *frc_size, uint8_t *start_id)
{
	uint16_t i;

	for(i=0; dlt_asdu_pb_tab[i].type_size != 0 ; i++)
	{
		if( (ib_id & dlt_asdu_pb_tab[i].type_mask) == dlt_asdu_pb_tab[i].type_value)
		{
			if(type_size != NULL) *type_size = dlt_asdu_pb_tab[i].type_size;
			if(frc_size != NULL) *frc_size  = dlt_asdu_pb_tab[i].frc_size;
			if(start_id != NULL) *start_id  = dlt_asdu_pb_tab[i].start_id;

			return RES_SUCCESS;
		}
	}

	return RES_NOT_FOUND;
}


int dlt_asdu_check_data_block(unsigned char *buff, uint32_t offset)
{
	int i;

	for(i=0; i<3; i++)
	{
		if(buff_get_le_uint8(buff, offset+i) == 0xFF) return i;
	}

	return -1;
}


uint32_t dlt_asdu_build_data_unit_id(uint32_t block_id, uint8_t pos, uint8_t num)
{
	return (block_id & ~(0xFF << (pos*8))) | (num << pos*8);
}


time_t dlt_asdu_parse_time_tag(unsigned char *buff, uint32_t *offset, uint8_t flags)
{
	struct tm time_tm;

	if(flags & DLT_ASDU_SEC)
	{
		time_tm.tm_sec  = buff_bcd_get_le_uint(buff, *offset, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_MIN)
	{
		time_tm.tm_min   = buff_bcd_get_le_uint(buff, *offset, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_HOUR)
	{
		time_tm.tm_hour  = buff_bcd_get_le_uint(buff, *offset, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_WDAY)
	{
		time_tm.tm_wday  = buff_bcd_get_le_uint(buff, *offset, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_MDAY)
	{
		time_tm.tm_mday  = buff_bcd_get_le_uint(buff, *offset, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_MONTH)
	{
		time_tm.tm_mon   = buff_bcd_get_le_uint(buff, *offset, 1) - 1;
		*offset += 1;
	}

	if(flags & DLT_ASDU_YEAR)
	{
		time_tm.tm_year  = buff_bcd_get_le_uint(buff, *offset, 1) + 100;
		*offset += 1;
	}

	return mktime(&time_tm);
}


void dlt_asdu_build_time_tag(unsigned char *buff, uint32_t *offset, time_t time_tag, uint8_t flags)
{
	struct tm time_tm;

	time_tm = *localtime(&time_tag);

	if(flags & DLT_ASDU_SEC)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_sec, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_MIN)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_min, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_HOUR)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_hour, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_WDAY)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_wday, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_MDAY)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_mday, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_MONTH)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_mon + 1, 1);
		*offset += 1;
	}

	if(flags & DLT_ASDU_YEAR)
	{
		buff_bcd_put_le_uint(buff, *offset, time_tm.tm_year - 100, 1);
		*offset += 1;
	}
}


uint16_t dlt_asdu_parse_energy(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu)
{
	int i, pos;
	uint32_t block_id;
	uint8_t type_size, frc_size, start_id;
	uint16_t res;

	pos = dlt_asdu_check_data_block(buff, *offset);

	res = dlt_asdu_find_type_params(buff_get_le_uint32(buff, *offset), &type_size, &frc_size, &start_id);

	if(res != RES_SUCCESS) return res;

	if(frc_size > 0)
		dlt_asdu->type = 36;
	else
		dlt_asdu->type = 35;

	// set to field fnc value cause of transmission - COT_Per_Cyc = 1 (cyclic transmission by IEC101/104 specifications)
	dlt_asdu->fnc = 1;

	if(pos == -1)
	{
		// check if buffer length is correct
		if(buff_len - *offset - 4 != type_size) return RES_INCORRECT;

		dlt_asdu->size = 1;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		dlt_asdu->data->id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		dlt_asdu->data->time_tag = time(NULL);

		dlt_asdu->data->value_type = ASDU_VAL_FLT;

		dlt_asdu->data->value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
		*offset += type_size;
	}
	else
	{
		// calculate number of items in block
		dlt_asdu->size = (buff_len - *offset - 4) / type_size;

		// check if buffer length is correct
		if(buff_len - *offset - 4 != dlt_asdu->size*type_size) return RES_INCORRECT;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		block_id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		for(i=0; i<dlt_asdu->size; i++)
		{
			dlt_asdu->data[i].id = dlt_asdu_build_data_unit_id(block_id, pos, i + start_id);

			dlt_asdu->data[i].time_tag = time(NULL);

			dlt_asdu->data[i].value_type = ASDU_VAL_FLT;

			dlt_asdu->data[i].value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
			*offset += type_size;
		}
	}

	return RES_SUCCESS;
}


uint16_t dlt_asdu_parse_inst_value(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu)
{
	int i, pos;
	uint32_t block_id;
	uint8_t type_size, frc_size, start_id;
	uint16_t res;

	pos = dlt_asdu_check_data_block(buff, *offset);

	res = dlt_asdu_find_type_params(buff_get_le_uint32(buff, *offset), &type_size, &frc_size, &start_id);

	if(res != RES_SUCCESS) return res;

	if(frc_size > 0)
		dlt_asdu->type = 36;
	else
		dlt_asdu->type = 35;

	// set to field fnc value cause of transmission - COT_Per_Cyc = 1 (cyclic transmission by IEC101/104 specifications)
	dlt_asdu->fnc = 1;

	if(pos == -1)
	{
		// check if buffer length is correct
		if(buff_len - *offset - 4 != type_size) return RES_INCORRECT;

		dlt_asdu->size = 1;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		dlt_asdu->data->id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		dlt_asdu->data->time_tag = time(NULL);

		if(frc_size > 0)
		{
			dlt_asdu->data->value_type = ASDU_VAL_FLT;
			dlt_asdu->data->value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
		}
		else
		{
			dlt_asdu->data->value_type = ASDU_VAL_UINT;
			dlt_asdu->data->value.ui = buff_bcd_get_le_uint(buff, *offset, type_size);
		}
		*offset += type_size;
	}
	else
	{
		// calculate number of items in block
		dlt_asdu->size = (buff_len - *offset - 4) / type_size;

		// check if buffer length is correct
		if(buff_len - *offset - 4 != dlt_asdu->size*type_size) return RES_INCORRECT;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		block_id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		for(i=0; i<dlt_asdu->size; i++)
		{
			dlt_asdu->data[i].id = dlt_asdu_build_data_unit_id(block_id, pos, i + start_id);

			dlt_asdu->data[i].time_tag = time(NULL);

			if(frc_size > 0)
			{
				dlt_asdu->data[i].value_type = ASDU_VAL_FLT;
				dlt_asdu->data[i].value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
			}
			else
			{
				dlt_asdu->data->value_type = ASDU_VAL_UINT;
				dlt_asdu->data->value.ui = buff_bcd_get_le_uint(buff, *offset, type_size);
			}
			*offset += type_size;
		}
	}

	return RES_SUCCESS;
}


uint16_t dlt_asdu_parse_event_log(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu)
{
	int i, pos;
	uint32_t block_id;
	uint8_t type_size, frc_size, start_id;
	uint16_t res;

	// check if it's data item or block
	pos = dlt_asdu_check_data_block(buff, *offset);

	res = dlt_asdu_find_type_params(buff_get_le_uint32(buff, *offset), &type_size, &frc_size, &start_id);

	if(res != RES_SUCCESS) return res;

	if(frc_size > 0)
		dlt_asdu->type = 36;
	else
		dlt_asdu->type = 35;

	// set to field fnc value cause of transmission - COT_Per_Cyc = 1 (cyclic transmission by IEC101/104 specifications)
	dlt_asdu->fnc = 1;

	if(pos == -1)
	{
		// check if buffer length is correct
		if(buff_len - *offset - 4 != type_size) return RES_INCORRECT;

		dlt_asdu->size = 1;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		dlt_asdu->data->id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		if(frc_size > 0)
		{
			dlt_asdu->data->value_type = ASDU_VAL_FLT;
			dlt_asdu->data->value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
		}
		else
		{
			dlt_asdu->data->value_type = ASDU_VAL_UINT;
			dlt_asdu->data->value.ui = buff_bcd_get_le_uint(buff, *offset, type_size);
		}
		*offset += type_size;
	}
	else
	{
		// calculate number of items in block
		dlt_asdu->size = (buff_len - *offset - 4) / type_size;

		// check if buffer length is correct
		if(buff_len - *offset - 4 != dlt_asdu->size*type_size) return RES_INCORRECT;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		block_id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		for(i=0; i<dlt_asdu->size; i++)
		{
			dlt_asdu->data[i].id = dlt_asdu_build_data_unit_id(block_id, pos, i + start_id);

			if(frc_size > 0)
			{
				dlt_asdu->data[i].value_type = ASDU_VAL_FLT;
				dlt_asdu->data[i].value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
			}
			else
			{
				dlt_asdu->data->value_type = ASDU_VAL_UINT;
				dlt_asdu->data->value.ui = buff_bcd_get_le_uint(buff, *offset, type_size);
			}
			*offset += type_size;
		}
	}

	return RES_SUCCESS;
}


uint16_t dlt_asdu_parse_parameter(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *dlt_asdu)
{
	int i, pos;
	uint32_t block_id;
	uint8_t type_size, frc_size, start_id;
	uint16_t res;

	// check if it's data item or block
	pos = dlt_asdu_check_data_block(buff, *offset);

	res = dlt_asdu_find_type_params(buff_get_le_uint32(buff, *offset), &type_size, &frc_size, &start_id);

	if(res != RES_SUCCESS) return res;

	if(frc_size > 0)
		dlt_asdu->type = 36;
	else
		dlt_asdu->type = 35;

	// set to field fnc value cause of transmission - COT_Req = 5 (request or requested data by IEC101/104 specifications)
	dlt_asdu->fnc = 5;

	if(pos == -1)
	{
		// check if buffer length is correct
		if(buff_len - *offset - 4 != type_size) return RES_INCORRECT;

		dlt_asdu->size = 1;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		dlt_asdu->data->id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		if(frc_size > 0)
		{
			dlt_asdu->data->value_type = ASDU_VAL_FLT;
			dlt_asdu->data->value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
		}
		else
		{
			dlt_asdu->data->value_type = ASDU_VAL_UINT;
			dlt_asdu->data->value.ui = buff_bcd_get_le_uint(buff, *offset, type_size);
		}
		*offset += type_size;
	}
	else
	{
		// calculate number of items in block
		dlt_asdu->size = (buff_len - *offset - 4) / type_size;

		// check if buffer length is correct
		if(buff_len - *offset - 4 != dlt_asdu->size*type_size) return RES_INCORRECT;

		dlt_asdu->data = (data_unit*) calloc(dlt_asdu->size, sizeof(data_unit));

		if(dlt_asdu->data == NULL)
		{
			dlt_asdu->size = 0;
			return RES_MEM_ALLOC;
		}

		block_id = buff_get_le_uint32(buff, *offset);
		*offset += 4;

		for(i=0; i<dlt_asdu->size; i++)
		{
			dlt_asdu->data[i].id = dlt_asdu_build_data_unit_id(block_id, pos, i + start_id);

			if(frc_size > 0)
			{
				dlt_asdu->data[i].value_type = ASDU_VAL_FLT;
				dlt_asdu->data[i].value.f = buff_bcd_get_le_flt(buff, *offset, type_size, frc_size);
			}
			else
			{
				dlt_asdu->data->value_type = ASDU_VAL_UINT;
				dlt_asdu->data->value.ui = buff_bcd_get_le_uint(buff, *offset, type_size);
			}
			*offset += type_size;
		}
	}

	return RES_SUCCESS;
}


uint16_t dlt_asdu_buff_parse(unsigned char *buff, uint32_t buff_len, asdu *dlt_asdu)
{
	if(!buff || buff_len < DLT_ASDU_LEN_MIN) return RES_INCORRECT;

	uint8_t d3;
	uint16_t res;
	uint32_t offset = 0;

	dlt_asdu->proto = PROTO_DLT645;

	d3 = buff_get_le_uint8(buff, 3);

	switch(d3)
	{
	case 0x00:
		res = dlt_asdu_parse_energy(buff, buff_len, &offset, dlt_asdu);

		break;

	case 0x02:
		res = dlt_asdu_parse_inst_value(buff, buff_len, &offset, dlt_asdu);

		break;
	case 0x03:
		res = dlt_asdu_parse_event_log(buff, buff_len, &offset, dlt_asdu);

		break;

	case 0x04:
		res = dlt_asdu_parse_parameter(buff, buff_len, &offset, dlt_asdu);

		break;

	default:
		res = RES_UNKNOWN;

		break;
	}

#ifdef _DEBUG
	if(res == RES_SUCCESS) printf("%s: ASDU parsed OK. Type = %d, IO num = %d, SQ = %d\n", "unitlink-dlt645", dlt_asdu->type, dlt_asdu->size, dlt_asdu->attr & 0x01);
#endif

	return res;
}

