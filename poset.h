#ifndef POSET_H
#define POSET_H

#include "rule.h"
#include "common.h"

typedef struct FieldNode	FieldNode;
typedef struct FNodeLink	FNodeLink;

struct FieldNode {
	uint8_t		dim;
	Range		val;
	FNodeLink	*in, *out;
};


struct FNodeLink {
	FieldNode	*src, *dst;
	FNodeLink	*next_in, *next_out;
};


#endif
