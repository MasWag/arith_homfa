/**
 * @author Masaki Waga
 * @date 2023/04/20
 */

#include <iostream>
#include <optional>
#include <unordered_map>

#include <CLI/CLI.hpp>
#include <seal/seal.h>
#include <tfhe++.hpp>
#include <cereal/cereal.hpp>

#include "archive.hpp"
#include "graph.hpp"
#include "tfhepp_util.hpp"
#include "utility.hpp"

#include "key_loader.hh"
#include "bootstrapping_key.hh"
#include "ckks_no_embed.hh"
#include "ckks_to_tfhe.hh"
#include "seal_config.hh"
#include "sized_cipher_reader.hh"
#include "sized_cipher_writer.hh"
#include "sized_tlwe_reader.hh"
#include "sized_tlwe_writer.hh"

namespace {
  enum class VERBOSITY { VERBOSE, NORMAL, QUIET };

  enum class TYPE {
    UNSPECIFIED,

    GENKEY_SEAL,
    GEN_PUBLIC_KEY_SEAL,
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

    bool make_all_live_states_final = false, minimized = false, reversed = false, negated = false, vertical = false;
    std::optional<ArithHomFA::SealConfig> sealConfig;
    std::optional<std::string> spec, skey, sealSecretKey, sealPublicKey, bkey, output_dir, debug_skey, formula, online_method;
    std::istream *input = &std::cin;
    std::ostream *output = &std::cout;
    std::optional<size_t> num_vars, queue_size, bootstrapping_freq, max_second_lut_depth, num_ap, output_freq;
  };

  void register_general_options(CLI::App &app, Args &args) {
    app.add_flag_callback("-v,--verbose", [&] { args.verbosity = VERBOSITY::VERBOSE; });
    app.add_flag_callback("-q,--quiet", [&] { args.verbosity = VERBOSITY::QUIET; });
  }

  void register_SEAL(CLI::App &app, Args &args) {
    CLI::App *ckks = app.add_subcommand("ckks", "Subcommands related to SEAL");
    std::vector<CLI::App *> subcommands;
    std::vector<CLI::App *> requiresKey;
    std::vector<CLI::App *> requiresPrivateOrPublicKey;
    std::vector<CLI::App *> withInput;
    CLI::App *genkey = ckks->add_subcommand("genkey", "Generate secret key");
    subcommands.push_back(genkey);
    CLI::App *genpkey = ckks->add_subcommand("genpkey", "Generate public key");
    subcommands.push_back(genpkey);
    requiresKey.push_back(genpkey);
    CLI::App *genrelinkey = ckks->add_subcommand("genrelinkey", "Generate relin key");
    subcommands.push_back(genrelinkey);
    requiresKey.push_back(genrelinkey);
    CLI::App *enc = ckks->add_subcommand("enc", "Encrypt input file");
    subcommands.push_back(enc);
    requiresPrivateOrPublicKey.push_back(enc);
    withInput.push_back(enc);
    CLI::App *dec = ckks->add_subcommand("dec", "Decrypt input file");
    subcommands.push_back(dec);
    requiresKey.push_back(dec);
    withInput.push_back(dec);

    // General options for CKKS
    for (auto subcommand: subcommands) {
      std::function<void(const std::string &)> configCallback = [&args](const std::string &path) {
        std::ifstream istream(path);
        if (!istream) {
          spdlog::error("Failed to open the SEAL's configuration file", strerror(errno));
          exit(1);
        }
        cereal::JSONInputArchive archive(istream);
        args.sealConfig = ArithHomFA::SealConfig::load(archive);
      };
      subcommand->add_option_function("-c,--config", configCallback, "The configuration file of SEAL")
          ->required()
          ->check(CLI::ExistingFile);

      std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
        args.output = new std::ofstream(path);
      };
      subcommand->add_option_function("-o,--output", output_callback, "The file to write the result");
    }

    // Options for subcommands with key
    for (auto subcommand: requiresKey) {
      subcommand->add_option("-K,--secret-key", args.sealSecretKey, "The secret key of SEAL")
          ->required()
          ->check(CLI::ExistingFile);
    }

    // Options for subcommands with either public or secret key
    for (auto subcommand: requiresPrivateOrPublicKey) {
      auto group = subcommand->add_option_group("requiresPrivateOrPublicKey");
      group->add_option("-K,--secret-key", args.sealSecretKey, "The secret key of SEAL")
          ->check(CLI::ExistingFile);
      group->add_option("-k,--public-key", args.sealPublicKey, "The public key of SEAL")
          ->check(CLI::ExistingFile);

      group->require_option(1);
    }

    // Options for subcommands with inputs
    for (auto subcommand: withInput) {
      std::function<void(const std::string &)> callback = [&args](const std::string &path) {
        args.input = new std::ifstream(path);
        if (args.input->fail()) {
          spdlog::error("Failed to open the input file", strerror(errno));
          exit(1);
        }
      };
      subcommand->add_option_function("-i,--input", callback, "The file to load the input");
    }

    // genkey
    genkey->parse_complete_callback([&args] { args.type = TYPE::GENKEY_SEAL; });
    // genpkey
    genpkey->parse_complete_callback([&args] { args.type = TYPE::GEN_PUBLIC_KEY_SEAL; });
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
    std::vector<CLI::App *> withSeal;
    CLI::App *genkey = tfhe->add_subcommand("genkey", "Generate secret key");
    subcommands.push_back(genkey);
    CLI::App *genbkey = tfhe->add_subcommand("genbkey", "Generate bootstrap key");
    subcommands.push_back(genbkey);
    requiresKey.push_back(genbkey);
    withSeal.push_back(genbkey);
    CLI::App *enc = tfhe->add_subcommand("enc", "Encrypt input file");
    subcommands.push_back(enc);
    requiresKey.push_back(enc);
    withInput.push_back(enc);
    CLI::App *dec = tfhe->add_subcommand("dec", "Decrypt input file");
    subcommands.push_back(dec);
    requiresKey.push_back(dec);
    withInput.push_back(dec);
    dec->add_flag("--vertical", args.vertical, "Use -1/4, 1/4 as message space");

    // General options for TFHE
    for (auto subcommand: subcommands) {
      std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
        args.output = new std::ofstream(path);
      };
      subcommand->add_option_function("-o,--output", output_callback, "The file to write the result");
    }

    // Options for subcommands with key
    for (auto subcommand: requiresKey) {
      subcommand->add_option("-K,--secret-key", args.skey, "The secret key of TFHEpp")
          ->required()
          ->check(CLI::ExistingFile);
    }

    // Options for subcommands with SEAL's configuration and secret key
    for (auto subcommand: withSeal) {
      std::function<void(const std::string &)> configCallback = [&args](const std::string &path) {
        std::ifstream istream(path);
        if (!istream) {
          spdlog::error("Failed to open the SEAL's configuration file", strerror(errno));
          exit(1);
        }
        cereal::JSONInputArchive archive(istream);
        args.sealConfig = ArithHomFA::SealConfig::load(archive);
      };
      subcommand->add_option_function("-c,--config", configCallback, "The configuration file of SEAL")
          ->required()
          ->check(CLI::ExistingFile);

      subcommand->add_option("-S,--seal-secret-key", args.sealSecretKey, "The secret key of SEAL")
          ->required()
          ->check(CLI::ExistingFile);
    }

    // Options for subcommands with inputs
    for (auto subcommand: withInput) {
      std::function<void(const std::string &)> callback = [&args](const std::string &path) {
        args.input = new std::ifstream(path);
        if (args.input->fail()) {
          spdlog::error("Failed to open the input file", strerror(errno));
          exit(1);
        }
      };
      subcommand->add_option_function("-i,--input", callback, "The file to load the input");
    }

    // genkey
    genkey->parse_complete_callback([&args] { args.type = TYPE::GENKEY_TFHEPP; });
    // genbkey
    genbkey->parse_complete_callback([&args] { args.type = TYPE::GENBKEY_TFHEPP; });
    // enc
    enc->parse_complete_callback([&args] { args.type = TYPE::ENC_TFHEPP; });
    // dec
    dec->parse_complete_callback([&args] { args.type = TYPE::DEC_TFHEPP; });
  }

  void register_ltl2spec(CLI::App &app, Args &args) {
    CLI::App *ltl2spec = app.add_subcommand("ltl2spec", "Convert LTL to spec format for ArithHomFA");
    std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
      args.output = new std::ofstream(path);
    };
    ltl2spec->add_option("-e,--formula", args.formula, "The LTL formula")->required();
    ltl2spec->add_option_function("-o,--output", output_callback, "The file to write the result");
    ltl2spec->add_option("-n,--num-vars", args.num_vars, "The number of variables in the given LTL formula")
        ->required();
    ltl2spec->add_option("--make-all-live-states-final", args.make_all_live_states_final,
                         "If make all live states final");
    ltl2spec->parse_complete_callback([&args] { args.type = TYPE::LTL2SPEC; });
  }

  void register_spec2spec(CLI::App &app, Args &args) {
    CLI::App *ltl2spec = app.add_subcommand("spec2spec", "Modify a specification for ArithHomFA");
    std::function<void(const std::string &)> input_callback = [&args](const std::string &path) {
      args.input = new std::ifstream(path);
      if (args.input->fail()) {
        spdlog::error("Failed to open the input file", strerror(errno));
        exit(1);
      }
    };
    ltl2spec->add_option_function("-i,--input", input_callback, "The file to load the input");
    std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
      args.output = new std::ofstream(path);
    };
    ltl2spec->add_flag("--reverse", args.reversed, "Reverse the given specification");
    ltl2spec->add_flag("--negate", args.negated, "Negate the given specification");
    ltl2spec->add_flag("--minimize", args.minimized, "Minimize the given specification");
    ltl2spec->add_option_function("-o,--output", output_callback, "The file to write the result");
    ltl2spec->parse_complete_callback([&args] { args.type = TYPE::SPEC2SPEC; });
  }

  void do_genkey_SEAL(const ArithHomFA::SealConfig &config, std::ostream &ostream) {
    const seal::SEALContext context = config.makeContext();
    seal::KeyGenerator keygen(context);
    const seal::SecretKey &secretKey = keygen.secret_key();

    secretKey.save(ostream);
  }

  void do_gen_public_key_SEAL(const ArithHomFA::SealConfig &config, const std::string &secretKeyPath,
                              std::ostream &ostream) {
    const seal::SEALContext context = config.makeContext();
    const seal::SecretKey secretKey = ArithHomFA::KeyLoader::loadSecretKey(context, secretKeyPath);
    seal::KeyGenerator keygen(context, secretKey);

    seal::PublicKey publicKey;
    keygen.create_public_key(publicKey);
    publicKey.save(ostream);
  }

  void do_genrelinkey_SEAL(const ArithHomFA::SealConfig &config, const std::string &secretKeyPath,
                           std::ostream &ostream) {
    const seal::SEALContext context = config.makeContext();
    const seal::SecretKey secretKey = ArithHomFA::KeyLoader::loadSecretKey(context, secretKeyPath);
    seal::KeyGenerator keygen(context, secretKey);

    seal::RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    relin_keys.save(ostream);
  }

  void do_genkey_TFHEpp(std::ostream &ostream) {
    spdlog::info("Generate secret key of TFHEpp");
    TFHEpp::SecretKey skey;
    write_to_archive(ostream, skey);
  }

  void do_genbkey_TFHEpp(const ArithHomFA::SealConfig &config, const std::string &secretKeyPath,
                         std::istream &&skey_stream, std::ostream &ostream) {
    spdlog::info("Generate bootstrapping key of TFHEpp");
    const seal::SEALContext context = config.makeContext();
    ArithHomFA::CKKSToTFHE converter(context);
    auto skey = read_from_archive<TFHEpp::SecretKey>(skey_stream);
    TFHEpp::Key<TFHEpp::lvl3param> lvl3Key;
    const seal::SecretKey secretKey = ArithHomFA::KeyLoader::loadSecretKey(context, secretKeyPath);
    converter.toLv3Key(secretKey, lvl3Key);
    ArithHomFA::BootstrappingKey bkey{skey, lvl3Key};
    write_to_archive(ostream, bkey);
  }

  void do_enc_SEAL_with_secret_key(const ArithHomFA::SealConfig &config, const std::string &secretKeyPath,
                                   std::istream &istream, std::ostream &ostream) {
    const seal::SEALContext context = config.makeContext();
    const double scale = config.scale;
    const seal::SecretKey secretKey = ArithHomFA::KeyLoader::loadSecretKey(context, secretKeyPath);
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

  void do_enc_SEAL_with_public_key(const ArithHomFA::SealConfig &config, const std::string &publicKeyPath,
                                   std::istream &istream, std::ostream &ostream) {
    const seal::SEALContext context = config.makeContext();
    const double scale = config.scale;
    const seal::PublicKey publicKey = ArithHomFA::KeyLoader::loadPublicKey(context, publicKeyPath);
    seal::Encryptor encryptor(context, publicKey);

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
      encryptor.encrypt(plain, cipher);
      // dump the cipher text to stdout
      writer.write(cipher);
    }
    spdlog::info("Given contents are encrypted with the CKKS scheme");
  }


  void do_enc_SEAL(const ArithHomFA::SealConfig &config, const std::optional<std::string> &secretKeyPath,
                   const std::optional<std::string> &publicKeyPath, std::istream &istream, std::ostream &ostream) {
    if (secretKeyPath) {
      do_enc_SEAL_with_secret_key(config, *secretKeyPath, istream, ostream);
    } else if (publicKeyPath) {
      do_enc_SEAL_with_public_key(config, *publicKeyPath, istream, ostream);
    } else {
      throw std::runtime_error("No key is given");
    }
  }

  void do_dec_SEAL(const ArithHomFA::SealConfig &config, const std::string &secretKeyPath, std::istream &istream,
                   std::ostream &ostream) {
    const seal::SEALContext context = config.makeContext();
    const seal::SecretKey secretKey = ArithHomFA::KeyLoader::loadSecretKey(context, secretKeyPath);
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

  void do_enc_TFHEpp(const std::string &skey_filename, std::istream &istream, std::ostream &ostream) {
    auto skey = read_from_archive<TFHEpp::SecretKey>(skey_filename);

    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> writer{ostream};
    bool content;
    while (istream.good()) {
      // get the content from stdin
      istream >> content;
      if (!istream.good()) {
        break;
      }
      spdlog::debug("Content: {}", content);
      writer.write(
          TFHEpp::tlweSymEncrypt<TFHEpp::lvl1param>(content ? 1u << 31 : 0, TFHEpp::lvl1param::α, skey.key.lvl1));
    }
    spdlog::info("Given contents are encrypted with the TFHE scheme");
  }

  void do_dec_TFHEpp(const std::string &skey_filename, std::istream &istream, std::ostream &ostream,
                     const bool vertical) {
    auto skey = read_from_archive<TFHEpp::SecretKey>(skey_filename);

    ArithHomFA::SizedTLWEReader<TFHEpp::lvl1param> reader{istream};
    while (istream.good()) {
      // get the cipher text from stdin
      TFHEpp::TLWE<TFHEpp::lvl1param> cipher;
      if (reader.read(cipher)) {
        const bool res = vertical ? TFHEpp::tlweSymDecrypt<TFHEpp::lvl1param>(cipher, skey.key.lvl1) :
                                    decrypt_TLWELvl1_to_bit(cipher, skey);
        ostream << (res ? "true" : "false") << std::endl;
      } else {
        break;
      }
    }
    spdlog::info("Given ciphertexts are decrypted with the TFHE scheme");
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

} // namespace

int main(int argc, char **argv) {
  Args args;
  CLI::App app{"Arith HomFA -- Oblivious Online STL Monitor via Fully Homomorphic Encryption"};
  register_general_options(app, args);
  register_SEAL(app, args);
  register_TFHEpp(app, args);
  register_ltl2spec(app, args);
  register_spec2spec(app, args);

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
#if defined(USE_80BIT_SECURITY)
    spdlog::debug("Use 80bit security parameter");
#elif defined(USE_CGGI19)
    spdlog::debug("Use CGGI19 security parameter");
#elif defined(USE_CONCRETE)
    spdlog::debug("Use concrete security parameter");
#elif defined(USE_TFHE_RS)
    spdlog::debug("Use TFHE-RS's security parameter");
#elif defined(USE_TERNARY)
    spdlog::debug("Use ternary security parameter");
#else
    spdlog::debug("Use 128bit security parameter");
#endif
  switch (args.type) {
    case TYPE::GENKEY_SEAL: {
      do_genkey_SEAL(*args.sealConfig, *args.output);
      break;
    }
    case TYPE::GEN_PUBLIC_KEY_SEAL: {
      do_gen_public_key_SEAL(*args.sealConfig, *args.sealSecretKey, *args.output);
      break;
    }
    case TYPE::GENRELINKEY_SEAL: {
      do_genrelinkey_SEAL(*args.sealConfig, *args.sealSecretKey, *args.output);
      break;
    }
    case TYPE::ENC_CKKS: {
      do_enc_SEAL(*args.sealConfig, args.sealSecretKey, args.sealPublicKey, *args.input, *args.output);
      break;
    }
    case TYPE::DEC_CKKS: {
      do_dec_SEAL(*args.sealConfig, *args.sealSecretKey, *args.input, *args.output);
      break;
    }
    case TYPE::GENKEY_TFHEPP: {
      do_genkey_TFHEpp(*args.output);
      break;
    }
    case TYPE::GENBKEY_TFHEPP: {
      do_genbkey_TFHEpp(*args.sealConfig, *args.sealSecretKey, std::ifstream(*args.skey), *args.output);
      break;
    }
    case TYPE::ENC_TFHEPP: {
      do_enc_TFHEpp(*args.skey, *args.input, *args.output);
      break;
    }
    case TYPE::DEC_TFHEPP: {
      do_dec_TFHEpp(*args.skey, *args.input, *args.output, args.vertical);
      break;
    }
    case TYPE::LTL2SPEC: {
      Graph gr = Graph::from_ltl_formula(*args.formula, *args.num_vars, args.make_all_live_states_final);
      spdlog::debug("Spec is constructed");
      gr.dump(*args.output);
      spdlog::debug("Spec is dumped");
      break;
    }
    case TYPE::SPEC2SPEC: {
      Graph gr = Graph::from_istream(*args.input);
      spdlog::debug("Spec is loaded");
      if (args.negated) {
        spdlog::debug("Negate the spec");
        gr = gr.negated();
      }
      if (args.reversed) {
        spdlog::debug("Reverse the spec");
        gr = gr.reversed();
      }
      if (args.minimized) {
        spdlog::debug("Minimize the spec");
        gr = gr.minimized();
      }
      gr.dump(*args.output);
      spdlog::debug("Spec is loaded");
      break;
    }
    case TYPE::UNSPECIFIED: {
      spdlog::info("No mode is specified");
      spdlog::info(app.help());
      break;
    }
  }
  // Close fstream
  if (typeid(*args.output) == typeid(std::ofstream)) {
    spdlog::debug("Output stream is closed");
    dynamic_cast<std::ofstream *>(args.output)->close();
    if (!args.output) {
      std::perror("ArithHomFA");
    }
  }

  return 0;
}
