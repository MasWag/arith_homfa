/**
* @author Masaki Waga
* @date 2023/06/06
*/

#pragma once
#include <memory>

#include "tfhepp_util.hpp"

#include "my_params.hh"

namespace ArithHomFA {

    class BootstrappingKey : public BKey {
    public:
        using high2midP = TFHEpp::lvl31param;
        using mid2lowP = TFHEpp::lvl10param;
        using brP = TFHEpp::lvl01param;
        std::shared_ptr<TFHEpp::KeySwitchingKey<high2midP>> kskh2m;
        std::shared_ptr<TFHEpp::KeySwitchingKey<mid2lowP>> kskm2l;
        std::shared_ptr<TFHEpp::BootstrappingKeyFFT<brP>> bkfft;

        BootstrappingKey(const TFHEpp::SecretKey& skey, const TFHEpp::Key<TFHEpp::lvl3param>& lvl3key, const TFHEpp::Key<typename mid2lowP::targetP>& lvlhalfkey) : BKey(skey) {
            kskh2m = std::make_unique<TFHEpp::KeySwitchingKey<high2midP>>();
            kskm2l = std::make_unique<TFHEpp::KeySwitchingKey<mid2lowP>>();
            bkfft = std::make_unique<TFHEpp::BootstrappingKeyFFT<brP>>();

            TFHEpp::ikskgen<high2midP>(*kskh2m, lvl3key, skey.key.get<typename high2midP::targetP>());
            TFHEpp::ikskgen<mid2lowP>(*kskm2l, skey.key.get<typename mid2lowP::domainP>(), lvlhalfkey);
            TFHEpp::bkfftgen<brP>(*bkfft,lvlhalfkey, skey.key.get<typename brP::targetP>());
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(ekey, tlwel1_trlwel1_ikskey, kskh2m, kskm2l, bkfft);
        }
    };

} // namespace ArithHomFA
