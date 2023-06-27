/**
 * @author Masaki Waga
 * @date 2023/06/27
 */

#pragma once
#include <memory>
#include <random>

#include "tfhepp_util.hpp"

#include "my_params.hh"

namespace ArithHomFA {

  class SecretKey : public TFHEpp::SecretKey {
  public:
    using mid2lowP = TFHEpp::lvl10param;
    TFHEpp::Key<typename mid2lowP::targetP> lvlhalfkey;
    std::uniform_int_distribution<int32_t> lvlhalfgen{0, 1};

    template <class Param> static TFHEpp::Key<Param> keyGen(std::uniform_int_distribution<int32_t> &generator) {
      TFHEpp::Key<Param> key;
      for (typename Param::T &i: key) {
        i = generator(TFHEpp::generator);
      }

      return key;
    }

    SecretKey() : TFHEpp::SecretKey(), lvlhalfkey(keyGen<typename mid2lowP::targetP>(lvlhalfgen)) {
    }

    template <class Archive> void serialize(Archive &ar) {
      ar(key.lvl0, key.lvl1, key.lvl2, lvlhalfkey, params);
    }
  };

} // namespace ArithHomFA
