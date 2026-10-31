#pragma once

#include <cstddef>
#include <vector>

using Matrix = std::vector<double>;

// Matrices use row-major storage: element (i, j) is matrix[i * size + j].
Matrix generate_matrix(std::size_t size);
Matrix multiply_matrices(const Matrix& a, const Matrix& b,
                         std::size_t size, std::size_t threads);
