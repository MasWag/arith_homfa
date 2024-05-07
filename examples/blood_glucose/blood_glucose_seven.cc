/**
 * @author Masaki Waga
 * @date 2023/05/07
 */

#include "spdlog/spdlog.h"

#include "../../src/ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute glucose > 70 && glucose < 180
     *
     * signal size == 1
     * predicate size == 2
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        // spdlog: https://github.com/gabime/spdlog is configured so that a user can output some message
        // spdlog::info("Hello");
        std::array<seal::Plaintext, 2> plains;
        this->encoder.encode(70, this->scale, plains.front());
        this->encoder.encode(180, this->scale, plains.back());
        this->evaluator.sub_plain(valuation.front(), plains.front(), result.front());
        this->evaluator.sub_plain(valuation.front(), plains.back(), result.back());
        this->evaluator.negate_inplace(result.back());
        // We assume that the ciphertexts are move to the last level by mod_switch or rescale.
        this->evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
        this->evaluator.mod_switch_to_inplace(result.back(), context.last_parms_id());
    }

    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
        result.front() = valuation.front() - 70;
        result.back() = 180 - valuation.back();
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 1;
    const std::size_t CKKSPredicate::predicateSize = 2;
    // The approximate maximum value of the difference between the signal and the threshold
    const std::vector<double> CKKSPredicate::references = {300, 300};
}
