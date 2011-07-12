/*
 * virtualize.c
 *
 *  Created on: 12.07.2011
 *      Author: alex AVAlon
 *
 *  This module make virtualization procedure for all data_types going from/to unitlinks
 *
 */

#include "../../common/common.h"
#include "../../common/multififo.h"

struct config_device_list{
	struct config_device cd;
	struct config_device *prev;
	struct config_device *next;
};

#define SCADA_ASDU_MAXSIZE 512

// Mapping meter to scada asdu
struct data_map{
	uint32_t	meterid;		// use find_by_int, meterid = position in meter asdu, meter.val = value according to meter.value_type
	char		valname[10];	// use find_by_string
	uint32_t	scadaid;		// scadaid  = position in scada asdu
};

// iec61850 - temporary, DATA
typedef struct _DATA{
	char	name[10];

	union {
		uint32_t	ui;			/* unsigned integer representation */
		int32_t		i;			/* integer representation */
		float		f;			/* float representation */
	} value;					/* transferring value (e.g. measured value, counter readings, etc.) */

	uint8_t			value_type; /* e.g. integer, unsigned integer, float, boolean, etc. (need this for "on fly" protocol converting) */
} DATA;

// Set of data for one logical node (LN)
struct data_list{
	DATA		dat;
	DATA		*next;
	DATA		*prev;
};

// Set of data for any LN and IED in really
// There: for SCADA_ASDU
typedef struct _DATASET{
	char 				name[20];
	struct data_list 	*fdat;
} DATASET;

// View dataset as ASDU for virtual IED
typedef struct SCADA_ASDU{
	int ASDUaddr;			// use find_by_int
	u08  ASDUframe[SCADA_ASDU_MAXSIZE];
	char iedname[80];		// use find_by_string
	DATASET ds;
};
// Variables for asdu actions
SCADA_ASDU *firstasdu;
SCADA_ASDU *actasdu;
int maxasdu;

// find struct in struct_list have 1 pars = val
void *find_by_int(int val){

}

int rcvdata(int len){

}

int rcvinit(ep_init_header *ih){

}

int start_virtualize(){
pid_t chldpid;

	// Create virtualization structures from common iec61850 configuration
 	 // NOW! Parse config of data to SCADA_ASDU frame

	// Run multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	return chldpid;
}
