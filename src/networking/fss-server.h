#ifndef FSS_SERVER_H
#define FSS_SERVER_H

#include "fss-common.h"
#include <cmath>

// Initializes server with information from the client, namely aes_keys for PRF and numBits in input domain
void initializeServer(Fss* fServer, Fss* fClient);

// Runs point(delta) FSS given key on input x for 2 parties/providers
mpz_class evaluateEq(Fss* f, ServerKeyEq *k, uint64_t x);

// Runs interval(step) FSS given key on input x for 2 parties/providers
uint64_t evaluateLt(Fss* f, ServerKeyLt* k, uint64_t x);

// Runs point(delta) FSS given key on input x for 3 or more parties/providers
uint32_t evaluateEqMParty(Fss *f, MPKey* key, uint32_t x);


uint64_t evaluateReLu(Fss* f, ServerKeyReLU* k0_0, ServerKeyReLU* k1_0,ServerKeyReLU* k0_1, ServerKeyReLU* k1_1, uint64_t x);
//uint64_t evaluateReLuClient(Fss* f, ServerKeyReLU* k0_0, ServerKeyReLU* k0_1, uint64_t x0, int sock);
//uint64_t evaluateReLuServer(Fss* f, ServerKeyReLU* k1_0, ServerKeyReLU* k1_1, uint64_t x1, int sock);
#endif
