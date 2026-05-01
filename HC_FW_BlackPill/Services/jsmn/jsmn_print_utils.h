/*
 * jsmn_print_utils.h
 *
 *  Created on: 29 Apr 2020
 *      Author: user
 */

#ifndef JSMN_PRINT_UTILS_H_
#define JSMN_PRINT_UTILS_H_

#include "jsmn_utils.h"

extern const char* jsmn_types_hrid[];

void jsmn_flatprint(jsmntok_t* tkn, const char* json_string);
void jsmn_tokenprint(jsmntok_t* tkn, const char* json_string);

void jsmn_verboseprint(jsmntok_t* tkn, const char* json_string);
void jsmn_verboseprint_branch(jsmntok_t* tkn, const char* json_string);
void jsmn_verboseprint_tree(jsmntok_t* tkn, const char* json_string);

int jsmn_update_span_counts(jsmntok_t* tkns, const char* json_string);
//int jsmn_update_span_counts(jsmntok_t* tkns, const char* json_string, int index);
void jsmn_keyvaluepairs(jsmntok_t* tkn, const char* json_string, const int nodes);
int jsmn_count_descendants(jsmntok_t* tkns, const char* json_string, int index);

void jsmn_immediate_children(jsmntok_t* tkns, const char* json_string);

#endif /* JSMN_PRINT_UTILS_H_ */
