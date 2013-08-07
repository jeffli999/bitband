#include "poset.h"

FieldNode	field_top[NFIELD], field_bot[NFIELD];



void poset_field_init()
{
	int			dim;
	FNodeLink	*p;

	for (dim = 0; dim < NFIELDS; dim++) {
		field_top[dim].dim = dim;
		field_bot[dim].dim = dim;

		p = calloc(1, sizeof(FNodeLink));
		p->src = &field_top[dim];
		p->dst = &field_top[dim];
		p->next_in = p->next_out = NULL;

		field_top[dim].in = NULL;
		field_top[dim].out = p;
		field_bot[dim].in = p;
		field_bot[dim].out = NULL;
	}
}



void poset_init()
{
	poset_field_init();
}
