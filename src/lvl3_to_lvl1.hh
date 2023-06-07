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
        static constexpr uint basebit = 4;
    public:
        /*!
         * @brief Constructor for Lvl3ToLvl1
         * @param bootKey A BootstrappingKey to be used for conversion
         */
        explicit Lvl3ToLvl1(BootstrappingKey bootKey) : bk(std::move(bootKey)) {}

        /*!
         * @brief Convert a TLWE ciphertext from level 3 to an array of TLWE ciphertexts at level 1
         * @tparam numdigits The size of the output array
         * @param input The level 3 TLWE ciphertext
         * @param output The array of level 1 TLWE ciphertexts
         */
        template<uint numdigits>
        void toLv1TLWE(const TFHEpp::TLWE<TFHEpp::lvl3param> &input,
                       std::array<TFHEpp::TLWE<typename BootstrappingKey::brP::targetP>, numdigits> &output) const {
            TFHEpp::HomDecomp<typename BootstrappingKey::high2midP, typename BootstrappingKey::mid2lowP, typename BootstrappingKey::brP, basebit, numdigits>(
                    output, input, *bk.kskh2m, *bk.kskm2l, *bk.bkfft);
        }

        /*!
         * @brief Convert a TLWE ciphertext from level 3 to a single TLWE ciphertext at level 1
         * @param input The level 3 TLWE ciphertext
         * @param output The level 1 TLWE ciphertext
         */
        void toLv1TLWE(const TFHEpp::TLWE<TFHEpp::lvl3param> &input,
                       TFHEpp::TLWE<typename BootstrappingKey::brP::targetP> &output) const {
            std::array<TFHEpp::TLWE<typename BootstrappingKey::brP::targetP>, 1> output_array{};
            toLv1TLWE<1>(input, output_array);
            output = output_array[0];
        }
    };

} // namespace ArithHomFA
