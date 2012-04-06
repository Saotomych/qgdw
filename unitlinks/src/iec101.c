/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include "../include/iec101.h"
#include "../../common/paths.h"

int main(int argc, char *argv[])
{
	init_allpaths();
	mf_semadelete(getpath2fifomain(), APP_NAME);

	exit(0);
}












