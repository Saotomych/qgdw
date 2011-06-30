/*
 * common.h
 *
 *  Created on: 07.06.2011
 *      Author: alex
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/*
 *
 * Constants and byte flags/masks
 *
 */
#define EP_USER_DATA			0		/* user data */
#define EP_MSG_INIT_CFG			1		/* initialization configuration */
#define EP_MSG_INIT_ACK			2		/* initialization acknowledgment */
#define EP_MSG_INIT_NACK		3		/* initialization negative acknowledgment */
#define EP_MSG_RECONNECT		4		/* re-connection request */
#define EP_MSG_CONNECT_ACK		5		/* connection acknowledgment */
#define EP_MSG_CONNECT_NACK		6		/* connection negative acknowledgment */
#define EP_MSG_CONNECT_CLOSE	7		/* connection close request */
#define EP_MSG_CONNECT_LOST		8		/* connection lost/closed by remote peer */



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



#endif /* COMMON_H_ */
