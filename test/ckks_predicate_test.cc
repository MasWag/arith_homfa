/**
 * @author Masaki Waga
 * @date 2023/05/02
 */

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>

#include "../src/ckks_predicate.hh"

BOOST_AUTO_TEST_SUITE(CKKSPredicateTest)

    RC_BOOST_PROP(eval, ()) {
        const auto value = *rc::gen::inRange(-10000, 10000);
        const ArithHomFA::SealConfig config = {
                8192, // poly_modulus_degree
                std::vector<int>{60, 40, 60}, // base_sizes
                std::pow(2, 40) // scale
        };
        std::vector<seal::Ciphertext> valuation, result;
        const auto context = config.makeContext();
        ArithHomFA::CKKSNoEmbedEncoder encoder(context);
        seal::SecretKey secretKey = seal::KeyGenerator(context).secret_key();
        seal::Encryptor encryptor(context, secretKey);
        seal::Plaintext plain;
        encoder.encode(value, config.scale, plain);
        valuation.resize(1);
        encryptor.encrypt_symmetric(plain, valuation.front());
        result.resize(1);
        ArithHomFA::CKKSPredicate predicate{config};
        predicate.eval(valuation, result);
        seal::Decryptor decryptor(context, secretKey);
        decryptor.decrypt(result.front(), plain);
        RC_ASSERT((encoder.decode(plain) > 0) == (value > 70));
    }

BOOST_AUTO_TEST_SUITE_END()
