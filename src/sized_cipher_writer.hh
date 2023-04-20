/**
* @author Masaki Waga
* @date 2023/04/07.
*/

#pragma once
#include <ostream>

#include "seal/seal.h"

namespace ArithHomFA {
    /*!
     * @brief Write a cipher text with its size to ostream
     */
    class SizedCipherWriter {
        std::stringstream midStream;
        std::ostream &ostream;
    public:
        explicit SizedCipherWriter(std::ostream &stream) : ostream(stream) {}

        void write(const seal::Ciphertext &cipher) {
            midStream.str(std::string());
            cipher.save(midStream);
            uint32_t length = midStream.str().size();
            ostream.write(reinterpret_cast<char *>(&length), sizeof(uint32_t));
            ostream.write(midStream.str().c_str(), length);
        }
    };
}