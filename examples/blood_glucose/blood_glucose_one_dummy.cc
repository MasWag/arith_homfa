/**
 * @author Masaki Waga
 * @date 2023/05/02
 */

#include "../../src/ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute glucose > 70
     *
     * signal size == 2
     * predicate size == 1
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        seal::Plaintext plain;
        this->encoder.encode(70, this->scale, plain);
        this->evaluator.sub_plain(valuation.front(), plain, result.front());
    }
    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
      result.front() = valuation.front() - 70;
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 2;
    const std::size_t CKKSPredicate::predicateSize = 1;
    const std::vector<double> CKKSPredicate::references = {100};
}
