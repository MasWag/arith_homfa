#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "../src/ckks_no_embed.hh"

BOOST_AUTO_TEST_SUITE(CKKSNoEmbedTest)

    RC_BOOST_PROP(encodeAndDecode,
                  (const bool& isNegative, const uint64_t& intValue)) {
        // We implicitly require that the given value is not too large. Otherwise the encoding fails.
        const double scale = std::pow(2.0, 45);
        const double &value = isNegative ? -static_cast<double>(intValue) / scale : static_cast<double>(intValue) / scale;

        seal::EncryptionParameters parms(seal::scheme_type::ckks);
        const std::size_t poly_modulus_degree = 8192;
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, { 60, 40, 60 }));
        seal::SEALContext context(parms);

        // Encode the given value
        ArithHomFA::CKKSNoEmbedEncoder encoder(context);
        seal::Plaintext plain;
        encoder.encode(value, scale, plain);

        // The result may not be exactly the same but should be close
        const double epsilon = 0.0001;
        RC_ASSERT(std::abs(encoder.decode(plain) - value) < epsilon);
    }

BOOST_AUTO_TEST_SUITE_END()
