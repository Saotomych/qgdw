/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _DLT645_H_
#define _DLT645_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */

/*
 *
 * Structures
 *
 */

/* DLT645 communication configuration */
typedef struct dlt645_cfg {
	/* Communication parameters */
	uint8_t		port;			/* serial port number */
	uint64_t	adr;			/* link address/ASDU address (slave) */
	uint8_t		host_type;		/* master/slave */
	uint8_t		net_type;		/* point-2-point/multiple point-2-point/etc */
} dlt645_cfg;

/*
 *
 * Functions
 *
 */




#ifdef __cplusplus
}
#endif

#endif //_DLT645_H_
