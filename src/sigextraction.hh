/**
* @author Kotaro Matsuoka
*/

#pragma once

#include <tfhe++.hpp>

namespace TFHEpp{
    /*!
     * @brief Generates a Polynomial with each coefficient subtracted by a base value
     * @tparam P The parameter set for the Polynomial
     * @tparam basebit The base to be subtracted
     * @return A Polynomial of the parameter type with coefficients subtracted by the base value
     */
    template<class P, uint basebit>
    constexpr Polynomial<P> subtractpolygen(){
        Polynomial<P> poly;
        for(int i = 0; i < P::n; i++) poly[i] = 1ULL<<(std::numeric_limits<typename P::T>::digits-basebit-2);
        return poly;
    }

    /*!
     * @brief Generates an offset
     * @tparam P The parameter set for the Polynomial
     * @tparam basebit The base value
     * @tparam numdigit The number of digits
     * @return An offset of the parameter type
     */
    template <class P, uint basebit, uint numdigit>
    constexpr typename P::T offsetgen()
    {
        typename P::T offset = 0;
        constexpr uint base = 1ULL<<basebit;
        for (int i = 1; i <= numdigit; i++)
            offset +=
                    base / 2 *
                    (1ULL << (std::numeric_limits<typename P::T>::digits - i * basebit));
        return offset;
    }

    /*!
     * @brief Homomorphically decomposes an input ciphertext into an array of level 1 ciphertexts
     * @tparam high2midP The parameter set for the transition from high to mid level
     * @tparam mid2lowP The parameter set for the transition from mid to low level
     * @tparam brP The bootstrapping parameter set
     * @tparam basebit The base value
     * @tparam numdigit The number of digits
     * @param cres Array of output ciphertexts
     * @param cin Input ciphertext
     * @param kskh2m The key switching key for high to mid level
     * @param kskm2l The key switching key for mid to low level
     * @param bkfft The bootstrapping key FFT
     */
    template<class high2midP, class mid2lowP, class brP, uint basebit, uint numdigit>
    void HomDecomp(std::array<TLWE<typename high2midP::targetP>,numdigit>& cres,
                   const TLWE<typename high2midP::domainP>& cin,
                   const KeySwitchingKey<high2midP> &kskh2m,
                   const KeySwitchingKey<mid2lowP> &kskm2l,
                   const BootstrappingKeyFFT<brP> &bkfft){
        TFHEpp::TLWE<typename mid2lowP::targetP> tlwelvlhalf;
        std::array<TFHEpp::TLWE<typename high2midP::domainP>, numdigit> switchedtlwe;
        TFHEpp::TLWE<typename high2midP::targetP> subtlwe;

        constexpr typename  high2midP::domainP::T offset = offsetgen<typename high2midP::domainP, basebit, numdigit>();

        // cres will be used as a reusable buffer
#pragma omp parallel for
        for(int digit = 1; digit <= numdigit; digit++) {
            for (int i = 0; i <= high2midP::domainP::k * high2midP::domainP::n; i++)
                switchedtlwe[digit - 1][i] = cin[i] << (high2midP::domainP::plain_modulusbit + 1 - basebit * digit);
            // switchedtlwe[high2midP::domainP::k*high2midP::domainP::n] += offset<<(high2midP::domainP::plain_modulusbit+1 - basebit*digit);
            IdentityKeySwitch<high2midP>(cres[digit - 1], switchedtlwe[digit - 1], kskh2m);
        }
        for(int digit = 1; digit <= numdigit; digit++) {
            if(digit!=1){
                for(int i = 0 ; i <= high2midP::targetP::k*high2midP::targetP::n; i++) cres[digit-1][i] += subtlwe[i];
                cres[digit-1][high2midP::targetP::k*high2midP::targetP::n] -= 1ULL<<(std::numeric_limits<typename high2midP::targetP::T>::digits-basebit-1);
            }
            IdentityKeySwitch<mid2lowP>(tlwelvlhalf,cres[digit-1],kskm2l);
            tlwelvlhalf[mid2lowP::targetP::k*mid2lowP::targetP::n] += 1ULL<<(std::numeric_limits<typename mid2lowP::targetP::T>::digits-basebit-1);
            if(digit!=numdigit) GateBootstrappingTLWE2TLWEFFT<brP>(subtlwe,tlwelvlhalf,bkfft,subtractpolygen<typename high2midP::targetP, basebit>());
            // else GateBootstrappingTLWE2TLWEFFT<brP>(cres,tlwelvlhalf,bkfft,μpolygen<typename high2midP::targetP, high2midP::targetP::μ>());
        }
    }
}
