/**
 * @author Masaki Waga
 * @date 2023/06/23.
 */

#pragma once

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

#include <seal/seal.h>

#include "archive.hpp"

#include "bootstrapping_key.hh"
#include "ckks_predicate.hh"
#include "ckks_to_tfhe.hh"
#include "key_loader.hh"
#include "seal_config.hh"
#include "sized_cipher_reader.hh"
#include "sized_cipher_writer.hh"
#include "sized_tlwe_writer.hh"

namespace ArithHomFA {
  class PointwiseRunner {
  public:
    PointwiseRunner(const ArithHomFA::SealConfig &config, const std::string &relinKeysPath, std::istream &istream,
                    std::ostream &ostream)
        : context(config.makeContext()), predicate(context, config.scale),
          relinKeys(KeyLoader::loadRelinKeys(context, relinKeysPath)), reader(istream), ckksWriter(ostream),
          tlweWriter(ostream) {
      predicate.setRelinKeys(this->relinKeys);
    }

    PointwiseRunner(const ArithHomFA::SealConfig &config, const std::string &bkey_filename,
                    const std::string &relinKeysPath, std::istream &istream, std::ostream &ostream)
        : context(config.makeContext()), predicate(context, config.scale),
          bkey(read_from_archive<ArithHomFA::BootstrappingKey>(bkey_filename)),
          relinKeys(KeyLoader::loadRelinKeys(context, relinKeysPath)), reader(istream), ckksWriter(ostream),
          tlweWriter(ostream) {
      predicate.setRelinKeys(this->relinKeys);
    }

    static void runPointwise(const seal::SEALContext &context, ArithHomFA::CKKSPredicate &predicate,
                             const seal::RelinKeys &relinKeys, ArithHomFA::SizedCipherReader &reader,
                             ArithHomFA::SizedCipherWriter &writer) {
      std::vector<seal::Ciphertext> valuations, results;
      valuations.resize(ArithHomFA::CKKSPredicate::getSignalSize());
      results.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
      while (reader.good()) {
        // get the content
        for (auto &valuation: valuations) {
          if (!reader.read(context, valuation)) {
            return;
          }
        }
        // Evaluate
        predicate.eval(valuations, results);
        // dump the cipher text
        for (const auto &result: results) {
          writer.write(result);
        }
      }
    }

    void runPointwise() {
      runPointwise(context, predicate, relinKeys, reader, ckksWriter);
    }

    static void runPointwiseTFHE(const seal::SEALContext &context, ArithHomFA::CKKSPredicate &predicate,
                                 const ArithHomFA::BootstrappingKey &bkey, const seal::RelinKeys &relinKeys,
                                 ArithHomFA::SizedCipherReader &reader,
                                 ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> &writer) {
      std::vector<seal::Ciphertext> valuations, results;
      valuations.resize(ArithHomFA::CKKSPredicate::getSignalSize());
      results.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
      ArithHomFA::CKKSToTFHE converter{context};
      converter.initializeConverter(bkey);
      while (reader.good()) {
        // get the content
        for (auto &valuation: valuations) {
          if (!reader.read(context, valuation)) {
            return;
          }
        }
        // Evaluate and dump the cipher text
        predicate.eval(valuations, results);
        for (int i = 0; i < results.size(); ++i) {
          TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
          converter.toLv1TLWE(results.at(i), tlwe, ArithHomFA::CKKSPredicate::getReferences().at(i));
          writer.write(tlwe);
        }
      }
    }

    void runPointwiseTFHE() {
      runPointwiseTFHE(context, predicate, bkey, relinKeys, reader, tlweWriter);
    }

  private:
    seal::SEALContext context;
    ArithHomFA::CKKSPredicate predicate;
    ArithHomFA::BootstrappingKey bkey;
    seal::RelinKeys relinKeys;
    ArithHomFA::SizedCipherReader reader;
    ArithHomFA::SizedCipherWriter ckksWriter;
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> tlweWriter;
  };
} // namespace ArithHomFA
