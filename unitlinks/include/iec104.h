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
#include "../../common/asdu.h"
#include "iec_asdu.h"
#include "../../devlinks/devlink.h"
#include "../../common/multififo.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */
#define APP_NAME	"unitlink-iec104"
#define APP_PATH 	"/rw/mx00/unitlinks"


/*
 *
 * Structures
 *
 */

typedef struct iec104_ep_ext {
	uint16_t		adr;		/* link (ASDU) address */

	uint16_t		send_num;	/* send sequence number */
	uint16_t		recv_num;	/* receive sequence number */

	uint16_t		ack_num;	/* last sent acknowledged ASDU */


} iec104_ep_ext;


static iec104_ep_ext *ep_exts[64];

/*
 *
 * Functions
 *
 */

int iec104_rcvdata(int len);


int iec104_rcvinit(char **buf, int len);


void sighandler_sigchld(int arg);


void sighandler_sigquit(int arg);


#ifdef __cplusplus
}
#endif

#endif //_IEC104_H_
