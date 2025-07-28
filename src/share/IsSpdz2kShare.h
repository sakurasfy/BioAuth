

#ifndef BIOAUTH_ISSPDZ2KSHARE_H
#define BIOAUTH_ISSPDZ2KSHARE_H

#include <type_traits>
#include <concepts>

#include "share/Spdz2kShare.h"


namespace bioauth {


template <typename>
struct is_spdz2k_share : std::false_type {};

template <std::size_t K, std::size_t S>
struct is_spdz2k_share<Spdz2kShare<K, S>> : std::true_type {};

/// Concept to check if a type is an instance of Spdz2kShare
template <typename T>
concept IsSpdz2kShare = is_spdz2k_share<T>::value;


} // namespace bioauth
#endif //BIOAUTH_ISSPDZ2KSHARE_H
