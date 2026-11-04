// Offline fixture generator; deliberately independent of multiply_matrices.
// Build: g++ -std=c++17 -O3 -ffp-contract=off tools/generate_2048.cpp -o generate_2048
// Run:   ./generate_2048 <output-directory>
// Files contain 2048*2048 little-endian IEEE-754 binary64 values, row-major.
// xorshift64 uses shifts 13, 7, 17 and the two fixed seeds below. Mapping
// (state % 2001 - 1000) / 10.0 produces dense signed fractional input data.
// The oracle accumulates k in increasing order, rounding every multiplication
// and addition separately to binary64, starting from positive zero. Volatile
// intermediates prevent contraction and extended-precision accumulation.
// Never compile with fast-math or reassociation enabled.

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::size_t size = 2048;
constexpr std::uint64_t seed_a = UINT64_C(0x6a09e667f3bcc909);
constexpr std::uint64_t seed_b = UINT64_C(0xbb67ae8584caa73b);
using Matrix = std::vector<double>;

static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559 &&
              std::numeric_limits<double>::radix == 2 &&
              std::numeric_limits<double>::digits == 53,
              "Fixtures require IEEE-754 binary64 doubles");

Matrix make_input(std::uint64_t state) {
    Matrix values(size * size);
    for (double& value : values) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        value = (static_cast<int>(state % 2001) - 1000) / 10.0;
    }
    return values;
}

Matrix reference_product(const Matrix& a, const Matrix& b) {
    Matrix transposed(size * size);
    for (std::size_t k = 0; k < size; ++k)
        for (std::size_t j = 0; j < size; ++j)
            transposed[j * size + k] = b[k * size + j];

    Matrix result(size * size);
    for (std::size_t i = 0; i < size; ++i) {
        for (std::size_t j = 0; j < size; ++j) {
            volatile double sum = 0.0;
            for (std::size_t k = 0; k < size; ++k) {
                volatile double product = a[i * size + k] * transposed[j * size + k];
                sum = sum + product;
            }
            result[i * size + j] = sum;
        }
        if ((i + 1) % 256 == 0)
            std::cout << "Reference rows: " << i + 1 << '/' << size << std::endl;
    }
    return result;
}

void require_missing(const std::filesystem::path& path) {
    // Also reject dangling symbolic links rather than following them on write.
    if (std::filesystem::exists(path) || std::filesystem::is_symlink(path))
        throw std::runtime_error("Refusing to overwrite: " + path.string());
}

void write_matrix(const std::filesystem::path& path, const Matrix& values) {
    require_missing(path);
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot create: " + path.string());

    std::array<char, 65536> buffer{};
    std::size_t used = 0;
    for (double value : values) {
        std::uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        for (unsigned byte = 0; byte < 8; ++byte)
            buffer[used++] = static_cast<char>((bits >> (byte * 8)) & 0xff);
        if (used == buffer.size()) {
            output.write(buffer.data(), static_cast<std::streamsize>(used));
            used = 0;
        }
    }
    if (used) output.write(buffer.data(), static_cast<std::streamsize>(used));
    output.close();
    if (!output) throw std::runtime_error("Failed to write: " + path.string());
    std::cout << "Wrote " << path.string() << " (" << values.size() * 8
              << " bytes)" << std::endl;
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: generate_2048 <output-directory>\n";
        return 1;
    }
    try {
        const std::filesystem::path directory(argv[1]);
        std::filesystem::create_directories(directory);
        for (const char* name : {"a.bin", "b.bin", "expected.bin"})
            require_missing(directory / name);

        const auto a = make_input(seed_a);
        const auto b = make_input(seed_b);
        const auto expected = reference_product(a, b);
        write_matrix(directory / "a.bin", a);
        write_matrix(directory / "b.bin", b);
        write_matrix(directory / "expected.bin", expected);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
