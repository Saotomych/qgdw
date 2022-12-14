/*
 * common.h
 *
 *  Created on: 07.06.2011
 *      Author: alex
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "../common/types.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <langinfo.h>
#include <ctype.h>
#include <termios.h> /* Объявления управления POSIX-терминалом */
#include <fcntl.h>   /* Управление файлами */
#include <time.h>

#include <sys/queue.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* XML Params */
#define WIN	1
#define DOS 2
#define UTF 3
#define KOI8R 4

/*
 *
 * Constants and byte flags/masks
 *
 */

#define EP_USER_DATA			0		/* user data */
#define EP_MSG_INIT_CFG			1		/* initialization configuration */
#define EP_MSG_INIT_ACK			2		/* initialization acknowledgment */
#define EP_MSG_INIT_NACK		3		/* initialization negative acknowledgment */
#define EP_MSG_RECONNECT		4		/* re-connection request - up2down */
#define EP_MSG_CONNECT_ACK		5		/* connection acknowledgment - down2up after connect ops */
#define EP_MSG_CONNECT_NACK		6		/* connection negative acknowledgment - down2up after refused connect ops */
#define EP_MSG_CONNECT_CLOSE	7		/* connection close request - up2down */
#define EP_MSG_CONNECT_LOST		8		/* connection lost/closed by remote peer - down2up after exception */
#define EP_MSG_CONNECT			9		/* connection request - up2down */
#define EP_MSG_QUIT				10		/* quit application - up2down */
#define EP_MSG_NEWDOBJ			11		/* new demand of data object - up2down */
#define EP_MSG_TIME_SYNC		12		/* time sync command - down2up */
#define EP_MSG_COMM_INTER		13		/* common interrogation command - down2up */
#define EP_MSG_GETDOBJ			14		/* get data object now - up2down */
#define EP_MSG_DCOLL_START		15		/* start data collection - up2down */
#define EP_MSG_DCOLL_STOP		16		/* stop data collection - up2down */
#define EP_MSG_TEST_CONN		17		/* test device connection - up2down */
#define EP_MSG_DEV_ONLINE		18		/* device is online - down2up */
#define EP_MSG_VARATTACH		20		/* to attach data for event - up2down */
#define EP_MSG_VARDETACH		21		/* to unattach data from event - up2down */
#define EP_MSG_VAREVENT			22		/* event variable has changed - down2 up */
#define EP_MSG_VARREFRESH		23		/* to refresh all attsched variables - up2down */

#define EP_MSG_LOG_DEV_EVENT	25		/* device log event - down2up */
#define EP_MSG_LOG_APP_EVENT	26		/* application log event - down2up */

// Messages for initialize endpoints
#define EP_MSG_NEWEP			0xF1	/* !internal multififo message! - down2up */
#define EP_MSG_EPRDY			0xF2	/* !internal multififo message! - down2up */

#define DOBJ_NAMESIZE			sizeof(int) * 4

#define IEC104_CHLENGHT			400
#define APP_NAME_LEN			sizeof(int) * 8

#define TRUE	1
#define FALSE	0

// unitlink list
#define UNITLINK_IEC104			"unitlink-iec104"
#define UNITLINK_M700			"unitlink-m700"
#define UNITLINK_DLT645			"unitlink-dlt645"

// first value for lnInst and ldInst
#define MASTER_START_INST		0
#define SLAVE_START_INST		100
#define ASDU_START_INST			100

// Exit codes
#define NO_CONFIG_FILE	1


/*
 *
 * Structures
 *
 */

typedef struct ep_data_header {
	uint32_t 		adr;		/* link (ASDU) address */
	uint32_t		numep;		/* endpoint number of receiver */
	uint32_t		sys_msg;	/* user data (0)/system message(1..n) */
	uint32_t		len;		/* length of the data following the header */
} __attribute__ ((packed)) ep_data_header;


typedef struct ep_init_header {
	char     appname[APP_NAME_LEN];
	uint32_t addr;				/* link (ASDU) address */
	uint32_t numep;
} __attribute__ ((packed)) ep_init_header;

#define EP_INIT_HEADER_INITIALIZER { {0}, 0, 0 }

typedef struct frame_dobj {
	ep_data_header	edh;
	uint32_t		id;			/* device's internal identifier of data unit */
	char 			name[DOBJ_NAMESIZE];
} __attribute__ ((packed)) frame_dobj;

#define FRAME_DOBJ_INITIALIZER { {0,0,0,0}, 0, {0} }

/* Just List */
typedef struct _LIST{
	void *next;
	void *prev;
} LIST;

typedef struct _varattach{
	time_t			time;
	uint32_t		uid;
	uint32_t		id;
	uint32_t		type;
	uint32_t		lenname;
	uint32_t		intr;
} __attribute__ ((packed)) varattach;

typedef struct _varevent{
	time_t			time;
	uint32_t		uid;
	uint32_t		vallen;
	uint32_t		validx;

	union {
		uint32_t	ui;					/* unsigned integer representation */
		int32_t		i;					/* integer representation */
		float		f;					/* float representation */
		time_t		tm;					/* time in seconds */
	} value;							/* transferring value (e.g. measured value, counter readings, etc.) */

} __attribute__ ((packed)) varevent;

// For args.c
extern void args_parser(int argc, char * argv[]);

#endif /* COMMON_H_ */
