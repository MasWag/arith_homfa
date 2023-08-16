#include <sstream>

#include <boost/test/unit_test.hpp>

#include "archive.hpp"
#include "tfhe++.hpp"

#include "secret_key.hh"

BOOST_AUTO_TEST_SUITE(SecretKeyTest)

  BOOST_AUTO_TEST_CASE(writeAndRead) {
    std::stringstream stream;
    ArithHomFA::SecretKey generatedKey;

    write_to_archive(stream, generatedKey);
    auto loadedKey = read_from_archive<ArithHomFA::SecretKey>(stream);

    BOOST_TEST(generatedKey.key.lvl0 == loadedKey.key.lvl0, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl1 == loadedKey.key.lvl1, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl2 == loadedKey.key.lvl2, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.lvlhalfkey == loadedKey.lvlhalfkey, boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(writeAndReadViaFile) {
    const auto keyFilename = std::tmpnam(nullptr);
    ArithHomFA::SecretKey generatedKey;

    {
      std::ofstream stream{keyFilename};
      write_to_archive(stream, generatedKey);
    }
    std::ifstream stream{keyFilename};
    auto loadedKey = read_from_archive<ArithHomFA::SecretKey>(stream);

    BOOST_TEST(generatedKey.key.lvl0 == loadedKey.key.lvl0, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl1 == loadedKey.key.lvl1, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl2 == loadedKey.key.lvl2, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.lvlhalfkey == loadedKey.lvlhalfkey, boost::test_tools::per_element());
  }

BOOST_AUTO_TEST_SUITE_END()
