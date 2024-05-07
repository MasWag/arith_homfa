#include <boost/test/unit_test.hpp>
#include <sstream>

#include "archive.hpp"
#include "bootstrapping_key.hh"
#include "ckks_to_tfhe.hh"
#include "seal_config.hh"
#include "tfhe++.hpp"

BOOST_AUTO_TEST_SUITE(TFHEppKeygenTest)

  BOOST_AUTO_TEST_CASE(genkey) {
    std::stringstream stream;
    TFHEpp::SecretKey skey;
    write_to_archive(stream, skey);
    auto loaded = read_from_archive<TFHEpp::SecretKey>(stream);
    BOOST_TEST((skey.params == loaded.params));
  }

  BOOST_AUTO_TEST_CASE(genbkey) {
    std::stringstream stream;
    TFHEpp::SecretKey skey;
    const auto scale = std::pow(2, 40);
    const ArithHomFA::SealConfig config = {
        8192,                         // poly_modulus_degree
        std::vector<int>{60, 40, 60}, // base_sizes
        scale                         // scale
    };
    const auto &context = config.makeContext();
    seal::KeyGenerator keygen(context);
    const auto &sealKey = keygen.secret_key();

    // CKKSToTFHE is necessary to make lvl3Key
    ArithHomFA::CKKSToTFHE converter(context);
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(sealKey, lvl3Key);
    ArithHomFA::BootstrappingKey bkey(skey, lvl3Key);

    write_to_archive(stream, bkey);
    auto loaded = read_from_archive<ArithHomFA::BootstrappingKey>(stream);
    BOOST_TEST(bkey.kskh2m->front().front().front() == loaded.kskh2m->front().front().front(),
               boost::test_tools::per_element());
    BOOST_TEST(bkey.kskm2l->front().front().front() == loaded.kskm2l->front().front().front(),
               boost::test_tools::per_element());
    BOOST_TEST(bkey.bkfft->front().front().front().front() == loaded.bkfft->front().front().front().front(),
               boost::test_tools::per_element());
  }
BOOST_AUTO_TEST_SUITE_END()
