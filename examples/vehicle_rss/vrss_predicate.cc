/**
 * @author Takashi Suwa
 * @date 2023/08/30
 */

#include "ckks_predicate.hh"

namespace ArithHomFA {
    /*!
     * @brief Compute predicates for monitoring RSS (responsibility-sensitive safety) properties
     *
     * signal size == 8
     * predicate size == 8
     *
     * single entry in the signal: (x_b, y_b, v_b, a_b, x_f, y_f, v_f, a_f), where
     * - x_b : the rear car's x-coordinate
     * - y_b : the rear car's y-coordinate
     * - v_b : the rear car's speed in the +y direction
     * - a_b : the rear car's acceleration in the +y direction
     * - x_f, y_f, v_f, a_f : the front car's equivalents
     *
     * predicates:
     * - p0 :<=> d_posMin(v_f, v_b) > 0
     * - p1 :<=> (y_f - y_b > 0)
     * - p2 :<=> (y_f - y_b - d_posMin(v_f, v_b) > 0)
     * - p3 :<=> (a_b <= a_maxAcc) <=> (a_maxAcc - a_b >= 0)
     * - p4 :<=> (a_b <= -a_minBr) <=> (-a_minBr - a_b >= 0)
     * - p5 :<=> (a_f >= -a_maxBr) <=> (a_f + a_maxBr >= 0)
     * - p6 :<=> (x_b - x_f < d_lat) <=> (x_f - x_b + d_lat > 0)
     * - p7 :<=> (x_f - x_b < d_lat) <=> (x_b - x_f + d_lat > 0)
     * where
     *   d_posMin(v_f, v_b) := d_bPreBr(v_b) + d_bBrake(v_b) - d_fBrake(v_f)
     *   d_bPreBr(v_b) := v_b * rho + a_maxAcc * rho^2 / 2
     *   d_bBrake(v_b) := (v_b + rho * a_maxAcc)^2 / (2 * a_minBr)
     *   d_fBrake(v_f) := (v_f)^2 / (2 * a_maxBr)
     *   a_maxAcc : the maximum of allowed acceleration
     *   a_maxBr : the maximum of allowed brake
     *   a_minBr : the minimum brake required when the situation is critical
     *   rho : the maximum response time imposed on the rear car when the positional relation becomes dangerous
     *   d_lat : the lateral distance criterion; two cars are seen to be on the same lane
           if the lateral distance between the two is smaller than d_lat
     */
    void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                              std::vector<seal::Ciphertext> &result) {
      /* Constants basically based on the setting of the simulator: */
      const double raw_rho = 0.1;
      const double raw_a_maxAcc = 2;
      const double raw_a_maxBr = 9;
      const double raw_a_minBr = 7;
      const double raw_d_lat = 4;
      seal::Plaintext
        rho, a_maxAcc, a_maxBr, a_minBr, d_lat,
        rho_times_a_maxAcc, half_rho_times_a_maxAcc, inv_double_a_minBr, inv_double_a_maxBr, plain_one;
      this->encoder.encode(raw_rho, this->scale, rho);
      this->encoder.encode(raw_a_maxAcc, this->scale, a_maxAcc);
      this->encoder.encode(raw_a_maxBr, this->scale, a_maxBr);
      this->encoder.encode(raw_a_minBr, this->scale, a_minBr);
      this->encoder.encode(raw_d_lat, this->scale, d_lat);
      this->encoder.encode(raw_rho * raw_a_maxAcc, this->scale, rho_times_a_maxAcc);
      this->encoder.encode(raw_rho * raw_a_maxAcc / 2, this->scale, half_rho_times_a_maxAcc);
      this->encoder.encode(1 / (2 * raw_a_minBr), this->scale, inv_double_a_minBr);
      this->encoder.encode(1 / (2 * raw_a_maxBr), this->scale, inv_double_a_maxBr);
      this->encoder.encode(1, this->scale, plain_one);

      /* Inputs:
       * x_b = valuation.at(0);
       * y_b = valuation.at(1);
       * v_b = valuation.at(2);
       * a_b = valuation.at(3);
       * x_f = valuation.at(4);
       * y_f = valuation.at(5);
       * v_f = valuation.at(6);
       * a_f = valuation.at(7);
       */

      /* Calculates d_bPreBr := (v_b + rho * a_maxAcc / 2) * rho: */
      seal::Ciphertext d_bPreBr;
      this->evaluator.add_plain(valuation.at(2), half_rho_times_a_maxAcc, d_bPreBr);
      this->evaluator.multiply_plain_inplace(d_bPreBr, rho);

      /* Calculates d_bBrake := (v_b + rho * a_maxAcc)^2 / (2 * a_minBr): */
      seal::Ciphertext d_bBrake;
      this->evaluator.add_plain(valuation.at(2), rho_times_a_maxAcc, d_bBrake);
      this->evaluator.square_inplace(d_bBrake);
      this->evaluator.relinearize_inplace(d_bBrake, this->relinKeys);
      this->evaluator.multiply_plain_inplace(d_bBrake, inv_double_a_minBr);
      // d_bBrake goes to the second level
      this->evaluator.rescale_to_next_inplace(d_bBrake);
      // Force to change the scale because they are almost the same
      if (std::abs(d_bBrake.scale() - d_bPreBr.scale()) / d_bBrake.scale() < 0.0001) {
        d_bBrake.scale() = d_bPreBr.scale();
      } else {
        std::cerr << "Error: unexpected scale mismatch" << std::endl;
        abort();
      }

      /* Calculates d_fBrake := (v_f)^2 / (2 * a_maxBr): */
      seal::Ciphertext d_fBrake;
      this->evaluator.square(valuation.at(6), d_fBrake);
      this->evaluator.relinearize_inplace(d_fBrake, this->relinKeys);
      this->evaluator.multiply_plain_inplace(d_fBrake, inv_double_a_maxBr);
      // d_fBrake goes to the second level
      this->evaluator.rescale_to_next_inplace(d_fBrake);
      // Force to change the scale because they are almost the same
      if (std::abs(d_fBrake.scale() - d_bPreBr.scale()) / d_fBrake.scale() < 0.0001) {
        d_fBrake.scale() = d_bPreBr.scale();
      } else {
        std::cerr << "Error: unexpected scale mismatch" << std::endl;
        abort();
      }

      /* Calculates result.at(0) := d_posMin := d_bPreBr + d_bBrake - d_fBrake: */
      this->evaluator.mod_switch_to_inplace(d_bPreBr, d_bBrake.parms_id());
      this->evaluator.add(d_bPreBr, d_bBrake, result.at(0));
      // Switch the modulus. result.at(0) switches to the second level
      this->evaluator.mod_switch_to_inplace(result.at(0), d_fBrake.parms_id());
      this->evaluator.sub_inplace(result.at(0), d_fBrake);

      /* Calculates result.at(1) := y_f - y_d: */
      this->evaluator.sub(valuation.at(5), valuation.at(1), result.at(1));

      /* Calculates result.at(2) := (y_f - y_d) - d_posMin: */
      // Multiply by one to change the scale without changing the value
      this->evaluator.multiply_plain_inplace(result.at(1), plain_one);
      // Switch the modulus. result.at(1) switches to the second level
      this->evaluator.mod_switch_to_inplace(result.at(1), result.at(0).parms_id());
      this->evaluator.sub(result.at(0), result.at(1), result.at(2));

      /* Calculates result.at(3) := a_maxAcc - a_b: */
      this->evaluator.sub_plain(valuation.at(3), a_maxAcc, result.at(3));
      this->evaluator.negate_inplace(result.at(3));

      /* Calculates result.at(4) := -a_minBr - a_b: */
      this->evaluator.sub_plain(valuation.at(3), a_minBr, result.at(4));
      this->evaluator.negate_inplace(result.at(4));

      /* Calculates result.at(5) := a_f + a_maxBr: */
      this->evaluator.add_plain(valuation.at(7), a_maxBr, result.at(5));

      /* Calculates result.at(6) := x_f - x_b + d_lat and
                    result.at(7) := x_b - x_f + d_lat: */
      this->evaluator.sub(valuation.at(4), valuation.at(0), result.at(7));
      this->evaluator.add_plain(result.at(7), d_lat, result.at(6));
      this->evaluator.negate_inplace(result.at(7));
      this->evaluator.add_plain_inplace(result.at(7), d_lat);
      // Switch to the last level
      for (int i = 0; i < 8; ++i) {
          if (result.at(i).scale() > scale * 2) {
              this->evaluator.rescale_to_inplace(result.at(i), context.last_parms_id());
          } else {
              this->evaluator.mod_switch_to_inplace(result.at(i), context.last_parms_id());
          }
      }
    }

    void CKKSPredicate::evalPredicateInternal(const std::vector<double> &valuation,
                                              std::vector<double> &result) {
      /* Constants basically based on the setting of the simulator: */
      const double rho = 1.0;
      const double a_maxAcc = 2;
      const double a_maxBr = 9;
      const double a_minBr = 7;
      const double d_lat = 4;

      const double x_b = valuation.at(0);
      const double y_b = valuation.at(1);
      const double v_b = valuation.at(2);
      const double a_b = valuation.at(3);
      const double x_f = valuation.at(4);
      const double y_f = valuation.at(5);
      const double v_f = valuation.at(6);
      const double a_f = valuation.at(7);

      const double d_bPreBr = (v_b + rho * a_maxAcc / 2) * rho;
      const double v_bPreBr = v_b + rho * a_maxAcc;
      const double d_bBrake = v_bPreBr * v_bPreBr / (2 * a_minBr);
      const double d_fBrake = v_f * v_f / (2 * a_maxBr);
      const double d_posMin = d_bPreBr + d_bBrake - d_fBrake;

      result.at(0) = d_posMin;
      result.at(1) = y_f - y_b;
      result.at(2) = y_f - y_b - d_posMin;
      result.at(3) = a_maxAcc - a_b;
      result.at(4) = -a_minBr - a_b;
      result.at(5) = a_f + a_maxBr;
      result.at(6) = x_f - x_b + d_lat;
      result.at(7) = x_b - x_f + d_lat;
    }

    // Define the signal and predicate sizes
    const std::size_t CKKSPredicate::signalSize = 8;
    const std::size_t CKKSPredicate::predicateSize = 8;
    // The approximate maximum value of the difference between the signal and the threshold
    const std::vector<double> CKKSPredicate::references = {250, 100, 350, 30, 30, 30, 10, 10};
}
