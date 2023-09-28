/**
 * @author Masaki Waga
 * @date 2023/05/05.
 */

#pragma once

#include <chrono>

#include "spdlog/spdlog.h"

namespace ArithHomFA {
  /*!
   * @brief Measure the execution time
   */
  class TicToc {
  private:
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::duration total = std::chrono::system_clock::duration::zero();
    bool measuring = false;

  public:
    void tic() {
      measuring = true;
      start = std::chrono::system_clock::now();
    }
    void toc() {
      total += std::chrono::system_clock::now() - start;
      assert(measuring);
      measuring = false;
    }
    [[nodiscard]] const auto &getTotal() const {
      return total;
    }
  };

  struct TicTocForRunner {
    TicToc predicate;
    TicToc ckks_to_tfhe;
    TicToc dfa;
    TicToc total;
    void print() const {
      spdlog::info("Execution time for Predicate evaluation: {} [us]",
                   std::chrono::duration_cast<std::chrono::microseconds>(predicate.getTotal()).count());
      spdlog::info("Execution time for bridging CKKS and TFHE: {} [us]",
                   std::chrono::duration_cast<std::chrono::microseconds>(ckks_to_tfhe.getTotal()).count());
      spdlog::info("Execution time for DFA evaluation: {} [us]",
                   std::chrono::duration_cast<std::chrono::microseconds>(dfa.getTotal()).count());
      spdlog::info("Total execution time for monitoring: {} [us]",
                   std::chrono::duration_cast<std::chrono::microseconds>(total.getTotal()).count());
    }
  };
} // namespace ArithHomFA
