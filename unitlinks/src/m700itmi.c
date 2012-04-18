/*
 * m700_itmi.c
 *
 *  Created on: 18.04.2012
 *      Author: Alex AVAlon
 */

//#include "../include/itmi.h"
#include "../../common/common.h"
#include "../../common/multififo.h"
#include "../../common/ts_print.h"
#include "../../common/paths.h"

struct signal{
	uint32_t		id;			/* device's internal identifier of data unit */
	char 			name[DOBJ_NAMESIZE];
} *signals;

int main(int argc, char *argv[])
{
	init_allpaths();
	mf_semadelete(getpath2fifomain(), "m700itmi");

	// read mainmap config
		// count Loc values
		// memory allocation for values
		// fill values

	// multififo init


	// open event0

	// wait event

	// Telemetering event?

	// Yes -> Send to startiec as map address

	return 0;
}
