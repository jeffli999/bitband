#include <stdio.h>
#include "common.h"


inline
int range_overlap(Range a, Range b)
{
	if (a.lo <= b.lo)
		return (a.hi >= b.lo);
	else
		return (b.hi >= a.lo);
}



// check whether a covers b, but not vice versa
inline
int range_cover(Range a, Range b)
{
	return ((a.lo <= b.lo) && (a.hi >= b.hi));
}


// return intersection of the two ranges
inline
Range range_sect(Range a, Range b)
{
	a.lo = (a.lo < b.lo) ? b.lo : a.lo;
	a.hi = (a.hi > b.hi) ? b.hi : a.hi;

	return a;
}



inline
uint32_t extract_bits(uint32_t a, int hi, int lo)
{
	a <<= BITS - hi - 1;
	a >>= BITS - (hi - lo) - 1;
	return a;
}



// clear bits with 0
inline
void clear_bits(uint32_t *a, int hi, int lo)
{
	int			len = hi - lo + 1;
	uint32_t	mask;

	mask = ((1 << len) - 1) << lo;
	*a = *a & (~mask);
}



// fill bits with 1
inline
void fill_bits(uint32_t *a, int hi, int lo)
{
	int			len = hi - lo + 1;
	uint32_t	mask;

	mask = ((1 << len) - 1) << lo;
	*a = *a | mask;
}



inline
void keep_bits(uint32_t *a, int hi, int lo)
{
	int			len = hi - lo + 1;
	uint32_t	mask;

	mask = ((1 << len) - 1) << lo;
	*a = *a & mask;
}



inline
void set_bits(uint32_t *a, int hi, int lo, uint32_t bits)
{
	uint32_t	mask;

	clear_bits(a, hi, lo);
	bits = bits << lo;
	*a = *a | bits;
}



inline
int all_one(uint32_t a, int hi, int lo)
{
	uint32_t	b;

	a = extract_bits(a, hi, lo);
	b = (1 << (hi - lo + 1)) - 1;
	return a == b ? 1 : 0;
}



// check whether a range is some prefix form
inline
int prefix_range(Range a)
{
	uint32_t	k;

	k = a.hi - a.lo;
	k = k & (k+1);
	if (k == 0)
		return 1;
	else
		return 0;
}



// Most Significant Bit
unsigned int MSB(unsigned int n)
{
	int     k = 0;

	while (n >>= 1)
		k++;
	return  k;
}


void dump_ip(unsigned int ip)
{
	printf("%d.", extract_bits(ip, 31, 24));
	printf("%d.", extract_bits(ip, 23, 16));
	printf("%d.", extract_bits(ip, 15, 8));
	printf("%d", extract_bits(ip, 7, 0));
}



void dump_ip_hex(unsigned int ip)
{
	printf("%02x.", extract_bits(ip, 31, 24));
	printf("%02x.", extract_bits(ip, 23, 16));
	printf("%02x.", extract_bits(ip, 15, 8));
	printf("%02x", extract_bits(ip, 7, 0));
}
