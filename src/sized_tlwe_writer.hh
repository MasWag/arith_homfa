/**
 * @author Masaki Waga
 * @date 2023/06/26.
 */

#pragma once
#include <ostream>

#include "tfhe++.hpp"

#include "archive.hpp"
#include "my_params.hh"

namespace ArithHomFA {
  /*!
   * @brief Write a cipher text with its size to ostream
   */
  template <class Param> class SizedTLWEWriter {
    std::ostream &ostream;

  public:
    explicit SizedTLWEWriter(std::ostream &stream) : ostream(stream) {
    }

    void write(const TFHEpp::TLWE<Param> &cipher) {
      std::stringstream midStream;
      write_to_archive(midStream, cipher);

      uint32_t length = midStream.str().size();
      ostream.write(reinterpret_cast<char *>(&length), sizeof(uint32_t));
      ostream.write(midStream.str().c_str(), length);
    }
  };
} // namespace ArithHomFA