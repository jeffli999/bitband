#include <stdio.h>
#include <stdlib.h>
#include "rule.h"



int loadrules(FILE *fp, Rule **ruleset)
{
	uint32_t 	sip1, sip2, sip3, sip4, sprefix;
	uint32_t 	dip1, dip2, dip3, dip4, dprefix;
	uint32_t 	sp1, sp2;
	uint32_t 	dp1, dp2;
	uint32_t 	prot, prot_prefix;
	uint32_t	ip, mask;
	Rule		*rule, *rules;
	int 		num_rules = 0, ruleset_size = 1024;//number of rules

	rules = (Rule *) malloc(ruleset_size * sizeof(Rule));

	while (1) {
		rule = &(rules[num_rules]);
		if (fscanf(fp,"@%d.%d.%d.%d/%d\t%d.%d.%d.%d/%d\t%d : %d\t%d : %d\t%x/%x\n", 
					&sip1, &sip2, &sip3, &sip4, &sprefix,
					&dip1, &dip2, &dip3, &dip4, &dprefix, 
					&(rule->field[2].lo), &(rule->field[2].hi),
					&(rule->field[3].lo), &(rule->field[3].hi),
					&prot, &prot_prefix) != 16) 
			break;
//printf("num_rules:%d\n", num_rules);		
		rule->id = num_rules;
		// source ip
		if (sprefix == 0) {
			rule->field[0].lo = 0;
			rule->field[0].hi = 0xffffffff;
		} else if (sprefix == 32) {
			ip = (sip1 << 24) + (sip2 << 16) + (sip3 << 8) + sip4;
			rule->field[0].lo = rule->field[0].hi = ip;
		} else {
			mask = ~((1 << (32 - sprefix)) - 1);
			ip = (sip1 << 24) + (sip2 << 16) + (sip3 << 8) + sip4;
			rule->field[0].lo = ip & mask;
			rule->field[0].hi = rule->field[0].lo + ~mask;
		}

		// dest ip
		if (dprefix == 0) {
			rule->field[1].lo = 0;
			rule->field[1].hi = 0xffffffff;
		} else if (dprefix == 32) {
			ip = (dip1 << 24) + (dip2 << 16) + (dip3 << 8) + dip4;
			rule->field[1].lo = rule->field[1].hi = ip;
		} else {
			mask = ~((1 << (32 - dprefix)) - 1);
			ip = (dip1 << 24) + (dip2 << 16) + (dip3 << 8) + dip4;
			rule->field[1].lo = ip & mask;
			rule->field[1].hi = rule->field[1].lo + ~mask;
		}

		// protocol
		if(prot_prefix == 0xff){
			rule->field[4].lo = prot;
			rule->field[4].hi = prot;
		}else if(prot_prefix== 0){
			rule->field[4].lo = 0;
			rule->field[4].hi = 0xff;
		}else{
			printf("Protocol mask error\n");
			return 0;
		}

		num_rules++;
		if (num_rules >= ruleset_size) {
			ruleset_size <<= 1;
			rules = (Rule *) realloc(rules, ruleset_size * sizeof(Rule));
		}
	}

	*ruleset = (Rule *) realloc(rules, num_rules * sizeof(Rule));
	return num_rules;
}



// dump rules in classbench format
void dump_rule(Rule *rule)
{
	int			i;
	Range		r;

	printf("@");
	// dump SIP & DIP
	for (i = 0; i < 2; i++) {
		r = rule->field[i];
		dump_ip_hex(r.lo);
		//dump_ip(r.lo);
		if (r.hi - r.lo == 0xffffffff)
			printf("/00  ");
		else
			printf("/%02d  ", 32-MSB(r.hi - r.lo + 1));
	}

	// dump SP & DP
	for (i = 2; i < 4; i++) {
		r = rule->field[i];
		printf("%04x : %04x  ", r.lo, r.hi);
		printf(" ");
	}

	// dump proto
	r = rule->field[4];
	if (r.lo == 0 && r.hi == 0xff)
		printf("00/00");
	else
		printf("%02x/FF", r.lo);

	printf("\n");
}



// dump rules in my format
void my_dump_rule(Rule *rule)
{
	int			i;
	Range		r;

	printf("rule[%4d]:\t", rule->id);

	// dump SIP & DIP
	for (i = 0; i < 2; i++) {
		r = rule->field[i];
		if (r.lo == r.hi) 
			dump_ip(r.lo);
		else if (r.lo == 0 && r.hi == 0xffffffff)
			printf("*");
		else {
			dump_ip(r.lo);
			printf("/%d", 31-MSB(r.hi-r.lo+1));
		}
		printf(",  ");
	}

	// dump SP & DP
	for (i = 2; i < 4; i++) {
		r = rule->field[i];
		if (r.lo == r.hi) 
			printf("%x", r.lo);
		else if (r.lo == 0 && r.hi == 0xffff)
			printf("*");
		else {
			printf("[%x-%x]", r.lo, r.hi);
		}
		printf(",  ");
	}

	// dump proto
	r = rule->field[4];
	if (r.lo == r.hi)
		printf("%d", r.lo);
	else if (r.lo == 0 && r.hi == 0xff)
		printf("*");
	else
		printf("[%d-%d]", r.lo, r.hi);

	printf("\n");
}



void dump_ruleset(Rule *rules, int num_rules)
{
	int			i;

	for (i = 0; i < num_rules; i++)
		dump_rule(&(rules[i]));
}
