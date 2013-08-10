#ifndef POSET_H
#define POSET_H

#include "rule.h"
#include "common.h"

typedef struct FieldNode	FieldNode;
typedef struct FNodeLink	FNodeLink;

struct FieldNode {
	uint8_t		dim;
	uint32_t	id;
	uint32_t	level;
	uint32_t	num_in, num_out;
	uint32_t	count;
	uint32_t	flag;
	Range		val;
	FNodeLink	*in, *out;
};


struct FNodeLink {
	FieldNode	*src, *dst;
	FNodeLink	*prev_out, *next_out;
	FNodeLink	*prev_in, *next_in;
};


void dump_field_posets();


#endif
