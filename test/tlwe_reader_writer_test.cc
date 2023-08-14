#include <sstream>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "tfhe++.hpp"

#include "sized_tlwe_reader.hh"
#include "sized_tlwe_writer.hh"

BOOST_AUTO_TEST_SUITE(TLWEReaderWriterTest)

  RC_BOOST_PROP(writeAndRead, (const std::vector<TFHEpp::TLWE<TFHEpp::lvl1param>> &given)) {
    std::stringstream stream;
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> writer{stream};
    ArithHomFA::SizedTLWEReader<TFHEpp::lvl1param> reader{stream};

    for (const auto &tlwe: given) {
      writer.write(tlwe);
    }

    TFHEpp::TLWE<TFHEpp::lvl1param> result;
    for (int i = 0; i < given.size(); ++i) {
      reader.read(result);
      RC_ASSERT(std::equal(given.at(i).begin(), given.at(i).end(), result.begin()));
    }
  }

BOOST_AUTO_TEST_SUITE_END()
