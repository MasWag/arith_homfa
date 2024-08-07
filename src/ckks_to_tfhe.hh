/**
 * @author Masaki Waga
 * @date 2023/04/20
 */

#pragma once

#include <optional>

#include <spdlog/spdlog.h>
#include "tfhe++.hpp"
#include <seal/seal.h>

#include "ckks_no_embed.hh"
#include "lvl3_to_lvl1.hh"
#include "rescaling.hh"


namespace ArithHomFA {
  /*!
   * @brief Translate CKKS to TFHE
   */
  class CKKSToTFHE {
  public:
    /*!
     * @brief Constructor for CKKSToTFHE
     * @param context The SEALContext for the class
     */
    explicit CKKSToTFHE(const seal::SEALContext &context)
        : context(context), evaluator(context), encoder(context) {
    }

    /*!
     * @brief Convert a secret key of SEAL to a lvl3Key of TFHEpp
     *
     * @param [in] secretKey_ The key to convert
     * @param [out] lvl3Key The resulting key
     */
    void toLv3Key(const seal::SecretKey &secretKey_,
                  TFHEpp::Key<TFHEpp::lvl3param> &lvl3Key) {
      auto secretKey = secretKey_;
      const auto context_data = context.key_context_data();
      const auto parms = context.key_context_data()->parms();
      const auto poly_modulus_degree = parms.poly_modulus_degree();
      const auto tables = context_data->small_ntt_tables();
      const std::size_t coeff_modulus_size = parms.coeff_modulus().size();
      // Assert the preconditions
      assert(secretKey.data().dyn_array().size() ==
             coeff_modulus_size * poly_modulus_degree);
      assert(poly_modulus_degree == lvl3Key.size());

      // Transform the secret key from NTT representation.
      if (secretKey.data().is_ntt_form()) {
        seal::util::RNSIter keyIter(secretKey.data().data(),
                                    poly_modulus_degree);
        seal::util::inverse_ntt_negacyclic_harvey(keyIter, coeff_modulus_size,
                                                  tables);
      }

      // Assert that the secret key is ternary.
#ifndef NDEBUG
      for (std::size_t i = 0; i < secretKey.data().dyn_array().size(); ++i) {
        const auto &coef = secretKey.data().dyn_array().at(i);
        assert(coef == 0 || coef == 1 ||
               coef ==
                   parms.coeff_modulus().at(i / poly_modulus_degree).value() -
                       1);
      }
#endif

      // Convert and assign key. We take the first part because the secret keys
      // are the same
      for (std::size_t i = 0; i < lvl3Key.size(); ++i) {
        const auto rawValue = secretKey.data().dyn_array().at(i);
        if (rawValue == 0 || rawValue == 1) {
          lvl3Key.at(i) = rawValue;
        } else {
          // assert that the value is -1 in the quotient ring
          assert(static_cast<uint64_t>(rawValue) + 1 -
                     parms.coeff_modulus().front().value() ==
                 0);
          lvl3Key.at(i) = -1;
        }
      }
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TRLWE of TFHEpp
     *
     * The resulting TRLWE is true if cipher is positive
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] trlwe The resulting TRLWE ciphertext
     *
     * @pre The degree of the given ciphertexts are the same
     */
    void toLv3TRLWE(seal::Ciphertext cipher,
                    TFHEpp::TRLWE<TFHEpp::lvl3param> &trlwe) const {
      // Assert the precondition
      const auto poly_modulus_degree = cipher.poly_modulus_degree();
      assert(poly_modulus_degree == TFHEpp::lvl3param::n);
      if (context.last_parms_id() != cipher.parms_id()) {
        spdlog::warn("CKKS ciphertext is not the last level. Switching such a ciphertext may cause an accuracy issue.");
      }

      const auto &context_data = *context.get_context_data(cipher.parms_id());

      assert(cipher.is_ntt_form());
      seal::util::PolyIter cipherIter = seal::util::iter(cipher);
      // We conduct inverse NTT transformation
      const auto tables = context_data.small_ntt_tables();
      for (std::size_t i = 0; i <= TFHEpp::lvl3param::k; ++i) {
        for (std::size_t j = 0; j < context_data.parms().coeff_modulus().size();
             ++j) {
          seal::util::inverse_ntt_negacyclic_harvey(cipherIter[i][j],
                                                    tables[j]);
        }
      }

      // CRT-compose the polynomial
      for (std::size_t i = 0; i <= TFHEpp::lvl3param::k; ++i) {
        context_data.rns_tool()->base_q()->compose_array(
            cipherIter[i], cipherIter.poly_modulus_degree(),
            seal::MemoryManager::GetPool());
      }

      // Rescale the coefficients to 2^64
      Rescaling rescale(context_data);
      for (std::size_t i = 0; i <= TFHEpp::lvl3param::k; ++i) {
#ifndef NDEBUG
        const auto decryption_modulus = context_data.total_coeff_modulus();
#endif
        const std::size_t decryption_modulus_size =
            context_data.parms().coeff_modulus().size();
        SEAL_ITERATE(
            seal::util::iter(seal::util::StrideIter<std::uint64_t *>(
                                 cipher.data(i), decryption_modulus_size),
                             // The indices of polynomials is the opposite
                             // between SEAL and TFHEpp
                             seal::util::PtrIter<TFHEpp::lvl3param::T *>(
                                 trlwe.at(1 - i).data())),
            poly_modulus_degree, [&](auto I) {
              assert(seal::util::is_less_than_uint(
                  std::get<0>(I), decryption_modulus, decryption_modulus_size));
              // The sign of the first polynomial in a ciphertext (in TFHEpp) is
              // must be negated
              std::get<1>(I) = i ? -rescale.rescale(std::get<0>(I))
                                 : rescale.rescale(std::get<0>(I));
            });
      }
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TRLWE of TFHEpp
     *
     * The resulting TRLWE is true if cipher is positive
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] trlwe The resulting TRLWE ciphertext
     * @param [in] reference the reference value to decide the amount of
     * amplification
     *
     * @pre The degree of the given ciphertexts are the same
     */
    void toLv3TRLWE(seal::Ciphertext cipher, TFHEpp::TRLWE<TFHEpp::lvl3param> &trlwe,
                    const double reference) const {
        // amplify the ciphertext
        this->amplify(cipher, reference);
        toLv3TRLWE(cipher, trlwe);
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TLWE of TFHEpp
     *
     * The resulting TLWE is true if cipher is positive
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] tlwe The resulting TLWE ciphertext
     * @param [in] reference the reference value to decide the amount of
     * amplification
     *
     * @pre The degree of the given ciphertexts are the same
     */
    void toLv3TLWE(const seal::Ciphertext &cipher, TFHEpp::TLWE<TFHEpp::lvl3param> &tlwe,
                   double reference) const {
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TRLWE<TFHEpp::lvl3param> trlwe;
      toLv3TRLWE(cipher, trlwe, reference);
      TFHEpp::SampleExtractIndex<TFHEpp::lvl3param>(tlwe, trlwe, 0);
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TLWE of TFHEpp
     *
     * The resulting TLWE is true if cipher is positive
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] tlwe The resulting TLWE ciphertext
     * amplification
     *
     * @pre The degree of the given ciphertexts are the same
     */
    void toLv3TLWE(const seal::Ciphertext &cipher, TFHEpp::TLWE<TFHEpp::lvl3param> &tlwe) const {
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TRLWE<TFHEpp::lvl3param> trlwe;
      toLv3TRLWE(cipher, trlwe);
      TFHEpp::SampleExtractIndex<TFHEpp::lvl3param>(tlwe, trlwe, 0);
    }

    void initializeConverter(const BootstrappingKey &bKey) {
      converter = Lvl3ToLvl1(bKey);
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TLWE of TFHEpp at lvl1
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] tlwe The resulting TLWE ciphertext at lvl1
     * @param [in] reference the reference value to decide the amount of
     * amplification
     *
     * @pre converter is initialized
     */
    void toLv1TLWE(const seal::Ciphertext &cipher, TFHEpp::TLWE<TFHEpp::lvl1param> &tlwe,
                   double reference) const {
      assert(converter);
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TLWE<TFHEpp::lvl3param> lvl3TLWE;
      toLv3TLWE(cipher, lvl3TLWE, reference);

      // Then convert TLWE lvl3 to TLWE lvl1 + bootstrapping
      // converter->toLv1TLWE(lvl3TLWE, tlwe);
      converter->toLv1TLWEWithBootstrapping(lvl3TLWE, tlwe);
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TLWE of TFHEpp at lvl1
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] tlwe The resulting TLWE ciphertext at lvl1
     *
     * @pre converter is initialized
     */
    void toLv1TLWE(const seal::Ciphertext &cipher, TFHEpp::TLWE<TFHEpp::lvl1param> &tlwe) const {
      assert(converter);
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TLWE<TFHEpp::lvl3param> lvl3TLWE;
      toLv3TLWE(cipher, lvl3TLWE);

      // Then convert TLWE lvl3 to TLWE lvl1 + bootstrapping
      // converter->toLv1TLWE(lvl3TLWE, tlwe);
      converter->toLv1TLWEWithBootstrapping(lvl3TLWE, tlwe);
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TRGSW of TFHEpp at lvl1
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] trgsw The resulting TRGSW ciphertext at lvl1
     * @param [in] reference the reference value to decide the amount of
     * amplification
     *
     * @pre converter is initialized
     */
    void toLv1TRGSWFFT(const seal::Ciphertext &cipher, TFHEpp::TRGSWFFT<TFHEpp::lvl1param> &trgsw,
                       double reference = 1024) const {
      assert(converter);
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TLWE<TFHEpp::lvl3param> lvl3TLWE;
      toLv3TLWE(cipher, lvl3TLWE, reference);
      TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;

      // Then convert TLWE lvl3 to TLWE lvl1 + bootstrapping
      converter->toLv1TLWEWithBootstrapping(lvl3TLWE, tlwe);

      // Finally apply circuit bootstrapping to make TRGSW
      CircuitBootstrappingFFTLvl11(trgsw, tlwe, *(converter->getBKey().ekey));
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TRGSW of TFHEpp at lvl1 with high accuracy.
     *
     * Since we use 64bit HomDecomp, we do not use amplify.
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] trgsw The resulting TRGSW ciphertext at lvl1
     *
     * @pre converter is initialized
     */
    void toLv1TRGSWFFTGood(const seal::Ciphertext &cipher, TFHEpp::TRGSWFFT<TFHEpp::lvl1param> &trgsw) const {
      assert(converter);
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TLWE<TFHEpp::lvl3param> lvl3TLWE;
      toLv3TLWE(cipher, lvl3TLWE);

      // Then convert TLWE lvl3 to TLWE lvl1 + bootstrapping
      TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
      converter->toLv1TLWEWithBootstrappingGood(lvl3TLWE, tlwe);

      // Finally apply circuit bootstrapping to make TRGSW
      CircuitBootstrappingFFTLvl11(trgsw, tlwe, *(converter->getBKey().ekey));
    }

    /*!
     * @brief Convert a CKKS ciphertext of SEAL to a TRGSW of TFHEpp at lvl1 with low accuracy.
     *
     * @param [in] cipher The CKKS ciphertext to convert
     * @param [out] trgsw The resulting TRGSW ciphertext at lvl1
     * @param [in] reference the reference value to decide the amount of
     * amplification
     *
     * @pre converter is initialized
     */
    void toLv1TRGSWFFTPoor(const seal::Ciphertext &cipher, TFHEpp::TRGSWFFT<TFHEpp::lvl1param> &trgsw,
                           double reference = 1024) const {
      assert(converter);
      // Convert CKKS ciphertext to TRLWE lvl3 first
      TFHEpp::TLWE<TFHEpp::lvl3param> lvl3TLWE;
      toLv3TLWE(cipher, lvl3TLWE, reference);

      // Then convert TLWE lvl3 to TLWE lvl1 + bootstrapping
      TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
      converter->toLv1TLWEWithBootstrappingPoor(lvl3TLWE, tlwe);

      // Finally apply circuit bootstrapping to make TRGSW
      CircuitBootstrappingFFTLvl11(trgsw, tlwe, *(converter->getBKey().ekey));
    }

    /*!
     * @brief Adaptively amplify the value so that the absolute value is
     * sufficiently large
     *
     * We amplify the value by multiplying modulus * ratio / (2.0 * reference *
     * scale)
     *
     * @param [inout] cipher the amplified ciphertext
     * @param [in] reference (approximate) upper bound of the value represented
     * by cipher. By default, we use reference = 2^32 * 0.001.
     *
     * @pre reference must be positive
     * @invariant The signature of the cipher does not change
     */
    void amplify(seal::Ciphertext &cipher, double reference = std::pow(2, 32) * 0.001) const {
      auto const &context_data = context.get_context_data(cipher.parms_id());

      assert(reference > 0);

      const double amplifiedRatio = 0.9;
      const double scaledModulus = std::pow(2.0, context_data->total_coeff_modulus_bit_count()) * amplifiedRatio;
      const double factor = scaledModulus / (2.0 * reference * cipher.scale());

      seal::Plaintext plain;
      encoder.encode(factor, 1.0, plain);
      if (cipher.parms_id() != plain.parms_id()) {
        evaluator.mod_switch_to_inplace(plain, cipher.parms_id());
      }

      evaluator.multiply_plain_inplace(cipher, plain);

      // while (context_data->next_context_data()){
      //   evaluator.mod_switch_to_next_inplace(cipher);
      //   context_data = context_data->next_context_data();
      // }
    }

  private:
    const seal::SEALContext &context;
    const seal::Evaluator evaluator;
    const ArithHomFA::CKKSNoEmbedEncoder encoder;
    std::optional<ArithHomFA::Lvl3ToLvl1> converter;
  };

} // namespace ArithHomFA
