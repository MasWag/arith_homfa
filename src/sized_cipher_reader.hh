/**
* @author Masaki Waga
* @date 2023/04/07.
*/

#pragma once
#include <ostream>

#include "seal/seal.h"

namespace ArithHomFA {
    /*!
     * @brief Read a cipher text with its size from istream
     */
    class SizedCipherReader {
        std::vector<seal::seal_byte> midArray;
        std::istream &istream;
    public:
        explicit SizedCipherReader(std::istream &stream) : istream(stream) {}

        bool read(const seal::SEALContext &context, seal::Ciphertext &cipher) {
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
            cipher.load(context, midArray.data(), length);

            return true;
        }
    };
}