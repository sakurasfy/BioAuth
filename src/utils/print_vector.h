

#ifndef BIOAUTH_UTILS_PRINT_VECTOR_H
#define BIOAUTH_UTILS_PRINT_VECTOR_H


#include <iostream>
#include <vector>
#include <string>

namespace bioauth {

template <typename T>
void PrintVector(const std::vector<T>& vec) {
    for (const auto& elem : vec) {
        std::cout << elem << " ";
    }
    std::cout << '\n';
}


template <typename T>
std::string VectorToString(const std::vector<T>& vec) {
    std::string result;
    for (const auto& elem : vec) {
        result += std::to_string(elem) + " ";
    }
    return result;
}

} // namespace bioauth

#endif //BIOAUTH_UTILS_PRINT_VECTOR_H
