/**
 * @brief Blood Glucose Monitoring Predicate
 * 
 * This predicate monitors blood glucose levels to ensure they stay
 * within safe ranges. The predicate checks if glucose > 70 mg/dL
 * to detect hypoglycemia (low blood sugar).
 */

#include "ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute glucose > 70
     *
     * signal size == 1 (glucose level in mg/dL)
     * predicate size == 1 (p0: glucose > 70)
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        seal::Plaintext plain;
        // Check if glucose level is above 70 mg/dL (hypoglycemia threshold)
        this->encoder.encode(70, this->scale, plain);
        this->evaluator.sub_plain(valuation.front(), plain, result.front());
        this->evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
    }

    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
        // Plain evaluation: glucose - 70
        result.front() = valuation.front() - 70;
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 1;
    const std::size_t CKKSPredicate::predicateSize = 1;
    
    // The approximate maximum value of the difference between the signal and the threshold
    // Normal glucose range is 70-180 mg/dL, so max difference from 70 is about 220
    const std::vector<double> CKKSPredicate::references = {220};
}