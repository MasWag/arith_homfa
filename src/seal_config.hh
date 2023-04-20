/**
* @author Masaki Waga
* @date 2023/04/20
*/

#pragma once

#include <ostream>

#include <seal/seal.h>

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include "cereal/archives/json.hpp"

namespace ArithHomFA {
    struct SealConfig {
        [[nodiscard]] seal::SEALContext makeContext() const {
            seal::EncryptionParameters parms(seal::scheme_type::ckks);
            parms.set_poly_modulus_degree(poly_modulus_degree);
            parms.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, base_sizes));

            return {parms};
        }

        std::size_t poly_modulus_degree;
        std::vector<int> base_sizes;

        template<class Archive>
        static SealConfig load(Archive &archive) {
            std::string nodeName = archive.getNodeName();
            if (nodeName != "SealConfig") {
                throw std::runtime_error(std::string("Unexpected NodeName: ") + nodeName);
            }
            std::size_t poly_modulus_degree;
            std::vector<int> base_sizes;
            archive.startNode();
            archive(CEREAL_NVP(poly_modulus_degree), CEREAL_NVP(base_sizes));

            return {poly_modulus_degree, base_sizes};
        }

        template<class Archive>
        void serialize(Archive &archive) const {
            archive(CEREAL_NVP(poly_modulus_degree), CEREAL_NVP(base_sizes));
        }

        bool operator==(const SealConfig &rhs) const {
            return poly_modulus_degree == rhs.poly_modulus_degree &&
                   base_sizes == rhs.base_sizes;
        }

        bool operator!=(const SealConfig &rhs) const {
            return !(rhs == *this);
        }

        friend std::ostream &operator<<(std::ostream &os, const SealConfig &config) {
            cereal::JSONOutputArchive output(os);
            output(cereal::make_nvp("SealConfig", config));

            return os;
        }
    };
}
