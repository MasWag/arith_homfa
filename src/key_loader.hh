/**
 * @author Masaki Waga
 * @date 2023/08/16.
 */

#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include <seal/seal.h>

namespace ArithHomFA {
  class KeyLoader {
  public:
    static seal::SecretKey loadSecretKey(const seal::SEALContext &context, const std::string &secretKeyPath) {
      seal::SecretKey secretKey;
      {
        std::ifstream secretKeyStream(secretKeyPath);
        if (secretKeyStream) {
          secretKey.load(context, secretKeyStream);
        } else {
          throw std::runtime_error("Failed to open SEAL's secret key file at: " + secretKeyPath);
        }
      }

      return secretKey;
    }

    static seal::RelinKeys loadRelinKeys(const seal::SEALContext &context, const std::string &relinKeysPath) {
      seal::RelinKeys relinKeys;
      {
        std::ifstream relinKeysStream(relinKeysPath);
        if (relinKeysStream) {
          relinKeys.load(context, relinKeysStream);
        } else {
          throw std::runtime_error("Failed to open the RelinKeys file at: " + relinKeysPath);
        }
      }

      return relinKeys;
    }
  };
} // namespace ArithHomFA