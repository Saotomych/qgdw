/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _ASDU_H_
#define _ASDU_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

/* Protocols */
#define PROTO_UNKNOWN		0
#define PROTO_IEC101		1
#define PROTO_IEC104		2
#define PROTO_DLT645		3


/* Value types */
#define ASDU_VAL_NONE		0	/* no value (used in ASDU like time synchronization, broadcast request, etc.) */
#define ASDU_VAL_INT		1	/* integer 32-bit */
#define ASDU_VAL_UINT		2	/* unsigned integer 32-bit */
#define ASDU_VAL_FLT		3	/* float */
#define ASDU_VAL_BOOL		4	/* boolean*/


/*
 *
 * Structures
 *
 */

/* data unit structure for universal ASDU */
typedef struct data_unit {
	uint32_t		id;			/* device's internal identifier of data unit */

	union {
		uint32_t	ui;			/* unsigned integer representation */
		int32_t		i;			/* integer representation */
		float		f;			/* float representation */
	} value;					/* transferring value (e.g. measured value, counter readings, etc.) */

	uint8_t			value_type; /* e.g. integer, unsigned integer, float, boolean, etc. (need this for "on fly" protocol converting) */
	uint8_t			attr;		/* value's additional attributes (e.g. quality descriptor, command qualifier, etc.) */

	int32_t			time_tag;	/* time tag in time_t representation */
	uint8_t			time_iv;	/* invalid(1)/valid(0) */
} data_unit;


/* universal ASDU structure */
typedef struct asdu {
	/* Header */
	uint16_t		proto;		/* protocol identifier */
	uint32_t		type;		/* protocol specific ASDU type */
	uint64_t		adr;		/* ASDU address */
	uint8_t			fnc;		/* protocol specific command/function/cause/etc. */
	uint8_t			attr;		/* additional protocol specific ASDU attributes */

	/* Data */
	uint8_t			size;		/* data units array size */
	data_unit		*data;		/* data units array */
} asdu;


/*
 *
 * Functions
 *
 */

/* create asdu structure and initialize insides */
asdu *asdu_create();

/* cleanup asdu structure insides and destroy it */
void asdu_destroy(asdu **unit);

/* cleanup data array of asdu structure insides */
void asdu_cleanup_data(asdu *unit);



#ifdef __cplusplus
}
#endif

#endif //_ASDU_H_