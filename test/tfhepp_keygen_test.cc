#include <boost/test/unit_test.hpp>
#include <sstream>

#include "tfhe++.hpp"
#include "archive.hpp"

BOOST_AUTO_TEST_SUITE(TFHEppKeygenTest)

  BOOST_AUTO_TEST_CASE(genkey) {
    std::stringstream stream;
    TFHEpp::SecretKey skey;
    write_to_archive(stream, skey);
    auto loaded = read_from_archive<TFHEpp::SecretKey>(stream);
    BOOST_TEST((skey.params == loaded.params));
  }

BOOST_AUTO_TEST_SUITE_END()
