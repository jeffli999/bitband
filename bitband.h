#ifndef BIT_BAND_H
#define BIT_BAND_H

#include <stdint.h>
#include "common.h"
#include "rule.h"

#define BAND_BITS	4
#define BAND_SIZE	16	// power2 of BAND_BITS

#define MAX_BANDS	8	// max #bands among sip, dip, sp, dp, prot
#define	TOTAL_BANDS	26	// sum of bit-bands of a rule (sip + dip + sp + ...)
#define CUT_BANDS	2	// #bands at each tree node to partition the space

extern int field_bands[NFIELDS];


// bit-band data structure, keep it 16-bit for operation efficiency
typedef struct {
	unsigned int	dim : 4;	// dimension of the band
	unsigned int	bid : 4;	// band id in dim (7~0 for sip, 3~0 for sp)
	unsigned int	val : 8;	// e.g., 0b0110
} Band;


// Ternary bits in (BAND_BITS) bit-bands, e.g., 0010****1101****
typedef struct {
	uint8_t		dim;		// dimension of the ternary bit string
	uint8_t		nbands;		// reduandant to band_map, for efficiency purpose
	uint8_t		band_lo;	// lowest non-* band (MAX_BANDS when non-existant)
	uint8_t		band_hi;	// highest non-* band (0 when non-existant)
	uint8_t		bandmap[MAX_BANDS];	// 0 for * bands; 1 for others; band_map[0] for LSB
	uint32_t	val;		// bit values
} TBits;



int range_tbits_overlap(Range *range, TBits *tbits);

inline
int free_band(TBits *tbits, int curr_band);

inline
Range tbits_range(TBits *tbits);

void dump_tbits(TBits *tb);

#endif
