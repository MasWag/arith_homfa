#include <valarray>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "../src/ckks_no_embed.hh"
#include "../src/ckks_predicate.hh"
#include "../src/ckks_to_tfhe.hh"

BOOST_AUTO_TEST_SUITE(CKKSToTFHETest)

  BOOST_AUTO_TEST_CASE(toLv3Key) {
    seal::EncryptionParameters parms(seal::scheme_type::ckks);
    const std::size_t log2_poly_modulus_degree = TFHEpp::lvl3param::nbit;
    const std::size_t poly_modulus_degree = 1 << log2_poly_modulus_degree;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 60}));
    seal::SEALContext context(parms);

    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();

    // Convert the key for TFHEpp
    ArithHomFA::CKKSToTFHE converter(context);
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(secretKey, lvl3Key);

    // Check the key length
    BOOST_CHECK_EQUAL(lvl3Key.size(), poly_modulus_degree);
    // Check that the generated key is ternary
    for (const auto &coefficient: lvl3Key) {
      BOOST_TEST((coefficient == -1 || coefficient == 0 || coefficient == 1));
    }
  }

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

  RC_BOOST_FIXTURE_PROP(amplify, CKKSToTFHEFixture, (const int32_t &intValue, const bool &useLargerParam)) {
    RC_PRE(intValue != 0);
    const double &value = static_cast<double>(intValue) * minValue;

    seal::SEALContext context = parms.at(useLargerParam);

    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();

    // Encrypt the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    seal::Decryptor decryptor(context, secretKey);
    seal::Ciphertext cipher;
    encoder.encode(value, scale, plain);
    encryptor.encrypt_symmetric(plain, cipher);

    // Assert the signature before amplification
    {
      decryptor.decrypt(cipher, plain);
      const auto result = encoder.decode(plain);
      // Encryption/decryption should not change the signature
      RC_ASSERT((result > 0) == (value > 0));
    }

    // Amplify the ciphertext
    ArithHomFA::CKKSToTFHE converter(context);
    converter.amplify(cipher, INT32_MAX * minValue);
    {
      decryptor.decrypt(cipher, plain);
      const auto result = encoder.decode(plain);
      // Amplification should not change the signature
      RC_ASSERT((result > 0) == (value > 0));
    }
  }

  RC_BOOST_FIXTURE_PROP(amplifyAfterRescale, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    const double threshold = std::pow(2, 18);
    const auto intValue = *rc::gen::inRange<int64_t>(static_cast<int64_t>(-threshold), static_cast<int64_t>(threshold));
    RC_PRE(intValue != 0);
    const double &value = static_cast<double>(intValue) * minValue;

    seal::SEALContext context = parms.at(useLargerParam);

    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();
    seal::RelinKeys relinKeys;
    keygen.create_relin_keys(relinKeys);

    // Encrypt the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    seal::Decryptor decryptor(context, secretKey);
    seal::Ciphertext cipher, absCipher;
    encoder.encode(value, scale, plain);
    encryptor.encrypt_symmetric(plain, cipher);
    encoder.encode(std::abs(value), scale, plain);
    encryptor.encrypt_symmetric(plain, absCipher);

    // Before rescaling, we increase the scale by multiplication
    seal::Evaluator evaluator(context);
    evaluator.multiply_inplace(cipher, absCipher);
    evaluator.relinearize_inplace(cipher, relinKeys);
    evaluator.rescale_to_next_inplace(cipher);

    // Assert the signature before amplification
    {
      decryptor.decrypt(cipher, plain);
      const auto result = encoder.decode(plain);
      // Encryption/decryption should not change the signature
      RC_ASSERT((result > 0) == (value > 0));
    }

    // Amplify the ciphertext
    ArithHomFA::CKKSToTFHE converter(context);
    converter.amplify(cipher, threshold * threshold * minValue * minValue);
    {
      decryptor.decrypt(cipher, plain);
      const auto result = encoder.decode(plain);
      // Amplification should not change the signature
      RC_ASSERT((result > 0) == (value > 0));
    }
  }

  RC_BOOST_FIXTURE_PROP(toLv3TRLWE, CKKSToTFHEFixture, (const int32_t &intValue, const bool &useLargerParam)) {
    // We implicitly require that the given value is not too large. Otherwise, the encoding fails.
    const double &value = static_cast<double>(intValue) * minValue;
    RC_PRE(value != 0);

    seal::SEALContext context = parms.at(useLargerParam);

    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();

    // Encrypt the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    encoder.encode(value, scale, plain);
    seal::Ciphertext cipher;
    encryptor.encrypt_symmetric(plain, cipher);

    // Convert the key for TFHEpp
    ArithHomFA::CKKSToTFHE converter(context);
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(secretKey, lvl3Key);

    // Convert the ciphertext for TFHEpp
    TFHEpp::TRLWE<TFHEpp::lvl3param> trlwe;
    converter.toLv3TRLWE(cipher, trlwe, INT32_MAX * minValue);

    // Decrypt the TRLWE with TFHEpp
    const auto trlwePlain = TFHEpp::trlweSymDecrypt<TFHEpp::lvl3param>(trlwe, lvl3Key);

    // Assert the result
    RC_ASSERT(trlwePlain.front() == (value > 0));
  }

  RC_BOOST_FIXTURE_PROP(toLv3TLWE, CKKSToTFHEFixture, (const int32_t &intValue, const bool &useLargerParam)) {
    // We implicitly require that the given value is not too large. Otherwise, the encoding fails.
    const double &value = static_cast<double>(intValue) * minValue;
    RC_PRE(value != 0);

    seal::SEALContext context = parms.at(useLargerParam);

    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();

    // Encrypt the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    encoder.encode(value, scale, plain);
    seal::Ciphertext cipher;
    encryptor.encrypt_symmetric(plain, cipher);

    // Convert the key for TFHEpp
    ArithHomFA::CKKSToTFHE converter(context);
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    converter.toLv3Key(secretKey, lvl3Key);

    // Convert the ciphertext for TFHEpp
    TFHEpp::TLWE<TFHEpp::lvl3param> tlwe;
    converter.toLv3TLWE(cipher, tlwe, INT32_MAX * minValue);

    // Decrypt the TLWE with TFHEpp
    const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl3param>(tlwe, lvl3Key);

    // Assert the result
    RC_ASSERT(tlwePlain == (value > 0));
  }

  template <class Param> static TFHEpp::Key<Param> keyGen(std::uniform_int_distribution<int32_t> & generator) {
    TFHEpp::Key<Param> key;
    for (typename Param::T &i: key) {
      i = generator(TFHEpp::generator);
    }

    return key;
  }

  RC_BOOST_FIXTURE_PROP(toLv1TLWE, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    // We require that the given value is not too large. Otherwise, the encoding fails.
    const auto threshold = static_cast<int64_t>(std::pow(2, 40));
    const auto intValue = *rc::gen::inRange<int64_t>(-threshold, threshold);
    const double &value = static_cast<double>(intValue) * minValue;
    RC_PRE(value != 0);

    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    encoder.encode(value, scale, plain);
    seal::Ciphertext cipher;
    encryptor.encrypt_symmetric(plain, cipher);

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
    auto &converter = converters.at(useLargerParam);

    // Set up the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    std::uniform_int_distribution<int32_t> lvlhalfgen(0, 1);
    static const TFHEpp::Key<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP> lvlhalfkey{
        keyGen<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP>(lvlhalfgen)};
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front(), lvlhalfkey);
      bootKeys.emplace_back(skey, lvl3Keys.back(), lvlhalfkey);
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }

    // Convert the ciphertext for TFHEpp (lvl1)
    TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
    converter.toLv1TLWE(cipher, tlwe, threshold * minValue);

    // Decrypt the TLWE with TFHEpp
    const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(tlwe, skey.key.lvl1);

    // Assert the result
    RC_ASSERT(tlwePlain == (value > 0));
  }

  // This test case requires quite some RAM and may be terminated by OOM Killer.
#if 0
  RC_BOOST_FIXTURE_PROP(toLv1TLWEAfterMultiplication, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    // We require that the given value is not too large.
    const double threshold = std::pow(2, 17);
    const auto gen = rc::gen::inRange<int64_t>(static_cast<int64_t>(-threshold), static_cast<int64_t>(threshold));
    std::valarray<double> values = {static_cast<double>(*gen), static_cast<double>(*gen)};
    values *= minValue;
    RC_PRE(values[0] != 0);
    RC_PRE(values[1] != 0);

    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();
    static std::vector<seal::RelinKeys> relinKeysV;
    if (relinKeysV.empty()) {
      relinKeysV.resize(2);
      keygens.at(0).create_relin_keys(relinKeysV.at(0));
      keygens.at(1).create_relin_keys(relinKeysV.at(1));
    }
    const seal::RelinKeys &relinKeys = relinKeysV.at(useLargerParam);

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    std::array<seal::Ciphertext, 2> ciphers;
    for (int i = 0; i < 2; ++i) {
      encoder.encode(values[i], scale, plain);
      encryptor.encrypt_symmetric(plain, ciphers.at(i));
    }

    // Multiply the values
    seal::Evaluator evaluator(context);
    evaluator.multiply_inplace(ciphers.at(0), ciphers.at(1));
    evaluator.relinearize_inplace(ciphers.at(0), relinKeys);
    evaluator.rescale_to_next_inplace(ciphers.at(0));

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
    auto &converter = converters.at(useLargerParam);

    // Setup the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    std::uniform_int_distribution<int32_t> lvlhalfgen(0, 1);
    static const TFHEpp::Key<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP> lvlhalfkey{
        keyGen<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP>(lvlhalfgen)};
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front(), lvlhalfkey);
      bootKeys.emplace_back(skey, lvl3Keys.back(), lvlhalfkey);
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }
    ArithHomFA::Lvl3ToLvl1 ThreeToOne(bootKeys.at(useLargerParam));

    // Convert the ciphertext for TFHEpp (lvl1)
    TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
    converter.toLv1TLWE(ciphers.at(0), tlwe, threshold * threshold * minValue * minValue);

    // Decrypt the TLWE with TFHEpp
    const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(tlwe, skey.key.lvl1);

    // Assert the result
    RC_ASSERT(tlwePlain == (values[0] * values[1] > 0));
  }

  RC_BOOST_FIXTURE_PROP(toLv1TRGSW, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    // We require that the given value is not too large. Otherwise, the encoding fails.
    const auto threshold = std::pow(2, 40);
    const auto intValue = *rc::gen::inRange<int64_t>(static_cast<int64_t>(-threshold), static_cast<int64_t>(threshold));
    const double &value = static_cast<double>(intValue) * minValue;
    RC_PRE(value != 0);

    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    encoder.encode(value, scale, plain);
    seal::Ciphertext cipher;
    encryptor.encrypt_symmetric(plain, cipher);

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
    auto &converter = converters.at(useLargerParam);

    // Setup the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    std::uniform_int_distribution<int32_t> lvlhalfgen(0, 1);
    static const TFHEpp::Key<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP> lvlhalfkey{
        keyGen<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP>(lvlhalfgen)};
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front(), lvlhalfkey);
      bootKeys.emplace_back(skey, lvl3Keys.back(), lvlhalfkey);
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }
    ArithHomFA::Lvl3ToLvl1 ThreeToOne(bootKeys.at(useLargerParam));

    // Convert the ciphertext for TFHEpp (lvl1)
    TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
    converter.toLv1TLWE(cipher, tlwe, threshold * minValue);
    TFHEpp::TRGSWFFT<TFHEpp::lvl1param> trgsw;
    TFHEpp::CircuitBootstrappingFFT<TFHEpp::lvl10param, TFHEpp::lvl02param, TFHEpp::lvl21param>(
        trgsw, tlwe, *bootKeys.at(useLargerParam).ekey);

    // Obtain the boolean value with CMux
    TFHEpp::TRLWE<TFHEpp::lvl1param> result, one{}, zero{};
    zero.at(1).front() = -1u << (std::numeric_limits<TFHEpp::lvl1param::T>::digits - 2);
    one.at(1).front() = 1u << (std::numeric_limits<TFHEpp::lvl1param::T>::digits - 2);
    TFHEpp::CMUXFFT<TFHEpp::lvl1param>(result, trgsw, one, zero);

    // Assert the result
    const auto resultPlain = TFHEpp::trlweSymDecrypt<TFHEpp::lvl1param>(result, skey.key.lvl1).front();
    RC_ASSERT(resultPlain == (value > 0));
  }
#endif

  RC_BOOST_FIXTURE_PROP(toLv1TLWEAfterEval, CKKSToTFHEFixture, (const bool &useLargerParam)) {
    // The upper bound the range of the input signal
    const double maxValue = 300.0;
    const double reference = maxValue - 70.0;
    // We require that the given value is in a certain range. Otherwise, the decryption fails.
    const auto intValue =
        *rc::gen::inRange<int64_t>(static_cast<int64_t>(71.5 / minValue), static_cast<int64_t>(72.0 / minValue));
    const double value = static_cast<double>(intValue) * minValue;
    RC_PRE(value != 0);

    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();

    // Encrypt the given value
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    encoder.encode(value, scale, plain);
    seal::Ciphertext cipher;
    encryptor.encrypt_symmetric(plain, cipher);

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
    auto &converter = converters.at(useLargerParam);

    // Set up the BootstrappingKey
    static const TFHEpp::SecretKey skey;
    std::uniform_int_distribution<int32_t> lvlhalfgen(0, 1);
    static const TFHEpp::Key<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP> lvlhalfkey {
        keyGen<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP>(lvlhalfgen)};
    static std::vector<ArithHomFA::BootstrappingKey> bootKeys;
    if (bootKeys.empty()) {
      // Construct bootKeys
      bootKeys.emplace_back(skey, lvl3Keys.front(), lvlhalfkey);
      bootKeys.emplace_back(skey, lvl3Keys.back(), lvlhalfkey);
      converters.front().initializeConverter(bootKeys.front());
      converters.back().initializeConverter(bootKeys.back());
    }

    // Evaluate the predicate
    std::vector<seal::Ciphertext> valuation, result;
    ArithHomFA::CKKSPredicate predicate{context, scale};
    valuation.resize(1);
    valuation.front() = cipher;
    result.resize(1);
    predicate.eval(valuation, result);

    // Convert the result to TFHEpp (lvl1)
    TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
    converter.toLv1TLWE(result.front(), tlwe, reference);

    // Decrypt the TLWE with TFHEpp
    const auto tlwePlain = TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(tlwe, skey.key.lvl1);

    // Assert the result
    RC_ASSERT(tlwePlain == (value > 70));
  }

BOOST_AUTO_TEST_SUITE_END()
