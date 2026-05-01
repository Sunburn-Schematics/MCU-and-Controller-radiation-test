/*
 * json_cmd_launcher.h
 *
 *  Created on: 30 Apr 2020
 *      Author: user
 */

#ifndef JSON_CMD_LAUNCHER_H_
#define JSON_CMD_LAUNCHER_H_

#include "jsmn_utils.h"

typedef enum {
	HNDLR_OK = 0,
	HNDLR_ERROR = -1
} hndlr_res_t;

typedef struct {
	char* name;
	hndlr_res_t (*hndlr)(jsmntok_t*, const char *);
} json_hndlr_t;


extern json_hndlr_t json_handlers[];



jsmnerr_t call_json_handler(jsmntok_t* tkns, char *json_string);
//jsmnerr_t call_json_handler(jsmntok_t* tkns, char *json_string, jsmn_search_t search);

#endif /* JSON_CMD_LAUNCHER_H_ */
