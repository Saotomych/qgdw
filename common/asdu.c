/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */

#include <malloc.h>
#include <string.h>
#include "asdu.h"
#include "ts_print.h"


/*
 *
 * Functions
 *
 */

asdu *asdu_create()
{
	asdu *unit = (asdu*) calloc(1, sizeof(asdu));

	return unit;
}


void asdu_destroy(asdu **unit)
{
	if(!*unit) return;

	asdu_cleanup_data(*unit);

	free(*unit);
	*unit = NULL;
}


void asdu_cleanup_data(asdu *unit)
{
	if(!unit || !unit->data) return;

	free(unit->data);
	unit->data = NULL;

	unit->size = 0;
}


uint16_t asdu_from_byte(unsigned char *buff, uint32_t buff_len, asdu **unit)
{
	if(buff_len < sizeof(asdu) || !unit) return RES_INCORRECT;

	*unit = asdu_create();

	memcpy((void*)(*unit), (void*)buff, sizeof(asdu));

	if((*unit)->size == 0 || buff_len < sizeof(asdu) + (*unit)->size * sizeof(data_unit))
	{
		asdu_destroy(unit);
		return RES_INCORRECT;
	}

	(*unit)->data = NULL;

	(*unit)->data = (data_unit*) malloc((*unit)->size * sizeof(data_unit));

	if(!(*unit)->data)
	{
		asdu_destroy(unit);
		return RES_MEM_ALLOC;
	}

	memcpy((void*)(*unit)->data, (void*)(buff + sizeof(asdu)), (*unit)->size * sizeof(data_unit));

	return RES_SUCCESS;
}


uint16_t asdu_to_byte(unsigned char **buff, uint32_t *buff_len, asdu *unit)
{
	if(!buff || !unit || !unit->data ) return RES_INCORRECT;

	*buff_len = sizeof(asdu) + unit->size * sizeof(data_unit);

	*buff = (unsigned char*) malloc(*buff_len);

	if(!*buff)
	{
		*buff_len = 0;

		return RES_MEM_ALLOC;
	}

	memcpy((void*)(*buff), (void*)unit, sizeof(asdu));

	memcpy((void*)(*buff + sizeof(asdu)), (void*)unit->data, unit->size * sizeof(data_unit));

	return RES_SUCCESS;
}


uint16_t asdu_map_read(asdu_map **m_list, const char *file_name, const char *app_name, uint8_t num_base)
{
	FILE *map_file = NULL;
	char r_buff[256] = {0};
	uint32_t map_num;
	uint32_t proto_id, base_id;
	char name[DOBJ_NAMESIZE] = {0};
	int res;

	map_file = fopen(file_name, "r");

	if(!map_file) return RES_NOT_FOUND;

	map_num = 0;

	while(fgets(r_buff, 255, map_file))
	{
		if(strstr(r_buff, "#")) continue;

		if(num_base == DEC_BASE)
			res = sscanf(r_buff, "%d %d %s", &proto_id, &base_id, name);
		else
			res = sscanf(r_buff, "%x %d %s", &proto_id, &base_id, name);

		if(res < 2) continue;

		asdu_add_map_item(m_list, proto_id, base_id, name, app_name, num_base);

		map_num++;
	}

	fclose(map_file);

	if(map_num)
		return RES_SUCCESS;
	else
		return RES_NOT_FOUND;
}


uint16_t asdu_add_map_item(asdu_map **m_list, uint32_t proto_id, uint32_t base_id, const char *name, const char *app_name, uint8_t num_base)
{
	asdu_map *last_map = NULL, *new_map = NULL;

	new_map = (asdu_map*) malloc(sizeof(asdu_map));

	if(!new_map) return RES_MEM_ALLOC;

	new_map->proto_id = proto_id;
	new_map->base_id = base_id;
	memcpy((void*)new_map->name, (void*)name, DOBJ_NAMESIZE);

	last_map = *m_list;

	while(last_map && last_map->next)
	{
		last_map = last_map->next;
	}

	new_map->prev = last_map;
	new_map->next = NULL;

	if(!*m_list)
		*m_list = new_map;
	else
		last_map->next = new_map;

#ifdef _DEBUG
	if(num_base == DEC_BASE)
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: New asdu_map added. proto_id = %d, base_id = %d, iec61850 = %s\n", app_name, new_map->proto_id, new_map->base_id, new_map->name);
	else
		ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: New asdu_map added. proto_id = 0x%X, base_id = %d, iec61850 = %s\n", app_name, new_map->proto_id, new_map->base_id, new_map->name);
#endif

	return RES_SUCCESS;
}


asdu_map *asdu_get_map_item(asdu_map **m_list, uint32_t id, uint8_t get_by)
{
	asdu_map *res_map = NULL;

	res_map = *m_list;

	while(res_map)
	{
		if(get_by == PROTO_ID && res_map->proto_id == id) break;
		if(get_by == BASE_ID && res_map->base_id == id) break;

		res_map = res_map->next;
	}

	return res_map;
}


void asdu_map_ids(asdu_map **m_list, asdu *cur_asdu, const char *app_name, uint8_t num_base)
{
	if(!cur_asdu) return;

	int i;
	asdu_map *res_map = NULL;

	for(i=0; i<cur_asdu->size; i++)
	{
		res_map = asdu_get_map_item(m_list, cur_asdu->data[i].id, PROTO_ID);

		if(res_map)
		{
			cur_asdu->data[i].id = res_map->base_id;
			memcpy((void*)cur_asdu->data[i].name, (void*)res_map->name, DOBJ_NAMESIZE);
		}
		else
		{
			cur_asdu->data[i].id = 0xFFFFFFFF;
			cur_asdu->data[i].name[0] = 0;
		}

#ifdef _DEBUG
		if(res_map)
		{
			if(num_base == DEC_BASE)
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Identifier mapped. Address = %d, proto_id = %d, base_id = %d\n", app_name, cur_asdu->adr, res_map->proto_id, res_map->base_id);
			else
				ts_printf(STDOUT_FILENO, TS_DEBUG, "%s: Identifier mapped. Address = %d, proto_id = 0x%X, base_id = %d\n", app_name, cur_asdu->adr, res_map->proto_id, res_map->base_id);
		}
#endif
	}
}






















