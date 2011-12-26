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

#define XMLLEN	0x400
#define DATALEN	XMLLEN*16

u08 env[XMLLEN];
u08 sbuf[DATALEN];

FILE *fabout;
FILE *fsetvars;

int EndScript;

int get_offset(const char *pTag){
int offset = 0;
char *pte, *pto;

	pte = strstr(pTag, "/");
	pto = strstr(pTag, "offset");
	if (pte <= pto) return -1;

	pto += 7;	// skip "offset="
	while(*pto != '"') pto++;	// find begin offset digits
	pto++;
	if (pte <= pto) return -1;
	if (!pto) return -1;

	offset = atoi(pto);

	if (offset > DATALEN) return -1;

	printf("offset = %d\n", offset);

	return offset;
}

int get_length(const char *pTag){
int length;
char *pte, *pto;

	pte = strstr(pTag, "/");
	pto = strstr(pTag, "length");
	if (pte <= pto) return -1;
	if (!pto) return -1;

	pto += 7;	// skip "length="
	while(*pto != '"') pto++;	// find begin offset digits
	pto++;
	if (pte <= pto) return -1;

	length = atoi(pto);

	if (length > DATALEN) return -1;

	printf("length = %d\n", length);

	return length;
}

void create_sign(const char *pTag){
uint off = get_offset(pTag);
uint len = get_length(pTag);
FILE *fkey;

	fkey = fopen("/tmp/.ssh/ssh_host_key_rsa", "w+");
	if (fkey == NULL) printf("Key file don't created\n");

	fwrite((char*)"-----BEGIN RSA PRIVATE KEY-----\n", 1, 32, fkey);
	fwrite(&sbuf[off], 1, len, fkey);
	fwrite((char*)"\n-----END RSA PRIVATE KEY-----\n", 1, 31, fkey);
	fclose(fkey);
}

void create_signpub(const char *pTag){
uint off = get_offset(pTag);
uint len = get_length(pTag);
FILE *fkey;

	fkey = fopen("/tmp/.ssh/authorized_keys", "w+");
	if (fkey == NULL) printf("Keypub file don't created\n");

	fwrite(&sbuf[off], 1, len, fkey);
	fclose(fkey);
}

void create_env_var(const char *pTag){
uint off = get_offset(pTag);
int len = 0;
u08 *pt;

	pt = &sbuf[off];
	while((*pt != 0xA) && (*pt) && (*pt != 0xFF)){ pt++; len++;}   // Find lenght of string
	*pt = 0;
	printf("set environment var %s\n", (char*) &sbuf[off]);

//	len=putenv((char*) &sbuf[off]);
	fwrite("export ", 1, 7, fsetvars);
	fwrite(&sbuf[off], 1, strlen((char*) &sbuf[off]), fsetvars);
	fwrite("\n", 1, 1, fsetvars);
}

void create_string(const char *pTag){
uint off = get_offset(pTag);
uint len = 0;
uchar *pt;

	pt = &sbuf[off];
	while((*pt != 0xA) && (*pt) && (*pt != 0xFF)){
		pt++; len++;
	}   // Find lenght of string
	*pt = 0xA;	// Set '\n' in end string
	pt[1] = 0;  // End of string

	fwrite(&sbuf[off], 1, strlen((char*) &sbuf[off]), fabout);

}

void TagSetXml(const char *pTag){
//  const char *pS=strstr(pTag,"encoding");
//  Encoding=WIN;
//  if (pS != NULL){
//    if (strstr(pTag,"Windows-1251")) Encoding=WIN;
//    if (strstr(pTag,"ASCII")) Encoding=DOS;
//    if (strstr(pTag,"KOI8-R")) Encoding=KOI8R;
//    if (strstr(pTag,"UTF")) Encoding=UTF;
//  }
}

void no_func(const char *pTag){

}

void end_func(const char *pTag){
	EndScript=1;
	printf("Personalize ready\n");
}

typedef struct _XML_Name{
	char *Name;
	void(*Function)(const char *pTag);
} XML_Name, *pXML_Name;

static const XML_Name XTags[] = {
  {"SIGN", create_sign},
  {"SIGNPUB", create_signpub},
  {"ENVAR", create_env_var},
  {"TEXT", create_string},
  {"M700", no_func},
  {"/M700", end_func},
  {"?xml", TagSetXml},
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
char *pt = strstr((char*)xml,"<?xml");

	if (pt) XMLParser(pt);
//	else XMLParser(DefaultConfig);

}

int main(int argc, char * argv[]){
FILE *mtdf;
int rlen;

	if (argc < 2) exit(1);
	mtdf = fopen(argv[1], "r");
	if (mtdf == NULL){
		printf("File not found\n");
		exit(2);
	}

	rlen = fread(env, 1, XMLLEN, mtdf);
	env[rlen-1] = 0;

	fseek(mtdf, XMLLEN, SEEK_SET);
	rlen = fread(sbuf, 1, DATALEN, mtdf);
	sbuf[rlen-1] = 0;
	fclose(mtdf);

	mkdir("/tmp/.ssh", 766);
	mkdir("/tmp/about", 766);
	fabout = fopen("/tmp/about/about.me", "w+");
	fsetvars = fopen("/tmp/about/setenv.sh", "w+");

	XMLSelectSource(env);

	fclose(fsetvars);
	fclose(fabout);
	mount("/tmp/.ssh", "/root/.ssh", "ubi", 0, 0);

	return 0;
}

