/*
 * jsmn_print_utils.c
 *
 *  Created on: 29 Apr 2020
 *      Author: Marty Hauff
 */

#include "jsmn_print_utils.h"

#include <stdio.h>
#include <ctype.h>

const char* jsmn_types_hrid[] = {
		"Undefined",
		"Object",
		"Array",
		"String",
		"Primitive"
};

void jsmn_flatprint(jsmntok_t* tkn, const char* json_string)
{
	for (int i = tkn->start; i < tkn->end; i++)
	{
		if (isspace (json_string[i])) {
			putchar(' ');
			while (isspace(json_string[++i]));
		}
		putchar (json_string[i]);
	}
}

void jsmn_tokenprint(jsmntok_t* tkn, const char* json_string)
{
	printf ("<%d> (%d) %s %d %d %d %d %d",
			tkn->index,
			tkn->parent,
			jsmn_types_hrid[tkn->type],
			tkn->start,
			tkn->end,
			tkn->size,
			tkn->descendants,
			tkn->depth
			);
}

void jsmn_verboseprint(jsmntok_t* tkn, const char* json_string)
{
	jsmn_tokenprint(tkn, json_string);
	putchar (' ');
	jsmn_flatprint(tkn, json_string);
}

void jsmn_verboseprint_branch(jsmntok_t* tkn, const char* json_string)
{
	int i = 0;
	int cnt = tkn->descendants;
	while (i <= cnt)
	{
		printf ("\n[%d] ", i++);
		if (tkn->type == JSMN_UNDEFINED)
			break;
		for (int j = 1; j < tkn->depth; j++)
			putchar('-');
		jsmn_verboseprint(tkn, json_string);
		tkn++;
	}
}

void jsmn_verboseprint_tree(jsmntok_t* tkn, const char* json_string)
{
	int i = 0;
	while (tkn->type != JSMN_UNDEFINED)
	{
		printf ("\n[%d] ", i++);
		for (int j = 1; j < tkn->depth; j++)
			putchar('-');
		jsmn_verboseprint(tkn, json_string);
		tkn++;
	}
}

void jsmn_immediate_children(jsmntok_t* tkns, const char* json_string)
{
	jsmntok_t* this = tkns;
	int max = this->index + this->descendants;

	printf ("\nImmediate Children of ");
	jsmn_verboseprint(this, json_string);
	printf ("\nNode %d has %d immediate children and %d total descendants", this->index, this->size, this->descendants);
	this++;			//Assume the first child token is the next one
	while (this->index < max)
	{
		if (this->type == JSMN_UNDEFINED)
			return;  //Something went very wrong!!

		if (this->parent == tkns->index)
		{
			printf("\n");
			jsmn_verboseprint(this, json_string);
			this += this->descendants;
		}
		this++;
	}
}


