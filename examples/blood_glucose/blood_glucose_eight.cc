/**
 * @author Masaki Waga
 * @date 2023/05/07
 */

#include "ckks_predicate.hh"

namespace ArithHomFA {
  /*!
   * @brief Compute Delta glucose > -5 && Delta glucose < 3
   *
   * signal size == 1
   * predicate size == 2
   */
  void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                            std::vector<seal::Ciphertext> &result) {
    static bool initial = true;
    static std::vector<seal::Ciphertext> lastValuation = valuation;
    std::array<seal::Plaintext, 2> plains;
    this->encoder.encode(-5, this->scale, plains.front());
    this->encoder.encode(3, this->scale, plains.back());
    if (initial) {
      // hack to prevent making transparent ciphertext
      result.at(0) = valuation.front();
      result.at(1) = valuation.front();
    } else {
      this->evaluator.sub_plain(valuation.front(), plains.front(), result.front());
      this->evaluator.sub_inplace(result.front(), lastValuation.front());
      this->evaluator.sub_plain(valuation.front(), plains.back(), result.back());
      this->evaluator.sub_inplace(result.back(), lastValuation.front());
      this->evaluator.negate_inplace(result.back());
    }
    lastValuation = valuation;
    initial = false;
    this->evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
    this->evaluator.mod_switch_to_inplace(result.back(), context.last_parms_id());
  }

  void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation, std::vector<double> &result) {
    static bool initial = true;
    static std::vector<double> lastValuation = valuation;
    if (initial) {
      // hack to prevent making transparent ciphertext
      result.at(0) = valuation.front();
      result.at(1) = valuation.front();
    } else {
      result.front() = (valuation.front() - lastValuation.front()) + 5;
      result.back() = 3 - (valuation.back() - lastValuation.back());
    }
    lastValuation = valuation;
    initial = false;
  }

  // Define the signal and predicate sizes
  const std::size_t CKKSPredicate::signalSize = 1;
  const std::size_t CKKSPredicate::predicateSize = 2;
  const std::vector<double> CKKSPredicate::references = {10, 10};
} // namespace ArithHomFA
