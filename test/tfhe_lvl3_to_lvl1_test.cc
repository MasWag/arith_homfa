#include <boost/test/unit_test.hpp>

#include <tfhe++.hpp>

#include "../src/bootstrapping_key.hh"
#include "../src/lvl3_to_lvl1.hh"

struct Lvl3ToLvl1TestFixture {
  template <class Param> static TFHEpp::Key<Param> keyGen(std::uniform_int_distribution<int32_t> &generator) {
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

  Lvl3ToLvl1TestFixture()
      : lvl3gen(-1, 1), lvlhalfgen(0, 1), lvl3key(keyGen<TFHEpp::lvl3param>(lvl3gen)),
        lvlhalfkey(keyGen<typename ArithHomFA::BootstrappingKey::mid2lowP::targetP>(lvlhalfgen)),
        bootKey(skey, lvl3key, lvlhalfkey), converter(bootKey) {
  }

  ~Lvl3ToLvl1TestFixture() = default;
};

BOOST_AUTO_TEST_SUITE(Lvl3ToLvl1Test)

  BOOST_FIXTURE_TEST_CASE(toLv1TLWE, Lvl3ToLvl1TestFixture) {
    // Note: This test often fails when we use 32 bit HomDecomp, which does not work for some corner cases.
    constexpr auto numtest = 10;
    constexpr uint64_t numdigits = ArithHomFA::Lvl3ToLvl1::numdigits;
    constexpr uint basebit = ArithHomFA::Lvl3ToLvl1::basebit;

    // Test input
    std::random_device seed_gen;
    std::default_random_engine engine(seed_gen());
    std::uniform_int_distribution<typename TFHEpp::lvl3param::T> messagegen(0,
                                                                            2 * TFHEpp::lvl3param::plain_modulus - 1);
    std::array<typename TFHEpp::lvl3param::T, numtest> plains{};
    for (typename TFHEpp::lvl3param::T &i: plains) {
      i = messagegen(engine);
    }
    std::array<TFHEpp::TLWE<TFHEpp::lvl3param>, numtest> ciphers{};
    for (uint i = 0; i < numtest; i++) {
      ciphers[i] = TFHEpp::tlweSymIntEncrypt<TFHEpp::lvl3param>(plains[i], TFHEpp::lvl3param::α, lvl3key);
    }

    // Test output
    std::array<std::array<TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP>, numdigits>, numtest>
        result_multiple{};
    std::array<TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP>, numtest> result_single{};

    // Convert TLWE
    for (uint i = 0; i < numtest; i++) {
      converter.toLv1TLWE(ciphers.at(i), result_multiple.at(i));
    }
    for (uint i = 0; i < numtest; i++) {
      converter.toLv1TLWE(ciphers.at(i), result_single.at(i));
    }

    // Check the correctness of the results
    for (uint test = 0; test < numtest; test++) {
      for (uint digit = (numdigits-8); digit < numdigits; digit++) {
        int plainResult = TFHEpp::tlweSymIntDecrypt<typename ArithHomFA::BootstrappingKey::high2midP::targetP, 1U<<basebit>(
            result_multiple[test][digit], skey.key.get<typename ArithHomFA::BootstrappingKey::high2midP::targetP>());
        const int plainExpected = ((plains[test] >> (basebit * (digit-8))) & ((1ULL << basebit) - 1));
        plainResult = (plainResult + (1U<<basebit)) % (1U<<basebit);
        BOOST_TEST_CONTEXT("test = " << test << ", digit = " << digit){
          BOOST_TEST(plainExpected == plainResult);
        }
      }
      int plainResult = TFHEpp::tlweSymIntDecrypt<typename ArithHomFA::BootstrappingKey::high2midP::targetP, 1U<<basebit>(
          result_single[test], skey.key.get<typename ArithHomFA::BootstrappingKey::high2midP::targetP>());
      plainResult = (plainResult + (1U<<basebit)) % (1U<<basebit);
      const int plainExpected = ((plains[test] >> (basebit * ((numdigits-8) - 1))) & ((1ULL << basebit) - 1));
      BOOST_TEST(plainExpected == plainResult);
    }
  }

  BOOST_FIXTURE_TEST_CASE(toLv1TLWEBool, Lvl3ToLvl1TestFixture) {
    constexpr auto numtest = 30;

    // Test input
    std::random_device seed_gen;
    std::default_random_engine engine(seed_gen());
    std::uniform_int_distribution<typename TFHEpp::lvl3param::T> messagegen(0,
                                                                            2 * TFHEpp::lvl3param::plain_modulus - 1);
    std::array<typename TFHEpp::lvl3param::T, numtest> plains{};
    for (typename TFHEpp::lvl3param::T &i: plains) {
      i = messagegen(engine);
    }
    std::array<TFHEpp::TLWE<TFHEpp::lvl3param>, numtest> ciphers{};
    for (uint i = 0; i < numtest; i++) {
      ciphers[i] = TFHEpp::tlweSymIntEncrypt<TFHEpp::lvl3param>(plains[i], TFHEpp::lvl3param::α, lvl3key);
    }

    // Convert TLWE
    std::array<TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP>, numtest> result_single{};
    for (uint i = 0; i < numtest; i++) {
      TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP> tlwe;
      converter.toLv1TLWE(ciphers.at(i), tlwe);
      // We use GateBootstrapping to get the signature
      tlwe[TFHEpp::lvl1param::k * TFHEpp::lvl1param::n] += 1ULL << (32 - 5);
      TFHEpp::GateBootstrapping<TFHEpp::lvl10param, TFHEpp::lvl01param, TFHEpp::lvl1param::μ>(result_single.at(i), tlwe,
                                                                                              *bootKey.ekey);
    }

    // Check the correctness of the results
    for (uint test = 0; test < numtest; test++) {
      BOOST_CHECK_EQUAL(TFHEpp::tlweSymDecrypt<TFHEpp::lvl3param>(ciphers.at(test), lvl3key),
                        TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(result_single.at(test), skey.key.get<TFHEpp::lvl1param>()));
    }
  }

  BOOST_FIXTURE_TEST_CASE(toLv1TLWEWithBootstrapping, Lvl3ToLvl1TestFixture) {
    constexpr auto numtest = 30;

    // Test input
    std::random_device seed_gen;
    std::default_random_engine engine(seed_gen());
    std::uniform_int_distribution<typename TFHEpp::lvl3param::T> messagegen(0,
                                                                            2 * TFHEpp::lvl3param::plain_modulus - 1);
    std::uniform_int_distribution<typename TFHEpp::lvl3param::T> maskgen(0,
                                                                            std::numeric_limits<typename TFHEpp::lvl3param::T>::max());
    std::array<typename TFHEpp::lvl3param::T, numtest> plains{};
    for (typename TFHEpp::lvl3param::T &i: plains) {
      i = messagegen(engine);
    }
    std::array<TFHEpp::TLWE<TFHEpp::lvl3param>, numtest> ciphers{};
    for (uint i = 0; i < numtest; i++) {
      ciphers[i] = TFHEpp::tlweSymIntEncrypt<TFHEpp::lvl3param>(plains[i], TFHEpp::lvl3param::α, lvl3key);
      ciphers[i][TFHEpp::lvl3param::n] += maskgen(engine);
    }

    // Convert TLWE
    std::array<TFHEpp::TLWE<typename ArithHomFA::BootstrappingKey::brP::targetP>, numtest> result_single{};
    for (uint i = 0; i < numtest; i++) {
      converter.toLv1TLWEWithBootstrapping(ciphers.at(i), result_single.at(i));
    }

    // Check the correctness of the results
    for (uint test = 0; test < numtest; test++) {
      BOOST_CHECK_EQUAL(TFHEpp::tlweSymDecrypt<TFHEpp::lvl3param>(ciphers.at(test), lvl3key),
                        TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(result_single.at(test), skey.key.get<TFHEpp::lvl1param>()));
    }
  }

  BOOST_AUTO_TEST_SUITE_END()
