/**
 * @brief Vehicle RSS (Responsibility-Sensitive Safety) predicate implementation
 * 
 * This predicate monitors vehicle safety conditions based on RSS principles.
 * It evaluates four safety conditions (p0, p1, p2, p3) related to:
 * - Longitudinal safe distance
 * - Lateral safe distance  
 * - Safe following distance
 * - Emergency braking conditions
 */

#include "ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute RSS safety predicates for vehicle monitoring
     *
     * signal size == 4 (velocity, distance, lateral_distance, acceleration)
     * predicate size == 4 (p0, p1, p2, p3)
     * 
     * p0: velocity - 30 (max safe velocity)
     * p1: distance - 10 (min safe distance)
     * p2: lateral_distance - 2 (min lateral distance)
     * p3: acceleration - 5 (max safe acceleration)
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
        seal::Plaintext plain;
        
        // p0: Check if velocity is below safe threshold (30 m/s)
        this->encoder.encode(30, this->scale, plain);
        this->evaluator.sub_plain(valuation[0], plain, result[0]);
        this->evaluator.negate_inplace(result[0]); // We want velocity < 30, so negate
        
        // p1: Check if distance is above minimum (10 m)
        this->encoder.encode(10, this->scale, plain);
        this->evaluator.sub_plain(valuation[1], plain, result[1]);
        
        // p2: Check if lateral distance is above minimum (2 m)
        this->encoder.encode(2, this->scale, plain);
        this->evaluator.sub_plain(valuation[2], plain, result[2]);
        
        // p3: Check if acceleration is below maximum (5 m/s²)
        this->encoder.encode(5, this->scale, plain);
        this->evaluator.sub_plain(valuation[3], plain, result[3]);
        this->evaluator.negate_inplace(result[3]); // We want acceleration < 5, so negate
        
        // Switch to last parameter set for all results
        for (auto& res : result) {
            this->evaluator.mod_switch_to_inplace(res, context.last_parms_id());
        }
    }

    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
        result[0] = 30 - valuation[0];  // velocity < 30
        result[1] = valuation[1] - 10;  // distance > 10
        result[2] = valuation[2] - 2;   // lateral_distance > 2
        result[3] = 5 - valuation[3];   // acceleration < 5
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 4;
    const std::size_t CKKSPredicate::predicateSize = 4;
    
    // The approximate maximum value of the difference between the signal and the threshold
    // These values represent the expected range of differences for each predicate
    const std::vector<double> CKKSPredicate::references = {50, 100, 20, 10};
}