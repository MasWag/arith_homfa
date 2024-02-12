/**
 * @author Masaki Waga
 * @date 2023/05/07
 */

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
        std::array<seal::Plaintext, 2> plains;
        this->encoder.encode(70, this->scale, plains.front());
        this->encoder.encode(180, this->scale, plains.back());
        this->evaluator.sub_plain(valuation.front(), plains.front(), result.front());
        this->evaluator.sub_plain(valuation.front(), plains.back(), result.back());
        this->evaluator.negate_inplace(result.back());
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
