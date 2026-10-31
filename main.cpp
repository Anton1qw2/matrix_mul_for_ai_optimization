#include "matrix.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

static std::size_t positive_integer(const char* text) {
    const std::string input(text);
    std::size_t value = 0;
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || value == 0) {
        throw std::invalid_argument("Arguments must be positive integers");
    }
    return value;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: matrix_app <size> [threads]\n";
        return 0;
    }
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: matrix_app <size> [threads]\n";
        return 1;
    }
    try {
        const auto size = positive_integer(argv[1]);
        const auto requested = argc == 3 ? positive_integer(argv[2]) :
            std::max(1u, std::thread::hardware_concurrency());
        const auto threads = std::min(size, requested);
        const auto a = generate_matrix(size);
        const auto b = generate_matrix(size);
        const auto start = std::chrono::steady_clock::now();
        const auto result = multiply_matrices(a, b, size, threads);
        const auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        long double checksum = 0;
        for (double value : result) checksum += value;
        std::cout << "Size: " << size << 'x' << size
                  << "\nThreads: " << threads
                  << "\nMultiplication time: " << std::fixed << std::setprecision(6)
                  << elapsed << " s\nChecksum: " << checksum << '\n';
        if (size <= 8) {
            std::cout << "Result:\n";
            for (std::size_t i = 0; i < size; ++i) {
                for (std::size_t j = 0; j < size; ++j)
                    std::cout << std::setw(13) << result[i * size + j] << ' ';
                std::cout << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
