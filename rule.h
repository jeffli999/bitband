#ifndef RULE_H
#define RULE_H

#include <stdio.h>
#include "common.h"

#define NFIELDS	5	// number of fields

typedef struct {
	int			id;
	Range		field[NFIELDS];
} Rule;


int loadrules(FILE *fp, Rule **rules);
void dump_ruleset();

#endif
