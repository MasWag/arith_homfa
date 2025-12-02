/**
 * @author Masaki Waga
 * @date 2023/05/07
 */

#include "ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute glucose < 350
     *
     * signal size == 1
     * predicate size == 1
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        seal::Plaintext plain;
        this->encoder.encode(350, this->scale, plain);
        this->evaluator.sub_plain(valuation.front(), plain, result.front());
    	this->evaluator.negate_inplace(result.front());
        this->evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
    }

    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
        result.front() = 350 - valuation.front();
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 1;
    const std::size_t CKKSPredicate::predicateSize = 1;
    // The approximate maximum value of the difference between the signal and the threshold
    const std::vector<double> CKKSPredicate::references = {300};
}
