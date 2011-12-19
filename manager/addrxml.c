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


u16 usasdu[MAXEP];
u08 maxfixasdu;

char mac_adr[18] = {0};

static u08 Encoding, EndScript;

uint16_t actasdu = ASDU_START_INST;
uint16_t actmaster_inst = MASTER_START_INST;
uint16_t actslave_inst = SLAVE_START_INST;

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

int get_mac(char *file_name)
{
	int ret = -1;
	char *macbuf, *p;
	FILE *f;
	struct stat fst;
	uint32_t adrlen;

	f = fopen(file_name, "r");
	if (f){
		// get size of setmacaddr file
		stat(file_name, &fst);

		macbuf = malloc(fst.st_size + 1);
		adrlen = fread(macbuf, 1, (size_t) (fst.st_size), f);
		macbuf[fst.st_size] = 0;
	}
	else
	{
		printf("Config Manager: \"%s\" file cannot be opened\n", file_name);
		return ret;
	}

	fclose(f);

	p = macbuf;

	// Find, test & copy MAC to macbuf
	while(*p != ':' && *p != '\0') p++;

	if(p != '\0')
	{
		p -= 2;

		if(ismac(p))
		{
			memcpy(mac_adr, p, 17);
			ret = 0;
		}
	}

	free(macbuf);

	return ret;
}

void lowrecordinit (LOWREC *lr){
	lr->sai.sin_addr.s_addr = 0;
	lr->sai.sin_port = 0;
	lr->link_adr = 0;
	lr->asdu = 0;
	lr->host_type = HOST_SLAVE;
	lr->connect = 0;
	lr->lninst = 0;
	lr->myep = 0;
	lr->scen = 0;
	lr->setspeed = 0;
	lr->scfg = 0;
}

// Scenario functions

// In data: dlt-address
// Out data: string to file lowlevel.3.<speed> , where speed=9600,2400,1200
// asdu -addr <dlt-address> -name "unitlink-dlt645.phy_tty.ttyS3" -port <speed>,8E1,NO
// asdu - dynamic, don't equal with any ASDU from usasdu
void TagM100300(const char *pTag){
char *p;
LOWREC lr;

		lowrecordinit(&lr);
		p = strstr((char*) pTag, "dlt=");
		if (p){
			//lr.addrdlt = atol(p+5);
			sscanf(p+5, "%llu", &lr.link_adr);
		}
		lr.scen = DLT645;
		lr.host_type = HOST_MASTER;
		lr.lninst = actmaster_inst;
		actmaster_inst++;
		lr.asdu = actasdu;
		actasdu++;

		createlowrecord(&lr);
}

// In data: MAC-address
// Out data: 3 strings to files lowlevel.2.<speed>, where speed=9600,2400,1200
// asdu -addr 01 -name "unitlink-m700.phy_tty.ttyS4" -port <speed>,8E1,NO
// asdu - dynamic, don't equal with any ASDU from usasdu
void TagM500700(const char *pTag){
LOWREC lr;
char *p;

		// Filter by MAC-address. This meter is only one.
		p = strstr((char*) pTag, "mac=");
		if (p){
			if (strstr(p, mac_adr)){
				lowrecordinit(&lr);
				lr.link_adr = 1;
				lr.scen = MX00;
				lr.host_type = HOST_MASTER;
				lr.lninst = actmaster_inst;
				actmaster_inst++;
				lr.asdu = actasdu;
				actasdu++;

				createlowrecord(&lr);
			}
			else
			{
				printf("Config Manager: MAC-addresses does not match\n");
			}
		}
}

// In data: ASDU IP-address port (default port = 2404)
// Out data: string to file lowlevel.1
// asdu -addr <IP-address> -name "unitlink-iec104.phy_tty.<port>" -port CONNECT
// or
// In data: ASDU only
// Out data: string to file lowlevel.1.<speed>, where speed=9600,2400,1200
// asdu -addr <asdu> -name "unitlink-iec101.phy_tty.ttyS3" -port <speed>,8E1,NO
// every fixed ASDU add to usasdu[maxfixasdu]
void TagKIPP(const char *pTag){
char *p;
LOWREC lr;
struct in_addr adr;
char sadr[16], *ps = sadr;

		lowrecordinit(&lr);
		p = strstr((char*) pTag, "asdu=");
		if (p){
			lr.asdu = atoi(p+6);
			lr.link_adr = lr.asdu;
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
		lr.host_type = HOST_MASTER;
		lr.lninst = actmaster_inst;
		actmaster_inst++;

		createlowrecord(&lr);
}

// In data: ASDU IP-address port (default port = 2404)
// Out data: string to file lowlevel.1
// asdu -addr <IP-address> -name "unitlink-iec104.phy_tty.<port>" -port LISTEN
void TagSCADA(const char *pTag){
char *p;
LOWREC lr;
struct in_addr adr;
char sadr[16], *ps = sadr;

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
		lr.host_type = HOST_SLAVE;
		lr.lninst = actslave_inst;
		actslave_inst++;

		createlowrecord(&lr);
}

// ssd functions

void TagSetHrdw(const char *pTag){
	printf("Config Manager: Start ADDR file to parse\n");
}

void TagEndHrdw(const char *pTag){
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
  {"SCADA", TagSCADA},
  {"M100", TagM100300},
  {"M300", TagM100300},
  {"M500", TagM500700},
  {"M700", TagM500700},
  {"KIPP", TagKIPP},
  {"Hardware", TagSetHrdw},
  {"/Hardware", TagEndHrdw},
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
