/*
 * jsmn_utils.c
 *
 *  Created on: 29 Apr 2020
 *      Author: Marty Hauff
 */

#include "jsmn_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int TERMINAL_TOKEN_IDX = 0;
jsmntok_t* TERMINAL_TOKEN;

static jsmntok_t* jsmn_terminate_tokens(jsmntok_t* tkns, const int index);
static int jsmn_add_util_metrics(jsmntok_t* tkns, const char* json_string);

const char* jsmn_err_hrid(jsmnerr_t err)
{
	switch (err)
	{
	case JSMN_OK : 				return("OK"); break;
	case JSMN_ERROR_NOMEM : 	return ("ERROR_NOMEM"); break;
	case JSMN_ERROR_INVAL : 	return ("ERROR_INVAL"); break;
	case JSMN_ERROR_PART  : 	return ("ERROR_PART"); break;
	case JSMN_TOKEN_NOT_FOUND : return ("TOKEN_NOT_FOUND"); break;
	case JSMN_INVALID_TOKEN  : 	return ("INVALID_TOKEN"); break;
	default : 					return ("UNKNOWN ERROR CODE");
	}
}

//Wrapper for jsmn_parse() but does everything in a single call
JSMN_API int jsmn_utils_parse(jsmn_parser *p, const char *js, const size_t len, jsmntok_t *tkns, const unsigned int num_tokens)
{
	int jres;

	jsmn_init(p);
	jres = jsmn_parse(p, js, len, tkns, num_tokens - 1);	//Leave space for the terminal token
	if (jres < 0)
		return jres;	//An error occurred during parsing
	jsmn_post_process(p, tkns, js);
	return TERMINAL_TOKEN_IDX;
}

JSMN_API int jsmn_post_process(jsmn_parser *parser, jsmntok_t *tkns, const char* json_string)
{
	jsmntok_t *tkn = tkns;

	//Put an end-cap on the token chain
	TERMINAL_TOKEN_IDX = parser->toknext;
	TERMINAL_TOKEN = jsmn_terminate_tokens(tkns, TERMINAL_TOKEN_IDX);

	//Add an index to each token so can more easily navigate the token tree
	for (int i = 0; i <= TERMINAL_TOKEN_IDX; i++)
		tkn++->index = i;

	//Add descendant and depth metrics for faster token processing (at the cost of memory)
	jsmn_add_util_metrics(tkns, json_string);

	return JSMN_OK;
}

/******************************************************/
jsmntok_t* jsmn_get_tkn_from_kstr (jsmntok_t* tkns, const char* json_string, const char* kstr)
{
	jsmntok_t* this = tkns;

//Enable the following line (to improve search performance) if JSON file will not have duplicate vstr names.
//	int end = this->index + this->descendants;
	int kstrlen = strlen(kstr);
	int tkn_strlen;

	while (this != TERMINAL_TOKEN)
	{
		if ((this->type == JSMN_STRING) && (this->size > 0))	//Must be a string with at least one child for it to be a key
		{
			tkn_strlen = this->end - this->start;
			if (kstrlen == tkn_strlen)
			{
				if (memcmp(kstr, json_string + this->start, tkn_strlen) == 0)
					return this;
			}
		}
		this++;
//		this = jsmn_tkn_walk(this, tkns, search);		//Wish I could get this to work :(
	}
	return TERMINAL_TOKEN;
}

jsmntok_t* jsmn_get_vtkn_from_ktkn (jsmntok_t* ktkn)
{
	if (ktkn == TERMINAL_TOKEN)
		return TERMINAL_TOKEN;

	if ((ktkn->type == JSMN_STRING) && (ktkn->size > 0))		//A key must be a string
		return (++ktkn);
	else
		return TERMINAL_TOKEN;
}

jsmnerr_t jsmn_get_str_from_tkn (jsmntok_t* tkn, const char* json_string, char* str, int len)
{
	if (tkn == TERMINAL_TOKEN)
		return JSMN_INVALID_TOKEN;
	len = (len < (tkn->end - tkn->start)) ? len : (tkn->end - tkn->start);
	memcpy(str, json_string + tkn->start, len);
	str[len] = '\0';
	return JSMN_OK;
}

jsmnerr_t jsmn_get_vstr_from_kstr(jsmntok_t* tkns, const char* json_string, const char* kstr, char* vstr, int vstrlen)
{
	jsmntok_t* ktkn = jsmn_get_tkn_from_kstr (tkns, json_string, kstr);
	jsmntok_t* vtkn = jsmn_get_vtkn_from_ktkn (ktkn);
	return jsmn_get_str_from_tkn (vtkn, json_string, vstr, vstrlen);
}

jsmnerr_t jsmn_str_to_int (const char* str, int lolim, int hilim, int* num)
{
	char* end;
	int tmp;
	int multiplier = 0;

	if (str[0] == '+')
		multiplier = 1;
	if (str[0] == '-')
		multiplier = -1;

	if (multiplier != 0)
		str++;

	tmp = strtol(str, &end, 10);
	if (*end == '\0')
	{
		if (multiplier != 0)
			tmp = *num + (tmp * multiplier);
		if (tmp <= lolim)
			tmp = lolim;
		if (tmp >= hilim)
			tmp = hilim;
		*num = tmp;
		return JSMN_OK;
	}
	return JSMN_ERROR_INVAL;
}


/******************************************************/
static jsmntok_t* jsmn_terminate_tokens(jsmntok_t* tkns, int index)
{
	jsmntok_t* tok = &tkns[index];
	tok->type = JSMN_UNDEFINED;
	tok->start = 0;
	tok->end = 0;
	tok->size = 0;
	tok->parent = -1;
	return tok;
}

static int jsmn_add_util_metrics(jsmntok_t* tkns, const char* json_string)
{
	static int depth = 0;
	int descendants;
	jsmntok_t* this = tkns;
	jsmntok_t* next = this + 1;

	depth++;
	this->descendants = 0;
	this->depth = depth;
	while (1)
	{
		if (next->type == JSMN_UNDEFINED)
			break;

		if (next->parent == this->index)
		{
			descendants = 1 + jsmn_add_util_metrics(next, json_string);
			this->descendants += descendants;
			next += descendants;		//Skip over all the nodes we have just traversed
		}
		else
			next++;
	}
	depth--;
	return this->descendants;
}

/******************************************************/
/*
 * 1-May-2020 - Not working yet
 * Problem seems to be at the point we iterate to a node that is
 * outside the branch space but SIBLING is enabled and base is on a
 * SIBLING node. Feels like we need to update base to the common
 * parent node with SIBLING is enabled... not sure.. need to work
 * it out before trying this again.
 */
jsmntok_t* jsmn_tkn_walk (jsmntok_t* this, jsmntok_t* base, jsmn_search_t search)
{
	jsmntok_t* root = this - this->index;
	jsmntok_t* next;

	if (search == THIS_ONLY)
		return TERMINAL_TOKEN;

	//See if we can go down first
	if (search & IN_BRANCH)
	{
		next = this + 1;
		if (next->index <= (base->index + base->descendants))	//If it's still within the current branch
			return next;										//then OK
	}

	//Next see if we can go across
	if (search & SIBLING)
	{
		if (this->parent == base->parent)			//This is already a sibling of base so jump to the next sibling
			return (this + 1 + this->descendants);

		next = base + 1 + base->descendants;		//Skip ahead by the size of the current base token
		if (next->parent == base->parent)				//Confirm it has the same parent before accepting
			return next;
	}

	//Can't go down or sideways, all that's left is up
	if (search & ANCESTOR)
		return &root[this->parent];

	//Otherwise have exhausted all options
	return TERMINAL_TOKEN;
}



