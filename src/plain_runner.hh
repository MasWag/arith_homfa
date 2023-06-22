/**
 * @author Masaki Waga
 * @date 2023/05/04.
 */

#pragma once

#include "ckks_predicate.hh"
#include "graph.hpp"
#include "tic_toc.hh"

namespace ArithHomFA {
  class PlainRunner {
  public:
    PlainRunner(const SealConfig &config, const std::string &spec_filename)
        : PlainRunner(config, Graph::from_file(spec_filename)) {
    }

    PlainRunner(const SealConfig &config, const Graph &graph)
        : graph(graph), state(graph.initial_state()), predicate(config.makeContext(), config.scale) {
    }

    bool feed(const std::vector<double> &valuations) {
      assert(valuations.size() == predicate.getSignalSize());
      results.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
      timer.predicate.tic();
      predicate.eval(valuations, results);
      timer.predicate.toc();

      for (const auto &result: results) {
        timer.dfa.tic();
        state = graph.next_state(state, result > 0);
        timer.dfa.toc();
      }

      return graph.is_final_state(state);
    }

    void printTime() const {
      timer.print();
    }

  private:
    Graph graph;
    Graph::State state;
    CKKSPredicate predicate;
    TicTocForRunner timer;
    // temporary variables
    std::vector<double> results;
  };
} // namespace ArithHomFA