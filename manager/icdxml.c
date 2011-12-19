/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include "icdxml.h"
#include "autoconfig.h"

extern LOWREC *lrs[];
extern uint32_t maxrec;
extern char *unitlink_list[];

lntype ln_type[] = {
		{"LLN0a",	"LLN0",	"M700"},
		{"MMXUa",	"MMXU",	"M700"},
		{"MSQIa",	"MSQI",	"M700"},
		{"MMTRa",	"MMTR",	"M700"},
		{"MSTAa",	"MSTA",	"M700a"},
		{"MSTAb",	"MSTA",	"M700b"},
		{NULL,		NULL}
};


dobj d_obj[] = {
		{"MMXUa",	"TotW"},
		{"MMXUa",	"TotVAr"},
		{"MMXUa",	"TotVA"},
		{"MMXUa",	"TotPF"},
		{"MMXUa",	"Hz"},
		{"MMXUa",	"PPVphsAB"},
		{"MMXUa",	"PPVphsCA"},
		{"MMXUa",	"PPVphsBC"},
		{"MMXUa",	"PhVphsA"},
		{"MMXUa",	"PhVphsB"},
		{"MMXUa",	"PhVphsC"},
		{"MMXUa",	"AphsA"},
		{"MMXUa",	"AphsB"},
		{"MMXUa",	"AphsC"},
		{"MMXUa",	"WphsA"},
		{"MMXUa",	"WphsB"},
		{"MMXUa",	"WphsC"},
		{"MMXUa",	"VArphsA"},
		{"MMXUa",	"VArphsB"},
		{"MMXUa",	"VArphsC"},
		{"MMXUa",	"VAphsA"},
		{"MMXUa",	"VAphsB"},
		{"MMXUa",	"VAphsC"},
		{"MMXUa",	"PFphsA"},
		{"MMXUa",	"PFphsB"},
		{"MMXUa",	"PFphsC"},
		{"MMXUa",	"ZphsA"},
		{"MMXUa",	"ZphsB"},
		{"MMXUa",	"ZphsC"},
		{"MSQIa",	"SeqAc1"},
		{"MSQIa",	"SeqAc2"},
		{"MSQIa",	"SeqAc3"},
		{"MSQIa",	"SeqVc1"},
		{"MSQIa",	"SeqVc2"},
		{"MSQIa",	"SeqVc3"},
		{"MMTRa",	"SupWh"},
		{"MMTRa",	"DmdWh"},
		{"MMTRa",	"SupVArh"},
		{"MMTRa",	"DmdVArh"},
		{"MSTAa",	"AvAmpsA"},
		{"MSTAa",	"AvVoltsA"},
		{"MSTAb",	"AvVoltsB"},
		{NULL,		NULL}
};


void icd_full(char *fname)
{
	FILE *fdesc = fopen(fname, "w");

	icd_begin(fdesc, "SCL for iec-104", "IEDName");

//	icd_substation(fdesc, "S12", "SPb");
	icd_ied(fdesc, IED_NAME, "Gallery, bay1");
//	icd_ied_scada(fdesc, SCADA_NAME, SCADA_DESC);
//	icd_ied_meter(fdesc, METER_NAME, METER_DESC);
	icd_dtype_tmpl(fdesc);

	icd_end(fdesc);

	fclose(fdesc);
}

void icd_begin(FILE *fdesc, char *id, char *name)
{
	fprintf(fdesc, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(fdesc, "<SCL xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.iec.ch/61850/2003/SCL\">\n");

	fprintf(fdesc, "  <Header id=\"%s\" nameStructure=\"%s\" />\n", id, name);
}


void icd_substation(FILE *fdesc, char *name, char *desc)
{
	int i;

	fprintf(fdesc, "  <Substation name=\"%s\" desc=\"%s\">\n", name, desc);
	fprintf(fdesc, "    <VoltageLevel name=\"VL1\">\n");
	fprintf(fdesc, "      <Bay name=\"B1\">\n");

	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->connect)
		{
			fprintf(fdesc, "        <LNode lnInst=\"%d\" lnClass=\"MMXU\" iedName=\"%s\" ldInst=\"%d\" lnType=\"MMXUa\" />\n", lrs[i]->lninst, lrs[i]->host_type==HOST_SLAVE?SCADA_NAME:METER_NAME, lrs[i]->asdu);
		}
	}

	fprintf(fdesc, "      </Bay>\n");
	fprintf(fdesc, "    </VoltageLevel>\n");
	fprintf(fdesc, "  </Substation>\n");
}


void icd_ied(FILE *fdesc, char *name, char *desc)
{
	int i, j;

	fprintf(fdesc, "  <IED name=\"%s\" desc=\"%s\">\n", name, desc);
	fprintf(fdesc, "    <AccessPoint name=\"AP2\">\n");
	fprintf(fdesc, "      <Server>\n");
	fprintf(fdesc, "      <Authentication />\n");

	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->connect)
		{
			fprintf(fdesc, "        <LDevice inst=\"%d\" desc=\"%s\">\n", lrs[i]->asdu, unitlink_list[lrs[i]->scen]);
			fprintf(fdesc, "          <LN0 lnClass=\"LLN0\" lnType=\"LLN0a\" />\n");
			for(j=1; ln_type[j].id != NULL; j++)
			{
				fprintf(fdesc, "          <LN inst=\"%d\" lnClass=\"%s\" lnType=\"%s\" prefix=\"%s\" />\n", lrs[i]->lninst, ln_type[j].class, ln_type[j].id, ln_type[j].prefix);
			}
			fprintf(fdesc, "        </LDevice>\n");
		}
	}

	fprintf(fdesc, "      </Server>\n");
	fprintf(fdesc, "    </AccessPoint>\n");
	fprintf(fdesc, "  </IED>\n");
}


void icd_ied_scada(FILE *fdesc, char *name, char *desc)
{
	int i;

	fprintf(fdesc, "  <IED name=\"%s\" desc=\"%s\">\n", name, desc);
	fprintf(fdesc, "    <AccessPoint name=\"AP1\">\n");
	fprintf(fdesc, "      <Server>\n");
	fprintf(fdesc, "      <Authentication />\n");

	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->host_type == HOST_SLAVE && lrs[i]->connect)
		{
			fprintf(fdesc, "        <LDevice inst=\"%d\" desc=\"%s\" />\n", lrs[i]->asdu, unitlink_list[lrs[i]->scen]);
			fprintf(fdesc, "          <LN0 lnClass=\"LLN0\" lnType=\"LLN0a\" inst=\"\" />\n");
			fprintf(fdesc, "          <LN inst=\"%d\" lnClass=\"MMXU\" lnType=\"MMXUa\" />\n", lrs[i]->lninst);
			fprintf(fdesc, "        </LDevice>\n");
		}
	}

	fprintf(fdesc, "      </Server>\n");
	fprintf(fdesc, "    </AccessPoint>\n");
	fprintf(fdesc, "  </IED>\n");
}


void icd_ied_meter(FILE *fdesc, char *name, char *desc)
{
	int i;

	fprintf(fdesc, "  <IED name=\"%s\" desc=\"%s\">\n", name, desc);
	fprintf(fdesc, "    <AccessPoint name=\"AP2\">\n");
	fprintf(fdesc, "      <Server>\n");
	fprintf(fdesc, "      <Authentication />\n");

	for(i=0; i<maxrec; i++)
	{
		if(lrs[i]->host_type == HOST_MASTER && lrs[i]->connect)
		{
			fprintf(fdesc, "        <LDevice inst=\"%d\" desc=\"%s\" />\n", lrs[i]->asdu, unitlink_list[lrs[i]->scen]);
			fprintf(fdesc, "          <LN0 lnClass=\"LLN0\" lnType=\"LLN0a\" inst=\"\" />\n");
			fprintf(fdesc, "          <LN inst=\"%d\" lnClass=\"MMXU\" lnType=\"MMXUa\" />\n", lrs[i]->lninst);
			fprintf(fdesc, "        </LDevice>\n");
		}
	}

	fprintf(fdesc, "      </Server>\n");
	fprintf(fdesc, "    </AccessPoint>\n");
	fprintf(fdesc, "  </IED>\n");
}


void icd_dtype_tmpl(FILE *fdesc)
{
	fprintf(fdesc, "  <DataTypeTemplates>\n");
	icd_lnode_type(fdesc);
	icd_do_type(fdesc);

	fprintf(fdesc, "    <DAType id=\"AnalogueValue\">\n");
	fprintf(fdesc, "      <BDA name=\"i\" bType=\"INT32\" />\n");
	fprintf(fdesc, "      <BDA name=\"f\" bType=\"FLOAT32\" />\n");
	fprintf(fdesc, "    </DAType>\n");

	fprintf(fdesc, "  </DataTypeTemplates>\n");
}


void icd_lnode_type(FILE *fdesc)
{
	int i, j;

	for(i=0; ln_type[i].id != NULL; i++)
	{
		fprintf(fdesc, "    <LNodeType id=\"%s\" lnClass=\"%s\">\n", ln_type[i].id, ln_type[i].class);
		fprintf(fdesc, "      <DO name=\"Mod\" type=\"INC\" />\n");
		fprintf(fdesc, "      <DO name=\"Beh\" type=\"INS\" />\n");
		fprintf(fdesc, "      <DO name=\"Health\" type=\"INS\" />\n");
		if(strstr(ln_type[i].class, "LLN0"))
		{
			fprintf(fdesc, "      <DO name=\"NamPlt\" type=\"LPLlln0\" />\n");
		}
		else
		{
			fprintf(fdesc, "      <DO name=\"NamPlt\" type=\"LPLgeneral\" />\n");
		}

		for(j=0; d_obj[j].name != NULL; j++)
		{
			if(strstr(ln_type[i].id, d_obj[j].lntypeid))
			{
				fprintf(fdesc, "      <DO name=\"%s\" type=\"MV\" desc=\"%d\" />\n", d_obj[j].name, j+1);
			}
		}

		fprintf(fdesc, "    </LNodeType>\n");
	}
}


void icd_do_type(FILE *fdesc)
{
	fprintf(fdesc, "    <DOType id=\"MV\" cdc=\"MV\">\n");
	fprintf(fdesc, "      <DA name=\"mag\" fc=\"MX\" bType=\"Struct\" type=\"AnalogueValue\" dchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"q\" fc=\"MX\" bType=\"Quality\" qchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"t\" fc=\"MX\" bType=\"Timestamp\" />\n");
	fprintf(fdesc, "    </DOType>\n");


	fprintf(fdesc, "    <DOType id=\"LPLgeneral\" cdc=\"LPL\">\n");
	fprintf(fdesc, "      <DA name=\"vendor\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"swRev\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"d\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "    </DOType>\n");


	fprintf(fdesc, "    <DOType id=\"LPLlln0\" cdc=\"LPL\">\n");
	fprintf(fdesc, "      <DA name=\"vendor\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"swRev\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"d\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"configRev\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"ldNs\" fc=\"EX\" bType=\"VisString255\">\n");
	fprintf(fdesc, "        <Val>IEC 61850-7-4:2003</Val>\n");
	fprintf(fdesc, "      </DA>\n");
	fprintf(fdesc, "    </DOType>\n");


	fprintf(fdesc, "    <DOType id=\"INS\" cdc=\"INS\">\n");
	fprintf(fdesc, "      <DA name=\"stVal\" fc=\"ST\" bType=\"INT32\" dchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"q\" fc=\"ST\" bType=\"Quality\" qchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"t\" fc=\"ST\" bType=\"Timestamp\" />\n");
	fprintf(fdesc, "    </DOType>\n");


	fprintf(fdesc, "    <DOType id=\"INC\" cdc=\"INC\">\n");
	fprintf(fdesc, "      <DA name=\"stVal\" fc=\"ST\" bType=\"INT32\" dchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"q\" fc=\"ST\" bType=\"Quality\" qchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"t\" fc=\"ST\" bType=\"Timestamp\" />\n");
	fprintf(fdesc, "    </DOType>\n");
}


void icd_end(FILE *fdesc)
{
	fprintf(fdesc, "</SCL>\n");
}














