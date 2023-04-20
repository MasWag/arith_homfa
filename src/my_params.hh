/**
 * @author Masaki Waga
 * @date 2023/04/08
 * @brief Custom parameters
 */

#pragma once

#include <cstdint>
#include <cmath>

namespace TFHEpp {

    struct lvl3param {
        static constexpr int32_t key_value_max = 1;
        static constexpr int32_t key_value_min = -1;
        static const std::uint32_t nbit = 13;  // dimension must be a power of 2 for
        // ease of polynomial multiplication.
        static constexpr std::uint32_t n = 1 << nbit;  // dimension
        static constexpr std::uint32_t k = 1;
        static constexpr std::uint32_t l = 4;
        static constexpr std::uint32_t Bgbit = 9;
        static constexpr std::uint32_t Bg = 1 << Bgbit;
        static const inline double α = std::pow(2.0, -47);  // fresh noise
        using T = uint64_t;                                 // Torus representation
        static constexpr T μ = 1ULL << 61;
        static constexpr uint32_t plain_modulus = 8;
        static constexpr double Δ = μ;
    };
}
