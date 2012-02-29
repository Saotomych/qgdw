/*
 * hmi.h
 *
 *  Created on: 16.12.2011
 *      Author: dmitry
 */

#ifndef HMI_H_
#define HMI_H_

#include <nano-X.h>
#include <nanowm.h>
#include "menu.h"

typedef struct _fact{
   char *action;
   int (*proloque)(void *arg);
   int (*epiloque)(void *arg);
} fact, *pfact;

// lowlevel.cfg information
typedef struct _LDEXTINFO{
	LIST l;
	char	*llstring;		// full low level string
	int		asduadr;		// req
	char 	*addr;			// req
	char	*nameul;		// req
	char	*portmode;		// req
	char	*sync;			// optional
} ldextinfo;

#endif


