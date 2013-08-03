#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "bitband.h"

int field_bands[NFIELDS] = {8, 8, 4, 4, 2};


// ===========================================
// operations on bit-band in ternary bits
// ===========================================

// return the least significant bit position of a band
inline
int band_lsb(int band_id)
{
	return band_id * BAND_BITS;
}



// return the most significant bit position of bands[i] in tbits
inline
int band_msb(int band_id)
{
	return (band_id + 1) * BAND_BITS - 1;
}



// strip off some bits of a rule field according to the cut, return 0 when fail
int range_strip(Range *range, int bid, uint32_t val)
{
	int			lo, hi;
	uint32_t	bits, n, msb, lsb;

	lo = band_lsb(bid); hi = band_msb(bid);

	// compute range.lo
	bits = extract_bits(range->lo, hi, lo);
	if (bits > val) {
		if (hi == 31)
			return 0;	// no overlap with the cut space
		n = range->lo + (1 << (hi+1));
		if (n < range->lo)
			return 0;	// overflow, no overlap
		range->lo = n;
	}
	if (lo == 0)
		lsb = 0;
	else if (bits != val)
		lsb = 0;
	else
		lsb = extract_bits(range->lo, lo-1, 0);
	if (hi == 31)
		msb = 0;
	else
		msb = (range->lo >> (hi+1)) << lo;
	range->lo = msb + lsb;

	// compute range.hi
	bits = extract_bits(range->hi, hi, lo);
	if (bits < val) {
		if (hi == 31)
			return 0;	// no overlap with the cut space
		if ((range->hi >> (hi+1)) == 0)
			return 0;	// underflow, no overlap
		range->hi = range->hi - (1 <<(hi+1));
	}
	if (lo == 0)
		lsb = 0;
	else if (bits != val)
		lsb = (1 << lo) - 1;
	else
		lsb = extract_bits(range->hi, lo-1, 0);
	if (hi == 31)
		msb = 0;
	else
		msb = (range->hi >> (hi+1)) << lo;
	range->hi =  msb + lsb;

	if (range->lo > range->hi)
		return 0;
	else
		return 1;
}
