/**
* @author Masaki Waga
* @date 2023/04/20
*/

#include <iostream>
#include <optional>
#include <CLI/CLI.hpp>
#include <unordered_map>
#include <tfhe++.hpp>
#include <seal/seal.h>

#include "archive.hpp"
#include "tfhepp_util.hpp"
#include "sized_cipher_writer.hh"
#include "seal_config.hh"
#include "sized_cipher_reader.hh"
#include "ckks_no_embed.hh"

#include "utility.hpp"
#include "graph.hpp"

namespace {
    enum class VERBOSITY {
        VERBOSE, NORMAL, QUIET
    };

    enum class TYPE {
        UNSPECIFIED,

        GENKEY_SEAL,
        GENKEY_TFHEPP,
        GENRELINKEY_SEAL,
        GENBKEY_TFHEPP,

        ENC_CKKS,
        DEC_CKKS,
        ENC_TFHEPP,
        DEC_TFHEPP,

        LTL2SPEC,
        SPEC2SPEC
    };

    struct Args {
        VERBOSITY verbosity = VERBOSITY::NORMAL;
        TYPE type = TYPE::UNSPECIFIED;

        bool minimized = false, reversed = false, negated = false,
                make_all_live_states_final = false, is_spec_reversed = false,
                sanitize_result = false;
        std::optional<ArithHomFA::SealConfig> sealConfig;
        seal::SecretKey sealSecretKey;
        std::optional<std::string> spec, skey, bkey, output_dir,
                debug_skey, formula, online_method;
        std::istream *input = &std::cin;
        std::ostream *output = &std::cout;
        std::optional<size_t> num_vars, queue_size, bootstrapping_freq,
                max_second_lut_depth, num_ap, output_freq;
    };

    void register_general_options(CLI::App &app, Args &args) {
        app.add_flag_callback("--verbose",
                              [&] { args.verbosity = VERBOSITY::VERBOSE; });
        app.add_flag_callback("--quiet",
                              [&] { args.verbosity = VERBOSITY::QUIET; });
    }

    void register_SEAL(CLI::App &app, Args &args) {
        CLI::App *ckks = app.add_subcommand("ckks", "Subcommands related to SEAL");
        std::vector<CLI::App *> subcommands;
        std::vector<CLI::App *> requiresKey;
        std::vector<CLI::App *> withInput;
        CLI::App *genkey = ckks->add_subcommand("genkey", "Generate secret key");
        subcommands.push_back(genkey);
        CLI::App *genrelinkey = ckks->add_subcommand("genrelinkey", "Generate relin key");
        subcommands.push_back(genrelinkey);
        requiresKey.push_back(genrelinkey);
        CLI::App *enc = ckks->add_subcommand("enc", "Encrypt input file");
        subcommands.push_back(enc);
        requiresKey.push_back(enc);
        withInput.push_back(enc);
        CLI::App *dec = ckks->add_subcommand("dec", "Decrypt input file");
        subcommands.push_back(dec);
        requiresKey.push_back(dec);
        withInput.push_back(dec);

        // General options for CKKS
        for (auto subcommand: subcommands) {
            std::function<void(const std::string &)> configCallback = [&args](const std::string &path) {
                std::ifstream istream(path);
                cereal::JSONInputArchive archive(istream);
                args.sealConfig = ArithHomFA::SealConfig::load(archive);
            };
            subcommand->add_option_function("-f,--file", configCallback, "The configuration file of SEAL")->required();

            std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
                args.output = new std::ofstream(path);
            };
            subcommand->add_option_function("-o,--output", output_callback, "The file to write the result");
        }

        // Options for subcommands with key
        for (auto subcommand: requiresKey) {
            std::function<void(const std::string &)> keyCallback = [&args](const std::string &path) {
                std::ifstream istream(path);
                args.sealSecretKey.load(args.sealConfig->makeContext(), istream);
            };
            subcommand->add_option_function("-K,--secret-key", keyCallback, "The secret key of SEAL")->required();
        }

        // Options for subcommands with key
        for (auto subcommand: withInput) {
            std::function<void(const std::string &)> callback = [&args](const std::string &path) {
                args.input = new std::ifstream(path);
            };
            subcommand->add_option_function("-i,--input", callback, "The file to load the input");
        }

        // genkey
        genkey->parse_complete_callback([&args] { args.type = TYPE::GENKEY_SEAL; });
        // genrelinkey
        genrelinkey->parse_complete_callback([&args] { args.type = TYPE::GENRELINKEY_SEAL; });
        // enc
        enc->parse_complete_callback([&args] { args.type = TYPE::ENC_CKKS; });
        // dec
        dec->parse_complete_callback([&args] { args.type = TYPE::DEC_CKKS; });
    }

    void register_TFHEpp(CLI::App &app, Args &args) {
        CLI::App *tfhe = app.add_subcommand("tfhe", "Subcommands related to TFHEpp");
        std::vector<CLI::App *> subcommands;
        std::vector<CLI::App *> requiresKey;
        std::vector<CLI::App *> withInput;
        CLI::App *genkey = tfhe->add_subcommand("genkey", "Generate secret key");
        subcommands.push_back(genkey);
        CLI::App *genbkey = tfhe->add_subcommand("genbkey", "Generate bootstrap key");
        subcommands.push_back(genbkey);
        requiresKey.push_back(genbkey);
        CLI::App *enc = tfhe->add_subcommand("enc", "Encrypt input file");
        subcommands.push_back(enc);
        requiresKey.push_back(enc);
        withInput.push_back(enc);
        CLI::App *dec = tfhe->add_subcommand("dec", "Decrypt input file");
        subcommands.push_back(dec);
        requiresKey.push_back(dec);
        withInput.push_back(dec);

        // General options for TFHE
        for (auto subcommand: subcommands) {
            std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
                args.output = new std::ofstream(path);
            };
            subcommand->add_option_function("-o,--output", output_callback, "The file to write the result");
        }

        // Options for subcommands with key
        for (auto subcommand: requiresKey) {
            subcommand->add_option("-K,--secret-key", args.skey, "The secret key of TFHEpp")->required()->check(
                    CLI::ExistingFile);
        }

        // Options for subcommands with inputs
        for (auto subcommand: withInput) {
            std::function<void(const std::string &)> callback = [&args](const std::string &path) {
                args.input = new std::ifstream(path);
            };
            subcommand->add_option_function("-i,--input", callback, "The file to load the input");
        }

        // genkey
        genkey->parse_complete_callback([&args] { args.type = TYPE::GENKEY_TFHEPP; });
        // genrelinkey
        genbkey->parse_complete_callback([&args] { args.type = TYPE::GENBKEY_TFHEPP; });
        // enc
        enc->parse_complete_callback([&args] { args.type = TYPE::ENC_TFHEPP; });
        enc->add_option("-a,--num-AP", args.num_ap, "Number of atomic propositions")->required();
        // dec
        dec->parse_complete_callback([&args] { args.type = TYPE::DEC_TFHEPP; });
    }

    void register_ltl2spec(CLI::App &app, Args &args) {
        CLI::App *ltl2spec = app.add_subcommand("ltl2spec", "Convert LTL to spec format for ArithHomFA");
        std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
            args.output = new std::ofstream(path);
        };
        ltl2spec->add_option("-e,--formula", *args.formula, "The LTL formula");
        ltl2spec->add_option_function("-o,--output", output_callback, "The file to write the result");
        ltl2spec->add_option("-n,--num-vars", *args.num_vars, "The number of variables in the given LTL formula");
        ltl2spec->add_option("--make-all-live-states-final", args.make_all_live_states_final,
                             "If make all live states final");
        ltl2spec->parse_complete_callback([&args] { args.type = TYPE::LTL2SPEC; });
    }

    void do_genkey_SEAL(const ArithHomFA::SealConfig &config, std::ostream &ostream) {
        const seal::SEALContext context = config.makeContext();
        seal::KeyGenerator keygen(context);
        const seal::SecretKey &secretKey = keygen.secret_key();

        secretKey.save(ostream);
    }

    void do_genrelinkey_SEAL(const ArithHomFA::SealConfig &config, const seal::SecretKey &secretKey,
                             std::ostream &ostream) {
        const seal::SEALContext context = config.makeContext();
        seal::KeyGenerator keygen(context, secretKey);

        seal::RelinKeys relin_keys;
        keygen.create_relin_keys(relin_keys);
        relin_keys.save(ostream);
    }

    void do_genkey_TFHEpp(std::ostream &ostream) {
        TFHEpp::SecretKey skey;
        write_to_archive(ostream, skey);
    }

    void do_genbkey_TFHEpp(std::istream &&skey_stream, std::ostream &ostream) {
        auto skey = read_from_archive<TFHEpp::SecretKey>(skey_stream);
        BKey bkey{skey};
        write_to_archive(ostream, bkey);
    }

    void do_enc_SEAL(const ArithHomFA::SealConfig &config, const seal::SecretKey &secretKey,
                     std::istream &istream, std::ostream &ostream) {
        const seal::SEALContext context = config.makeContext();
        const double scale = config.scale;
        seal::Encryptor encryptor(context, secretKey);

        ArithHomFA::CKKSNoEmbedEncoder encoder(context);

        double content;
        ArithHomFA::SizedCipherWriter writer(ostream);
        while (istream.good()) {
            // get the content from stdin
            istream >> content;
            if (!istream.good()) {
                break;
            }
            seal::Plaintext plain;
            seal::Ciphertext cipher;
            encoder.encode(content, scale, plain);
            encryptor.encrypt_symmetric(plain, cipher);
            // dump the cipher text to stdout
            writer.write(cipher);
        }
        spdlog::info("Given contents are encrypted with the CKKS scheme");
    }

    void do_dec_SEAL(const ArithHomFA::SealConfig &config, const seal::SecretKey &secretKey,
                     std::istream &istream, std::ostream &ostream) {
        const seal::SEALContext context = config.makeContext();
        const double scale = config.scale;
        seal::Decryptor decryptor(context, secretKey);

        ArithHomFA::CKKSNoEmbedEncoder encoder(context);

        ArithHomFA::SizedCipherReader reader(istream);
        while (istream.good()) {
            // get the cipher text from stdin
            seal::Ciphertext cipher;
            if (reader.read(context, cipher)) {
                seal::Plaintext plain;
                decryptor.decrypt(cipher, plain);
                const double content = encoder.decode(plain);
                ostream << content << std::endl;
            } else {
                break;
            }
        }
        spdlog::info("Given ciphertexts are decrypted with the CKKS scheme");
    }

    void do_enc_TFHEpp(const std::string &skey_filename, std::istream &istream,
                       std::ostream &ostream, const size_t num_ap) {
        auto skey = read_from_archive<SecretKey>(skey_filename);

        TRGSWLvl1FFTSerializer ser{ostream};

        size_t rest = 0;
        while (istream) {
            int ch = istream.get();
            if (ch == EOF) {
                break;
            }
            uint8_t v = ch;
            if (rest == 0) {
                rest = num_ap;
            }
            for (size_t i = 0; i < 8 && rest != 0; i++, rest--) {
                bool b = (v & 1u) != 0;
                v >>= 1;
                ser.save(encrypt_bit_to_TRGSWLvl1FFT(b, skey));
            }
        }
    }

    void do_dec_TFHEpp(const std::string &skey_filename,
                       std::istream &istream, std::ostream &ostream) {
        auto skey = read_from_archive<SecretKey>(skey_filename);
        auto enc_res = read_from_archive<TLWELvl1>(istream);
        bool res = decrypt_TLWELvl1_to_bit(enc_res, skey);
        ostream << (res ? "true" : "false");
    }

    void dumpBasicInfo(int argc, char **argv) {
        spdlog::info(R"(============================================================)");

        // Logo: thanks to:
        // https://patorjk.com/software/taag/#p=display&f=Standard&t=ArithHomFA
        spdlog::info(R"(     _         _ _   _     _   _                 _____ _)");
        spdlog::info(R"(    / \   _ __(_) |_| |__ | | | | ___  _ __ ___ |  ___/ \)");
        spdlog::info(R"(   / _ \ | '__| | __| '_ \| |_| |/ _ \| '_ ` _ \| |_ / _ \)");
        spdlog::info(R"(  / ___ \| |  | | |_| | | |  _  | (_) | | | | | |  _/ ___ \)");
        spdlog::info(R"( /_/   \_\_|  |_|\__|_| |_|_| |_|\___/|_| |_| |_|_|/_/   \_\)");
        spdlog::info(R"(                                                            )");

        // Show build config
        spdlog::info("Built with:");
#if defined(DEBUG)
        spdlog::info("\tType: debug");
#else
        spdlog::info("\tType: release");
#endif
#if defined(GIT_REVISION)
        spdlog::info("\tGit revision:" GIT_REVISION);
#else
        spdlog::info("\tGit revision: unknown");
#endif
#if defined(HOMFA_ENABLE_PROFILE)
        spdlog::info("\tProfiling: enabled");
#else
        spdlog::info("\tProfiling: disabled");
#endif

        // Show execution setting
        spdlog::info("Executed with:");
        {
            std::stringstream ss;
            for (int i = 0; i < argc; i++)
                ss << argv[i] << " ";
            spdlog::info("\tArgs: {}", ss.str());
        }
        {
            std::stringstream ss;
            if (char *envvar = std::getenv("CPUPROFILE"); envvar != nullptr)
                ss << "CPUPROFILE=" << envvar << " ";
            if (char *envvar = std::getenv("HEAPPROFILE"); envvar != nullptr)
                ss << "HEAPPROFILE=" << envvar << " ";
            spdlog::info("\tEnv var: {}", ss.str());
        }
        spdlog::info("\tConcurrency:\t{}", std::thread::hardware_concurrency());

        spdlog::info(R"(============================================================)");
    }

}

int main(int argc, char **argv) {
    Args args;
    CLI::App app{"Arith HomFA -- Oblivious Online STL Monitor via Fully Homomorphic Encryption"};
    register_general_options(app, args);
    register_SEAL(app, args);
    register_TFHEpp(app, args);
    register_ltl2spec(app, args);

    CLI11_PARSE(app, argc, argv);

    switch (args.verbosity) {
        case VERBOSITY::QUIET:
            spdlog::set_level(spdlog::level::err);
            break;
        case VERBOSITY::VERBOSE:
            spdlog::set_level(spdlog::level::debug);
            break;
        default:;
    }

    if (args.output == &std::cout) {
        auto err_logger = spdlog::stderr_color_mt("stderr");
        spdlog::set_default_logger(err_logger);
    }

    dumpBasicInfo(argc, argv);
    switch (args.type) {
        case TYPE::GENKEY_SEAL: {
            do_genkey_SEAL(*args.sealConfig, *args.output);
            break;
        }
        case TYPE::GENRELINKEY_SEAL: {
            do_genrelinkey_SEAL(*args.sealConfig, args.sealSecretKey, *args.output);
            break;
        }
        case TYPE::ENC_CKKS: {
            do_enc_SEAL(*args.sealConfig, args.sealSecretKey, *args.input, *args.output);
            break;
        }
        case TYPE::DEC_CKKS: {
            do_dec_SEAL(*args.sealConfig, args.sealSecretKey, *args.input, *args.output);
            break;
        }
        case TYPE::GENKEY_TFHEPP: {
            do_genkey_TFHEpp(*args.output);
            break;
        }
        case TYPE::GENBKEY_TFHEPP: {
            do_genbkey_TFHEpp(std::ifstream(*args.skey), *args.output);
            break;
        }
        case TYPE::ENC_TFHEPP: {
            do_enc_TFHEpp(*args.skey, *args.input, *args.output, *args.num_ap);
            break;
        }
        case TYPE::DEC_TFHEPP: {
            do_dec_TFHEpp(*args.skey, *args.input, *args.output);
            break;
        }
        case TYPE::LTL2SPEC: {
            Graph gr = Graph::from_ltl_formula(*args.formula, *args.num_vars, args.make_all_live_states_final);
            gr.dump(*args.output);
        }
        case TYPE::SPEC2SPEC: {
            // do_spec2spec(args.spec, args.minimized, args.reversed, args.negated);
            break;
        }
        case TYPE::UNSPECIFIED: {
            spdlog::info("No mode is specified");
            spdlog::info(app.help());
        }
    }
    // Close fstream
    if (typeid(*args.output) == typeid(std::ofstream)) {
      dynamic_cast<std::ofstream*>(args.output)->close();
    }

    return 0;
}
