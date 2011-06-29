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
#include "../../common/asdu.h"
#include "dlt_asdu.h"
#include "../../devlinks/devlink.h"
#include "../../common/multififo.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */
#define APP_NAME	"unitlink-dlt645"
#define APP_PATH 	"/rw/mx00/unitlinks"

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

int dlt645_rcvdata(int len);


int dlt645_rcvinit(char *buf, int len);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);



#ifdef __cplusplus
}
#endif

#endif //_DLT645_H_
