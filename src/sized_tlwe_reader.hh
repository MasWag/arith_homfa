/**
 * @author Masaki Waga
 * @date 2023/06/26.
 */

#pragma once
#include <ostream>
#include <sstream>
#include <type_traits>

#include "tfhe++.hpp"

#include "archive.hpp"

namespace ArithHomFA {
  /*!
   * @brief Read a ciphertext with its size from istream
   */
  template <class Param> class SizedTLWEReader {
    std::vector<char> midArray;
    std::istream &istream;

  public:
    explicit SizedTLWEReader(std::istream &stream) : istream(stream) {
    }

    bool read(TFHEpp::TLWE<Param> &cipher) {
      if (!istream.good()) {
        return false;
      }
      uint32_t length;
      istream.read(reinterpret_cast<char *>(&length), sizeof(uint32_t));
      if (!istream.good()) {
        return false;
      }
      midArray.resize(length);
      istream.read(midArray.data(), length);
      if (!istream.good()) {
        return false;
      }
      std::stringstream midStream;
      midStream.write(midArray.data(), length);
      read_from_archive(cipher, midStream);

      return true;
    }
  };
} // namespace ArithHomFA