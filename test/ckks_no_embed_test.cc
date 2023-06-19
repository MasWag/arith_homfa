#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "../src/ckks_no_embed.hh"

BOOST_AUTO_TEST_SUITE(CKKSNoEmbedTest)

  struct CKKSNoEmbedTestFixture {
    const double scale = std::pow(2.0, 40);
    const double minValue = 0.001;
    seal::EncryptionParameters parms;
    const std::size_t poly_modulus_degree = 8192;
    std::vector<seal::Modulus> smallModulus;
    std::vector<seal::Modulus> largeModulus;
    seal::Plaintext plain;
    CKKSNoEmbedTestFixture() : parms(seal::scheme_type::ckks) {
      parms.set_poly_modulus_degree(poly_modulus_degree);
      smallModulus = seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 60});
      largeModulus = seal::CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60});
    };
  };

  RC_BOOST_FIXTURE_PROP(encodeAndDecode, CKKSNoEmbedTestFixture,
                        (const int32_t& intValue, const bool& useLargerParam)) {
    // We implicitly require that the given value is not too large. Otherwise, the encoding fails.
    const double &value = static_cast<double>(intValue) * minValue;

    parms.set_coeff_modulus(useLargerParam ? largeModulus : smallModulus);
    seal::SEALContext context(parms);

    // Encode the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    encoder.encode(value, scale, plain);

    // The result may not be exactly the same but should be close
    const double epsilon = 0.0001;
    RC_ASSERT(std::abs(encoder.decode(plain) - value) < epsilon);
  }

  RC_BOOST_FIXTURE_PROP(encodeEncryptDecryptAndDecode, CKKSNoEmbedTestFixture,
                        (const int32_t& intValue, const bool& useLargerParam)) {
    // We implicitly require that the given value is not too large. Otherwise, the encoding fails.
    const double &value = static_cast<double>(intValue) * minValue;

    parms.set_coeff_modulus(useLargerParam ? largeModulus : smallModulus);
    seal::SEALContext context(parms);
    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();

    // Encode the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    seal::Decryptor decryptor(context, secretKey);
    seal::Ciphertext cipher;
    encoder.encode(value, scale, plain);
    encryptor.encrypt_symmetric(plain, cipher);
    decryptor.decrypt(cipher, plain);

    // The result may not be exactly the same but should be close
    const double epsilon = 0.0001;
    RC_ASSERT(std::abs(encoder.decode(plain) - value) < epsilon);
  }

  RC_BOOST_FIXTURE_PROP(encodeEncryptDecryptAndDecodeAfterRescale, CKKSNoEmbedTestFixture,
                        (const int16_t& intValue, const bool& useLargerParam)) {
    // We implicitly require that the given value is not too large. Otherwise, the encoding fails.
    const double &value = static_cast<double>(intValue) * minValue;

    parms.set_coeff_modulus(useLargerParam ? largeModulus : smallModulus);
    seal::SEALContext context(parms);
    // Generate Key
    seal::KeyGenerator keygen(context);
    const auto &secretKey = keygen.secret_key();
    seal::RelinKeys relinKeys;
    keygen.create_relin_keys(relinKeys);

    // Encode the given value
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    seal::Decryptor decryptor(context, secretKey);
    seal::Ciphertext cipher, absCipher;
    encoder.encode(value, scale, plain);
    encryptor.encrypt_symmetric(plain, cipher);
    encoder.encode(std::abs(value), scale, plain);
    encryptor.encrypt_symmetric(plain, absCipher);
    seal::Evaluator evaluator(context);
    evaluator.multiply_inplace(cipher, absCipher);
    evaluator.relinearize_inplace(cipher, relinKeys);
    evaluator.rescale_to_next_inplace(cipher);
    decryptor.decrypt(cipher, plain);

    // The result may not be exactly the same but should be close
    const double epsilon = 0.0001;
    RC_ASSERT(std::abs(encoder.decode(plain) - value * std::abs(value)) < epsilon);
  }

BOOST_AUTO_TEST_SUITE_END()
