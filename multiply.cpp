#include "matrix.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

Matrix multiply_matrices(const Matrix& a, const Matrix& b,
                         std::size_t size, std::size_t threads) {
    if (size == 0 || size > std::numeric_limits<std::size_t>::max() / size ||
        a.size() != size * size || b.size() != size * size || threads == 0) {
        throw std::invalid_argument("Invalid matrix dimensions or thread count");
    }
    threads = std::min(threads, size);
    Matrix result(size * size, 0.0);
    auto calculate = [&](std::size_t begin, std::size_t end) {
        // Each worker owns separate rows; shared inputs are read-only.
        for (std::size_t i = begin; i < end; ++i) {
            for (std::size_t k = 0; k < size; ++k) {
                const double value = a[i * size + k];
                for (std::size_t j = 0; j < size; ++j) {
                    result[i * size + j] += value * b[k * size + j];
                }
            }
        }
    };
    if (threads == 1) {
        calculate(0, size);
        return result;
    }
    std::vector<std::thread> workers;
    workers.reserve(threads);
    try {
        std::size_t begin = 0;
        for (std::size_t index = 0; index < threads; ++index) {
            const auto count = size / threads + (index < size % threads ? 1 : 0);
            workers.emplace_back(calculate, begin, begin + count);
            begin += count;
        }
    } catch (...) {
        for (auto& worker : workers) worker.join();
        throw;
    }
    for (auto& worker : workers) worker.join();
    return result;
}
