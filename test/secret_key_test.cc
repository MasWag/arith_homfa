#include <sstream>

#include <boost/test/unit_test.hpp>

#include "archive.hpp"
#include "tfhe++.hpp"

BOOST_AUTO_TEST_SUITE(SecretKeyTest)

  BOOST_AUTO_TEST_CASE(writeAndRead) {
    std::stringstream stream;
    TFHEpp::SecretKey generatedKey;

    write_to_archive(stream, generatedKey);
    auto loadedKey = read_from_archive<TFHEpp::SecretKey>(stream);

    BOOST_TEST(generatedKey.key.lvl0 == loadedKey.key.lvl0, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl1 == loadedKey.key.lvl1, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl2 == loadedKey.key.lvl2, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvlhalf == loadedKey.key.lvlhalf, boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(writeAndReadViaFile) {
    const auto keyFilename = std::tmpnam(nullptr);
    TFHEpp::SecretKey generatedKey;

    {
      std::ofstream stream{keyFilename};
      write_to_archive(stream, generatedKey);
    }
    std::ifstream stream{keyFilename};
    auto loadedKey = read_from_archive<TFHEpp::SecretKey>(stream);

    BOOST_TEST(generatedKey.key.lvl0 == loadedKey.key.lvl0, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl1 == loadedKey.key.lvl1, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvl2 == loadedKey.key.lvl2, boost::test_tools::per_element());
    BOOST_TEST(generatedKey.key.lvlhalf == loadedKey.key.lvlhalf, boost::test_tools::per_element());
  }

BOOST_AUTO_TEST_SUITE_END()
