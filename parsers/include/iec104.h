/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC104_H_
#define _IEC104_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <signal.h>
#include "p_num.h"
#include "iec_def.h"
#include "apdu_frame.h"
#include "asdu.h"
#include "iec_asdu.h"
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

int iec104_rcvdata(char *buf, int len);


int iec104_rcvinit(char *buf, int len);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);


#ifdef __cplusplus
}
#endif

#endif //_IEC104_H_
