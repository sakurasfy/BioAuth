// by sakara

#ifndef BIOAUTH_MOD2POWN_H
#define BIOAUTH_MOD2POWN_H

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace  bioauth {

/// Represent an integer modulo 2^N, N should be less than or equal to 128
template <std::size_t N>
using Mod2PowN_t =
std::conditional_t<N <= 32,
                   uint32_t,
                   std::conditional_t<N <= 64,
                                      uint64_t,
                                      __uint128_t>>;

} // namespace bioauth
#endif //BIOAUTH_MOD2POWN_H
