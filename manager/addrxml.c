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

char Speed[6]={"9600\0"};

static u08 Encoding, EndScript;

void lowrecordinit (LOWREC *lr){
	lr->addr = 0;
	lr->addrdlt = 0;
	lr->asdu = 0;
	lr->connect = 0;
	lr->copied = 0;
	lr->ldinst = 0;
	lr->myep = 0;
	lr->port = 0;
	lr->scen = 0;
}

// Scenario functions

// In data: dlt-address
// Out data: string to file lowlevel.3.<speed> , where speed=9600,2400,1200
// asdu -addr <dlt-address> -name "unitlink-dlt645" -port "ttyS3" -pars <speed>,8E1,NO
// asdu - dynamic, dont equal with every ASDU from usasdu
void TagM100300(const char *pTag){
	if (configstep == 3){

	}
}

// In data: MAC-address
// Out data: 3 strings to files lowlevel.2.<speed>, where speed=9600,2400,1200
// asdu -addr 01 -port "ttyS4" -name "unitlink-m700" -pars <speed>,8E1,NO
// asdu - dynamic, dont equal with every ASDU from usasdu
void TagM500700(const char *pTag){
char *p;
	if (configstep == 2){
		p = strstr((char*) pTag, "-asdu");
		if (p){
		}
		else return;
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
	if (configstep == 1){
		lowrecordinit(&lr);
		p = strstr((char*) pTag, "asdu");
		if (p){
			lr.asdu = atoi(p+5);
		}
		else return;
		p = strstr((char*) pTag, "ip");
		if (p){
			p = strstr((char*) pTag, "port");
			if (p){
				lr.port = atoi(p+5);
			}else{
				lr.port = 2404;
			}
			lr.scen = IEC104;
		}else lr.scen = IEC101;
	}
}

// ssd functions

void TagSetSCL(const char *pTag){
	printf("IEC61850: Start SCL file to parse\n");
}

void TagEndSCL(const char *pTag){
	EndScript=1;
	printf("IEC61850: Stop SCL file to parse\n");
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

	if (pt) XMLParser(pt);

}
