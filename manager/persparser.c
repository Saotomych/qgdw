/*
 * persparser.c
 *
 *  Created on: 19.12.2011
 *      Author: Alex AVAlon
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include "../common/common.h"

#define WIN		1
#define DOS		2

#define XMLLEN	0x1000

u08 env[XMLLEN];

FILE *fabout;
FILE *fsetvars;

int EndScript;
int quiet, verbose;

// Statistics
int signs=0;
int vars=0;
int infos=0;

char* get_name(const char *pTag){
char *pte, *pto, *ptn;

	pto = strstr(pTag, "Name");
	if (pto == NULL) 	pto = strstr(pTag, "name");

	pto += 5;	// skip "name="
	while(*pto != '"') pto++;	// find begin offset digits
	pto++;
	ptn = pto;
	while(*pto != '"') pto++;	// find begin offset digits
	if (!pto) return -1;
	*pto = 0;	// End string of name

	if (verbose) printf("- name = %s\n",ptn);

	return ptn;
}

char *get_value(const char *pTag){
char *ptn = 0, *pte = 0;
	ptn = strstr(pTag, ">");
	if (ptn == NULL) return -1;
	ptn++;
	pte = strstr(pTag, "</");
	if (pte == NULL) return -1;
	if (pte <= ptn) return -1;
	*pte = 0;

	if (verbose) printf("- value = %s\n",ptn);

	return ptn;
}

// Functions for sign

void create_sign(const char *pTag){
uint len;
FILE *fkey;

	len = strlen(pTag);
	if (len == -1) return;

	fkey = fopen("/tmp/.ssh/ssh_host_key_rsa", "w+");
	if ((!quiet) &&	(fkey == NULL)){
		printf("error: Key file don't created\n");
		return;
	}

	fwrite((char*)"-----BEGIN RSA PRIVATE KEY-----\n", 1, 32, fkey);
	fwrite(pTag, 1, len, fkey);
	fwrite((char*)"\n-----END RSA PRIVATE KEY-----\n", 1, 31, fkey);
	fclose(fkey);
}

void create_signpub(const char *pTag){
uint len;
FILE *fkey;

	len = strlen(pTag);
	if (len == -1) return;

	fkey = fopen("/tmp/.ssh/authorized_keys", "w+");
	if ((!quiet) && (fkey == NULL)){
		printf("error: SSHpub file don't created\n");
		return;
	}

	fwrite(pTag, 1, len, fkey);
	fclose(fkey);
}

void create_signssl(const char *pTag){
uint len;
FILE *fkey;

	len = strlen(pTag);
	if (len == -1) return;

	fkey = fopen("/tmp/.ssl/authorized_keys", "w+");
	if ((!quiet) && (fkey == NULL)){
		printf("error: SSLpub file don't created\n");
		return;
	}

	fwrite(pTag, 1, len, fkey);
	fclose(fkey);
}

// Functions of parser

void create_key(const char *pTag){
char *vl = get_value(pTag);
char *name = get_name(pTag);

	if (!strcmp(name,"SIGNSSL")){
		if (!quiet) printf("settings: create pub ssl sign\n");
		create_signssl(vl);
		signs++;
		return;
	}
	if (!strcmp(name,"SIGNPUB")){
		if (!quiet) printf("settings: create pub ssh sign\n");
		create_signpub(vl);
		signs++;
		return;
	}
	if (!strcmp(name,"SIGN")){
		if (!quiet) printf("settings: create private ssh sign\n");
		create_sign(vl);
		signs++;
	}
}

void create_env_var(const char *pTag){
char *vl = get_value(pTag);
char *name = get_name(pTag);
uint len;

	if (vl == -1) return;
	if (name == -1) return;
	len = strlen(vl);

	*(vl+len) = 0;
	if (verbose) printf("settings: set environment var %s as %s\n", name, vl);

	fwrite("export ", 1, 7, fsetvars);
	fwrite(name, 1, strlen(name), fsetvars);
	fwrite("=", 1, 1, fsetvars);
	fwrite(vl, 1, len, fsetvars);
	fwrite("\n", 1, 1, fsetvars);

	vars++;

}

void create_string(const char *pTag){
char *vl = get_value(pTag);
char *name = get_name(pTag);
uint len;

	if (vl == -1) return;
	if (name == -1) return;
	len = strlen(vl);

	*(vl+len) = 0;
	if (verbose) printf("settings: set info '%s': %s\n", name, vl);

	fwrite(name, 1, strlen(name), fabout);
	fwrite(": ", 1, 1, fabout);
	fwrite(vl, 1, len, fabout);
	fwrite("\n", 1, 1, fabout);

	infos++;

}

void bgn_func(const char *pTag){
	EndScript=0;
}

void end_func(const char *pTag){
	EndScript=1;
}

typedef struct _XML_Name{
	char *Name;
	void(*Function)(const char *pTag);
} XML_Name, *pXML_Name;

static const XML_Name XTags[] = {
  {"KEY", create_key},
  {"ENVAR", create_env_var},
  {"TEXT", create_string},
  {"EIS", bgn_func},
  {"/EIS", end_func},
};

void OpenTag(const char *pS){
const char *pT=pS;
unsigned char s, i;

  while(*pS < 'A') pS++;
  pS--;
  if ((*pS != '?') && (*pS != '/')) pS++;

  for(i = 0; i < sizeof(XTags)/sizeof(XML_Name); i++){
    s=0; pS=pT;
    while(*pS == XTags[i].Name[s]){
        pS++; s++;

    }
    if ((XTags[i].Name[s] == 0) && (*pS <= 'A')){
        if (verbose) printf("settings: new tag %s\n", XTags[i].Name);
    	XTags[i].Function(pS);
    	break;
    }
  }
}

void XMLParser(const char *XMLScript){
const char *pS=XMLScript;

  EndScript = 0;
  while (!EndScript){
    if (*(pS++) == '<'){
      OpenTag(pS);
    }
  }

}

void XMLSelectSource(u08 *xml){
char *pt = strstr((char*)xml,"<EIS");

	if (pt) XMLParser(pt);
//	else XMLParser(DefaultConfig);

}

int main(int argc, char * argv[]){
FILE *mtdf;
int rlen, i;

	if (argc < 2) exit(1);

	for(rlen = 1; rlen < (argc-1); rlen++){
		if (argv[1][0] == '-'){
			// Keys exist
			i=1;
			while(argv[rlen][i]){
				switch(argv[rlen][i]){
				case 'v':
					// Verbose mode on
					verbose = 1;
					break;
				case 'q':
					// Quiet mode on
					quiet = 1;
					break;
				}
				i++;
			}
		}
	}

	if (verbose) quiet = 0; // verbose has priority

	if (!quiet) printf("settings: start personal settings...\n");

	mtdf = fopen(argv[argc-1], "r");
	if (mtdf == NULL){
		printf("File not found\n");
		exit(2);
	}

	rlen = fread(env, 1, XMLLEN, mtdf);
	env[rlen-1] = 0;

	fclose(mtdf);

	mkdir("/tmp/.ssh", S_IRWXU);
	mkdir("/tmp/.ssl", S_IRWXU);
	mkdir("/tmp/about", S_IRWXU);
	fabout = fopen("/tmp/about/about.me", "w+");
	fsetvars = fopen("/tmp/about/setenv.sh", "w+");

	XMLSelectSource(env);

	fclose(fsetvars);
	fclose(fabout);
	mount("/tmp/.ssh", "/root/.ssh", "ubi", 0, 0);
	mount("/tmp/.ssl", "/root/.ssl", "ubi", 0, 0);

	if (!quiet){
		printf("Personal settings ready:\n");
		printf("- security signs: %d\n", signs);
		printf("- environment variables: %d\n", vars);
		printf("- informations strings: %d\n", infos);
	}

	return 0;
}

