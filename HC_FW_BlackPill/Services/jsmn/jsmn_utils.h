/*
 * jsmn_utils.h
 *
 *  Created on: 29 Apr 2020
 *      Author: user
 */

#ifndef JSMN_UTILS_H_
#define JSMN_UTILS_H_

#define JSMN_UTIL_LINKS
#include "jsmn.h"

extern jsmntok_t* TERMINAL_TOKEN;
extern int TERMINAL_TOKEN_IDX;

typedef enum {
	THIS_ONLY = 0x00,		//Search will span the current token ONLY
	IN_BRANCH = 0x01,		//Search will span the current token and all descendants within the branch
	SIBLING   = 0x02,		//Search will span the current token and all tokens with a common parent
	ANCESTOR  = 0x04,		//Search will span upwards from the current token through the parent chain
	LINEAR    = (IN_BRANCH | SIBLING | ANCESTOR),
	FROM_ROOT = 0x80		//Search will start from the root (Token 0) regardless of the current token
} jsmn_search_t;

const char* jsmn_err_hrid(jsmnerr_t err);

JSMN_API int jsmn_utils_parse(jsmn_parser *p, const char *js, const size_t len, jsmntok_t *tkns, const unsigned int num_tokens);

JSMN_API int jsmn_post_process(jsmn_parser *parser, jsmntok_t *tkns, const char* json_string);

jsmntok_t* jsmn_get_tkn_from_kstr (jsmntok_t* tkns, const char* json_string, const char* kstr);
jsmntok_t* jsmn_get_vtkn_from_ktkn (jsmntok_t* ktkn);

jsmnerr_t jsmn_get_str_from_tkn (jsmntok_t* tkn, const char* json_string, char* str, int len);
jsmnerr_t jsmn_get_vstr_from_kstr(jsmntok_t* tkns, const char* json_string, const char* kstr, char* vstr, int vstrlen);

jsmnerr_t jsmn_str_to_int (const char* str, int lolim, int hilim, int* num);

jsmntok_t* jsmn_tkn_walk (jsmntok_t* this, jsmntok_t* base, jsmn_search_t search);

#endif /* JSMN_UTILS_H_ */
