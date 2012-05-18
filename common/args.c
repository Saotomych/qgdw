/*
 * args.c
 *
 *  Created on: 17.05.2012
 *      Author: Alex AVAlon
 */
#include "common.h"
#include "ts_print.h"

// Get input args and call function for arg and parameters
void args_parser(int argc, char * argv[]){
int i;

	for (i = 0; i < argc; i++){
		if (!strcmp(argv[i], "--verbose")){
			ts_setmode(TS_VERBOSE);
		}

		if (!strcmp(argv[i], "--debug")){
			ts_setmode(TS_DEBUG);
		}

	}
}

