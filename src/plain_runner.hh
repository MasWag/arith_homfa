/**
 * @author Masaki Waga
 * @date 2023/05/04.
 */

#pragma once

#include "ckks_predicate.hh"
#include "graph.hpp"

namespace ArithHomFA {
  class PlainRunner {
  public:
    PlainRunner(const SealConfig &config, const std::string &spec_filename)
        : PlainRunner(config, Graph::from_file(spec_filename)) {
    }

    PlainRunner(const SealConfig &config, const Graph &graph)
        : graph(graph), state(graph.initial_state()), predicate(config) {
    }

    bool feed(const std::vector<double> &valuations) {
      assert(valuations.size() == predicate.getSignalSize());
      results.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
      predicate.eval(valuations, results);

      for (const auto &result: results) {
        state = graph.next_state(state, result > 0);
      }

      return graph.is_final_state(state);
    }

  private:
    Graph graph;
    Graph::State state;
    CKKSPredicate predicate;
    // temporary variables
    std::vector<double> results;
  };
}  // namespace ArithHomFA