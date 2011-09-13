/*- Common module - XML Parser for SCL
 *  Created on: 13.07.2011
 *      Author: Alex AVAlon
 *
 *	Module task: Parse XML config for IEC61850
 *
*/

#include "../common/common.h"
#include "iec61850.h"

static u08 Encoding, EndScript;

//*** Tag RAW working ***//

void TagSetHeader(const char *pTag){

}

void TagEndHeader(const char *pTag){

}

void TagEndIED(const char *pTag){

}

void TagEndSubs(const char *pTag){

}

void TagEndLD(const char *pTag){

}

void TagEndLN(const char *pTag){

}

void TagEndLNType(const char *pTag){

}

void TagEndDobj(const char *pTag){

}

void TagEndDType(const char *pTag){

}

void TagEndAttr(const char *pTag){

}

void TagEndEnum(const char *pTag){

}

void TagEndEnumVal(const char *pTag){

}

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
  {"Header", TagSetHeader},
  {"/Header", TagEndHeader},
  {"IED", ssd_create_ied},
  {"/IED", TagEndIED},
  {"Substation", ssd_create_subst},
  {"/Substation", TagEndSubs},
  {"LDevice", ssd_create_ld},
  {"/LDevice", TagEndLD},
  {"LNode", ssd_create_ln},
  {"/LNode", TagEndLN},
  {"LNodeType", ssd_create_lntype},
  {"/LNodeType", TagEndLNType},
  {"DO", ssd_create_dobj},
  {"/DO", TagEndDobj},
  {"DOType", ssd_create_dobjtype},
  {"/DOType", TagEndDType},
  {"DA", ssd_create_attr},
  {"/DA", TagEndAttr},
  {"EnumType", ssd_create_enum},
  {"/EnumType", TagEndEnum},
  {"EnumVal", ssd_create_enumval},
  {"/EnumVal", TagEndEnumVal},
  {"SCL", TagSetSCL},
  {"/SCL", TagEndSCL},
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
