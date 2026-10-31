#include "matrix.h"

#include <limits>
#include <random>
#include <stdexcept>

Matrix generate_matrix(std::size_t size) {
    if (size == 0 || size > std::numeric_limits<std::size_t>::max() / size ||
        size * size > Matrix().max_size()) {
        throw std::invalid_argument("Invalid matrix size");
    }
    Matrix result(size * size);
    std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> distribution(-10.0, 10.0);
    for (double& value : result) value = distribution(engine);
    return result;
}
