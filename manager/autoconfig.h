/*
 * autoconfig.h
 *
 *  Created on: 14.09.2011
 *      Author: alex
 */

#ifndef AUTOCONFIG_H_
#define AUTOCONFIG_H_

#define MAXLEVEL 3

// Scens
#define IEC104		1
#define IEC101		2
#define MX00		3
#define DLT645		4

typedef struct lowrecord{
	char 		*scfg;
	// Vars for ICD
	uint32_t	ldinst;
	// Flags
	u08			connect;
	u08 		copied;
	// Vars for lowlevel.cfg
	uint16_t	asdu;
	struct sockaddr_in 	sai;
	long		addrdlt;
	uint32_t	myep;		// Endpoint number
	uint16_t	scen;		// Case for type lowlevel string
	uint16_t	setspeed;
} LOWREC;

extern void XMLSelectSource(char *xml);

extern int createlowrecord(LOWREC *lr);

#endif /* AUTOCONFIG_H_ */
