/**
 * @author Masaki Waga
 * @date 2023/05/02
 */

#include "spdlog/spdlog.h"

#include "../../src/ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute glucose > 70
     *
     * signal size == 1
     * predicate size == 1
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        // spdlog: https://github.com/gabime/spdlog is configured so that a user can output some message
        // spdlog::info("Hello");
        seal::Plaintext plain;
        this->encoder.encode(70, this->scale, plain);
        this->evaluator.sub_plain(valuation.front(), plain, result.front());
        // We assume that the ciphertexts are move to the last level by mod_switch or rescale.
        this->evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
    }
    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
      result.front() = valuation.front() - 70;
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 1;
    const std::size_t CKKSPredicate::predicateSize = 1;
    const std::vector<double> CKKSPredicate::references = {100};
}
