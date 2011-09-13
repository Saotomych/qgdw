/*
 * addrxml.c
 *
 *  Created on: 12.09.2011
 *      Author: alex
 */

#include "../common/common.h"
#include "../common/multififo.h"

uint configstep;

u16 usasdu[MAXEP];
u08 maxfixasdu;

static u08 Encoding, EndScript;
// Scenario functions

// In data: dlt-address
// Out data: string to file lowlevel.3.<speed> , where speed=9600,2400,1200
// asdu -addr <dlt-address> -name "unitlink-dlt645" -port "ttyS3" -pars <speed>,8E1,NO
// asdu - dynamic
void TagM100300(const char *pTag){
	if (configstep == 3){

	}
}

// In data: MAC-address
// Out data: 3 strings to files lowlevel.2.<speed>, where speed=9600,2400,1200
// asdu -addr 01 -port "ttyS4" -name "unitlink-m700" -pars <speed>,8E1,NO
// asdu - dynamic
void TagM500700(const char *pTag){
	if (configstep == 2){

	}
}

// In data: ASDU IP-address port
// Out data: string to file lowlevel.1
// asdu -addr <IP-address> -name "unitlink-iec104" -port 2204 -mode CONNECT
// or
// In data: ASDU only
// Out data: string to file lowlevel.1.<speed>, where speed=9600,2400,1200
// asdu -name "unitlink-iec101" -port "ttyS3" -pars <speed>,8E1,NO
// every ASDU set to usasdu[maxfixasdu]
void TagKIPP(const char *pTag){
	if (configstep == 1){

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
