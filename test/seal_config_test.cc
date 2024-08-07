#include <boost/test/unit_test.hpp>
#include <sstream>

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include "../src/seal_config.hh"

BOOST_AUTO_TEST_SUITE(SealConfigTest)

    BOOST_AUTO_TEST_CASE(save) {
        const ArithHomFA::SealConfig config = {
                8192, // poly_modulus_degree
                std::vector<int>{60, 40, 60}, // base_sizes
                std::pow(2, 40) // scale
        };

        std::stringstream stream;
        {
            cereal::JSONOutputArchive output(stream);
            output(cereal::make_nvp("SealConfig", config));
        }

        std::string expected = "{\n"
                               "    \"SealConfig\": {\n"
                               "        \"poly_modulus_degree\": 8192,\n"
                               "        \"base_sizes\": [\n"
                               "            60,\n"
                               "            40,\n"
                               "            60\n"
                               "        ],\n"
                               "        \"scale\": 1099511627776.0\n"
                               "    }\n"
                               "}";

        BOOST_TEST(expected == stream.str());
    }

    BOOST_AUTO_TEST_CASE(load) {
        const ArithHomFA::SealConfig expected = {
                8192, // poly_modulus_degree
                std::vector<int>{60, 40, 60}, // base_sizes
                std::pow(2, 40) // scale
        };

        std::stringstream stream;
        std::string asJsonStr  = "{\n"
                               "    \"SealConfig\": {\n"
                               "        \"poly_modulus_degree\": 8192,\n"
                               "        \"base_sizes\": [\n"
                               "            60,\n"
                               "            40,\n"
                               "            60\n"
                               "        ],\n"
                               "        \"scale\": 1099511627776.0\n"
                               "    }\n"
                               "}\n";
        stream << asJsonStr;

        ArithHomFA::SealConfig loaded;
        {
            cereal::JSONInputArchive input(stream);
            loaded = ArithHomFA::SealConfig::load(input);
        }
        BOOST_TEST(expected == loaded);
    }
BOOST_AUTO_TEST_SUITE_END()
