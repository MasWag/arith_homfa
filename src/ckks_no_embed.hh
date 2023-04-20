/**
 * @author Masaki Waga
 * @date 2023/04/17
 */

#pragma once

#include <seal/seal.h>

namespace ArithHomFA {
    /*!
     * @brief Simple encoding of a double value to a polynomial without embedding for CKKS
     */
    class CKKSNoEmbedEncoder {
    private:
        seal::CKKSEncoder encoder;
        seal::SEALContext context;
    public:
        explicit CKKSNoEmbedEncoder(const seal::SEALContext &context) : encoder(context), context(context) {}

        void encode(const double value, const double scale, seal::Plaintext &plain) const {
            encoder.encode(value, scale, plain);
        }

        void decode(const seal::Plaintext &plain, double &value) const {
            value = this->decode(plain);
        }

        /*
         * The source code is based on https://github.com/microsoft/SEAL/blob/206648d0e4634e5c61dcf9370676630268290b59/native/src/seal/ckks.h#L635
         */
        double decode(const seal::Plaintext &plain) const {
            // Verify parameters.
            if (!is_valid_for(plain, this->context)) {
                throw std::invalid_argument("plain is not valid for encryption parameters");
            }
            if (!plain.is_ntt_form()) {
                throw std::invalid_argument("plain is not in NTT form");
            }

            auto &context_data = *this->context.get_context_data(plain.parms_id());
            auto &parms = context_data.parms();
            std::size_t coeff_modulus_size = parms.coeff_modulus().size();
            std::size_t coeff_count = parms.poly_modulus_degree();
            std::size_t rns_poly_uint64_count = seal::util::mul_safe(coeff_count, coeff_modulus_size);

            auto ntt_tables = context_data.small_ntt_tables();

            // Check that scale is positive and not too large
            if (plain.scale() <= 0 ||
                (static_cast<int>(log2(plain.scale())) >= context_data.total_coeff_modulus_bit_count())) {
                throw std::invalid_argument("scale out of bounds");
            }

            auto decryption_modulus = context_data.total_coeff_modulus();
            // Probably, this is the threshold to check if the number is negative or not
            auto upper_half_threshold = context_data.upper_half_threshold();
            int logn = seal::util::get_power_of_two(coeff_count);

            // Quick sanity check
            if ((logn < 0) || (coeff_count < SEAL_POLY_MOD_DEGREE_MIN) || (coeff_count > SEAL_POLY_MOD_DEGREE_MAX)) {
                throw std::logic_error("invalid parameters");
            }

            double inv_scale = double(1.0) / plain.scale();

            // Create mutable copy of input
            auto plain_copy(seal::util::allocate_uint(rns_poly_uint64_count, seal::MemoryManager::GetPool()));
            seal::util::set_uint(plain.data(), rns_poly_uint64_count, plain_copy.get());

            // Transform each polynomial from NTT domain
            for (std::size_t i = 0; i < coeff_modulus_size; i++) {
                seal::util::inverse_ntt_negacyclic_harvey(plain_copy.get() + (i * coeff_count), ntt_tables[i]);
            }

            // CRT-compose the polynomial
            context_data.rns_tool()->base_q()->compose_array(plain_copy.get(), coeff_count,
                                                             seal::MemoryManager::GetPool());

            // Create floating-point representations of the multi-precision integer coefficients
            double two_pow_64 = std::pow(2.0, 64);
            auto res(seal::util::allocate<std::complex<double>>(coeff_count, seal::MemoryManager::GetPool()));
            for (std::size_t i = 0; i < coeff_count; i++) {
                res[i] = 0.0;
                if (seal::util::is_greater_than_or_equal_uint(
                        plain_copy.get() + (i * coeff_modulus_size), upper_half_threshold, coeff_modulus_size)) {
                    double scaled_two_pow_64 = inv_scale;
                    for (std::size_t j = 0; j < coeff_modulus_size; j++, scaled_two_pow_64 *= two_pow_64) {
                        if (plain_copy[i * coeff_modulus_size + j] > decryption_modulus[j]) {
                            // between N and UINT64_MAX (positive or zero)
                            auto diff = plain_copy[i * coeff_modulus_size + j] - decryption_modulus[j];
                            res[i] += diff ? static_cast<double>(diff) * scaled_two_pow_64 : 0.0;
                        } else {
                            // between N / 2 and N (negative or zero)
                            auto diff = decryption_modulus[j] - plain_copy[i * coeff_modulus_size + j];
                            res[i] -= diff ? static_cast<double>(diff) * scaled_two_pow_64 : 0.0;
                        }
                    }
                } else {
                    // between 0 and N / 2 (positive or zero)
                    double scaled_two_pow_64 = inv_scale;
                    for (std::size_t j = 0; j < coeff_modulus_size; j++, scaled_two_pow_64 *= two_pow_64) {
                        auto curr_coeff = plain_copy[i * coeff_modulus_size + j];
                        res[i] += curr_coeff ? static_cast<double>(curr_coeff) * scaled_two_pow_64 : 0.0;
                    }
                }

                // Scaling instead incorporated above; this can help in cases
                // where otherwise pow(two_pow_64, j) would overflow due to very
                // large coeff_modulus_size and very large scale
                // res[i] = res_accum * inv_scale;
            }

            // Just return the coefficient
            return res[0].real();
        }
    };
}