/*- Common module - XML Parser for SCL
 *  Created on: 13.07.2011
 *      Author: Alex AVAlon
 *
 *	Module task: Parse XML config for IEC61850
 *
*/

#include "../common/common.h"
#include "../common/iec61850.h"

u08 Encoding, EndScript;

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

void TagStartTarifs(const char *pTag){

}

void TagSetSeason(const char *pTag){

}

void TagEndSeason(const char *pTag){

}

void TagSetWDays(const char *pTag){

}

void TagSetHolidays(const char *pTag){

}

void TagSetHighdays(const char *pTag){

}

void TagEndHighdays(const char *pTag){

}

void TagSetWDay(const char *pTag){

}

void TagSetHlday(const char *pTag){

}

void TagSetHgday(const char *pTag){

}

void TagSetSet(const char *pTag){

}

//*** End Tag RAW working ***//

typedef struct _XML_Name{
	char *Name;
	void(*Function)(const char *pTag);
} XML_Name, *pXML_Name;

static const XML_Name XTags[] = {
  // Tags for IEC config
  {"Header", TagSetHeader},
  {"/Header", TagEndHeader},
  {"IED", cid_create_ied},
  {"/IED", TagEndIED},
  {"Substation", cid_create_subst},
  {"/Substation", TagEndSubs},
  {"LDevice", cid_create_ld},
  {"/LDevice", TagEndLD},
  {"LN", cid_create_ln},
  {"/LN", TagEndLN},
//  {"LNode", cid_create_ln},
//  {"/LNode", TagEndLN},
  {"LNodeType", cid_create_lntype},
  {"/LNodeType", TagEndLNType},
  {"DO", cid_create_dobj},
  {"/DO", TagEndDobj},
  {"DOType", cid_create_dobjtype},
  {"/DOType", TagEndDType},
  {"DA", cid_create_attr},
  {"/DA", TagEndAttr},
  {"EnumType", cid_create_enum},
  {"/EnumType", TagEndEnum},
  {"EnumVal", cid_create_enumval},
  {"/EnumVal", TagEndEnumVal},
  {"SCL", TagSetSCL},
  {"/SCL", TagEndSCL},
  {"?xml", TagSetXml},
  // Tags for tarif config
  {"Tarifs", TagStartTarifs},
  {"Season", TagSetSeason},
  {"/Season", TagEndSeason},
  {"Workdays", TagSetWDays},
  {"Holidays", TagSetHolidays},
  {"Highdays", TagSetHighdays},
  {"/Highdays", TagEndHighdays},
  {"Workday", TagSetWDay},
  {"Holiday", TagSetHlday},
  {"Highday", TagSetHgday},
  {"Set", TagSetSet},

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
