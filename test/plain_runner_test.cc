/**
 * @author Masaki Waga
 * @date 2023/05/04.
 */

#include <boost/test/unit_test.hpp>

#include "../src/plain_runner.hh"

BOOST_AUTO_TEST_SUITE (PlainRunnerTest)
  BOOST_AUTO_TEST_CASE(EvalGlobally) {
    Graph graph = Graph::from_ltl_formula("G(p0)", 1, true);
    const ArithHomFA::SealConfig config = {
        8192,                         // poly_modulus_degree
        std::vector<int>{60, 40, 60}, // base_sizes
        std::pow(2, 40)               // scale
    };
    ArithHomFA::PlainRunner runner{config, graph};
    std::vector<double> input = {100, 90, 80, 70, 60, 80, 90};
    std::vector<bool> expected = {true, true, true, false, false, false, false};
    for (std::size_t i = 0; i < input.size(); ++i) {
      BOOST_CHECK_EQUAL(expected.at(i), runner.feed({input.at(i)}));
    }
  }
BOOST_AUTO_TEST_SUITE_END()
