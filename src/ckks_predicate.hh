/**
 * @author Masaki Waga
 * @date 2023/05/02
 */

#pragma once

#include <vector>
#include <memory>
#include <cassert>

#include <seal/seal.h>

#include "../src/seal_config.hh"
#include "../src/ckks_no_embed.hh"

namespace ArithHomFA {
    /*!
     * @brief Class defining the predicate in the given specification
     */
    class CKKSPredicate {
    public:
        explicit CKKSPredicate(const seal::SEALContext &context, double scale) : context(context), scale(scale),
                                                                                 encoder(context), evaluator(context) {}

        /*!
         * @brief Evaluate the predicates with the given valuation
         *
         * @param [in] valuation The evaluated signal valuation
         * @param [out] result The ciphertexts such that result.at(i) > 0 if and only if the i-th predicate is true
         *
         * @pre valuation.size() == this->signalSize
         * @pre result.size() == this->predicateSize
         */
        void eval(const std::vector<seal::Ciphertext> &valuation, std::vector<seal::Ciphertext> &result) {
            // Assert the preconditions
            if (valuation.size() != ArithHomFA::CKKSPredicate::signalSize ||
                result.size() != ArithHomFA::CKKSPredicate::predicateSize) {
                throw std::runtime_error("Invalid size of valuation or result is given");
            }
            // Call the actual evaluation
            evalPredicateInternal(valuation, result);
        }

        /*!
         * @brief Evaluate the predicates with the given valuation without encryption
         *
         * @param [in] valuation The evaluated signal valuation
         * @param [out] result The plaintext such that result.at(i) > 0 if and only if the i-th predicate is true
         *
         * @pre valuation.size() == this->signalSize
         * @pre result.size() == this->predicateSize
         */
        void eval(const std::vector<double> &valuation, std::vector<double> &result) {
            // Assert the preconditions
            if (valuation.size() != ArithHomFA::CKKSPredicate::signalSize ||
                result.size() != ArithHomFA::CKKSPredicate::predicateSize) {
                throw std::runtime_error("Invalid size of valuation or result is given");
            }
            // Call the actual evaluation
            evalPredicateInternal(valuation, result);
        }

        static size_t getSignalSize() {
            return signalSize;
        }

        static size_t getPredicateSize() {
            return predicateSize;
        }

        static const std::vector<double> getReferences() {
            return references;
        }

        void setRelinKeys(const seal::RelinKeys &keys) {
            this->relinKeys = keys;
        }

    protected:
        const seal::SEALContext &context;
        double scale;
        CKKSNoEmbedEncoder encoder;
        seal::Evaluator evaluator;
        seal::RelinKeys relinKeys;

        // The following variables and functions must be defined by a user
        //! The dimension of the input signal
        const static std::size_t signalSize;
        //! The number of the output predicates
        const static std::size_t predicateSize;
        //! (approximate) upper bound of the value of each signal
        const static std::vector<double> references;

        //! Function for the actual evaluation
        void evalPredicateInternal(const std::vector<seal::Ciphertext> &, std::vector<seal::Ciphertext> &);

        //! Function for the actual evaluation
        void evalPredicateInternal(const std::vector<double> &, std::vector<double> &);
    };
}