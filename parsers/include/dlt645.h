/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _DLT645_H_
#define _DLT645_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <signal.h>
#include "p_num.h"
#include "dlt_frame.h"
#include "asdu.h"
#include "dlt_asdu.h"
#include "../../common/multififo.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */

/*
 *
 * Structures
 *
 */


/*
 *
 * Functions
 *
 */

int dlt645_rcvdata(char *buf, int len);


int dlt645_rcvinit(char *buf, int len);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);



#ifdef __cplusplus
}
#endif

#endif //_DLT645_H_
