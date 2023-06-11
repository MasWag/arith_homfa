/*!
* @author Masaki Waga
* @date 2023/06/06.
*/

#pragma once

#include <ostream>
#include <utility>

#include <tfhe++.hpp>

#include "bootstrapping_key.hh"
#include "sigextraction.hh"

namespace ArithHomFA {

    /*!
     * @class Lvl3ToLvl1
     * @brief Convert TLWE ciphertexts from level 3 to level 1
     */
    class Lvl3ToLvl1 {
    private:
        //! @brief A BootstrappingKey to be used for conversion
        BootstrappingKey bk;
    public:
        static constexpr uint basebit = 4;
        static constexpr uint numdigits = 32 / basebit;
        /*!
         * @brief Constructor for Lvl3ToLvl1
         * @param bootKey A BootstrappingKey to be used for conversion
         */
        explicit Lvl3ToLvl1(BootstrappingKey bootKey) : bk(std::move(bootKey)) {}

        /*!
         * @brief Convert a TLWE ciphertext from level 3 to an array of TLWE ciphertexts at level 1
         * @param input The level 3 TLWE ciphertext
         * @param output The array of level 1 TLWE ciphertexts
         */
        void toLv1TLWE(const TFHEpp::TLWE<TFHEpp::lvl3param> &input,
                       std::array<TFHEpp::TLWE<TFHEpp::lvl1param>, numdigits> &output) const {
            TFHEpp::HomDecomp<typename BootstrappingKey::high2midP, typename BootstrappingKey::mid2lowP, typename BootstrappingKey::brP, basebit, numdigits>(
                    output, input, *bk.kskh2m, *bk.kskm2l, *bk.bkfft);
        }

        /*!
         * @brief Convert a TLWE ciphertext from level 3 to a single TLWE ciphertext at level 1
         * @param input The level 3 TLWE ciphertext
         * @param output The level 1 TLWE ciphertext
         */
        void toLv1TLWE(const TFHEpp::TLWE<TFHEpp::lvl3param> &input,
                       TFHEpp::TLWE<TFHEpp::lvl1param> &output) const {
            std::array<TFHEpp::TLWE<TFHEpp::lvl1param>, numdigits> output_array{};
            toLv1TLWE(input, output_array);
            output = output_array.back();
        }

        /*!
         * @brief Convert a TLWE ciphertext from level 3 to a single TLWE ciphertext at level 1
         * @param input The level 3 TLWE ciphertext
         * @param output The level 1 TLWE ciphertext after bootstrapping
         */
        void toLv1TLWEWithBoostrapping(const TFHEpp::TLWE<TFHEpp::lvl3param> &input,
                                       TFHEpp::TLWE<TFHEpp::lvl1param> &output) const {
            TFHEpp::TLWE<TFHEpp::lvl1param> tmp;
            toLv1TLWE(input, tmp);
            tmp[TFHEpp::lvl1param::k*TFHEpp::lvl1param::n] += 1ULL<<(32-5);
            TFHEpp::GateBootstrapping<TFHEpp::lvl10param, TFHEpp::lvl01param, TFHEpp::lvl1param::μ>(output, tmp,
                                                                                                    *bk.ekey);
        }
    };

} // namespace ArithHomFA
