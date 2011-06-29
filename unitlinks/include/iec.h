/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC_H_
#define _IEC_H_


#include <stdint.h>
#include "iec_def.h"
#include "iec101.h"
#include "iec104.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */

/*
 *
 * Structures
 *
 */

/* IEC instance */
typedef struct iec_instance {
	/* Communication configuration */
	union {
		iec101_cfg iec101;
		iec104_cfg iec104;
	} comm_cfg;

	/* Function pointers */

} iec101_instance;

/*
 *
 * Functions
 *
 */



#ifdef __cplusplus
}
#endif

#endif //_IEC101_H_
