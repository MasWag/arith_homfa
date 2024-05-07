#include <cstdio>
#include <filesystem>
#include <valarray>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "../src/ahomfa_runner.hh"
#include "../src/ckks_no_embed.hh"
#include "../src/ckks_predicate.hh"
#include "../src/ckks_to_tfhe.hh"
#include "../src/sized_cipher_reader.hh"
#include "../src/sized_cipher_writer.hh"
#include "../src/sized_tlwe_reader.hh"
#include "../src/sized_tlwe_writer.hh"

BOOST_AUTO_TEST_SUITE(PointwiseRunnerTest)

  struct CKKSToTFHEFixture {
    const double scale = std::pow(2.0, 40);
    double minValue = 0.001;
    std::vector<seal::EncryptionParameters> parms;
    const std::size_t log2_poly_modulus_degree = TFHEpp::lvl3param::nbit;
    const std::size_t poly_modulus_degree = 1 << log2_poly_modulus_degree;
    std::vector<seal::Modulus> smallModulus;
    std::vector<seal::Modulus> largeModulus;
    seal::Plaintext plain;
    std::vector<seal::SEALContext> contexts;

    CKKSToTFHEFixture() {
      parms.emplace_back(seal::scheme_type::ckks);
      parms.emplace_back(seal::scheme_type::ckks);
      parms.front().set_poly_modulus_degree(poly_modulus_degree);
      parms.back().set_poly_modulus_degree(poly_modulus_degree);
      smallModulus = seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 60});
      largeModulus = seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60});
      parms.front().set_coeff_modulus(smallModulus);
      parms.back().set_coeff_modulus(largeModulus);
      contexts.emplace_back(parms.front());
      contexts.emplace_back(parms.back());
    }
  };

  RC_BOOST_FIXTURE_PROP(pointwiseTFHE, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();
    seal::RelinKeys relinKeys;
    keygens.at(useLargerParam).create_relin_keys(relinKeys);

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);

    // Convert the key for TFHEpp
    static std::array<ArithHomFA::CKKSToTFHE, 2> converters{ArithHomFA::CKKSToTFHE(contexts.front()),
                                                            ArithHomFA::CKKSToTFHE(contexts.back())};
    static std::vector<TFHEpp::Key<TFHEpp::lvl3param>> lvl3Keys;
    if (lvl3Keys.empty()) {
      lvl3Keys.resize(2);
      for (int i = 0; i < 2; ++i) {
        converters.at(i).toLv3Key(keygens.at(i).secret_key(), lvl3Keys.at(i));
      }
    }

    // Set up the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front());
      bootKeys.emplace_back(skey, lvl3Keys.back());
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }
    const auto &bkey = bootKeys.at(useLargerParam);

    // Evaluate the predicate
    std::vector<seal::Ciphertext> valuation, result;
    ArithHomFA::CKKSPredicate predicate{context, scale};

    // Define the stream
    std::stringstream inputStream, outputStream;
    ArithHomFA::SizedCipherReader inputReader(inputStream);
    ArithHomFA::SizedCipherWriter inputWriter(inputStream);
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> outputWriter(outputStream);
    ArithHomFA::SizedTLWEReader<TFHEpp::lvl1param> outputReader(outputStream);

    // Randomly generate the original values and the ciphertexts
    std::array<double, 10> values{};
    for (auto &value: values) {
      // We require that the given value is in a certain range. Otherwise, the decryption fails.
      const auto intValue =
          *rc::gen::inRange<int64_t>(static_cast<int64_t>(74.0 / minValue), static_cast<int64_t>(74.5 / minValue));
      value = static_cast<double>(intValue) * minValue;
      RC_PRE(value != 0);
      encoder.encode(value, scale, plain);
      seal::Ciphertext cipher;
      encryptor.encrypt_symmetric(plain, cipher);
      inputWriter.write(cipher);
    }

    // Execute the pointwise predicate evaluation
    ArithHomFA::PointwiseRunner::runPointwiseTFHE(context, predicate, bkey, relinKeys, inputReader, outputWriter);

    // Assert the result
    for (auto &value: values) {
      TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
      outputReader.read(tlwe);
      const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(tlwe, skey.key.lvl1);
      RC_ASSERT(tlwePlain == (value > 70));
    }
  }

  RC_BOOST_FIXTURE_PROP(pointwiseTFHEViaFile, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();
    seal::RelinKeys relinKeys;
    keygens.at(useLargerParam).create_relin_keys(relinKeys);

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);

    // Convert the key for TFHEpp
    static std::array<ArithHomFA::CKKSToTFHE, 2> converters{ArithHomFA::CKKSToTFHE(contexts.front()),
                                                            ArithHomFA::CKKSToTFHE(contexts.back())};
    static std::vector<TFHEpp::Key<TFHEpp::lvl3param>> lvl3Keys;
    if (lvl3Keys.empty()) {
      lvl3Keys.resize(2);
      for (int i = 0; i < 2; ++i) {
        converters.at(i).toLv3Key(keygens.at(i).secret_key(), lvl3Keys.at(i));
      }
    }

    // Set up the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front());
      bootKeys.emplace_back(skey, lvl3Keys.back());
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }
    const auto &bkey = bootKeys.at(useLargerParam);

    // Evaluate the predicate
    std::vector<seal::Ciphertext> valuation, result;
    ArithHomFA::CKKSPredicate predicate{context, scale};

    // Define the stream
    const auto inputFilename = std::tmpnam(nullptr);
    const auto outputFilename = std::tmpnam(nullptr);
    std::ofstream inputWStream{inputFilename};
    std::ifstream inputRStream{inputFilename};
    std::ofstream outputWStream{outputFilename};
    std::ifstream outputRStream{outputFilename};
    ArithHomFA::SizedCipherReader inputReader(inputRStream);
    ArithHomFA::SizedCipherWriter inputWriter(inputWStream);
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> outputWriter(outputWStream);
    ArithHomFA::SizedTLWEReader<TFHEpp::lvl1param> outputReader(outputRStream);

    // Randomly generate the original values and the ciphertexts
    std::array<double, 10> values{};
    for (auto &value: values) {
      // We require that the given value is in a certain range. Otherwise, the decryption fails.
      const auto intValue =
          *rc::gen::inRange<int64_t>(static_cast<int64_t>(74.0 / minValue), static_cast<int64_t>(74.5 / minValue));
      value = static_cast<double>(intValue) * minValue;
      RC_PRE(value != 0);
      encoder.encode(value, scale, plain);
      seal::Ciphertext cipher;
      encryptor.encrypt_symmetric(plain, cipher);
      inputWriter.write(cipher);
    }

    // Execute the pointwise predicate evaluation
    ArithHomFA::PointwiseRunner::runPointwiseTFHE(context, predicate, bkey, relinKeys, inputReader, outputWriter);

    // Assert the result
    for (auto &value: values) {
      TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
      outputReader.read(tlwe);
      const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(tlwe, skey.key.lvl1);
      RC_ASSERT(tlwePlain == (value > 70));
    }
    std::remove(inputFilename);
    std::remove(outputFilename);
  }

  BOOST_FIXTURE_TEST_CASE(BG1FromPlain, CKKSToTFHEFixture) {
    const bool useLargerParam = false;
    std::filesystem::path inputPath = std::filesystem::path{PROJECT_ROOT_DIR}.append("test").append("adult#001_night.bg.txt");
    std::ifstream istream(inputPath.c_str());
    BOOST_REQUIRE(istream.good());

    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();
    seal::RelinKeys relinKeys;
    keygens.at(useLargerParam).create_relin_keys(relinKeys);

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);

    // Convert the key for TFHEpp
    static std::array<ArithHomFA::CKKSToTFHE, 2> converters{ArithHomFA::CKKSToTFHE(contexts.front()),
                                                            ArithHomFA::CKKSToTFHE(contexts.back())};
    static std::vector<TFHEpp::Key<TFHEpp::lvl3param>> lvl3Keys;
    if (lvl3Keys.empty()) {
      lvl3Keys.resize(2);
      for (int i = 0; i < 2; ++i) {
        converters.at(i).toLv3Key(keygens.at(i).secret_key(), lvl3Keys.at(i));
      }
    }

    // Set up the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front());
      bootKeys.emplace_back(skey, lvl3Keys.back());
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }
    const auto &bkey = bootKeys.at(useLargerParam);

    // Evaluate the predicate
    std::vector<seal::Ciphertext> valuation, result;
    ArithHomFA::CKKSPredicate predicate{context, scale};

    // Define the stream
    std::stringstream inputStream, outputStream;
    ArithHomFA::SizedCipherReader inputReader(inputStream);
    ArithHomFA::SizedCipherWriter inputWriter(inputStream);
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> outputWriter(outputStream);
    ArithHomFA::SizedTLWEReader<TFHEpp::lvl1param> outputReader(outputStream);

    // Randomly generate the original values and the ciphertexts
    std::vector<double> values;
    while (istream.good()) {
      double value;
      istream >> value;
      encoder.encode(value, scale, plain);
      values.push_back(value);
      seal::Ciphertext cipher;
      encryptor.encrypt_symmetric(plain, cipher);
      inputWriter.write(cipher);
    }

    // Execute the pointwise predicate evaluation
    ArithHomFA::PointwiseRunner::runPointwiseTFHE(context, predicate, bkey, relinKeys, inputReader, outputWriter);

    // Assert the result
    for (auto &value: values) {
      TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
      outputReader.read(tlwe);
      const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(tlwe, skey.key.lvl1);
      BOOST_TEST(tlwePlain == (value > 70));
    }
  }

BOOST_AUTO_TEST_SUITE_END()
