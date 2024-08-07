/**
 * @author Masaki Waga
 * @date 2023/06/23.
 */

#pragma once

#include <execution>
#include <ranges>
#include <omp.h>

#include <boost/iterator/zip_iterator.hpp>

#include "graph.hpp"
#include "offline_dfa.hpp"

#include "abstract_runner.hh"
#include "ckks_predicate.hh"
#include "ckks_to_tfhe.hh"
#include "online_dfa.hpp"
#include "seal_config.hh"
#include "tic_toc.hh"

namespace ArithHomFA {
  /*!
   * @brief Class for online monitoring with the block algorithm
   */
  template<RunnerMode mode>
  class BlockRunner : public AbstractRunner<mode> {
  public:
    BlockRunner(const seal::SEALContext &context, double scale, const std::string &spec_filename, std::size_t blockSize,
                const BootstrappingKey &bkey, const std::vector<double> &references)
        : BlockRunner(context, scale, Graph::from_file(spec_filename), blockSize, bkey, references) {
    }

    BlockRunner(const seal::SEALContext &context, double scale, const Graph &graph, std::size_t blockSize,
                const BootstrappingKey &bkey, const std::vector<double> &references)
        : runner(graph, std::numeric_limits<std::size_t>::max(), *bkey.ekey, false), predicate(context, scale),
          bkey(bkey), converter(context), references(references), blockSize(blockSize) {
      converter.initializeConverter(this->bkey);
      queued_inputs_.reserve(predicate.getPredicateSize() * blockSize);
      // The trivial TLWE representing true
      latestResult[TFHEpp::lvl1param::n] = (1u << 31); // 1/2
    }

    /*!
     * @brief Feeds a valuation to the DFA with valuations
     *
     * @note In the block algorithm, the input is handled for each block. Therefore, output changes only block-wise. For
     * example, if blockSize == 4, the output changes only after feeding i-th input, where i == 4 * N.
     */
    TFHEpp::TLWE<TFHEpp::lvl1param> feed(const std::vector<seal::Ciphertext> &valuations) override {
      this->timer.total.tic();
      assert(valuations.size() == predicate.getSignalSize());
      // Evaluate the predicates
      std::vector<seal::Ciphertext> ckksCiphers(ArithHomFA::CKKSPredicate::getPredicateSize());
      this->timer.predicate.tic();
      predicate.eval(valuations, ckksCiphers);
      this->timer.predicate.toc();
      std::move(ckksCiphers.begin(), ckksCiphers.end(), std::back_inserter(queued_inputs_));

      // We do not construct TRGSW until the queue is filled
      if (queued_inputs_.size() < predicate.getPredicateSize() * blockSize) {
        return latestResult;
      }

      // Construct TRGSW
      tlwes.resize(queued_inputs_.size());
      trgsws.resize(queued_inputs_.size());
      // Enable nested parallelization
      omp_set_nested(1);
      this->timer.ckks_to_tfhe.tic();
      // Note: this parallelization can decelerate if the queue is small
      if constexpr (mode == RunnerMode::normal) {
#pragma omp parallel for default(none) shared(queued_inputs_, trgsws, converter, bkey)
        for (std::size_t i = 0; i < queued_inputs_.size(); ++i) {
          converter.toLv1TRGSWFFT(queued_inputs_.at(i), trgsws.at(i), this->references.at(i % ArithHomFA::CKKSPredicate::getPredicateSize()));
        }
      } else if constexpr (mode == RunnerMode::fast) {
#pragma omp parallel for default(none) shared(queued_inputs_, trgsws, converter, bkey)        
        for (std::size_t i = 0; i < queued_inputs_.size(); ++i) {
          converter.toLv1TRGSWFFTPoor(queued_inputs_.at(i), trgsws.at(i), this->references.at(i % ArithHomFA::CKKSPredicate::getPredicateSize()));
        }
      } else {
#pragma omp parallel for default(none) shared(queued_inputs_, trgsws, converter, bkey)
        for (std::size_t i = 0; i < queued_inputs_.size(); ++i) {
          converter.toLv1TRGSWFFTGood(queued_inputs_.at(i), trgsws.at(i));
        }
      }
      this->timer.ckks_to_tfhe.toc();
      omp_set_nested(0);
      queued_inputs_.clear();

      for (const auto &trgsw: trgsws) {
        this->timer.dfa.tic();
        runner.eval_one(trgsw);
        this->timer.dfa.toc();
      }

      this->timer.dfa.tic();
      latestResult = runner.result();
      this->timer.dfa.toc();
      this->timer.total.toc();

      return latestResult;
    }

    void setRelinKeys(const seal::RelinKeys &keys) {
      this->predicate.setRelinKeys(keys);
    }

  private:
    OnlineDFARunner4 runner;
    CKKSPredicate predicate;
    const BootstrappingKey &bkey;
    CKKSToTFHE converter;
    const std::vector<double> references;
    const std::size_t blockSize;
    std::vector<seal::Ciphertext> queued_inputs_;
    TFHEpp::TLWE<TFHEpp::lvl1param> latestResult{};
    // temporary variables
    std::vector<TFHEpp::TLWE<TFHEpp::lvl1param>> tlwes;
    std::vector<TFHEpp::TRGSWFFT<TFHEpp::lvl1param>> trgsws;
  };
} // namespace ArithHomFA
