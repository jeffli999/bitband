#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define		NODES_CHUNK		8192
#define		REDUN_NRULES	256		// don't check rule redundancy if #rules > it
#define		REDUN_NCHECK	16		// only check at most this number of redundant candidates

int		total_nodes, leaf_nodes, max_depth, trie_nodes_size;
Trie	*root_node, **trie_nodes, *max_depth_leaf;


// data structures for dfs based trie construction
Band	dfs_cuts[MAX_DEPTH];
int		dfs_uncuts[MAX_DEPTH][NFIELDS];	// field bands not cut yet
Rule	*dfs_rules_strip[MAX_DEPTH];
int		dfs_rule_redun[MAX_DEPTH][REDUN_NRULES][REDUN_NCHECK];

int		*rule_map_p2c, *rule_map_c2p;	// mapping rule order between parent and child
int		curr_depth;



// check rule redundancy by efficiently comparing with earlier included rules in child
int check_rule_redun(Range *r0, Rule *rules_child, int rid_parent, Band *cut)
{
	int				rid_p, rid_c, i, k = 0;
	Range			r1;
	int				*redun_list = dfs_rule_redun[curr_depth][rid_parent];

	for (i = 0; i < REDUN_NCHECK; i++) {
		if ((rid_p = redun_list[i]) == -1)
			break;
		if ((rid_c = rule_map_p2c[rid_p]) == -1)
			continue;
		r1 = rules_child[rid_c].field[cut->dim];
		if (range_cover(r1, *r0))
			return 1;	// redundant! as it is contained by an earlier rule
	}
	return 0;
}



// select rules from parent that overlap with the cut space, check rule redundancy only for
// nodes with #rules < REDUN_NRULES. both parent and child rules are in stripped forms
int select_rules(Rule *rules_parent, Rule *rules_child, int nrules_parent, Band *cut)
{
	int		nrules_child = 0, is_redun, i;
	Range	*range;

	for (i = 0; i < nrules_parent; i++) {
		rule_map_p2c[i] = -1;
		rules_child[nrules_child] = rules_parent[i];
		range = &rules_child[nrules_child].field[cut->dim];
		if(range_strip(range, cut->bid, cut->val) == 0)
			continue;
		if (nrules_parent > REDUN_NRULES || nrules_child == 0)
			is_redun = 0;
		else
			is_redun = check_rule_redun(range, rules_child, i, cut);
		if (!is_redun) {
			rule_map_p2c[i] = nrules_child;
			rule_map_c2p[nrules_child] = i;
			nrules_child++;
		}
	}
	return nrules_child;
}



void calc_rule_redun(Trie *v, Band *cut)
{
	Rule	*rules = dfs_rules_strip[v->depth];
	Rule	*ri, *rj;
	int		i, j, dim, nredun;
	
	for (i = 1; i < v->nrules; i++) {
		ri = &rules[i];
		nredun = 0;
		for (j = i-1; j >= 0; j--) {
			rj = &rules[j];
			for (dim = 0; dim < NFIELDS; dim++) {
				if (dim == cut->dim)
					continue;
				if (!range_cover(rj->field[dim], ri->field[dim]))
					break;
			}
			if (dim == NFIELDS) {
				dfs_rule_redun[v->depth][i][nredun++] = j;
				if (nredun == REDUN_NCHECK)
					break;
			}
		}
		if (nredun < REDUN_NCHECK)
			dfs_rule_redun[v->depth][i][nredun] = -1;	// the end of redundant candidates
	}
}



int try_cut(Trie *v, Band *cut, int *total_rules)
{
	int		val, nrules, max_nrules = 0;
	Rule	*rules_parent, *rules_child;

	rules_parent = dfs_rules_strip[v->depth];
	rules_child = malloc(v->nrules*sizeof(Rule));
	*total_rules = 0;

	for (cut->val = 0; cut->val < BAND_SIZE; cut->val++) {
		nrules = select_rules(rules_parent, rules_child, v->nrules, cut);
		if (nrules > max_nrules)
			max_nrules = nrules;
		*total_rules += nrules;
	}
	
	free(rules_child);
	return max_nrules;
}



void choose_cut(Trie *v)
{
	Band	*cut;
	int		dim, bid, nrules, max_rules, total_rules, max_total;
	int		best_dim, best_bid;

	max_rules = v->nrules + 1;
	max_total = v->nrules * BAND_SIZE + 1;
	cut = &dfs_cuts[v->depth];

	for (dim = 0; dim < NFIELDS; dim++) {
		cut->dim = dim;
		if (v->nrules <= REDUN_NRULES)
			calc_rule_redun(v, cut);
		for (bid = 0; bid < dfs_uncuts[v->depth][dim]; bid++) {
			cut->bid = bid;
			nrules = try_cut(v, cut, &total_rules);
			if (nrules > max_rules)
				continue;
			if (nrules < max_rules || total_rules < max_total) {
				best_dim = dim;
				best_bid = bid;
				max_rules = nrules;
				max_total = total_rules;
			}
		}
	}
	cut->dim = best_dim;
	cut->bid = best_bid;
}



// cut v with two bands from the five fields
Trie* new_child(Trie *v, Band *cut)
{
	Trie	*u;
	Rule	*rules_parent, *rules_child;
	int		i;
   
	if (dfs_rules_strip[curr_depth+1] == NULL)
		dfs_rules_strip[curr_depth+1] = malloc(v->nrules*sizeof(Rule));
	else
		dfs_rules_strip[curr_depth+1] =
			realloc(dfs_rules_strip[curr_depth+1], v->nrules*sizeof(Rule));
	rules_parent = dfs_rules_strip[curr_depth];
	rules_child  = dfs_rules_strip[curr_depth+1];

	u = &v->children[v->nchildren];
	u->nrules = select_rules(rules_parent, rules_child, v->nrules, cut);
	if (u->nrules == 0)
		return NULL;
	// TODO: delay node redundancy implementation
#if 0
	redund = check_node_redun(v);
	if (redund)
		return NULL;
#endif
	u->rules = malloc(u->nrules*sizeof(Rule *));
	for (i = 0; i < u->nrules; i++)
		u->rules[i] = v->rules[rule_map_c2p[i]];
	u->parent = v;
	u->depth = v->depth + 1;
	u->id = total_nodes;
	u->child_id = v->nchildren++;
	u->cut = *cut;
	u->type = u->nrules > LEAF_RULES ? NONLEAF : LEAF;

	u->nchildren = 0;
	u->children = malloc(MAX_CHILDREN * sizeof(Trie));

	if (total_nodes >= trie_nodes_size) {
		trie_nodes_size += NODES_CHUNK;
		trie_nodes = realloc(trie_nodes, trie_nodes_size*sizeof(Trie *));
	}
	trie_nodes[total_nodes++] = u;
	if (u->type == LEAF)
		leaf_nodes++;
	if (total_nodes > 3000000) {
		dump_path(u, 2);
		printf("Stop working: > 1000000 rules\n");
		exit(1);
	}

	return u;
}



void create_children(Trie *v)
{
	int		dim, val;
	Band	*cut;
	Trie	*u;

	if (curr_depth >= MAX_DEPTH-1) {
		dump_path(v, 2);
		printf("Stop working: > trie depth > %d, v[%d]#%d@%d\n", 
				MAX_DEPTH, v->id, v->nrules, v->depth);
		exit(1);
	}

	if (curr_depth > 0)	{
		for (dim = 0; dim < NFIELDS; dim++)
			dfs_uncuts[curr_depth][dim] = dfs_uncuts[curr_depth-1][dim];
	}

	choose_cut(v);
	cut = &dfs_cuts[v->depth];
	dfs_uncuts[v->depth][cut->dim]--;
	if (v->nrules <= REDUN_NRULES)
		calc_rule_redun(v, cut);

	for (val = 0; val < BAND_SIZE; val++) {
		cut->val = val;
		u = new_child(v, cut);
		if ((u != NULL) && (u->nrules > LEAF_RULES)) {
			curr_depth++;
			create_children(u);	
		} else {
			if (curr_depth > max_depth) {
				max_depth = curr_depth;
				if (u != NULL) {
					max_depth_leaf = u;
					if (max_depth == 18)
						dump_path(u, 2);
				}
			}
		}
	}
	v->children = realloc(v->children, v->nchildren*sizeof(Trie));
	dfs_uncuts[v->depth][cut->dim]++;
	curr_depth--;
}



Trie* init_trie(Rule *rules, int nrules)
{
	int		depth, i;
	Trie 	*node = calloc(1, sizeof(Trie));

	// init dfs related data structure
	dfs_uncuts[0][0] = dfs_uncuts[0][1] = 8;
	dfs_uncuts[0][2] = dfs_uncuts[0][3] = 4;
	dfs_uncuts[0][4] = 2;
	dfs_rules_strip[0] = malloc(nrules*sizeof(Rule));
	rule_map_c2p = malloc(nrules * sizeof(int));
	rule_map_p2c = malloc(nrules * sizeof(int));

	// create root node
	node->type = NONLEAF;
	node->id = total_nodes++;
	node->child_id = 0;
	node->nrules = nrules;
	node->rules = malloc(nrules*sizeof(Rule *));
	for (i = 0; i < nrules; i++) {
		node->rules[i] = &rules[i];
		dfs_rules_strip[0][i] = rules[i];
	}

	node->nchildren = 0;
	node->children = malloc(MAX_CHILDREN * sizeof(Trie));

	trie_nodes = (Trie **) malloc(NODES_CHUNK*sizeof(Trie *));
	trie_nodes[0] = node;
	trie_nodes_size = NODES_CHUNK;
	
	return node;
}



// trie construction with a depth-first traverse (dfs)
Trie* build_trie(Rule *rules, int nrules)
{
	Trie*	v;
	int		i;

	root_node = init_trie(rules, nrules);
	create_children(root_node);
/*
	for (i = 0; i < total_nodes; i++)
		dump_node(trie_nodes[i], 0);
*/
	printf("total nodes:%d, leaf nodes:%d, max depth:%d\n", total_nodes, leaf_nodes, max_depth+1);
}



void dump_rules(Rule **rules, int nrules)
{
	int			i;

	for (i = 0; i < nrules; i++) {
		printf("[%4d] ", rules[i]->id);
		dump_rule(rules[i]);
	}
}



void dump_node_rules(Trie *v)
{
	dump_rules(v->rules, v->nrules);
}



/*
void dump_path(Trie *v, int detail)
{
	while (v != root_node) {
		dump_node(v, detail);
		v = v->parent;
	}
}
*/



void dump_node(Trie *v, int detail)
{
	int			i;

	if (v->nrules > LEAF_RULES)
		printf("N");
	else
		printf("n");
	if (v->parent != NULL)
		printf("[%d#%d<-%d#%d]@%d/%d ", 
				v->id, v->nrules, v->parent->id, v->parent->nrules, v->depth, v->child_id);
	else
		printf("[%d#%d]@%d ", v->id, v->nrules, v->depth);

	if (v->id > 0) {
		printf("d%d-b%d-v%x", v->cut.dim, v->cut.bid, v->cut.val);
	}
	printf("  #N:%d\n", v->nchildren);

	if (detail == 1) {
		for (i = 0; i < v->nrules; i++)
			printf("R%d, ", v->rules[i]->id);
		printf("\n");
	} else if (detail == 2) {
		dump_node_rules(v);
	}

	if (detail > 1) {
		for (i = 0; i < v->nchildren; i++)
			printf("N%d, ", v->children[i].id);
		printf("\n\n");
	}
}



void dump_path(Trie *v, int detail)
{
	int		i;

	while (v->id != 0) {
		for (i = 0; i < NFIELDS; i++) {
			printf("%d-", dfs_uncuts[v->depth][i]);
		}
		printf("\n");
		if (detail >= 2) {
			dump_node(v, detail);
			for (i = 0; i < v->nrules; i++) {
				printf("[%4d] ", dfs_rules_strip[v->depth][i].id);
				dump_rule(&dfs_rules_strip[v->depth][i]);
			}
			printf("\n");
		} else
			dump_node(v, 0);
		v = v->parent;
	}
}
