/**
 * @author Masaki Waga
 * @date 2023/06/22.
 */

#pragma once

#include <execution>
#include <ranges>

#include <boost/iterator/zip_iterator.hpp>

#include "graph.hpp"
#include "offline_dfa.hpp"

#include "abstract_runner.hh"
#include "ckks_predicate.hh"
#include "ckks_to_tfhe.hh"
#include "seal_config.hh"
#include "tic_toc.hh"

namespace ArithHomFA {
  /*!
   * @brief Class for offline monitoring
   */
  class OfflineRunner : public AbstractRunner {
  public:
    OfflineRunner(const seal::SEALContext &context, double scale, const std::string &spec_filename, size_t input_size,
                  size_t boot_interval, const BootstrappingKey &bkey, const std::vector<double> &references)
        : OfflineRunner(context, scale, Graph::from_file(spec_filename), input_size, boot_interval, bkey, references) {
    }

    OfflineRunner(const seal::SEALContext &context, double scale, const Graph &graph, size_t input_size,
                  size_t boot_interval, const BootstrappingKey &bkey, const std::vector<double> &references)
        : runner(graph, input_size, boot_interval, bkey.ekey, false), predicate(context, scale), bkey(bkey),
          converter(context), references(references) {
      converter.initializeConverter(this->bkey);
    }

    /*!
     * @brief Feeds a valuation to the DFA with valuations
     *
     * @note We expect that the sequence of valuations is given from back to front, and result is for the current
     * suffix. Namely, let \f$w = a_1, a_2, \dots, a_n\f$ be the monitored sequence. We expect that the valuations is
     * fed from \f$a_n\f$ to $\fa_1\f$. The result after feeding \f$w = a_i, a_{i+1}, \dots, a_n\f$ shows if \f$w = a_i,
     * a_{i+1}, \dots, a_n\f$ satisfies the specification.
     */
    TFHEpp::TLWE<TFHEpp::lvl1param> feed(const std::vector<seal::Ciphertext> &valuations) override {
      assert(valuations.size() == predicate.getSignalSize());
      // Evaluate the predicates
      ckksCiphers.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
      timer.predicate.tic();
      predicate.eval(valuations, ckksCiphers);
      timer.predicate.toc();

      // Construct TRGSW
      tlwes.resize(ckksCiphers.size());
      trgsws.resize(ckksCiphers.size());
      timer.ckks_to_tfhe.tic();
      std::for_each(std::execution::par,
                    boost::make_zip_iterator(boost::make_tuple(ckksCiphers.begin(), tlwes.begin(), trgsws.begin())),
                    boost::make_zip_iterator(boost::make_tuple(ckksCiphers.end(), tlwes.end(), trgsws.end())),
                    [&](const auto &t) {
                      const seal::Ciphertext &ckksCipher = boost::get<0>(t);
                      TFHEpp::TLWE<TFHEpp::lvl1param> &tlwe = boost::get<1>(t);
                      TFHEpp::TRGSWFFT<TFHEpp::lvl1param> &trgsw = boost::get<2>(t);
                      converter.toLv1TLWE(ckksCipher, tlwe);
                      TFHEpp::CircuitBootstrappingFFT<TFHEpp::lvl10param, TFHEpp::lvl02param, TFHEpp::lvl21param>(
                          trgsw, tlwe, *bkey.ekey);
                    });
      timer.ckks_to_tfhe.toc();

      for (const auto &trgsw: std::ranges::reverse_view(trgsws)) {
        timer.dfa.tic();
        runner.eval_one(trgsw);
        timer.dfa.toc();
      }

      return runner.result();
    }

    void setRelinKeys(const seal::RelinKeys &keys) {
      this->predicate.setRelinKeys(keys);
    }

  private:
    OfflineDFARunner runner;
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