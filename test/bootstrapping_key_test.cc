#include <sstream>

#include <boost/test/unit_test.hpp>
#include <seal/seal.h>

#include "archive.hpp"
#include "tfhe++.hpp"

#include "bootstrapping_key.hh"
#include "ckks_to_tfhe.hh"
#include "secret_key.hh"

BOOST_AUTO_TEST_SUITE(BootstrappingKeyTest)

  // This does not require much RAM but takes quite some time, e.g., 1 hour.
  BOOST_AUTO_TEST_CASE(writeAndRead) {
    // Generate the secret key of TFHEpp
    ArithHomFA::SecretKey sKey;

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
    ArithHomFA::BootstrappingKey bKey{sKey, lvl3Key, sKey.lvlhalfkey};

    std::stringstream stream;
    write_to_archive(stream, bKey);
    auto loadedKey = read_from_archive<ArithHomFA::BootstrappingKey>(stream);

    for (std::size_t i = 0; i < bKey.ekey->iksklvl10->size(); ++i) {
      for (std::size_t j = 0; j < bKey.ekey->iksklvl10->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.ekey->iksklvl10->at(i).at(j).size(); ++k) {
          BOOST_TEST(bKey.ekey->iksklvl10->at(i).at(j).at(k) == loadedKey.ekey->iksklvl10->at(i).at(j).at(k),
                     boost::test_tools::per_element());
        }
      }
    }

    for (std::size_t i = 0; i < bKey.ekey->bkfftlvl01->size(); ++i) {
      for (std::size_t j = 0; j < bKey.ekey->bkfftlvl01->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.ekey->bkfftlvl01->at(i).at(j).size(); ++k) {
          for (std::size_t l = 0; l < bKey.ekey->bkfftlvl01->at(i).at(j).at(l).size(); ++l) {
            BOOST_TEST(bKey.ekey->bkfftlvl01->at(i).at(j).at(k).at(l) ==
                           loadedKey.ekey->bkfftlvl01->at(i).at(j).at(k).at(l),
                       boost::test_tools::per_element());
          }
        }
      }
    }

    for (std::size_t i = 0; i < bKey.ekey->bkfftlvl02->size(); ++i) {
      for (std::size_t j = 0; j < bKey.ekey->bkfftlvl02->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.ekey->bkfftlvl02->at(i).at(j).size(); ++k) {
          for (std::size_t l = 0; l < bKey.ekey->bkfftlvl02->at(i).at(j).at(l).size(); ++l) {
            BOOST_TEST(bKey.ekey->bkfftlvl02->at(i).at(j).at(k).at(l) ==
                           loadedKey.ekey->bkfftlvl02->at(i).at(j).at(k).at(l),
                       boost::test_tools::per_element());
          }
        }
      }
    }

    for (auto it = bKey.ekey->privksklvl21.begin(); it != bKey.ekey->privksklvl21.end(); ++it) {
      auto it2 = loadedKey.ekey->privksklvl21.find(it->first);
      BOOST_TEST((it2 != loadedKey.ekey->privksklvl21.end()));
      BOOST_TEST(it->second->size() == it2->second->size());
      for (std::size_t i = 0; i < it->second->size(); ++i) {
        BOOST_TEST(it->second->at(i).size() == it2->second->at(i).size());
        for (std::size_t j = 0; j < it->second->at(i).size(); ++j) {
          BOOST_TEST(it->second->at(i).at(j).size() == it2->second->at(i).at(j).size());
          for (std::size_t k = 0; k < it->second->at(i).at(j).size(); ++k) {
            BOOST_TEST(it->second->at(i).at(j).at(k).size() == it2->second->at(i).at(j).at(k).size());
            for (std::size_t l = 0; l < it->second->at(i).at(j).at(k).size(); ++l) {
              BOOST_TEST(it->second->at(i).at(j).at(k).at(l) == it2->second->at(i).at(j).at(k).at(l),
                         boost::test_tools::per_element());
            }
          }
        }
      }
    }

    for (std::size_t i = 0; i < bKey.tlwel1_trlwel1_ikskey->size(); ++i) {
      for (std::size_t j = 0; j < bKey.tlwel1_trlwel1_ikskey->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.tlwel1_trlwel1_ikskey->at(i).at(j).size(); ++k) {
          for (std::size_t l = 0; l < bKey.tlwel1_trlwel1_ikskey->at(i).at(j).at(k).size(); ++l) {
            BOOST_TEST(bKey.tlwel1_trlwel1_ikskey->at(i).at(j).at(k).at(l) ==
                           loadedKey.tlwel1_trlwel1_ikskey->at(i).at(j).at(k).at(l),
                       boost::test_tools::per_element());
          }
        }
      }
    }

    for (std::size_t i = 0; i < bKey.kskh2m->size(); ++i) {
      for (std::size_t j = 0; j < bKey.kskh2m->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.kskh2m->at(i).at(j).size(); ++k) {
          BOOST_TEST(bKey.kskh2m->at(i).at(j).at(k) == loadedKey.kskh2m->at(i).at(j).at(k),
                     boost::test_tools::per_element());
        }
      }
    }

    for (std::size_t i = 0; i < bKey.kskm2l->size(); ++i) {
      for (std::size_t j = 0; j < bKey.kskm2l->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.kskm2l->at(i).at(j).size(); ++k) {
          BOOST_TEST(bKey.kskm2l->at(i).at(j).at(k) == loadedKey.kskm2l->at(i).at(j).at(k),
                     boost::test_tools::per_element());
        }
      }
    }

    for (std::size_t i = 0; i < bKey.bkfft->size(); ++i) {
      for (std::size_t j = 0; j < bKey.bkfft->at(i).size(); ++j) {
        for (std::size_t k = 0; k < bKey.bkfft->at(i).at(j).size(); ++k) {
          for (std::size_t l = 0; l < bKey.bkfft->at(i).at(j).at(l).size(); ++l) {
            BOOST_TEST(bKey.bkfft->at(i).at(j).at(k).at(l) == loadedKey.bkfft->at(i).at(j).at(k).at(l),
                       boost::test_tools::per_element());
          }
        }
      }
    }
  }

  BOOST_AUTO_TEST_SUITE_END()
