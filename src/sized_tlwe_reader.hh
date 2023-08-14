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
#include "my_params.hh"

namespace ArithHomFA {
  /*!
   * @brief Read a ciphertext with its size from istream
   */
  template <class Param> class SizedTLWEReader {
    std::vector<typename Param::T> midArray;
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
      istream.read(reinterpret_cast<char *>(midArray.data()), length);
      if (!istream.good()) {
        return false;
      }
      std::stringstream midStream;
      midStream.write(reinterpret_cast<char *>(midArray.data()), midArray.size() * sizeof(typename Param::T));
      read_from_archive(cipher, midStream);

      return true;
    }
  };
} // namespace ArithHomFA