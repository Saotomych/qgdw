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
#include <iconv.h>
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

//#include <iostream>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

/*
 *
 * Structures
 *
 */

typedef struct ep_data_header {
	uint16_t 		adr;		/* link (ASDU) address */
	uint32_t		sys_msg;	/* user data (0)/system message(1..n) */
	uint32_t		len;		/* user data length */
} ep_data_header;

typedef struct ep_init_header {
	char *isstr[5];
	u32 addr;
	int numch;
} ep_init_header;

typedef struct config_device{
	char   	*name;
	char 	*protoname;			// ptr to protokol name
	char 	*phyname;			// physlink number
	u32		addr;
} config_device;

#endif /* COMMON_H_ */
