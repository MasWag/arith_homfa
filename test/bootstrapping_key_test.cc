#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <seal/seal.h>

#include "archive.hpp"
#include "tfhe++.hpp"

#include "bootstrapping_key.hh"
#include "ckks_to_tfhe.hh"

BOOST_AUTO_TEST_SUITE(BootstrappingKeyTest)

  // This does not require much RAM but takes quite some time, e.g., 1 hour.
  BOOST_AUTO_TEST_CASE(writeAndRead) {
    // Generate the secret key of TFHEpp
    TFHEpp::SecretKey sKey;

    // Generate the secret key of SEAL
    seal::EncryptionParameters params{seal::scheme_type::ckks};
    const std::size_t log2_poly_modulus_degree = TFHEpp::lvl3param::nbit;
    const std::size_t poly_modulus_degree = 1 << log2_poly_modulus_degree;
    params.set_poly_modulus_degree(poly_modulus_degree);
    params.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 60}));
    seal::SEALContext context{params};
    seal::KeyGenerator keygen{context};
    const auto &secretKey = keygen.secret_key();

    // Construct the lvl3 key
    ArithHomFA::CKKSToTFHE converter{context};
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(secretKey, lvl3Key);

    // Generate the bootstrapping key
    ArithHomFA::BootstrappingKey bKey{sKey, lvl3Key, sKey.key.get<TFHEpp::lvlhalfparam>()};

    std::stringstream stream;
    write_to_archive(stream, bKey);
    auto loadedKey = read_from_archive<ArithHomFA::BootstrappingKey>(stream);

    const auto compareShared = [](const auto &lhs, const auto &rhs) {
      BOOST_REQUIRE(lhs);
      BOOST_REQUIRE(rhs);
      const bool isEqual = (*lhs == *rhs);
      BOOST_TEST(isEqual);
    };

    compareShared(bKey.ekey->get<TFHEpp::KeySwitchingKey<TFHEpp::lvl10param>>(),
                  loadedKey.ekey->get<TFHEpp::KeySwitchingKey<TFHEpp::lvl10param>>());
    compareShared(bKey.ekey->get<TFHEpp::BootstrappingKeyFFT<TFHEpp::lvl01param>>(),
                  loadedKey.ekey->get<TFHEpp::BootstrappingKeyFFT<TFHEpp::lvl01param>>());
    compareShared(bKey.ekey->get<TFHEpp::BootstrappingKeyFFT<TFHEpp::lvl02param>>(),
                  loadedKey.ekey->get<TFHEpp::BootstrappingKeyFFT<TFHEpp::lvl02param>>());

    const auto &privkskLvl21 =
        bKey.ekey->get_map<TFHEpp::PrivateKeySwitchingKey<TFHEpp::lvl21param>>();
    const auto &loadedPrivkskLvl21 =
        loadedKey.ekey->get_map<TFHEpp::PrivateKeySwitchingKey<TFHEpp::lvl21param>>();
    BOOST_TEST(privkskLvl21.size() == loadedPrivkskLvl21.size());
    for (const auto &[label, keyPtr]: privkskLvl21) {
      auto it = loadedPrivkskLvl21.find(label);
      BOOST_REQUIRE(it != loadedPrivkskLvl21.end());
      compareShared(keyPtr, it->second);
    }

    compareShared(bKey.tlwel1_trlwel1_ikskey, loadedKey.tlwel1_trlwel1_ikskey);
    compareShared(bKey.kskh2m, loadedKey.kskh2m);
    compareShared(bKey.kskm2l, loadedKey.kskm2l);
    compareShared(bKey.bkfft, loadedKey.bkfft);
  }

  BOOST_AUTO_TEST_SUITE_END()
