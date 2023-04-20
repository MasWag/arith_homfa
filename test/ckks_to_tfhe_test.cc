#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "../src/ckks_no_embed.hh"
#include "../src/ckks_to_tfhe.hh"

BOOST_AUTO_TEST_SUITE(CKKSToTFHETest)

    BOOST_AUTO_TEST_CASE(toLv3Key) {
        seal::EncryptionParameters parms(seal::scheme_type::ckks);
        const std::size_t log2_poly_modulus_degree = TFHEpp::lvl3param::nbit;
        const std::size_t poly_modulus_degree = 1 << log2_poly_modulus_degree;
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, { 60, 40, 60 }));
        seal::SEALContext context(parms);

        // Generate Key
        seal::KeyGenerator keygen(context);
        const auto& secretKey = keygen.secret_key();

        // Convert the key for TFHEpp
        ArithHomFA::CKKSToTFHE converter(context);
        TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
        converter.toLv3Key(secretKey, lvl3Key);

        // Check the key length
        BOOST_CHECK_EQUAL(lvl3Key.size(), poly_modulus_degree);
        // Check that the generated key is ternary
        for (const auto& coefficient: lvl3Key) {
            BOOST_TEST((coefficient == -1 || coefficient == 0 || coefficient == 1));
        }
    }

    RC_BOOST_PROP(toLv3TRLWE,
                  (const bool& isNegative)) {
        // We implicitly require that the given value is not too large. Otherwise the encoding fails.
        const uint64_t intValue = *rc::gen::inRange<uint64_t>(static_cast<uint64_t>(std::pow(2.0, 30)),
                                                              UINT64_MAX);
        const double scale = std::pow(2.0, 45);
        const double &value = isNegative ? -static_cast<double>(intValue) / scale : static_cast<double>(intValue) / scale;
        // We require that the given value is not very close to zero. Otherwise the result may be incorrect due to noise.
        RC_PRE(std::abs(value) > 0.01);

        seal::EncryptionParameters parms(seal::scheme_type::ckks);
        const std::size_t log2_poly_modulus_degree = TFHEpp::lvl3param::nbit;
        const std::size_t poly_modulus_degree = 1 << log2_poly_modulus_degree;
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, { 60, 40, 60 }));
        seal::SEALContext context(parms);

        // Generate Key
        seal::KeyGenerator keygen(context);
        const auto& secretKey = keygen.secret_key();

        // Encrypt the given value
        ArithHomFA::CKKSNoEmbedEncoder encoder(context);
        seal::Encryptor encryptor(context, secretKey);
        seal::Plaintext plain;
        encoder.encode(value, scale, plain);
        seal::Ciphertext cipher;
        encryptor.encrypt_symmetric(plain, cipher);

        // Convert the key for TFHEpp
        ArithHomFA::CKKSToTFHE converter(context);
        TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
        converter.toLv3Key(secretKey, lvl3Key);

        // Convert the ciphertext for TFHEpp
        TFHEpp::TRLWE<TFHEpp::lvl3param> trlwe;
        converter.toLv3TRLWE(cipher, trlwe);

        // Decrypt the TRLWE with TFHEpp
        const auto trlwePlain = TFHEpp::trlweSymDecrypt<TFHEpp::lvl3param>(trlwe, lvl3Key);
        const bool negative = !trlwePlain.front();

        // Assert the result
        if (value > 0) {
            RC_ASSERT(!negative);
        } else if (value < 0) {
            RC_ASSERT(negative);
        }
    }

BOOST_AUTO_TEST_SUITE_END()
