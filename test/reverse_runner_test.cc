/**
 * @author Masaki Waga
 * @date 2023/06/23.
 */

#include <boost/test/unit_test.hpp>

#include "archive.hpp"

#include "../src/reverse_runner.hh"

BOOST_AUTO_TEST_SUITE(ReverseRunnerTest)

  struct CKKSConfigFixture {
    const double scale = std::pow(2, 40);
    const ArithHomFA::SealConfig config = {
        8192,                         // poly_modulus_degree
        std::vector<int>{60, 40, 60}, // base_sizes
        scale                         // scale
    };
    const seal::SEALContext context = config.makeContext();
    ArithHomFA::CKKSToTFHE converter{context};
    ArithHomFA::CKKSNoEmbedEncoder encoder{context};
  };

  BOOST_AUTO_TEST_CASE(EvalGlobally) {
    Graph graph = Graph::from_ltl_formula("G(p0)", 1, true);
    const auto scale = std::pow(2, 40);
    const ArithHomFA::SealConfig config = {
        8192,                         // poly_modulus_degree
        std::vector<int>{60, 40, 60}, // base_sizes
        scale                         // scale
    };
    const auto &context = config.makeContext();

    // Make keys
    seal::KeyGenerator keygen(context);
    const auto &sealKey = keygen.secret_key();
    ArithHomFA::SecretKey skey;
    // CKKSToTFHE is necessary to make lvl3Key
    ArithHomFA::CKKSToTFHE converter(context);
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(sealKey, lvl3Key);
    ArithHomFA::BootstrappingKey bkey(skey, lvl3Key);

    // Instantiate encoder and encryptor
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, sealKey);

    std::vector<double> input = {100, 90, 80, 75, 60, 80, 90};
    ArithHomFA::ReverseRunner runner{context, scale, graph, 10, bkey, {1000}};
    std::vector<bool> expected = {true, true, true, true, false, false, false};
    seal::Plaintext plain;
    seal::Ciphertext cipher;
    for (std::size_t i = 0; i < input.size(); ++i) {
      encoder.encode(input.at(i), scale, plain);
      encryptor.encrypt_symmetric(plain, cipher);
      const auto result = decrypt_TLWELvl1_to_bit(runner.feed({cipher}), skey);
      BOOST_CHECK_EQUAL(expected.at(i), result);
    }

    runner.printTime();
  }

  BOOST_FIXTURE_TEST_CASE(EvalGFLongFalse, CKKSConfigFixture, *boost::unit_test::disabled()) {
    Graph graph = Graph::from_ltl_formula("G(p0 -> F[0,25] !p0)", 1, true);

    // Make keys
    seal::KeyGenerator keygen(context);
    const auto &sealKey = keygen.secret_key();
    ArithHomFA::SecretKey skey;
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(sealKey, lvl3Key);
    ArithHomFA::BootstrappingKey bKey(skey, lvl3Key);

    // Instantiate encryptor
    seal::Encryptor encryptor(context, sealKey);

    // Prepare for the runner
    ArithHomFA::ReverseRunner runner{context, scale, graph, 50000, bKey, {150}};

    bool wasFalse = false;
    for (int i = 0; i < 10000; ++i) {
      seal::Plaintext plain;
      encoder.encode(75, scale, plain);
      seal::Ciphertext cipher;
      encryptor.encrypt_symmetric(plain, cipher);
      const auto result = decrypt_TLWELvl1_to_bit(runner.feed({cipher}), skey);
      wasFalse = wasFalse || !result;
      if (i % 1000 == 0) {
        std::cout << i << " " << (wasFalse ? "wasFalse" : "not wasFalse") << std::endl;
      }
      BOOST_TEST(!(wasFalse && result));
    }

    runner.printTime();
  }

  BOOST_FIXTURE_TEST_CASE(EvalGFLongTrue, CKKSConfigFixture, *boost::unit_test::disabled()) {
    Graph graph = Graph::from_ltl_formula("G(p0 -> F[0,25] !p0)", 1, true);

    // Make keys
    seal::KeyGenerator keygen(context);
    const auto &sealKey = keygen.secret_key();
    ArithHomFA::SecretKey skey;
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(sealKey, lvl3Key);
    ArithHomFA::BootstrappingKey bKey(skey, lvl3Key);

    // Instantiate encryptor
    seal::Encryptor encryptor(context, sealKey);

    // Prepare for the runner
    ArithHomFA::ReverseRunner runner{context, scale, graph, 50000, bKey, {150}};

    bool wasFalse = false;
    for (int i = 0; i < 10000; ++i) {
      seal::Plaintext plain;
      encoder.encode(65, scale, plain);
      seal::Ciphertext cipher;
      encryptor.encrypt_symmetric(plain, cipher);
      const auto result = decrypt_TLWELvl1_to_bit(runner.feed({cipher}), skey);
      wasFalse = wasFalse || !result;
      if (i % 1000 == 0) {
        std::cout << i << " " << (wasFalse ? "wasFalse" : "not wasFalse") << std::endl;
      }
      BOOST_TEST(!(wasFalse && result));
    }

    runner.printTime();
  }

  // This test case requires files containing keys. Since thay are huge and we do not include in the repository, we
  // disable this test by default.
  BOOST_FIXTURE_TEST_CASE(EvalGloballyFromFiles, CKKSConfigFixture, *boost::unit_test::disabled()) {
    Graph graph = Graph::from_ltl_formula("G(p0)", 1, true);

    // Load keys
    seal::SecretKey sealKey;
    {
      std::ifstream sealKeyStream("../examples/ckks.key");
      sealKey.load(context, sealKeyStream);
    }
    const auto skey = read_from_archive<ArithHomFA::SecretKey>("../examples/tfhe.key");
    const auto bkey = read_from_archive<ArithHomFA::BootstrappingKey>("../examples/tfhe.bkey");

    // Instantiate encryptor
    seal::Encryptor encryptor(context, sealKey);

    std::vector<double> input = {100, 90, 80, 75, 60, 80, 90};
    ArithHomFA::ReverseRunner runner{context, scale, graph, 10, bkey, {1000}};
    std::vector<bool> expected = {true, true, true, true, false, false, false};
    seal::Plaintext plain;
    seal::Ciphertext cipher;
    for (std::size_t i = 0; i < input.size(); ++i) {
      encoder.encode(input.at(i), scale, plain);
      encryptor.encrypt_symmetric(plain, cipher);
      const auto result = decrypt_TLWELvl1_to_bit(runner.feed({cipher}), skey);
      BOOST_CHECK_EQUAL(expected.at(i), result);
    }

    runner.printTime();
  }
BOOST_AUTO_TEST_SUITE_END()
