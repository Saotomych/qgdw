/*
 * autoconfig.h
 *
 *  Created on: 14.09.2011
 *      Author: alex
 */

#ifndef AUTOCONFIG_H_
#define AUTOCONFIG_H_

#include "../common/common.h"
#include "../common/multififo.h"

#define MAXLEVEL 		3

#define APP_NAME		"manager"
#define ADDR_FILE		"addr.cfg"
#define LLEVEL_FILE		"lowlevel.cfg"
#define LLEVEL_FILEBAK	"lowlevel.bak"
#define ICD_FILE		"ieclevel.icd"
#define ICD_FILEBAK		"ieclevel.bak"
#define MAC_FILE		"setenv.sh"

// Timer constants
#define EP_INIT_TIME	2		/* delay for EP initialization */
#define ALARM_PER		1		/* timers check period */
#define RECV_TIMEOUT	3		/* default waiting timeout for full response from device */


// Scens IDs
#define IEC104			1
#define IEC101			2
#define MX00			3
#define DLT645			4

#define HOST_MASTER		1
#define HOST_SLAVE		2

typedef struct lowrecord{
	char 		*scfg;
	// Vars for ICD
	uint32_t	lninst;

	// Flags
	uint8_t		connect;

	// Vars for lowlevel.cfg
	uint16_t	asdu;
	uint8_t		host_type;
	struct sockaddr_in 	sai;
	uint64_t	link_adr;
	uint32_t	myep;		// Endpoint number
	uint16_t	scen;		// Case for type lowlevel string
	uint16_t	setspeed;
} LOWREC;

extern int get_mac(char *file_name);

extern void XMLSelectSource(char *xml);

extern int createlowrecord(LOWREC *lr);


#endif /* AUTOCONFIG_H_ */
