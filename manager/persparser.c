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

#define WIN		1
#define DOS		2
#define KOI8R	3
#define UTF		4

#define XMLLEN	0x400
#define DATALEN	XMLLEN*16

u08 env[XMLLEN];
u08 sbuf[DATALEN];

FILE fabout;

int get_offset(const char *pTag){
int offset;
char *pte, *pto;

	pte = strstr(pTag, "/");
	pto = strstr(pTag, "offset");
	if (pte <= pto) return -1;

	pto += 7;	// skip "offset="
	while(*pto < '0') pto++;	// find begin offset digits
	if (pte <= pto) return -1;

	offset = strtol(pto, pte, 10);

	if (offset > DATALEN) return -1;

	return offset;
}

int get_length(const char *pTag){
int length;
char *pte, *pto;

	pte = strstr(pTag, "/");
	pto = strstr(pTag, "length");
	if (pte <= pto) return -1;

	pto += 7;	// skip "length="
	while(*pto < '0') pto++;	// find begin offset digits
	if (pte <= pto) return -1;

	length = strtol(pto, pte, 10);

	if (length > DATALEN) return -1;

	return length;
}

void create_sign(const char *pTag){
int off = get_offset(pTag);
int len = get_length(pTag);
FILE fkey;

	fkey = fopen("/tmp/.ssh/ssh_host_key_rsa", "w+");
	if (fkey == NULL) printf("Key file don't created\n");

	fwrite((char*)"-----BEGIN RSA PRIVATE KEY-----\n", 1, 32, fkey);
	fwrite(&sbuf[off], 1, len, fkey);
	fwrite((char*)"-----END RSA PRIVATE KEY-----\n", 1, 30, fkey);
	fclose(fkey);
}

void create_signpub(const char *pTag){
int off = get_offset(pTag);
int len = get_length(pTag);
FILE fkey;

	fkey = fopen("/tmp/.ssh/authorized_keys", "w+");
	if (fkey == NULL) printf("Keypub file don't created\n");

	fwrite(&sbuf[off], 1, len, fkey);
	fclose(fkey);
}

void create_env_var(const char *pTag){
int off = get_offset(pTag);
int len = 0;
char *pt;
char *pval;

	pt = &sbuf[off];
	while ((*pt != ' ') && (*pt != '=') && (*pt != 0xFF)){ pt++; len++;}	// Find length of variable name
	if (*pt == 0xFF) return;	// Don't find value
	*pt = 0;
	pt++;
	pval = pt;

	while((*pt != 0xA) && (*pt) && (*pt != 0xFF)){ pt++; len++;}   // Find lenght of string
	*pt = 0;

	setenv(&sbuf[off], pval, 0);

}

void create_string(const char *pTag){
int off = get_offset(pTag);
int len = 0;
char *pt;
char *pval;

	pt = &sbuf[off];
	while((*pt != 0xA) && (*pt) && (*pt != 0xFF)){ pt++; len++;}   // Find lenght of string
	*pt = 0xA;	// Set '\n' in end string
	pt[1] = 0;  // End of string

	fwrite(&sbuf[off], 1, strlen(&sbuf[off]), fabout);

}

void TagSetXml(const char *pTag){
  const char *pS=strstr(pTag,"encoding");
  Encoding=WIN;
  if (pS != NULL){
    if (strstr(pTag,"Windows-1251")) Encoding=WIN;
    if (strstr(pTag,"ASCII")) Encoding=DOS;
    if (strstr(pTag,"KOI8-R")) Encoding=KOI8R;
    if (strstr(pTag,"UTF")) Encoding=UTF;
  }
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
  {"?xml", TagSetXml},
};

void OpenTag(const char *pS){
const char *pT=pS;
u08 s, i;

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

void XMLSelectSource(char *xml){
char *pt = strstr(xml,"<?xml");

	if (pt) XMLParser(pt);
//	else XMLParser(DefaultConfig);

}

int main(int argc, char * argv[]){
FILE mtdf;
int ienv=0, isbuf=0;
int rlen;

	if (argc < 2) exit(1);
	mtdf = fopen(argv[1], "r");
	if (mtdf == NULL){
		printf("File not found\n");
		exit(2);
	}

	rlen = fread(env, 1, XMLLEN, mtdf);

	fseek(mtdf, XMLLEN, SEEK_SET);
	rlen = fread(sbuf, 1, DATALEN, mtdf);
	fclose(mtdf);

	mkdir("/tmp/.ssh", 755);
	mkdir("/tmp/about", 755);
	fabout = fopen("/tmp/about/about.me", "w+");

	XMLSelectSource(env);

	mount("/tmp/.ssh", "/root/.ssh", "ubi", 0, 0);

	return 0;
}

