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

    const auto generatedLvl0 = generatedKey.key.get<TFHEpp::lvl0param>();
    const auto generatedLvl1 = generatedKey.key.get<TFHEpp::lvl1param>();
    const auto generatedLvl2 = generatedKey.key.get<TFHEpp::lvl2param>();
    const auto generatedLvlHalf = generatedKey.key.get<TFHEpp::lvlhalfparam>();
    const auto loadedLvl0 = loadedKey.key.get<TFHEpp::lvl0param>();
    const auto loadedLvl1 = loadedKey.key.get<TFHEpp::lvl1param>();
    const auto loadedLvl2 = loadedKey.key.get<TFHEpp::lvl2param>();
    const auto loadedLvlHalf = loadedKey.key.get<TFHEpp::lvlhalfparam>();
    BOOST_TEST(generatedLvl0 == loadedLvl0, boost::test_tools::per_element());
    BOOST_TEST(generatedLvl1 == loadedLvl1, boost::test_tools::per_element());
    BOOST_TEST(generatedLvl2 == loadedLvl2, boost::test_tools::per_element());
    BOOST_TEST(generatedLvlHalf == loadedLvlHalf, boost::test_tools::per_element());
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

    const auto fileGeneratedLvl0 = generatedKey.key.get<TFHEpp::lvl0param>();
    const auto fileGeneratedLvl1 = generatedKey.key.get<TFHEpp::lvl1param>();
    const auto fileGeneratedLvl2 = generatedKey.key.get<TFHEpp::lvl2param>();
    const auto fileGeneratedLvlHalf = generatedKey.key.get<TFHEpp::lvlhalfparam>();
    const auto fileLoadedLvl0 = loadedKey.key.get<TFHEpp::lvl0param>();
    const auto fileLoadedLvl1 = loadedKey.key.get<TFHEpp::lvl1param>();
    const auto fileLoadedLvl2 = loadedKey.key.get<TFHEpp::lvl2param>();
    const auto fileLoadedLvlHalf = loadedKey.key.get<TFHEpp::lvlhalfparam>();
    BOOST_TEST(fileGeneratedLvl0 == fileLoadedLvl0, boost::test_tools::per_element());
    BOOST_TEST(fileGeneratedLvl1 == fileLoadedLvl1, boost::test_tools::per_element());
    BOOST_TEST(fileGeneratedLvl2 == fileLoadedLvl2, boost::test_tools::per_element());
    BOOST_TEST(fileGeneratedLvlHalf == fileLoadedLvlHalf, boost::test_tools::per_element());
  }

BOOST_AUTO_TEST_SUITE_END()
