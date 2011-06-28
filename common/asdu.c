/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <malloc.h>
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














