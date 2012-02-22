/*
 * tarif.c
 *
 *  Created on: 21.02.2012
 *      Author: Alex AVAlon
 */

#include "common.h"
#include "xml.h"
#include "tarif.h"

int tarif_parser(char *filename){
char *tfile;
FILE *fcid;
int cidlen, ret = 0;
struct stat fst;

	// Get size of main config file
	if (stat(filename, &fst) == -1){
		printf("Tarif: Config file not found\n");
	}

	tfile = malloc(fst.st_size);

	// Loading main config file
	fcid = fopen(filename, "r");
	cidlen = fread(tfile, 1, (size_t) (fst.st_size), fcid);
	if(!strstr(tfile, "</Tarifs>"))
	{
		printf("Tarif: config file is incomplete\n");
		exit(1);
	}
	if (cidlen == fst.st_size) XMLSelectSource(tfile);
	else ret = -1;

	return ret;
}

