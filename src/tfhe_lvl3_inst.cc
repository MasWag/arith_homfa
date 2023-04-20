/**
 * @author Masaki Waga
 * @date 2023/04/08
 * @brief Custom parameters
 */

#include <tfhe++.hpp>

#include "my_params.hh"

namespace TFHEpp {
    using namespace std;

    template <class P>
    TRLWE<P> trlweSymEncryptZero(const double α, const Key<P> &key)
    {
        uniform_int_distribution<typename P::T> Torusdist(
                0, std::numeric_limits<typename P::T>::max());
        TRLWE<P> c;
        for (typename P::T &i : c[P::k]) i = ModularGaussian<P>(0, α);
        for (std::size_t k = 0; k < P::k; k++) {
            for (typename P::T &i : c[k]) i = Torusdist(generator);
            std::array<typename P::T, P::n> partkey;
            for (std::size_t i = 0; i < P::n; i++) partkey[i] = key[k * P::n + i];
            Polynomial<P> temp;
            PolyMul<P>(temp, c[k], partkey);
            for (std::size_t i = 0; i < P::n; i++) c[P::k][i] += temp[i];
        }
        return c;
    }
#define P TFHEpp::lvl3param
    template TRLWE<P> trlweSymEncryptZero<P>(const double α, const Key<P> &key);
#undef P
    template<class P>
    TRLWE <P> trlweSymEncrypt(const array<typename P::T, P::n> &p, const double α,
                              const Key <P> &key) {
        TRLWE <P> c = trlweSymEncryptZero<P>(α, key);
        for (std::size_t i = 0; i < P::n; i++) c[P::k][i] += p[i];
        return c;
    }
#define P TFHEpp::lvl3param

    template TRLWE<P> trlweSymEncrypt<P>(const array<typename P::T, P::n> &p, \
                                         const double α, const Key<P> &key);
#undef P
    template <class P>
    void SampleExtractIndex(TLWE<P> &tlwe, const TRLWE<P> &trlwe, const int index)
    {
        for (std::size_t k = 0; k < P::k; k++) {
            for (int i = 0; i <= index; i++)
                tlwe[k * P::n + i] = trlwe[k][index - i];
            for (std::size_t i = index + 1; i < P::n; i++)
                tlwe[k * P::n + i] = -trlwe[k][P::n + index - i];
        }
        tlwe[P::k * P::n] = trlwe[P::k][index];
    }

#define P TFHEpp::lvl3param
    template void SampleExtractIndex<P>(TLWE<P> & tlwe, const TRLWE<P> &trlwe, \
                                        const int index);

#undef P

    template <class P>
    bool tlweSymDecrypt(const TLWE<P> &c, const Key<P> &key)
    {
        typename P::T phase = c[P::k * P::n];
        for (std::size_t k = 0; k < P::k; k++)
            for (std::size_t i = 0; i < P::n; i++)
                phase -= c[k * P::n + i] * key[k * P::n + i];
        bool res =
                static_cast<typename std::make_signed<typename P::T>::type>(phase) > 0;
        return res;
    }

#define P TFHEpp::lvl3param
    template bool tlweSymDecrypt<P>(const TLWE<P> &c, const Key<P> &key);
#undef P

    template <class P>
    array<bool, P::n> trlweSymDecrypt(const TRLWE<P> &c, const Key<P> &key)
    {
        Polynomial<P> phase = c[P::k];
        for (int k = 0; k < P::k; k++) {
            Polynomial<P> mulres;
            std::array<typename P::T, P::n> partkey;
            for (int i = 0; i < P::n; i++) partkey[i] = key[k * P::n + i];
            PolyMul<P>(mulres, c[k], partkey);
            for (int i = 0; i < P::n; i++) phase[i] -= mulres[i];
        }

        array<bool, P::n> p;
        for (int i = 0; i < P::n; i++)
            p[i] = static_cast<typename make_signed<typename P::T>::type>(
                           phase[i]) > 0;
        return p;
    }
#define P TFHEpp::lvl3param
    template array<bool, P::n> trlweSymDecrypt<P>(const TRLWE<P> &c, const Key<P> &key);

#undef P
}