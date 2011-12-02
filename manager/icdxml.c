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

dobj    *d_obj     = NULL;
uint32_t d_obj_num = 0;

dobj d_obj_def[] = {
		{"PhVphsA",		0},
		{"PhVphsB",		1},
		{"PhVphsC",		2},
		{"AphsA",		3},
		{"AphsB",		4},
		{"AphsC",		5},
		{"PPVphsAB",	6},
		{"PPVphsCA",	7},
		{"PPVphsBC",	8},
		{"WphsA",		8},
		{"WphsB",		10},
		{"WphsC",		11},
		{"VArphsA",		12},
		{"VArphsB",		13},
		{"VArphsC",		14}
};
uint32_t d_obj_num_def = 15;


void icd_full(char *fname)
{
	if(!d_obj)
	{
		printf("Config Manager: Default DOBJ list will be used.\n");

		d_obj     = d_obj_def;
		d_obj_num = d_obj_num_def;
	}

	FILE *fdesc = fopen(fname, "w");

	icd_begin(fdesc, "SCL for iec-104", "IEDName");
	icd_substation(fdesc, "S12", "SPb");
	icd_ied_scada(fdesc, SCADA_NAME, SCADA_DESC);
	icd_ied_meter(fdesc, METER_NAME, METER_DESC);
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
			fprintf(fdesc, "          <LN0 lnClass=\"LLN0\" lnType=\"LLN0a\" inst=\"\" desc=\"\" />\n");
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
			fprintf(fdesc, "          <LN0 lnClass=\"LLN0\" lnType=\"LLN0a\" />\n");
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
	fprintf(fdesc, "      <BDA name=\"f\" bType=\"FLOAT32\" />\n");
	fprintf(fdesc, "    </DAType>\n");

	fprintf(fdesc, "  </DataTypeTemplates>\n");
}


void icd_lnode_type(FILE *fdesc)
{
	int i = 0;

	fprintf(fdesc, "    <LNodeType id=\"MMXUa\" lnClass=\"MMXU\">\n");
	fprintf(fdesc, "      <DO name=\"Mod\" type=\"INC\" />\n");
	fprintf(fdesc, "      <DO name=\"Beh\" type=\"INS\" />\n");
	fprintf(fdesc, "      <DO name=\"Health\" type=\"INS\" />\n");
	fprintf(fdesc, "      <DO name=\"NamPlt\" type=\"LPL_MMXU\" />\n");

	for(i=0; i<d_obj_num; i++)
	{
		fprintf(fdesc, "      <DO name=\"%s\" type=\"MV\" desc=\"%d\" />\n", d_obj[i].name, d_obj[i].asdu_pos);
	}

	fprintf(fdesc, "    </LNodeType>\n");

	fprintf(fdesc, "    <LNodeType id=\"LLN0a\" lnClass=\"LLN0\">\n");
	fprintf(fdesc, "      <DO name=\"Mod\" type=\"INC\" />\n");
	fprintf(fdesc, "      <DO name=\"Beh\" type=\"INS\" />\n");
	fprintf(fdesc, "      <DO name=\"Health\" type=\"INS\" />\n");
	fprintf(fdesc, "      <DO name=\"NamPlt\" type=\"LPL_LLN0\" />\n");
	fprintf(fdesc, "    </LNodeType>\n");
}


void icd_do_type(FILE *fdesc)
{
	fprintf(fdesc, "    <DOType id=\"MV\" cdc=\"MV\">\n");
	fprintf(fdesc, "      <DA name=\"mag\" fc=\"MX\" bType=\"Struct\" type=\"AnalogueValue\" dchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"q\" fc=\"MX\" bType=\"Quality\" qchg=\"true\" />\n");
	fprintf(fdesc, "      <DA name=\"t\" fc=\"MX\" bType=\"Timestamp\" />\n");
	fprintf(fdesc, "    </DOType>\n");


	fprintf(fdesc, "    <DOType id=\"LPL_MMXU\" cdc=\"LPL\">\n");
	fprintf(fdesc, "      <DA name=\"vendor\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"swRev\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "      <DA name=\"d\" fc=\"DC\" bType=\"VisString255\" />\n");
	fprintf(fdesc, "    </DOType>\n");


	fprintf(fdesc, "    <DOType id=\"LPL_LLN0\" cdc=\"LPL\">\n");
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


int icd_add_dobj_item(uint32_t base_id, char *name)
{
	dobj *d_obj_new = NULL;

	d_obj_new = (dobj*) realloc((void*) d_obj, sizeof(dobj)*(d_obj_num+1));

	if(!d_obj_new) return -1;

	d_obj = d_obj_new;

	d_obj[d_obj_num].name = (char*) calloc(DOBJ_NAMESIZE, sizeof(char));

	memcpy((void*)d_obj[d_obj_num].name, (void*)name, DOBJ_NAMESIZE);
	d_obj[d_obj_num].asdu_pos = base_id;

	printf("Config Manager: New DOBJ map was added. asdu_pos = %d, dobj_name = \"%s\"\n", d_obj[d_obj_num].asdu_pos, d_obj[d_obj_num].name);

	d_obj_num++;

	return 0;
}


void icd_map_read(const char *file_name)
{
	FILE *map_file = NULL;
	char r_buff[256] = {0};
	uint32_t map_num;
	uint32_t base_id;
	char name[DOBJ_NAMESIZE];

	map_file = fopen(file_name, "r");

	if(map_file)
	{
		map_num = 0;

		while(fgets(r_buff, 255, map_file))
		{
			if(*r_buff == '#') continue;

			sscanf(r_buff, "%d %s", &base_id, name);

			icd_add_dobj_item(base_id, name);

			map_num++;
		}
	}

}














