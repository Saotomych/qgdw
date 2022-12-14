/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "../include/iec_asdu_def.h"
#include "../include/iec_asdu.h"
#include "../../common/resp_codes.h"
#include "../../common/p_num.h"
#include "../../common/ts_print.h"


uint8_t iec_asdu_check_io_buffer_len(uint32_t buff_len, uint32_t offset, uint8_t type_size, asdu *iec_asdu, uint8_t ioa_len)
{
	// check if buffer long enough by calculating needed buffer size for information objects
	if(((iec_asdu->attr & 0x01) == 0 && buff_len - offset < iec_asdu->size * (ioa_len + type_size)) ||
	   ((iec_asdu->attr & 0x01) == 1 && buff_len - offset < ioa_len + iec_asdu->size * type_size))
	{
		return RES_INCORRECT;
	}

	return RES_SUCCESS;
}


uint32_t iec_asdu_calculate_buffer_len(uint8_t type_buf_size, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len, uint8_t	ioa_len)
{
	uint32_t buff_len = 0;

	buff_len += 1 + 1 + cot_len + coa_len; // type size + num size + cause size + common object address size

	// add information objects size depends on SQ flag
	buff_len += (iec_asdu->attr & 0x01) ? (ioa_len + iec_asdu->size * type_buf_size) : (iec_asdu->size * (ioa_len + type_buf_size));

	return buff_len;
}


uint32_t iec_asdu_parse_obj_address(unsigned char *buff, uint32_t *offset, uint8_t address_len)
{
	uint32_t address;

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

	case 3:
		address  = buff_get_le_uint24(buff, *offset);
		*offset += 3;
		break;

	case 4:
		address  = buff_get_le_uint32(buff, *offset);
		*offset += 4;
		break;

	default:
		address = 0;
		break;
	}

	return address;
}


void iec_asdu_build_obj_address(unsigned char *buff, uint32_t *offset, uint32_t address, uint8_t address_len)
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

	case 3:
		buff_put_le_uint24(buff, *offset, address);
		*offset += 3;
		break;

	case 4:
		buff_put_le_uint32(buff, *offset, address);
		*offset += 4;
		break;

	default:
		break;
	}
}


void time_t_to_cp56time2a(time_t timet, cp56time2a *cp56t2a)
{
	struct tm time_tm;

	time_tm = *localtime(&timet);

	cp56t2a->msec  = time_tm.tm_sec * 1000;
	cp56t2a->min   = time_tm.tm_min;
	cp56t2a->res1  = 0;
	cp56t2a->iv    = 0;
	cp56t2a->hour  = time_tm.tm_hour;
	cp56t2a->su    = time_tm.tm_isdst;
	cp56t2a->mday  = time_tm.tm_mday;
	cp56t2a->wday  = time_tm.tm_wday ? time_tm.tm_wday : 7;
	cp56t2a->month = time_tm.tm_mon + 1;
	cp56t2a->year  = time_tm.tm_year - 100;
}


void current_cp56time2a(cp56time2a *cp56t2a)
{
	time_t time_tm;

	time_tm = time(NULL);

	time_t_to_cp56time2a(time_tm, cp56t2a);
}


time_t cp56time2a_to_time_t(cp56time2a *cp56t2a)
{
	struct tm time_tm;

	time_tm.tm_sec   = (int) (cp56t2a->msec / 1000);
	time_tm.tm_min   = cp56t2a->min;
	time_tm.tm_hour  = cp56t2a->hour;
	time_tm.tm_isdst = cp56t2a->su;
	time_tm.tm_mday  = cp56t2a->mday;
	time_tm.tm_wday  = cp56t2a->wday == 7 ? 0 : cp56t2a->wday;
	time_tm.tm_mon   = cp56t2a->month - 1;
	time_tm.tm_year  = cp56t2a->year + 100;

	return mktime(&time_tm);
}


void iec_asdu_parse_cp56time2a(unsigned char *buff, uint32_t *offset, cp56time2a *cp56t2a)
{
	cp56t2a->msec  = buff_get_le_uint16(buff, *offset);
	*offset += 2;

	cp56t2a->min   = buff_get_le_uint8(buff, *offset) & 0x3F;
	cp56t2a->res1  = (buff_get_le_uint8(buff, *offset) & 0x40) >> 6;
	cp56t2a->iv    = (buff_get_le_uint8(buff, *offset) & 0x80) >> 7;
	*offset += 1;

	cp56t2a->hour  = buff_get_le_uint8(buff, *offset) & 0x1F;
	cp56t2a->su    = (buff_get_le_uint8(buff, *offset) & 0x80) >> 7;
	*offset += 1;

	cp56t2a->mday  = buff_get_le_uint8(buff, *offset) & 0x1F;
	cp56t2a->wday  = (buff_get_le_uint8(buff, *offset) & 0xE0) >> 5;
	*offset += 1;

	cp56t2a->month = buff_get_le_uint8(buff, *offset) & 0x0F;
	*offset += 1;

	cp56t2a->year 	= buff_get_le_uint8(buff, *offset) & 0x7F;
	*offset += 1;
}


void iec_asdu_build_cp56time2a(unsigned char *buff, uint32_t *offset, cp56time2a *cp56t2a)
{
	uint8_t bytex;

	buff_put_le_uint16(buff, *offset, cp56t2a->msec);
	*offset += 2;

	bytex = cp56t2a->min | (cp56t2a->res1 << 6) | (cp56t2a->iv << 7);
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	bytex = cp56t2a->hour | (cp56t2a->su << 7);
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	bytex = cp56t2a->mday | (cp56t2a->wday << 5 );
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	buff_put_le_uint8(buff, *offset, cp56t2a->month);
	*offset += 1;

	buff_put_le_uint8(buff, *offset, cp56t2a->year);
	*offset += 1;
}


void iec_asdu_parse_C_CS(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_NONE;

	cp56time2a time_tag;
	iec_asdu_parse_cp56time2a(buff, offset, &time_tag);
	unit->time_tag = cp56time2a_to_time_t(&time_tag);
	unit->time_iv  = time_tag.iv;
}


void iec_asdu_build_C_CS(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	cp56time2a time_tag;
	time_t_to_cp56time2a(unit->time_tag, &time_tag);
	time_tag.iv = unit->time_iv;
	iec_asdu_build_cp56time2a(buff, offset, &time_tag);
}


void iec_asdu_parse_C_CI(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_QCC_RQT;
	unit->attr       = buff_get_le_uint8(buff, *offset) & IEC_ASDU_QCC_FRZ;
	*offset += 1;
}


void iec_asdu_build_C_CI(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | unit->attr;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;
}


void iec_asdu_parse_C_IC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   = buff_get_le_uint8(buff, *offset);
	*offset += 1;
}


void iec_asdu_build_C_IC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	buff_put_le_uint8(buff, *offset, unit->value.ui);
	*offset += 1;
}


void iec_asdu_parse_C_SE(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	if(type == C_SE_NA_1 || type == C_SE_NB_1)
	{
		unit->value_type = ASDU_VAL_INT;
		unit->value.i    = buff_get_le_uint16(buff, *offset);
		*offset += 2;
	}

	if(type == C_SE_NC_1)
	{
		unit->value_type = ASDU_VAL_FLT;
		unit->value.f    = buff_get_le_ieee_float(buff, *offset);
		*offset += 4;
	}

	unit->attr = buff_get_le_uint8(buff, *offset);
	*offset += 1;
}


void iec_asdu_build_C_SE(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	if(type == C_SE_NA_1 || type == C_SE_NB_1)
	{
		buff_put_le_uint16(buff, *offset, unit->value.i);
		*offset += 2;
	}

	if(type == C_SE_NC_1)
	{
		buff_put_le_ieee_float(buff, *offset, unit->value.f);
		*offset += 4;
	}

	buff_put_le_uint8(buff, *offset, unit->attr);
	*offset += 1;
}


void iec_asdu_parse_C_RC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_RCO_RCS;
	unit->attr       = buff_get_le_uint8(buff, *offset) & ~IEC_ASDU_RCO_RCS;
	*offset += 1;
}


void iec_asdu_build_C_RC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | unit->attr;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;
}


void iec_asdu_parse_C_DC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_DCO_DCS;
	unit->attr       = buff_get_le_uint8(buff, *offset) & ~IEC_ASDU_DCO_DCS;
	*offset += 1;
}


void iec_asdu_build_C_DC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | unit->attr;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;
}


void iec_asdu_parse_C_SC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_BOOL;
	unit->value.ui   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_SCO_SCS;
	unit->attr       = buff_get_le_uint8(buff, *offset) & ~IEC_ASDU_SCO_SCS;
	*offset += 1;
}


void iec_asdu_build_C_SC(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | unit->attr;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;
}


void iec_asdu_parse_M_IT(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   = buff_get_le_uint32(buff, *offset);
	*offset += 4;

	unit->attr = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	if(type == M_IT_TB_1)
	{
		cp56time2a time_tag;
		iec_asdu_parse_cp56time2a(buff, offset, &time_tag);
		unit->time_tag = cp56time2a_to_time_t(&time_tag);
		unit->time_iv  = time_tag.iv;
	}
}


void iec_asdu_build_M_IT(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	buff_put_le_uint32(buff, *offset, unit->value.ui);
	*offset += 4;

	buff_put_le_uint8(buff, *offset, unit->attr);
	*offset += 1;

	if(type == M_IT_TB_1)
	{
		cp56time2a time_tag;
		time_t_to_cp56time2a(unit->time_tag, &time_tag);
		time_tag.iv = unit->time_iv;
		iec_asdu_build_cp56time2a(buff, offset, &time_tag);
	}
}


void iec_asdu_parse_M_ME(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	if(type == M_ME_NA_1 || type == M_ME_NB_1 || type == M_ME_TD_1 || type == M_ME_TE_1)
	{
		unit->value_type = ASDU_VAL_INT;
		unit->value.i    = buff_get_le_uint16(buff, *offset);
		*offset += 2;
	}

	if(type == M_ME_NC_1 || type == M_ME_TF_1)
	{
		unit->value_type = ASDU_VAL_FLT;
		unit->value.f    = buff_get_le_ieee_float(buff, *offset);
		*offset += 4;
	}

	unit->attr = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	if(type == M_ME_TD_1 || type == M_ME_TE_1 || type == M_ME_TF_1)
	{
		cp56time2a time_tag;
		iec_asdu_parse_cp56time2a(buff, offset, &time_tag);
		unit->time_tag = cp56time2a_to_time_t(&time_tag);
		unit->time_iv  = time_tag.iv;
	}
}


void iec_asdu_build_M_ME(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	if(type == M_ME_NA_1 || type == M_ME_NB_1 || type == M_ME_TD_1 || type == M_ME_TE_1)
	{
		buff_put_le_uint16(buff, *offset, unit->value.i);
		*offset += 2;
	}

	if(type == M_ME_NC_1 || type == M_ME_TF_1)
	{
		buff_put_le_ieee_float(buff, *offset, unit->value.f);
		*offset += 4;
	}

	buff_put_le_uint8(buff, *offset, unit->attr);
	*offset += 1;

	if(type == M_ME_TD_1 || type == M_ME_TE_1 || type == M_ME_TF_1)
	{
		cp56time2a time_tag;
		time_t_to_cp56time2a(unit->time_tag, &time_tag);
		time_tag.iv = unit->time_iv;
		iec_asdu_build_cp56time2a(buff, offset, &time_tag);
	}
}


void iec_asdu_parse_M_ST(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   =  buff_get_le_uint8(buff, *offset) & IEC_ASDU_VTI_VAL;
	unit->attr       = (buff_get_le_uint8(buff, *offset) & IEC_ASDU_VTI_TS) >> 4;
	*offset += 1;

	unit->attr |= buff_get_le_uint8(buff, *offset);
	*offset += 1;

	if(type == M_ST_TB_1)
	{
		cp56time2a time_tag;
		iec_asdu_parse_cp56time2a(buff, offset, &time_tag);
		unit->time_tag = cp56time2a_to_time_t(&time_tag);
		unit->time_iv  = time_tag.iv;
	}
}


void iec_asdu_build_M_ST(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | ((unit->attr & 0x08) << 4);
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	bytex = unit->attr & ~0x08;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	if(type == M_ST_TB_1)
	{
		cp56time2a time_tag;
		time_t_to_cp56time2a(unit->time_tag, &time_tag);
		time_tag.iv = unit->time_iv;
		iec_asdu_build_cp56time2a(buff, offset, &time_tag);
	}
}


void iec_asdu_parse_M_DP(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_UINT;
	unit->value.ui   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_DIQ_DPI;
	unit->attr       = buff_get_le_uint8(buff, *offset) & ~IEC_ASDU_DIQ_DPI;
	*offset += 1;

	if(type == M_DP_TB_1)
	{
		cp56time2a time_tag;
		iec_asdu_parse_cp56time2a(buff, offset, &time_tag);
		unit->time_tag = cp56time2a_to_time_t(&time_tag);
		unit->time_iv  = time_tag.iv;
	}
}


void iec_asdu_build_M_DP(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | unit->attr;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	if(type == M_DP_TB_1)
	{
		cp56time2a time_tag;
		time_t_to_cp56time2a(unit->time_tag, &time_tag);
		time_tag.iv = unit->time_iv;
		iec_asdu_build_cp56time2a(buff, offset, &time_tag);
	}
}


void iec_asdu_parse_M_SP(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	unit->value_type = ASDU_VAL_BOOL;
	unit->value.ui   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_SIQ_SPI;
	unit->attr       = buff_get_le_uint8(buff, *offset) & ~IEC_ASDU_SIQ_SPI;
	*offset += 1;

	if(type == M_SP_TB_1)
	{
		cp56time2a time_tag;
		iec_asdu_parse_cp56time2a(buff, offset, &time_tag);
		unit->time_tag = cp56time2a_to_time_t(&time_tag);
		unit->time_iv  = time_tag.iv;
	}
}


void iec_asdu_build_M_SP(unsigned char *buff, uint32_t *offset, data_unit *unit, uint8_t type)
{
	uint8_t bytex;

	bytex = unit->value.ui | unit->attr;
	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	if(type == M_SP_TB_1)
	{
		cp56time2a time_tag;
		time_t_to_cp56time2a(unit->time_tag, &time_tag);
		time_tag.iv = unit->time_iv;
		iec_asdu_build_cp56time2a(buff, offset, &time_tag);
	}
}


uint8_t iec_asdu_parse_header(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len)
{
	if(!buff || buff_len - *offset < 1 + 1 + cot_len + coa_len) return RES_INCORRECT;

	iec_asdu->type = buff_get_le_uint8(buff, *offset);
	*offset += 1;

	iec_asdu->size  = buff_get_le_uint8(buff, *offset) & IEC_ASDU_HEAD_NUM;
	iec_asdu->attr = (buff_get_le_uint8(buff, *offset) & IEC_ASDU_HEAD_SQ) >> 7;
	*offset += 1;

	iec_asdu->fnc   = buff_get_le_uint8(buff, *offset) & IEC_ASDU_HEAD_CAUSE;
	iec_asdu->attr |= buff_get_le_uint8(buff, *offset) & ~IEC_ASDU_HEAD_CAUSE;
	*offset += 1;

	*offset += cot_len - 1;

	iec_asdu->adr = iec_asdu_parse_obj_address(buff, offset, coa_len);

	return RES_SUCCESS;
}


uint8_t iec_asdu_build_header(unsigned char *buff, uint32_t buff_len, uint32_t *offset, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len)
{
	uint8_t bytex;

	buff_put_le_uint8(buff, *offset, iec_asdu->type);
	*offset += 1;

	if(iec_asdu->proto == PROTO_IEC101 || iec_asdu->proto == PROTO_IEC104)
		bytex = iec_asdu->size | ((iec_asdu->attr & 0x01) << 7);
	else
		bytex = iec_asdu->size;

	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	// build cause of transmission
	bytex = iec_asdu->fnc | (iec_asdu->attr & ~IEC_ASDU_HEAD_CAUSE);

	buff_put_le_uint8(buff, *offset, bytex);
	*offset += 1;

	iec_asdu_build_obj_address(buff, offset, 0, cot_len - 1);

	iec_asdu_build_obj_address(buff, offset, iec_asdu->adr, coa_len);

	return RES_SUCCESS;
}


typedef void (*asdu_pb_funcp)(unsigned char*, uint32_t*, data_unit*, uint8_t type);


struct iec_asdu_parse_build_tab{
	uint8_t			type;
	uint8_t			type_size;
	asdu_pb_funcp	parse_funcp;
	asdu_pb_funcp	build_funcp;
}
iec_asdu_pb_tab[] = {
	{ M_SP_NA_1,	1,		&iec_asdu_parse_M_SP,		&iec_asdu_build_M_SP },
	{ M_DP_NA_1,	1,		&iec_asdu_parse_M_DP,		&iec_asdu_build_M_DP },
	{ M_ST_NA_1,	2,		&iec_asdu_parse_M_ST,		&iec_asdu_build_M_ST },
	{ M_ME_NA_1,	3,		&iec_asdu_parse_M_ME,		&iec_asdu_build_M_ME },
	{ M_ME_NB_1,	3,		&iec_asdu_parse_M_ME,		&iec_asdu_build_M_ME },
	{ M_ME_NC_1,	5,		&iec_asdu_parse_M_ME,		&iec_asdu_build_M_ME },
	{ M_IT_NA_1,	5,		&iec_asdu_parse_M_IT,		&iec_asdu_build_M_IT },
	{ M_SP_TB_1,	8,		&iec_asdu_parse_M_SP,		&iec_asdu_build_M_SP },
	{ M_DP_TB_1,	8,		&iec_asdu_parse_M_DP,		&iec_asdu_build_M_DP },
	{ M_ST_TB_1,	9,		&iec_asdu_parse_M_ST,		&iec_asdu_build_M_ST },
	{ M_ME_TD_1,	10,		&iec_asdu_parse_M_ME,		&iec_asdu_build_M_ME },
	{ M_ME_TE_1,	10,		&iec_asdu_parse_M_ME,		&iec_asdu_build_M_ME },
	{ M_ME_TF_1,	12,		&iec_asdu_parse_M_ME,		&iec_asdu_build_M_ME },
	{ M_IT_TB_1,	12,		&iec_asdu_parse_M_IT,		&iec_asdu_build_M_IT },
	{ C_SC_NA_1,	1,		&iec_asdu_parse_C_SC,		&iec_asdu_build_C_SC },
	{ C_DC_NA_1,	1,		&iec_asdu_parse_C_DC,		&iec_asdu_build_C_DC },
	{ C_RC_NA_1,	1,		&iec_asdu_parse_C_RC,		&iec_asdu_build_C_RC },
	{ C_SE_NA_1,	3,		&iec_asdu_parse_C_SE,		&iec_asdu_build_C_SE },
	{ C_SE_NB_1,	3,		&iec_asdu_parse_C_SE,		&iec_asdu_build_C_SE },
	{ C_SE_NC_1,	5,		&iec_asdu_parse_C_SE,		&iec_asdu_build_C_SE },
	{ C_IC_NA_1,	1,		&iec_asdu_parse_C_IC,		&iec_asdu_build_C_IC },
	{ C_CI_NA_1,	1,		&iec_asdu_parse_C_CI,		&iec_asdu_build_C_CI },
	{ C_CS_NA_1,	7,		&iec_asdu_parse_C_CS,		&iec_asdu_build_C_CS },
	{ 0, 0, NULL, NULL }
};


uint16_t iec_asdu_find_pb_func(uint8_t type, uint8_t *type_size, asdu_pb_funcp *p_funcp, asdu_pb_funcp *b_funcp)
{
	uint8_t i;

	for(i=0; iec_asdu_pb_tab[i].type != 0 ; i++)
	{
		if(iec_asdu_pb_tab[i].type == type)
		{
			if(type_size != NULL) *type_size = iec_asdu_pb_tab[i].type_size;
			if(p_funcp != NULL) *p_funcp = iec_asdu_pb_tab[i].parse_funcp;
			if(b_funcp != NULL) *b_funcp = iec_asdu_pb_tab[i].build_funcp;

			return RES_SUCCESS;
		}
	}

	return RES_UNKNOWN;
}


uint16_t iec_asdu_buff_parse(unsigned char *buff, uint32_t buff_len, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len, uint8_t ioa_len)
{
	if(!buff || buff_len < IEC_ASDU_LEN_MIN || !cot_len || !coa_len || !ioa_len) return RES_INCORRECT;

	uint16_t res, i;
	uint8_t type_size;
	asdu_pb_funcp asdu_object_parse;
	uint32_t offset = 0;

	iec_asdu->proto = PROTO_IEC101;

	res = iec_asdu_parse_header(buff, buff_len, &offset, iec_asdu, cot_len, coa_len);

	if(res != RES_SUCCESS) return res;

	res = iec_asdu_find_pb_func(iec_asdu->type, &type_size, &asdu_object_parse, NULL);

	if(res != RES_SUCCESS) return res;

	res = iec_asdu_check_io_buffer_len(buff_len, offset, type_size, iec_asdu, ioa_len);

	if(res != RES_SUCCESS) return res;

	iec_asdu->data = (data_unit*) calloc(iec_asdu->size, sizeof(data_unit));

	if(iec_asdu->data == NULL)
	{
		iec_asdu->size = 0;
		return RES_MEM_ALLOC;
	}

	for(i=0; i<iec_asdu->size; i++)
	{
		// get address of information object depending on SQ flag
		if((iec_asdu->attr & 0x01) == 0 || ((iec_asdu->attr & 0x01) == 1 && i == 0))
			iec_asdu->data[i].id = iec_asdu_parse_obj_address(buff, &offset, ioa_len);
		else
			iec_asdu->data[i].id = iec_asdu->data[i-1].id + 1;

		asdu_object_parse(buff, &offset, &iec_asdu->data[i], iec_asdu->type);
	}

	// set SQ flag in attributes field to zero
	iec_asdu->attr = iec_asdu->attr & 0xFE;

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: ASDU buffer parsed OK. Type = %d, IO num = %d, SQ = %d\n", "unitlink-iec10x", iec_asdu->type, iec_asdu->size, iec_asdu->attr & 0x01);
#endif

	return res;
}


uint16_t iec_asdu_buff_build(unsigned char **buff, uint32_t *buff_len, asdu *iec_asdu, uint8_t cot_len, uint8_t coa_len, uint8_t ioa_len)
{
	*buff_len = 0;
	uint32_t offset = 0;

	if(!buff || !iec_asdu || !iec_asdu->data || !cot_len || !coa_len || !ioa_len) return RES_INCORRECT;

	uint16_t res, i;
	uint8_t type_size;
	asdu_pb_funcp asdu_object_build;

	res = iec_asdu_find_pb_func(iec_asdu->type, &type_size, NULL, &asdu_object_build);

	if(res != RES_SUCCESS) return res;

	*buff_len = iec_asdu_calculate_buffer_len(type_size, iec_asdu, cot_len, coa_len, ioa_len);

	*buff = (unsigned char*) malloc(*buff_len);

	if(!*buff)
	{
		*buff_len = 0;

		return RES_MEM_ALLOC;
	}

	iec_asdu_build_header(*buff, *buff_len, &offset, iec_asdu, cot_len, coa_len);

	for(i=0; i<iec_asdu->size; i++)
	{
		// put address of information object depending on SQ flag
		if((iec_asdu->attr & 0x01) == 0 || ((iec_asdu->attr & 0x01) == 1 && i == 0))
			iec_asdu_build_obj_address(*buff, &offset, iec_asdu->data[i].id, ioa_len);

		asdu_object_build(*buff, &offset, &iec_asdu->data[i], iec_asdu->type);
	}

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: ASDU buffer builded OK. Type = %d, IO num = %d, SQ = %d\n", "unitlink-iec10x", iec_asdu->type, iec_asdu->size, iec_asdu->attr & 0x01);
#endif

	return res;
}

