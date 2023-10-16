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
   * @brief Class for online monitoring with the reverse algorithm
   */
  class ReverseRunner : public AbstractRunner {
  public:
    ReverseRunner(const seal::SEALContext &context, double scale, const std::string &spec_filename,
                  size_t boot_interval, const BootstrappingKey &bkey, const std::vector<double> &references,
                  bool reversed = false)
        : ReverseRunner(context, scale, Graph::from_file(spec_filename), boot_interval, bkey, references, reversed) {
    }

    ReverseRunner(const seal::SEALContext &context, double scale, const Graph &graph, size_t boot_interval,
                  const BootstrappingKey &bkey, const std::vector<double> &references, bool reversed = false)
        : runner(graph, boot_interval, reversed, bkey.ekey, false), predicate(context, scale), bkey(bkey),
          converter(context), references(references) {
      converter.initializeConverter(this->bkey);
    }

    /*!
     * @brief Feeds a valuation to the DFA with valuations
     */
    TFHEpp::TLWE<TFHEpp::lvl1param> feed(const std::vector<seal::Ciphertext> &valuations) override {
      timer.total.tic();
      assert(valuations.size() == predicate.getSignalSize());
      // Evaluate the predicates
      ckksCiphers.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
      timer.predicate.tic();
      predicate.eval(valuations, ckksCiphers);
      timer.predicate.toc();

      // Construct TRGSW
      tlwes.resize(ckksCiphers.size());
      trgsws.resize(ckksCiphers.size());
      // Enable nested parallelization
      omp_set_nested(1);
      timer.ckks_to_tfhe.tic();
      // Note: this parallelization can decelerate if the queue is small
#pragma omp parallel for default(none) shared(ckksCiphers, trgsws, converter, bkey)
      for (std::size_t i = 0; i < ckksCiphers.size(); ++i) {
        TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
        converter.toLv1TLWE(ckksCiphers.at(i), tlwe, this->references.at(i));
        CircuitBootstrappingFFT(trgsws.at(i), tlwe, *bkey.ekey);
      }
      timer.ckks_to_tfhe.toc();
      omp_set_nested(0);
      timer.total.toc();

      return this->feedRaw(trgsws);
    }

    /*!
     * @brief Directly feeds a valuation to the DFA with valuations, mainly for debugging
     */
    TFHEpp::TLWE<TFHEpp::lvl1param> feedRaw(const std::vector<TFHEpp::TRGSWFFT<TFHEpp::lvl1param>> &ciphers) {
      timer.total.tic();
      for (const auto &trgsw: ciphers) {
        timer.dfa.tic();
        runner.eval_one(trgsw);
        timer.dfa.toc();
      }

      timer.dfa.tic();
      auto result = runner.result();
      timer.dfa.toc();
      timer.total.toc();

      return result;
    }

    void setRelinKeys(const seal::RelinKeys &keys) {
      this->predicate.setRelinKeys(keys);
    }

  private:
    OnlineDFARunner2 runner;
    CKKSPredicate predicate;
    const BootstrappingKey &bkey;
    CKKSToTFHE converter;
    const std::vector<double> references;
    // temporary variables
    std::vector<seal::Ciphertext> ckksCiphers;
    std::vector<TFHEpp::TLWE<TFHEpp::lvl1param>> tlwes;
    std::vector<TFHEpp::TRGSWFFT<TFHEpp::lvl1param>> trgsws;
  };
} // namespace ArithHomFA
