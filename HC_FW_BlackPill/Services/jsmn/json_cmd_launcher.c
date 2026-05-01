/*
 * json_cmd_launcher.c
 *
 *  Created on: 30 Apr 2020
 *      Author: Marty Hauff
 *
 *  Parse JSON input and launch individual command processors
 */

#include "json_cmd_launcher.h"

//jsmnerr_t call_json_handler(jsmntok_t* tkns, char *json_string, jsmn_search_t search)
jsmnerr_t call_json_handler(jsmntok_t* tkns, char *json_string)
{
	jsmntok_t* this = tkns;
	jsmntok_t* ktkn;
	int hres;

//	for (int i = 0; i < sizeof(json_handlers)/sizeof(json_handlers[0]); i++)
	for (int i = 0; json_handlers[i].name[0] != '\0'; i++)
	{
		this = tkns;
		ktkn = jsmn_get_tkn_from_kstr (this, json_string, (char*)(json_handlers[i].name));
		while (ktkn != TERMINAL_TOKEN)
		{
			hres = json_handlers[i].hndlr(ktkn, json_string);
			if (hres == HNDLR_OK)
				this = ktkn + 1 + ktkn->descendants;
			else
				this++;
			ktkn = jsmn_get_tkn_from_kstr (this, json_string, (char*)(json_handlers[i].name));
		}
	}
	return JSMN_OK;
}

