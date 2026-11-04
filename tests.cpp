#include "matrix.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "testdata/2048"
#endif

namespace {
constexpr std::size_t large_size = 2048;
static_assert(CHAR_BIT == 8 && sizeof(double) == 8 &&
              std::numeric_limits<double>::is_iec559,
              "Fixtures require IEEE-754 binary64 doubles and 8-bit bytes");

std::uint64_t bits(double value) {
    std::uint64_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

void require_bit_equal(const Matrix& actual, const Matrix& expected,
                       std::size_t size, const std::string& label) {
    if (actual.size() != expected.size()) {
        throw std::runtime_error(label + ": incorrect result size");
    }
    if (std::memcmp(actual.data(), expected.data(), actual.size() * sizeof(double)) == 0)
        return;
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (bits(actual[index]) != bits(expected[index])) {
            std::ostringstream message;
            message << label << ": bit mismatch at (" << index / size << ", "
                    << index % size << "), expected 0x" << std::hex
                    << std::setfill('0') << std::setw(16) << bits(expected[index])
                    << ", got 0x" << std::setw(16) << bits(actual[index]);
            throw std::runtime_error(message.str());
        }
    }
}

Matrix load_fixture(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open fixture: " + path.string());
    Matrix matrix(large_size * large_size);
    const auto bytes = static_cast<std::streamsize>(matrix.size() * sizeof(double));
    if (!input.read(reinterpret_cast<char*>(matrix.data()), bytes)) {
        throw std::runtime_error("Truncated fixture: " + path.string());
    }
    if (input.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("Unexpected trailing fixture data: " + path.string());
    }
    // The files are little-endian; retain the same bit patterns on big-endian hosts.
    const std::uint16_t endian_probe = 1;
    if (*reinterpret_cast<const unsigned char*>(&endian_probe) != 1) {
        for (double& value : matrix) {
            auto* first = reinterpret_cast<unsigned char*>(&value);
            std::reverse(first, first + sizeof(double));
        }
    }
    return matrix;
}

void small_tests() {
    const Matrix a{1, 2, 3, 4};
    const Matrix b{5, 6, 7, 8};
    const Matrix expected{19, 22, 43, 50};
    for (auto threads : {1u, 2u, 8u}) {
        require_bit_equal(multiply_matrices(a, b, 2, threads), expected, 2,
                          "2x2, threads=" + std::to_string(threads));
    }
    const auto random_a = generate_matrix(7);
    const auto random_b = generate_matrix(7);
    require_bit_equal(multiply_matrices(random_a, random_b, 7, 1),
                      multiply_matrices(random_a, random_b, 7, 3), 7,
                      "7x7, thread consistency");
    try {
        multiply_matrices(a, b, 2, 0);
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("Zero threads must throw invalid_argument");
}

void large_tests(const std::filesystem::path& directory) {
    // Load all three frozen matrices. Never compute the oracle during a test run.
    const auto a = load_fixture(directory / "a.bin");
    const auto b = load_fixture(directory / "b.bin");
    const auto expected = load_fixture(directory / "expected.bin");
    // Three workers exercise uneven row partitions and scalar remainder rows.
    for (auto threads : {1u, 3u, 8u}) {
        const auto label = "2048x2048, threads=" + std::to_string(threads);
        require_bit_equal(multiply_matrices(a, b, large_size, threads),
                          expected, large_size, label);
        std::cout << label << ": bit-exact match\n" << std::flush;
    }
}
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: matrix_tests [fixture-directory]\n";
        return 1;
    }
    try {
        small_tests();
        large_tests(argc == 2 ? std::filesystem::path(argv[1]) :
                               std::filesystem::u8path(TEST_DATA_DIR));
        std::cout << "Tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Tests failed: " << error.what() << '\n';
        return 1;
    }
}
