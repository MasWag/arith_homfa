/**
 * @author Masaki Waga
 * @date 2023/06/23.
 */

#pragma once

#include "tfhe++.hpp"
#include <seal/seal.h>

#include "seal_config.hh"
#include "tic_toc.hh"

namespace ArithHomFA {
  /*!
   * @brief Abstract class for monitoring
   */
  class AbstractRunner {
  public:
    /*!
     * @brief Feeds a valuation to the DFA with valuations
     */
    virtual TFHEpp::TLWE<TFHEpp::lvl1param> feed(const std::vector<seal::Ciphertext> &valuations) = 0;

    /*!
     * @brief Prints the time consumed by different stages of computations.
     */
    void printTime() const {
      timer.print();
    }

  protected:
    TicTocForRunner timer;
    static void CircuitBootstrappingFFT(auto &trgsw, auto &tlwe, auto &ekey) {
        TFHEpp::CircuitBootstrappingFFT<TFHEpp::lvl10param, TFHEpp::lvl02param, TFHEpp::lvl21param>(trgsw, tlwe, ekey);
    }
  };
} // namespace ArithHomFA