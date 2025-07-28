// By sakara
/// @file

#ifndef BIOAUTH_SPDZ2KSHARE_H
#define BIOAUTH_SPDZ2KSHARE_H

#include <cstddef>
#include <vector>
#include <algorithm>
#include <execution>

#include "Mod2PowN.h"


namespace bioauth{


template <std::size_t K, std::size_t S>
class Spdz2kShare {
    static_assert(K <= 128, "K should be less than or equal to 128");
    static_assert(S <= 128, "S should be less than or equal to 128");

public:
    // We don't declare any data members here, instead the values are stored in gates
    using KType = Mod2PowN_t<K>; // integer over Z_{2^K}
    using SType = Mod2PowN_t<S>; // integer over Z_{2^S}
    using KSType = Mod2PowN_t<K + S>; // integer over Z_{2^{K+S}}

    using ClearType = KType; // The clear value
    using SemiShrType = KSType; // The share of the values held by the each party (without MAC)
    using KeyShrType = SType; // The share of the MAC key held by the each party
    using GlobalKeyType = KSType; // The global key used for MAC

    constexpr static std::size_t kBits = K;
    constexpr static std::size_t sBits = S;

    static SemiShrType RemoveUpperBits(SemiShrType value);
    static std::vector<SemiShrType> RemoveUpperBits(const std::vector<SemiShrType>& values);
    static void RemoveUpperBitsInplace(std::vector<SemiShrType>& values);
};

using Spdz2kShare32 = Spdz2kShare<32, 32>;
using Spdz2kShare64 = Spdz2kShare<64, 64>;
using Spdz2kShare128 = Spdz2kShare<128, 128>;



template <std::size_t K, std::size_t S>
typename Spdz2kShare<K, S>::SemiShrType Spdz2kShare<K, S>::
RemoveUpperBits(SemiShrType value) {
    if constexpr (S > 0) {
        return (value << S) >> S;
    } else {
        return value;
    }
}


template <std::size_t K, std::size_t S>
std::vector<typename Spdz2kShare<K, S>::SemiShrType> Spdz2kShare<K, S>::
RemoveUpperBits(const std::vector<SemiShrType>& values) {
    std::vector<SemiShrType> ret(values.size());

#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(values.begin(), values.end(), ret.begin(),
                   [](SemiShrType value) { return RemoveUpperBits(value); });
#else
    std::transform(std::execution::par_unseq,
                   values.begin(), values.end(), ret.begin(),
                   [](SemiShrType value) { return RemoveUpperBits(value); });
#endif

    return ret;
}


template <std::size_t K, std::size_t S>
void Spdz2kShare<K, S>::
RemoveUpperBitsInplace(std::vector<SemiShrType>& values) {
#ifdef _LIBCPP_HAS_NO_INCOMPLETE_PSTL
    std::transform(values.begin(), values.end(), values.begin(),
                   [](SemiShrType value) { return RemoveUpperBits(value); });
#else
    std::transform(std::execution::par_unseq,
                   values.begin(), values.end(), values.begin(),
                   [](SemiShrType value) { return RemoveUpperBits(value); });
#endif
}


} // namespace bioauth

#endif //BIOAUTH_SPDZ2KSHARE_H
