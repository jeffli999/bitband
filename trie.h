#ifndef TRIE_H
#define TRIE_H

#include "bitband.h"
#include "rule.h"

#define MAX_CHILDREN	BAND_SIZE
#define	SMALL_NODE		16			// node is small with rules less than this
#define MAX_DEPTH		32


enum { LEAF, NONLEAF };

typedef struct trie_t	Trie;

struct trie_t {
	int			id;				// global id in the trie
	uint8_t		depth;
	int			child_id;		// id among its siblings of the same parent
	uint8_t		type;
	uint16_t	nequals;		// number of equal nodes pointing to me
	int			nrules;
	Rule**		rules;

	Trie*		parent;
	Band		cut;

	int			nchildren;
	Trie*		children;
};


Trie* build_trie(Rule *rules, int nrules, int leaf_rules);

void dump_trie(Trie *root, int detail);
void dump_node(Trie *v, int simple);
void dump_node_rules(Trie *v);
void dump_nodes(int max, int min);
void dump_rules(Rule **rules, int nrules);
void dump_path(Trie *v, int detail);


#endif
