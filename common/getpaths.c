/*
 * getpaths.c
 *
 *  Created on: 06.03.2012
 *      Author: Alex AVAlon
 */

#include "common.h"

static char defprepath[] = {"/rw/mx00/"};
static char defpathbin[] = {"bin"};
static char defpathconfig[] = {"configs"};
static char defpathlibs[] = {"lib"};
static char defpathmenu[] = {"menus"};
static char defpathfonts[] = {"pcf"};
static char defpathul[] = {"unitlinks"};
static char defpathphy[] = {"phyints"};
static char defpathmain[] = {"mainapp"};
static char defpathabout[] = {"about"};

static char *basepath;		// Path to all project from environment MXPATH or default
static char *pathbin;		// Path to binary files from environment MXBINPATH or default
static char *pathconfig;	// Path to config files from environment MXCFGPATH or default
static char *pathlibs;		// Path to project library files from environment MXLIBPATH or default
static char *pathmenus;	// Path to project visible menu files from environment MXMENUPATH or default
static char *pathfonts;	// Path to project visible fonts files from environment MXFNTPATH or default
static char *pathul;		// Path to unitlink's named fifo buffers (MXFIFOUL)
static char *pathphy;		// Path to physical link's named fifo buffers (MXFIFOPHY)
static char *pathmain;		// Path to main iec soft's named fifo buffers (MXFIFOMAIN)
static char *pathabout;		// Path to about files (MXABOUTPATH)

static int lenbasepath;

static char *makepath(char *envname, char *defptr){
char *ptr, *eptr;
char lenname = lenbasepath;

	eptr = getenv(envname);
	if (eptr == NULL){
		// Set default path
		lenname += strlen(defptr) + 2;
		ptr = malloc(lenname);
		strcpy(ptr, basepath);
		strcat(ptr, defptr);
		strcat(ptr, "/");
	}else{
		if ((*eptr == '.') || (*eptr == '/')){
			lenname = strlen(eptr) + 2;
			ptr = malloc(lenname);
			strcpy(ptr, eptr);
			strcat(ptr, "/");
		}else{
			lenname += strlen(eptr) + 2;
			ptr = malloc(lenname);
			strcpy(ptr, basepath);
			strcat(ptr, eptr);
			strcat(ptr, "/");
		}
	}

	return ptr;
}

// Making all buffers for pathnames and all pathnames
// Rules:
// 1. Path to project must have go from root, first symbol = '/'
// 2. If any other path begins as '.' it's assign as relative path and begins as '/' it's assign as absolute path from root
// 3. If any other path begins as not '.' and not '/' he will be beginning from prepath
void init_allpaths(){

	// Make path to project
	basepath = getenv("MXPATH");
	if (basepath == NULL) basepath = defprepath;
	lenbasepath = strlen(basepath);

	// Make all path
	pathbin = makepath("MXBINPATH", defpathbin);
	pathconfig = makepath("MXCFGPATH", defpathconfig);
	pathlibs = makepath("MXLIBPATH", defpathlibs);
	pathmenus = makepath("MXMENUPATH", defpathmenu);
	pathfonts = makepath("MXFNTPATH", defpathfonts);
	pathul = makepath("MXFIFOUL", defpathul);
	pathphy = makepath("MXFIFOPHY", defpathphy);
	pathmain = makepath("MXFIFOMAIN", defpathmain);
	pathabout = makepath("MXABOUTPATH", defpathabout);
}

char *getpath2base(){
	return basepath;
}

char *getpath2bin(){
	return pathbin;
}

char *getpath2configs(){
	return pathconfig;
}

char *getpath2libs(){
	return pathlibs;
}

char *getpath2menu(){
	return pathmenus;
}

char *getpath2fonts(){
	return pathfonts;
}

char *getpath2fifoul(){
	return pathul;
}

char *getpath2fifophy(){
	return pathphy;
}

char *getpath2fifomain(){
	return pathmain;
}

char *getpath2about(){
	return pathabout;
}
