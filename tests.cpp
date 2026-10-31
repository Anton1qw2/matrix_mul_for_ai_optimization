#include "matrix.h"
#include <iostream>
#include <stdexcept>

int main() {
    const Matrix a{1, 2, 3, 4};
    const Matrix b{5, 6, 7, 8};
    const Matrix expected{19, 22, 43, 50};
    for (auto threads : {1u, 2u, 8u}) {
        if (multiply_matrices(a, b, 2, threads) != expected) return 1;
    }
    const auto random_a = generate_matrix(7);
    const auto random_b = generate_matrix(7);
    if (multiply_matrices(random_a, random_b, 7, 1) !=
        multiply_matrices(random_a, random_b, 7, 3)) return 2;
    try {
        multiply_matrices(a, b, 2, 0);
        return 3;
    } catch (const std::invalid_argument&) {}
    std::cout << "Tests passed\n";
}
