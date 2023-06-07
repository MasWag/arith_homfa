#include <boost/test/unit_test.hpp>

#include <tfhe++.hpp>

#include "../src/bootstrapping_key.hh"
#include "../src/lvl3_to_lvl1.hh"

struct Lvl3ToLvl1TestFixture {
    template<class Param>
    static TFHEpp::Key<Param> keyGen(std::uniform_int_distribution<int32_t> &generator) {
        TFHEpp::Key<Param> key;
        for (typename Param::T &i: key) {
            i = generator(TFHEpp::generator);
        }

        return key;
    }

    TFHEpp::SecretKey skey;
    std::uniform_int_distribution<int32_t> lvl3gen;
    std::uniform_int_distribution<int32_t> lvlhalfgen;

    TFHEpp::Key<TFHEpp::lvl3param> lvl3key;
    TFHEpp::Key<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP> lvlhalfkey;

    ArithHomFA::BootstrappingKey bootKey;
    ArithHomFA::Lvl3ToLvl1 converter;

    Lvl3ToLvl1TestFixture() : lvl3gen(-1, 1), lvlhalfgen(0, 1),
                              lvl3key(keyGen<TFHEpp::lvl3param>(lvl3gen)),
                              lvlhalfkey(keyGen<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP>(lvlhalfgen)),
                              bootKey(skey, lvl3key, lvlhalfkey),
                              converter(bootKey) {}

    ~Lvl3ToLvl1TestFixture() = default;
};

BOOST_AUTO_TEST_SUITE(Lvl3ToLvl1Test)

    BOOST_FIXTURE_TEST_CASE(toLv1TLWE, Lvl3ToLvl1TestFixture) {
        constexpr auto numtest = 10;
        constexpr uint64_t numdigits = 8;
        constexpr uint basebit = 4;
        constexpr uint base = 1ULL << basebit;

        // Test input
        std::random_device seed_gen;
        std::default_random_engine engine(seed_gen());
        std::uniform_int_distribution<typename TFHEpp::lvl3param::T> messagegen(
                0, 2 * TFHEpp::lvl3param::plain_modulus - 1);
        TFHEpp::TLWE<TFHEpp::lvl3param> input;
        std::array<typename TFHEpp::lvl3param::T, numtest> plains{};
        for (typename TFHEpp::lvl3param::T &i: plains) {
            i = messagegen(engine);
        }
        std::array<TFHEpp::TLWE<TFHEpp::lvl3param>, numtest> ciphers{};
        for (uint i = 0; i < numtest; i++) {
            ciphers[i] = TFHEpp::tlweSymIntEncrypt<TFHEpp::lvl3param>(plains[i], TFHEpp::lvl3param::α, lvl3key);
        }

        // Test output
        std::array<std::array<TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP>, numdigits>, numtest> result_multiple{};
        std::array<TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP>, numtest> result_single{};

        // Convert TLWE
        for (uint i = 0; i < numtest; i++) {
            converter.toLv1TLWE<numdigits>(ciphers.at(i), result_multiple.at(i));
        }
        for (uint i = 0; i < numtest; i++) {
            converter.toLv1TLWE(ciphers.at(i), result_single.at(i));
        }

        // Check the correctness of the results
        constexpr typename TFHEpp::lvl3param::T offset = TFHEpp::offsetgen<TFHEpp::lvl3param, basebit, numdigits>();
        for (uint test = 0; test < numtest; test++) {
            for (uint digit = 0; digit < numdigits; digit++) {
                int plainResult = TFHEpp::tlweSymIntDecrypt<typename ArithHomFA::BootstrappingKey::high2midP::targetP>(
                        result_multiple[test][digit],
                        skey.key.get<typename ArithHomFA::BootstrappingKey::high2midP::targetP>());
                const int plainExpected = ((plains[test] >> (basebit * digit)) & ((1ULL << basebit) - 1));
                BOOST_TEST(plainExpected == plainResult);
            }
            int plainResult = TFHEpp::tlweSymIntDecrypt<typename ArithHomFA::BootstrappingKey::high2midP::targetP>(
                    result_single[test],
                    skey.key.get<typename ArithHomFA::BootstrappingKey::high2midP::targetP>());
            const int plainExpected = ((plains[test] >> (basebit * 0)) & ((1ULL << basebit) - 1));
            BOOST_TEST(plainExpected == plainResult);
        }
    }

BOOST_AUTO_TEST_SUITE_END()