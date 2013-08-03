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

	fp = fopen(argv[1], "r");

	if (fp == NULL) {
		fprintf(stderr, "Failed to open file\n");
		exit(0);
	}

	num_rules = loadrules(fp, &ruleset);
	fclose(fp);
	//dump_ruleset(ruleset, num_rules);
	build_trie(ruleset, num_rules);

	//test_band();
}
