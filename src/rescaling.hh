/**
 * @author Masaki Waga
 * @date 2023/04/19
 */

#pragma once

#include <cassert>

#include <seal/seal.h>

namespace ArithHomFA {
  /*!
   * @brief Rescale a coefficient in an arbitrary modulus to 64bit
   *
   * In this class, we use bigint
   */
  class Rescaling {
  public:
    explicit Rescaling(const seal::SEALContext::ContextData &context_data) {
      original_modulus_size = context_data.parms().coeff_modulus().size();
      upper_half_threshold = context_data.upper_half_threshold();
      numerator = seal::util::allocate_uint(original_modulus_size + 1, seal::MemoryManager::GetPool());
      seal::util::set_zero_uint(original_modulus_size + 1, numerator.get());
      quotient = seal::util::allocate_uint(original_modulus_size + 1, seal::MemoryManager::GetPool());
      decryption_modulus = seal::util::allocate_zero_uint(original_modulus_size + 1, seal::MemoryManager::GetPool());
      seal::util::set_uint(context_data.total_coeff_modulus(), original_modulus_size, decryption_modulus.get());
    }

    /*!
     * @brief Rescale a coefficient
     *
     * @param coefficient Pointer corresponding to the coefficient in SEAL's bigint representation
     *
     * @pre coefficient is in the modulus specified in the constructor
     */
    std::uint64_t rescale(const std::uint64_t *const coefficient) const {
      // 0. Assert the precondition
      assert(seal::util::is_less_than_uint(coefficient, decryption_modulus.get(), original_modulus_size));

      // 1. Check the sign
      bool isNegative =
          seal::util::is_greater_than_or_equal_uint(coefficient, upper_half_threshold, original_modulus_size);

      // 2. Take the absolute value and multiply 2^64 (left shift of 64bits)
      if (isNegative) {
        seal::util::sub_uint(decryption_modulus.get(), coefficient, original_modulus_size, numerator.get() + 1);
      } else {
        seal::util::set_uint(coefficient, original_modulus_size, numerator.get() + 1);
      }
      assert(seal::util::is_less_than_or_equal_uint(numerator.get() + 1, upper_half_threshold, original_modulus_size));

      // 3. Divide by the law
      seal::util::divide_uint_inplace(numerator.get(), decryption_modulus.get(), original_modulus_size + 1,
                                      quotient.get(), seal::MemoryManager::GetPool());

      // 4. Assert that the value is in 2^64
      for (std::size_t j = 1; j < original_modulus_size + 1; ++j) {
        assert(quotient[j] == 0);
      }

      // 5. Put the result to the TRLWE
      std::uint64_t result = isNegative ? -*quotient : *quotient;
      assert(isNegative == (result >> 63));

      return result;
    }

  private:
    seal::util::Pointer<std::uint64_t> decryption_modulus;
    seal::util::Pointer<std::uint64_t> numerator;
    seal::util::Pointer<std::uint64_t> quotient;
    std::size_t original_modulus_size;
    const std::uint64_t *upper_half_threshold;
  };
} // namespace ArithHomFA