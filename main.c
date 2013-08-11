#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "bitband.h"
#include "rule.h"
#include "trie.h"


FILE		*fp = NULL;
Rule		*ruleset = NULL;
int			num_rules = 0;


int main(int argc, char **argv)
{
	int		leaf_rules;
	
	if (argc != 3) {
		printf("%s <leaf_rules> <bench>\n", argv[0]);
		exit(1);
	}

	leaf_rules = atoi(argv[1]);
	leaf_rules = leaf_rules==0 ? 4 : leaf_rules;

	fp = fopen(argv[2], "r");

	if (fp == NULL) {
		fprintf(stderr, "Failed to open file\n");
		exit(0);
	}

	num_rules = loadrules(fp, &ruleset);
	fclose(fp);
	build_field_poset(ruleset, num_rules);
	//dump_ruleset(ruleset, num_rules);
	//build_trie(ruleset, num_rules, leaf_rules);

	//test_band();
}
