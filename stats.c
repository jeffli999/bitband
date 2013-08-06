#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

extern int	total_rules, LEAF_RULES;
extern int	total_nodes, leaf_nodes, max_depth, trie_nodes_size;
extern Trie	*root_node, **trie_nodes, *max_depth_leaf;

// data structures for statistics
int		depth_nodes[MAX_DEPTH], depth_leaf_nodes[MAX_DEPTH];
Trie 	*depth_max_node[MAX_DEPTH];
int		*rule_duplicates;


typedef struct RuleStat	RuleStat;

struct RuleStat {
	int			nrules;
	Rule		**rules;
	int			head_rule;
	int			count;
	RuleStat	*next;
};

#define RULE_STAT_NUM	32
#define	HASH_SIZE		31

RuleStat	*rulestat[RULE_STAT_NUM][HASH_SIZE];



void calc_rulestat()
{
	int			i, h, n, count, dup_count;
	Trie		*v;
	RuleStat	*rs;

	for (i = 0; i < total_nodes; i++) {
		v = trie_nodes[i];
		n = v->nrules;	
		if (n > RULE_STAT_NUM || n == 0)
			continue;

		h = v->rules[0]->id % HASH_SIZE;
		for (rs = rulestat[n-1][h]; rs != NULL; rs = rs->next) {
			if (rs->head_rule != v->rules[0]->id)
				continue;
			if (memcmp(v->rules, rs->rules, n*sizeof(Rule*)) == 0) {
				rs->count++;
				break;
			}
		}
		if (rs == NULL) {
			rs = malloc(sizeof(RuleStat));
			rs->nrules = n;
			rs->rules = v->rules;
			rs->head_rule = v->rules[0]->id;
			rs->count = 1;
			rs->next = rulestat[n-1][h];
			rulestat[n-1][h] = rs;
		}
	}

	printf("\nRulesets and their frequencies\n");
	printf("------------------------------------------\n");
	for (i = 1; i < RULE_STAT_NUM; i++) {
		printf("#Rules: %d\n\t", i);
		n = 0;
		count = dup_count = 0;
		for (h = 0; h < HASH_SIZE; h++) {
			for (rs = rulestat[i][h]; rs != NULL; rs = rs->next) {
				count += rs->count;
				dup_count += rs->count - 1;
				printf("H[%4d#%3d],  ", rs->head_rule, rs->count);
				if (++n == 6) {
					n = 0;
					printf("\n\t");
				}
			}
		}
		printf("\ncount: %5d, dup_count: %5d\n", count, dup_count);
	}
}



void calc_depth_nodes()
{
	Trie	*v;
	int		i;

	for (i = 0; i < total_nodes; i++) {
		v = trie_nodes[i];
		depth_nodes[v->depth]++;
		if (v->type == LEAF)
			depth_leaf_nodes[v->depth]++;
		if (depth_max_node[v->depth] == NULL || v->nrules > depth_max_node[v->depth]->nrules)
			depth_max_node[v->depth] = v;
	}
}



void calc_most_dup()
{
	int		i, j, k, rid, ndup0, ndup1;
	int		most_dup_rules[9];

	rule_duplicates = calloc(total_rules, sizeof(int));

	for (k = 0; k < 8; k++)
		most_dup_rules[k] = -1;

	for (i = 0; i < total_nodes; i++) {
		for (j = 0; j < trie_nodes[i]->nrules; j++) {
			rid = trie_nodes[i]->rules[j]->id;
			rule_duplicates[rid]++;
		}
	}

	for (rid = 0; rid < total_rules; rid++) {
		if (rule_duplicates[rid] <= 64)
			continue;
		ndup0 = rule_duplicates[rid];
		for (k = 7; k >= 0; k--) {
			if (most_dup_rules[k] == -1)
				continue;
			ndup1 = rule_duplicates[most_dup_rules[k]];
			if (ndup0 < ndup1)
				break;
			most_dup_rules[k+1] = most_dup_rules[k];
		}
		if (k < 7) {
			most_dup_rules[k+1] = rid;
		}
	}

	printf("\nMost duplicate rules\n");
	printf("--------------------------\n");
	for (k = 0; k < 8; k++) {
		rid = most_dup_rules[k];
		if (rid < 0)
			break;
		printf("rule[%d]: %d times\n", rid, rule_duplicates[rid]);
	}
	printf("\n");
}



void dump_stats()
{
	int		i, j;

#if 1
	for (i = 0; i < total_nodes; i++)
		dump_node(trie_nodes[i], 0);
#endif
	//dump_path(trie_nodes[2422], 2);
	
	//calc_rulestat();
	calc_most_dup();
	calc_depth_nodes();
	printf("Nodes at vaious depths\n");
	printf("-------------------------------------\n");
	for (i = 1; i < MAX_DEPTH; i++) {
		if (depth_nodes[i] == 0)
			break;
		printf("depth[%2d]: %5d/%5d \tmax_node: %d#%d\n", i, depth_nodes[i], depth_leaf_nodes[i], 
				depth_max_node[i]->id, depth_max_node[i]->nrules);
	}

	printf("total nodes: %d, leaf nodes: %d, max depth: %d\n", total_nodes, leaf_nodes, max_depth+1);
}




