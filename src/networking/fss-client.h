#ifndef FSS_CLIENT_H
#define FSS_CLIENT_H


#include "fss-common.h"
#include <cmath>
#include <gmp.h>
#include <gmpxx.h>

// Initializes client. numBits is number of bits in input domain
void initializeClient(Fss* f, uint32_t numBits, uint32_t numParties);

// Creates keys for a function that evaluates to b when input x = a.
void generateTreeEq(Fss* f, ServerKeyEq* k0, ServerKeyEq* k1, uint64_t a_i, uint64_t b_i);

// Creates keys for a function that evaluates to b when input x < a.
void generateTreeLt(Fss* f, ServerKeyLt* k0_0, ServerKeyLt* k1_0,ServerKeyLt* k0_1,ServerKeyLt* k1_1, uint64_t a_i, uint64_t b_0,uint64_t b_1);

// Creates keys for a function that evalutes to b when input x < a for 3 or more parties
void generateTreeEqMParty(Fss* f, uint64_t a, uint64_t b, MPKey* keys);

void generateKeyReLu(Fss* f, ServerKeyReLU* k0_0, ServerKeyReLU* k1_0,ServerKeyReLU* k0_1, ServerKeyReLU* k1_1);
#endif

