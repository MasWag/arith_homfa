#include <sstream>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "tfhe++.hpp"

#include "../src/ahomfa_runner.hh"
#include "sized_cipher_reader.hh"
#include "sized_cipher_writer.hh"

BOOST_AUTO_TEST_SUITE(CKKSReaderWriterTest)

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

  RC_BOOST_FIXTURE_PROP(writeAndRead, CKKSToTFHEFixture,
                        (const std::vector<int32_t> &given, const bool &useLargerParam)) {
    // Generate Key
    static std::array<seal::KeyGenerator, 2> keygens{contexts.front(), contexts.back()};
    const auto &secretKey = keygens.at(useLargerParam).secret_key();
    seal::RelinKeys relinKeys;
    keygens.at(useLargerParam).create_relin_keys(relinKeys);

    // Set up the encryptor
    seal::SEALContext context = contexts.at(useLargerParam);
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    seal::Encryptor encryptor(context, secretKey);
    seal::Decryptor decryptor(context, secretKey);

    // Define the stream
    std::stringstream stream;
    ArithHomFA::SizedCipherWriter writer{stream};
    ArithHomFA::SizedCipherReader reader{stream};

    for (const auto &value: given) {
      seal::Plaintext plain;
      seal::Ciphertext cipher;
      encoder.encode(static_cast<double>(value) * minValue, scale, plain);
      encryptor.encrypt_symmetric(plain, cipher);
      writer.write(cipher);
    }

    for (const auto &value: given) {
      seal::Plaintext plain;
      seal::Ciphertext cipher;
      reader.read(context, cipher);
      decryptor.decrypt(cipher, plain);

      // The value may not be exactly the same, but should be close the original value.
      RC_ASSERT(std::abs(encoder.decode(plain) - static_cast<double>(value) * minValue) < 0.001);
    }
  }

BOOST_AUTO_TEST_SUITE_END()
