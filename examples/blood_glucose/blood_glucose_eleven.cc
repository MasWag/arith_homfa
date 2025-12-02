/**
 * @author Masaki Waga
 * @date 2023/05/07
 */

#include "ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute glucose > 200 and glucose < 200
     *
     * signal size == 1
     * predicate size == 1
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        seal::Plaintext plain;
        this->encoder.encode(200, this->scale, plain);
        this->evaluator.sub_plain(valuation.front(), plain, result.front());
        this->evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
    }

    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
        result.front() = valuation.front() - 200;
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 1;
    const std::size_t CKKSPredicate::predicateSize = 1;
    const std::vector<double> CKKSPredicate::references = {300};
}
