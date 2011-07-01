/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
#include <string.h>
#include "asdu.h"


/*
 *
 * Functions
 *
 */

asdu *asdu_create()
{
	// try to allocate memory for the structure
	asdu *unit = (asdu*) calloc(1, sizeof(asdu));

	return unit;
}


void asdu_destroy(asdu **unit)
{
	// fast check input data
	if(!*unit) return;

	// cleanup insides
	asdu_cleanup_data(*unit);

	free(*unit);
	*unit = NULL;
}


void asdu_cleanup_data(asdu *unit)
{
	// fast check input data
	if(!unit || !unit->data) return;

	// free memory allocated for the data array
	free(unit->data);
	unit->data = NULL;

	unit->size = 0;
}



uint8_t asdu_from_byte(unsigned char *buff, uint32_t buff_len, asdu **unit)
{
	// fast check input data
	if(buff_len < sizeof(asdu) || !unit) return RES_ASDU_INCORRECT;

	*unit = asdu_create();

	memcpy((void*)(*unit), (void*)buff, sizeof(asdu));

	if((*unit)->size == 0 || buff_len < sizeof(asdu) + (*unit)->size * sizeof(data_unit))
	{
		asdu_destroy(unit);
		return RES_ASDU_INCORRECT;
	}

	(*unit)->data = NULL;

	(*unit)->data = (data_unit*) malloc((*unit)->size * sizeof(data_unit));

	if(!(*unit)->data)
	{
		asdu_destroy(unit);
		return RES_ASDU_MEM_ALLOC;
	}

	memcpy((void*)(*unit)->data, (void*)(buff + sizeof(asdu)), (*unit)->size * sizeof(data_unit));

	return RES_ASDU_SUCCESS;
}


uint8_t asdu_to_byte(unsigned char **buff, uint32_t *buff_len, asdu *unit)
{
	// fast check input data
	if(!buff || !unit || !unit->data ) return RES_ASDU_INCORRECT;

	// calculate buffer length
	*buff_len = sizeof(asdu) + unit->size * sizeof(data_unit);

	// allocate memory for the buffer
	*buff = (unsigned char*) malloc(*buff_len);

	if(!*buff)
	{
		*buff_len = 0;

		return RES_ASDU_MEM_ALLOC;
	}

	memcpy((void*)(*buff), (void*)unit, sizeof(asdu));

	memcpy((void*)(*buff + sizeof(asdu)), (void*)unit->data, unit->size * sizeof(data_unit));

	return RES_ASDU_SUCCESS;
}














