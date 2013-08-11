#include <stdlib.h>
#include "poset.h"


// ============================================================================
// section for queue related operations
// ============================================================================

void	**queue;
int		qhead, qtail, qsize = 102400;


void init_queue(int size)
{
	queue = calloc(qsize, size);
}


inline
void reset_queue()
{
	qhead = qtail = 0;
}



inline
int queue_empty()
{
	return (qhead == qtail);
}



inline
void enqueue(void* node)
{
	queue[qhead++] = node;
}


inline
void* dequeue()
{
	return queue[qtail++];
}



// ============================================================================
// section for field poset
// ============================================================================



FieldNode	field_top[NFIELDS], field_bottom[NFIELDS];
FieldNode	**field_set, **tmp_fields;
FieldNode	**field_nodes[NFIELDS];		// sorted in node levels
int			nfield_nodes[NFIELDS];



void poset_field_init(int nrules)
{
	int			dim;
	FNodeLink	*p;

	for (dim = 0; dim < NFIELDS; dim++) {
		field_top[dim].dim = dim;
		field_bottom[dim].dim = dim;

		p = calloc(1, sizeof(FNodeLink));
		p->src = &field_bottom[dim];
		p->dst = &field_top[dim];
		p->next_in = p->next_out = NULL;

		field_top[dim].out = NULL;
		field_top[dim].in = p;
		field_bottom[dim].out = p;
		field_bottom[dim].in = NULL;

		field_nodes[dim] = malloc(nrules*sizeof(FieldNode *));
	}

	field_set = malloc(nrules*sizeof(FieldNode*));
	tmp_fields = malloc(nrules*sizeof(FieldNode*));
}



void poset_init(int nrules)
{
	poset_field_init(nrules);
}



FNodeLink* new_field_edge(FieldNode *src, FieldNode *dst)
{
	FNodeLink	*e = malloc(sizeof(FNodeLink));

	e->src = src; e->dst = dst;
	e->prev_out = NULL;
	e->next_out = src->out;
	if (src->out)
		src->out->prev_out = e;
	src->out = e;

	e->prev_in = NULL;
	e->next_in = dst->in;
	if (dst->in)
		dst->in->prev_in = e;
	dst->in = e;

	src->num_out++;
	dst->num_in++;
}



FNodeLink* del_field_edge(FNodeLink *e)
{
	FieldNode	*src, *dst;

	src = e->src; dst = e->dst;
	if (e->prev_out != NULL)
		e->prev_out->next_out = e->next_out;
	else
		src->out = e->next_out;
	if (e->next_out != NULL)
		e->next_out->prev_out = e->prev_out;

	if (e->prev_in != NULL)
		e->prev_in->next_in = e->next_in;
	else
		dst->in = e->next_in;
	if (e->next_in != NULL)
		e->next_in->prev_in = e->prev_in;
	src->num_out--;
	dst->num_in--;
}



// return the number of fields covering me (return 1 in case of finding me)
int	collect_field_covers(Rule *rule, int dim, FieldNode **covered, int ncovered, FieldNode **covers)
{
	Range		r0 = rule->field[dim];
	FieldNode	*v, *u;
	FNodeLink	*e;
	int			lowest, num_covers = 0, i;
	uint32_t	flag = (uint32_t)&rule->field[dim];
	
	reset_queue();
	for (i = 0; i < ncovered; i++)
		enqueue(covered[i]);

	while (!queue_empty()) {
		v = dequeue();
		lowest = 1;
		for (e = v->out; e != NULL; e = e->next_out) {
			u = e->dst;
			if (u->flag == flag || u == &field_top[dim])
				continue;

			u->flag = flag;
			if (range_cover(u->val, r0))
				covers[num_covers++] = u;
			else
				enqueue(u);
		}
	}
	if (num_covers == 0)
		covers[num_covers++] = &field_top[dim];

	return num_covers;
}



// return the number of fields covered by me (return 1 in case of finding me)
int collect_field_covered(Rule *rule, int dim, FieldNode **covered)
{
	Range		r0 = rule->field[dim];
	FieldNode	*v, *u;
	FNodeLink	*e;
	int			highest, num_covered = 0;
	uint32_t	flag = (uint32_t)&rule->field[dim];
	
	reset_queue();
	enqueue(&field_bottom[dim]);
	while (!queue_empty()) {
		v = dequeue();
		highest = 1;
		for (e = v->out; e != NULL; e = e->next_out) {
			u = e->dst;
			if (u == &field_top[dim])
				continue;

			if (range_equal(u->val, r0)) {
				covered[0] = u;
				return 1;
			}
			if (range_cover(r0, u->val)) {
				highest = 0;	// v is not the highest cover because of u
				if (u->flag != flag) {
					enqueue(u);
					u->flag = flag;
				}
			}
		}
		if (highest)
			covered[num_covered++] = v;
	}
	return num_covered;
}



// check whether there is a direct edge from node src to dst, and return the edge if found
FNodeLink* check_edge(FieldNode *src, FieldNode *dst)
{
	FNodeLink	*e;

	if (src->num_out < dst->num_in) {
		for (e = src->out; e != NULL; e = e->next_out) {
			if (e->dst == dst)
				break;
		}
	} else {
		for (e = dst->in; e != NULL; e = e->next_in) {
			if (e->src == src)
				break;
		}
	}
	return e;
}



int sift_field_covers(FieldNode **covers, int ncovers)
{
	int		i, j;
	Range	r0, r1;

	for (i = 0; i < ncovers-1; i++) {
		r0 = covers[i]->val;
		j = i + 1;
		while (j < ncovers) {
			r1 = covers[j]->val;
			if (range_cover(r1, r0)) {
				covers[j] = covers[ncovers-1];
				ncovers--;
			} else if (range_cover(r0, r1)) {
				covers[i] = covers[j];
				covers[j] = covers[ncovers-1];
				ncovers--;
				j = i + 1;
			} else {
				j++;
			}
		}
	}
	return ncovers;
}



// Given a field in rule, collect its covers and covered ones in the poset, then remove any direct
// edges between the two set to get rid of redundancy, and finally build edges from covers to this
// field, and edges from this field to covered ones. This is not an efficient implementation, 
// but with clear logic flow to serve as a baseline
FieldNode update_field_poset(Rule *rule, int dim)
{
	int			ncovered, ncovers, i, j;
	FieldNode	*v, *u, *w;
	FNodeLink	*e, *e1;
	uint32_t	flag = (uint32_t)&rule->field[dim];

	ncovered = collect_field_covered(rule, dim, field_set);

	// do nothing if rule field is already in poset
	if (ncovered == 1) {
		if (range_equal(rule->field[dim], field_set[0]->val)) {
			field_set[0]->count++;
			return;
		}
	}

	ncovers = collect_field_covers(rule, dim, field_set, ncovered, tmp_fields);
	ncovers = sift_field_covers(tmp_fields, ncovers);

	w = calloc(1, sizeof(FieldNode));
	w->dim = dim;
	w->count = 1;
	w->val = rule->field[dim];

	for (i = 0; i < ncovered; i++) {
		for (j = 0; j < ncovers; j++) {
			e = check_edge(field_set[i], tmp_fields[j]);
			if (e != NULL)
				del_field_edge(e);
		}
	}
	for (i = 0; i < ncovered; i++)
		new_field_edge(field_set[i], w);
	for (i = 0; i < ncovers; i++)
		new_field_edge(w, tmp_fields[i]);
}



int max_field_level(FieldNode *v)
{
	FNodeLink	*e;
	int			level, max_level = 0;

	if (v->in == NULL)
		return 0;
	for (e = v->in; e != NULL; e = e->next_in) {
		level = max_field_level(e->src);
		if (level > max_level)
			max_level = level;
	}
	v->level = max_level + 1;
	return v->level;
}



void label_field_nodes(int dim)
{
	FNodeLink	*e;
	int			level, max_level = 0;

	field_bottom[dim].level = 0;
	for (e = field_top[dim].in; e != NULL; e = e->next_in) {
		level = max_field_level(e->src);
		if (level > max_level)
			max_level = level;
	}
	field_top[dim].level = max_level + 1;
}



void insert_field_node(FieldNode *v, FieldNode **array, int num)
{
	int		i;

	for (i = num-1; i >= 0; i--) {
		if (v->level >= array[i]->level)
			break;
		array[i+1] = array[i];
	}
	array[i+1] = v;
}



void sort_field_nodes(int dim)
{
	FieldNode	*v, *u;
	FNodeLink	*e;
	int			level, flag, i;

	reset_queue();
	enqueue(&field_bottom[dim]);

	flag = (uint32_t)field_nodes[dim];
	while (!queue_empty()) {
		v = dequeue();
		for (e = v->out; e != NULL; e = e->next_out) {
			u = e->dst;
			if (u->flag == flag)
				continue;
			if (u == &field_top[dim])
				continue;
			insert_field_node(u, field_nodes[dim], nfield_nodes[dim]);
			nfield_nodes[dim]++;
			u->flag = flag;
			enqueue(u);
		}
	}

	for (i = 0; i < nfield_nodes[dim]; i++) {
		field_nodes[dim][i]->id = i;
	}
}



void dump_field_nodes(int dim)
{
	FieldNode	*v, *u;
	FNodeLink	*e;
	int			i, level = 0;

	for (i = 0; i < nfield_nodes[dim]; i++) {
		v = field_nodes[dim][i];
		if (v->level > level) {
			level = v->level;
			printf("\nLevel[%d]\n", level);
		}

		printf("[%4d] ", v->id);
		if (v->dim < 2) {
			dump_ip_hex(v->val.lo);
			if (v->val.hi - v->val.lo == 0xffffffff)
				printf("/00");
			else
				printf("/%02d", 32-MSB(v->val.hi - v->val.lo + 1));
		} else if (v->dim < 4) {
			printf("%04x : %04x", v->val.lo, v->val.hi);
		} else {
			if (v->val.lo == 0 && v->val.hi == 0xff)
				printf("00/00");
			else
				printf("%02x/FF", v->val.lo);
		}

		printf(": ");
		for (e = v->out; e != NULL; e = e->next_out) {
			printf("%4d, ", e->dst->id);
		}
		printf("\n");

	}
	printf("\n");
}



void build_field_poset(Rule *rules, int nrules)
{
	int		dim, i;

	poset_init(nrules);
	qsize = nrules;
	init_queue(sizeof(FieldNode*));

	for (i = 0; i < nrules; i++) {
		for (dim = 0; dim < NFIELDS; dim++)
			update_field_poset(&rules[i], dim);
	}
	for (dim = 0; dim < NFIELDS; dim++) {
		label_field_nodes(dim);
		sort_field_nodes(dim);
		printf("Field Nodes[%d]\n", dim);
		printf("==============================================================\n");
		dump_field_nodes(dim);
	}
}



#if 0
void dump_field_poset(FieldNode *bottom, FieldNode *top)
{
	FieldNode	*v, *u;
	FNodeLink	*e;
	int			level = 0, empty_level = 0;

	reset_queue();
	enqueue(bottom);

	while (!queue_empty()) {
		v = dequeue();
		empty_level = 1;
		for (e = v->out; e != NULL; e = e->next_out) {
			u = e->dst;
			if (u == top)
				continue;
			if (u->flag == (uint32_t)bottom)
				continue;

			if (u->dim < 2) {
				dump_ip_hex(u->val.lo);
				if (u->val.hi - u->val.lo == 0xffffffff)
					printf("/00");
				else
					printf("/%02d", 32-MSB(u->val.hi - u->val.lo + 1));
			} else if (u->dim < 4) {
				printf("%04x : %04x", u->val.lo, u->val.hi);
			} else {
				if (u->val.lo == 0 && u->val.hi == 0xff)
					printf("00/00");
				else
					printf("%02x/FF", u->val.lo);
			}
			printf("[%d]\n", u->level);
			u->flag = (uint32_t)bottom;
			enqueue(u);
		}
	}
	printf("\n");
}
#endif
