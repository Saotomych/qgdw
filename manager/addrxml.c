/*
 * addrxml.c
 *
 *  Created on: 12.09.2011
 *      Author: alex
 *
 * Module parse addr.cfg and create lowrecords for each record of addr.cfg
 *
 */

#include "../common/common.h"
#include "../common/multififo.h"
#include "autoconfig.h"

uint configstep;

u16 usasdu[MAXEP];
u08 maxfixasdu;

static u08 Encoding, EndScript;

int ishex(char *p){
	if ((*p >= '0') && (*p <='9')) return TRUE;
	if ((*p >= 'A') && (*p <='F')) return TRUE;
	if ((*p >= 'a') && (*p <='f')) return TRUE;
	return FALSE;
}

int ismac(char *p){
int i;
	for(i=0; i<5; i++){
		if ((ishex(&p[0])) && (ishex(&p[1])) && (p[2] == ':')) p+=3;
		else return FALSE;
	}
	if ((ishex(&p[0])) && (ishex(&p[1]))) return TRUE;
	return FALSE;
}

void lowrecordinit (LOWREC *lr){
	lr->sai.sin_addr.s_addr = 0;
	lr->sai.sin_port = 0;
	lr->addrdlt = 0;
	lr->asdu = 0;
	lr->connect = 0;
	lr->copied = 0;
	lr->ldinst = 0;
	lr->myep = 0;
	lr->scen = 0;
	lr->setspeed = 0;
	lr->scfg = 0;
}

// Scenario functions

// In data: dlt-address
// Out data: string to file lowlevel.3.<speed> , where speed=9600,2400,1200
// asdu -addr <dlt-address> -name "unitlink-dlt645" -port "ttyS3" -pars <speed>,8E1,NO
// asdu - dynamic, dont equal with every ASDU from usasdu
void TagM100300(const char *pTag){
char *p;
LOWREC lr;

	if (configstep == 3){
		lowrecordinit(&lr);
		p = strstr((char*) pTag, "dlt=");
		if (p){
			lr.addrdlt = atol(p+5);
		}
		lr.scen = DLT645;
		createlowrecord(&lr);
	}
}

// In data: MAC-address
// Out data: 3 strings to files lowlevel.2.<speed>, where speed=9600,2400,1200
// asdu -addr 01 -port "ttyS4" -name "unitlink-m700" -pars <speed>,8E1,NO
// asdu - dynamic, dont equal with every ASDU from usasdu
char *macbuf;
void TagM500700(const char *pTag){
LOWREC lr;
char *p;

	if (configstep == 2){
		// Filter by MAC-address. This meter is one only.
		p = strstr((char*) pTag, "mac=");
		if (p){
			if (!strcmp(p, macbuf)){
				lowrecordinit(&lr);
				lr.scen = MX00;
				createlowrecord(&lr);
			}
		}
	}
}

// In data: ASDU IP-address port (default port = 2404)
// Out data: string to file lowlevel.1
// asdu -addr <IP-address> -name "unitlink-iec104" -port 2204 -mode CONNECT
// or
// In data: ASDU only
// Out data: string to file lowlevel.1.<speed>, where speed=9600,2400,1200
// asdu -name "unitlink-iec101" -port "ttyS3" -pars <speed>,8E1,NO
// every fixed ASDU add to usasdu[maxfixasdu]
void TagKIPP(const char *pTag){
char *p;
LOWREC lr;
struct in_addr adr;
char sadr[16], *ps = sadr;

	if (configstep == 1){
		lowrecordinit(&lr);
		p = strstr((char*) pTag, "asdu=");
		if (p){
			lr.asdu = atoi(p+6);
		}
		else return;
		p = strstr((char*) pTag, "ip=");
		if (p){
			// copy ip-address
			p = p + 4;
			while(*p != '"'){*ps=*p; ps++; p++;}
			*ps = 0;
			// set ip-address
			inet_aton(sadr, &adr);
			lr.sai.sin_addr.s_addr = adr.s_addr;
			// set port
			p = strstr((char*) pTag, "port=");
			if (p){
				lr.sai.sin_port = atoi(p+6);
			}else{
				lr.sai.sin_port = 2404;
			}
			lr.scen = IEC104;
		}else lr.scen = IEC101;
		createlowrecord(&lr);
	}
}

// ssd functions

void TagSetSCL(const char *pTag){
	printf("Config Manager: Start ADDR file to parse\n");
}

void TagEndSCL(const char *pTag){
	EndScript=1;
	printf("Config Manager: Stop ADDR file to parse\n");
}

void TagSetXml(const char *pTag){
  const char *pS=strstr(pTag,"encoding");
  Encoding=WIN;
  if (pS != NULL){
    if (strstr(pTag,"Windows-1251")) Encoding=WIN;
    if (strstr(pTag,"windows-1251")) Encoding=WIN;
    if (strstr(pTag,"ASCII")) Encoding=DOS;
    if (strstr(pTag,"KOI8-R")) Encoding=KOI8R;
    if (strstr(pTag,"UTF")) Encoding=UTF;
  }
}

//*** End Tag RAW working ***//

typedef struct _XML_Name{
	char *Name;
	void(*Function)(const char *pTag);
} XML_Name, *pXML_Name;

static const XML_Name XTags[] = {
  {"M100", TagM100300},
  {"M300", TagM100300},
  {"M500", TagM500700},
  {"M700", TagM500700},
  {"KIPP", TagKIPP},
  {"Hardware", TagSetSCL},
  {"/Hardware", TagEndSCL},
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
FILE *f;
struct stat fst;
uint32_t adrlen, i;

	// Read MAC-address
	f = fopen("/etc/setmacaddr", "r");
	if (f){
		// Get size of main config file
		if (stat("/etc/setmacaddr", &fst) == -1){
			printf("IEC61850: Addr.cfg file not found\n");
		}

		macbuf = malloc(fst.st_size);
		adrlen = fread(macbuf, 1, (size_t) (fst.st_size), f);
		if (adrlen == fst.st_size){
			// Find, test & copy MAC to macbuf
			i = 0;
			do{
				while ((ishex(&macbuf[i]) == FALSE) && (i < adrlen)) i++;
				if (ismac(&macbuf[i])){
					memcpy(macbuf, &macbuf[i], 16);
					macbuf[17] = 0;
					break;
				}
				i++;
			}while(i < adrlen);
			if (i >= adrlen) macbuf[0] = 0;
		}
	}

	configstep = 1;
	if (pt) XMLParser(pt);

	configstep = 2;
	if (pt) XMLParser(pt);

	configstep = 3;
	if (pt) XMLParser(pt);

	free(macbuf);

}
