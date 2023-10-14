/**
 * @author Masaki Waga
 * @date 2023/05/02
 */

#include <iostream>
#include <optional>

#include <CLI/CLI.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <seal/seal.h>
#include <tfhe++.hpp>

#include "archive.hpp"
#include "graph.hpp"
#include "tfhepp_util.hpp"
#include "utility.hpp"

#include "ahomfa_runner.hh"
#include "block_runner.hh"
#include "ckks_predicate.hh"
#include "offline_runner.hh"
#include "plain_runner.hh"
#include "reverse_runner.hh"
#include "seal_config.hh"
#include "sized_cipher_reader.hh"
#include "sized_cipher_writer.hh"
#include "sized_tlwe_writer.hh"

namespace {
  enum class VERBOSITY { VERBOSE, NORMAL, QUIET };

  enum class TYPE {
    UNSPECIFIED,

    POINTWISE,
    POINTWISE_TFHE,
    PLAIN,
    REVERSE,
    BLOCK,
    OFFLINE
  };

  struct Args {
    VERBOSITY verbosity = VERBOSITY::NORMAL;
    TYPE type = TYPE::UNSPECIFIED;

    bool reversed;
    std::optional<ArithHomFA::SealConfig> sealConfig;
    std::optional<std::string> spec, bkey, debug_skey, relKey;
    std::istream *input = &std::cin;
    std::ostream *output = &std::cout;
    std::optional<size_t> bootstrapping_freq, output_freq;
  };

  void register_general_options(CLI::App &app, Args &args) {
    app.add_flag_callback("-v,--verbose", [&] { args.verbosity = VERBOSITY::VERBOSE; });
    app.add_flag_callback("-q,--quiet", [&] { args.verbosity = VERBOSITY::QUIET; });
  }

  void add_common_flags(CLI::App &app, Args &args) {
    std::function<void(const std::string &)> callback = [&args](const std::string &path) {
      args.input = new std::ifstream(path);
      if (args.input->fail()) {
        spdlog::error("Failed to open the input file", strerror(errno));
        exit(1);
      }
    };
    app.add_option_function("-i,--input", callback, "The file to load the input");

    std::function<void(const std::string &)> output_callback = [&args](const std::string &path) {
      args.output = new std::ofstream(path);
    };
    app.add_option_function("-o,--output", output_callback, "The file to write the result");
  }

  void add_seal_flags(CLI::App &app, Args &args) {
    std::function<void(const std::string &)> configCallback = [&args](const std::string &path) {
      std::ifstream istream(path);
      if (!istream) {
        spdlog::error("Failed to open the SEAL's configuration file", strerror(errno));
        exit(1);
      }
      cereal::JSONInputArchive archive(istream);
      args.sealConfig = ArithHomFA::SealConfig::load(archive);
    };
    app.add_option_function("-c,--config", configCallback, "Configuration file of SEAL")->required();
    app.add_option("-r,--relinearization-key", args.relKey, "The relinearization key of SEAL")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("--debug-seal-key", args.debug_skey, "The secret key of SEAL (for debugging)")
        ->check(CLI::ExistingFile);
  }

  void add_tfhepp_flags(CLI::App &app, Args &args) {
    app.add_option("-b,--bootstrapping-key", args.bkey, "The bootstrapping key of TFHEpp")
        ->required()
        ->check(CLI::ExistingFile);
  }

  void add_spec_flag(CLI::App &app, Args &args) {
    app.add_option("-f,--specification", args.spec, "The specification to be monitored")->required();
  }

  void register_pointwise(CLI::App &app, Args &args) {
    CLI::App *pointwise = app.add_subcommand("pointwise", "Evaluate the given signal point-wise (for debugging)");
    add_common_flags(*pointwise, args);
    add_seal_flags(*pointwise, args);
    pointwise->parse_complete_callback([&args] { args.type = TYPE::POINTWISE; });
    register_general_options(*pointwise, args);
  }

  void register_pointwise_tfhe(CLI::App &app, Args &args) {
    CLI::App *pointwise = app.add_subcommand(
        "pointwise-tfhe", "Evaluate the given signal point-wise and make it TLWElv1 (for debugging)");
    add_common_flags(*pointwise, args);
    add_seal_flags(*pointwise, args);
    add_tfhepp_flags(*pointwise, args);
    pointwise->parse_complete_callback([&args] { args.type = TYPE::POINTWISE_TFHE; });
    register_general_options(*pointwise, args);
  }

  void register_plain(CLI::App &app, Args &args) {
    CLI::App *plain = app.add_subcommand("plain", "Execute a monitor with plaintext (for debugging)");
    add_common_flags(*plain, args);
    add_spec_flag(*plain, args);
    // SEAL's configuration is required even for plain
    std::function<void(const std::string &)> configCallback = [&args](const std::string &path) {
      std::ifstream istream(path);
      if (!istream) {
        spdlog::error("Failed to open the SEAL's configuration file", strerror(errno));
        exit(1);
      }
      cereal::JSONInputArchive archive(istream);
      args.sealConfig = ArithHomFA::SealConfig::load(archive);
    };
    plain->add_option_function("-c,--config", configCallback, "Configuration file of SEAL")->required();
    plain->parse_complete_callback([&args] { args.type = TYPE::PLAIN; });
    register_general_options(*plain, args);
  }

  void register_offline(CLI::App &app, Args &args) {
    CLI::App *offline = app.add_subcommand("offline", "Execute a monitor with the offline algorithm");
    add_common_flags(*offline, args);
    add_seal_flags(*offline, args);
    add_tfhepp_flags(*offline, args);
    add_spec_flag(*offline, args);
    offline->add_option("-l,--bootstrapping-freq", args.bootstrapping_freq)->required()->check(CLI::PositiveNumber);
    offline->parse_complete_callback([&args] { args.type = TYPE::OFFLINE; });
    register_general_options(*offline, args);
  }

  void register_reverse(CLI::App &app, Args &args) {
    CLI::App *reverse = app.add_subcommand("reverse", "Execute a monitor with the reverse algorithm");
    add_common_flags(*reverse, args);
    add_seal_flags(*reverse, args);
    add_tfhepp_flags(*reverse, args);
    add_spec_flag(*reverse, args);
    reverse->add_option("-l,--bootstrapping-freq", args.bootstrapping_freq)->required()->check(CLI::PositiveNumber);
    reverse->add_flag("--reversed", args.reversed, "The given specification is already reversed");
    reverse->parse_complete_callback([&args] { args.type = TYPE::REVERSE; });
    register_general_options(*reverse, args);
  }

  void register_block(CLI::App &app, Args &args) {
    CLI::App *block = app.add_subcommand("block", "Execute a monitor with the block algorithm");
    add_common_flags(*block, args);
    add_seal_flags(*block, args);
    add_tfhepp_flags(*block, args);
    add_spec_flag(*block, args);
    block->add_option("-l,--block-size", args.output_freq)->required()->check(CLI::PositiveNumber);
    block->parse_complete_callback([&args] { args.type = TYPE::BLOCK; });
    register_general_options(*block, args);
  }

  void do_pointwise(const ArithHomFA::SealConfig &config, const std::string &relinKeysPath, std::istream &istream,
                    std::ostream &ostream) {
    const auto context = config.makeContext();
    ArithHomFA::CKKSPredicate predicate{context, config.scale};
    seal::RelinKeys relinKeys;
    {
      std::ifstream relinKeysStream(relinKeysPath);
      if (!relinKeysStream) {
        spdlog::error("Failed to open relinearization key", strerror(errno));
        exit(1);
      }
      relinKeys.load(context, relinKeysStream);
    };
    predicate.setRelinKeys(relinKeys);
    ArithHomFA::SizedCipherReader reader{istream};
    ArithHomFA::SizedCipherWriter writer{ostream};
    std::vector<seal::Ciphertext> valuations, results;
    valuations.resize(ArithHomFA::CKKSPredicate::getSignalSize());
    results.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
    while (istream.good()) {
      // get the content
      for (auto &valuation: valuations) {
        if (!reader.read(context, valuation)) {
          return;
        }
      }
      // Evaluate
      predicate.eval(valuations, results);
      // dump the cipher text
      for (const auto &result: results) {
        writer.write(result);
      }
    }
  }

  void do_pointwise_tfhe(const ArithHomFA::SealConfig &config, const std::string &bkey_filename,
                         const std::string &relinKeysPath, std::istream &istream, std::ostream &ostream) {
    const auto context = config.makeContext();
    ArithHomFA::CKKSPredicate predicate{context, config.scale};
    auto bkey = read_from_archive<ArithHomFA::BootstrappingKey>(bkey_filename);
    seal::RelinKeys relinKeys;
    {
      std::ifstream relinKeysStream(relinKeysPath);
      if (!relinKeysStream) {
        spdlog::error("Failed to open the relinearization key", strerror(errno));
        exit(1);
      }
      relinKeys.load(context, relinKeysStream);
    };
    predicate.setRelinKeys(relinKeys);
    ArithHomFA::SizedCipherReader reader{istream};
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> writer{ostream};
    std::vector<seal::Ciphertext> valuations, results;
    valuations.resize(ArithHomFA::CKKSPredicate::getSignalSize());
    results.resize(ArithHomFA::CKKSPredicate::getPredicateSize());
    ArithHomFA::CKKSToTFHE converter{context};
    converter.initializeConverter(bkey);
    while (istream.good()) {
      // get the content
      for (auto &valuation: valuations) {
        if (!reader.read(context, valuation)) {
          return;
        }
      }
      // Evaluate and dump the cipher text
      predicate.eval(valuations, results);
      for (int i = 0; i < results.size(); ++i) {
        TFHEpp::TLWE<TFHEpp::lvl1param> tlwe;
        converter.toLv1TLWE(results.at(i), tlwe, ArithHomFA::CKKSPredicate::getReferences().at(i));
        writer.write(tlwe);
      }
    }
  }

  void do_plain(const ArithHomFA::SealConfig &config, const std::string &graphFilename, std::istream &istream,
                std::ostream &ostream) {
    const auto graph = Graph::from_file(graphFilename);
    ArithHomFA::PlainRunner runner(config, graph);
    std::vector<double> valuations;
    valuations.resize(ArithHomFA::CKKSPredicate::getSignalSize());
    while (istream.good()) {
      // Get the content
      for (auto &valuation: valuations) {
        istream >> valuation;
        if (!istream.good()) {
          return;
        }
      }
      // Evaluate
      const auto result = runner.feed(valuations);
      // Print the result
      ostream << std::boolalpha << result << std::endl;
    }

    runner.printTime();
  }

  void do_offline(const ArithHomFA::SealConfig &config, const std::string &spec_filename,
                  const std::string &bkey_filename, const std::string &relinKeysPath, std::istream &istream,
                  std::ostream &ostream, int boot_interval) {
    const seal::SEALContext context = config.makeContext();
    spdlog::debug("Parameters:");
    spdlog::debug("\tscale: {}", config.scale);
    spdlog::debug("\tspec_filename: {}", spec_filename);
    spdlog::debug("\tbkey_filename: {}", bkey_filename);
    spdlog::debug("\trelinKeysPath: {}", relinKeysPath);
    spdlog::debug("\tboot_interval: {}", boot_interval);
    auto bkey = read_from_archive<ArithHomFA::BootstrappingKey>(bkey_filename);
    assert(bkey.ekey && bkey.tlwel1_trlwel1_ikskey && bkey.bkfft && bkey.kskh2m && bkey.kskm2l);
    seal::RelinKeys relinKeys;
    {
      std::ifstream relinKeysStream(relinKeysPath);
      if (!relinKeysStream) {
        spdlog::error("Failed to open the relinearization key", strerror(errno));
        exit(1);
      }
      relinKeys.load(context, relinKeysStream);
    }

    ArithHomFA::SizedCipherReader reader(istream);
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> writer(ostream);
    std::vector<seal::Ciphertext> ciphers;

    while (istream.good()) {
      // get the cipher text from stdin
      seal::Ciphertext cipher;
      if (reader.read(context, cipher)) {
        ciphers.emplace_back(std::move(cipher));
      } else {
        break;
      }
    }

    assert(ciphers.size() % ArithHomFA::CKKSPredicate::getSignalSize() == 0);
    ArithHomFA::OfflineRunner runner(context, config.scale, spec_filename,
                                     ciphers.size() / ArithHomFA::CKKSPredicate::getSignalSize(), boot_interval, bkey,
                                     ArithHomFA::CKKSPredicate::getReferences());
    runner.setRelinKeys(relinKeys);

    std::vector<seal::Ciphertext> valuations;
    valuations.reserve(ArithHomFA::CKKSPredicate::getSignalSize());
    for (const auto &cipher: boost::adaptors::reverse(ciphers)) {
      valuations.push_back(cipher);
      if (valuations.size() == ArithHomFA::CKKSPredicate::getSignalSize()) {
        writer.write(runner.feed(valuations));
        valuations.clear();
      }
    }

    runner.printTime();
  }

  void run_online(const seal::SEALContext &context, ArithHomFA::AbstractRunner *runner, std::istream &istream,
                  std::ostream &ostream, const std::optional<std::string> &debug_skey) {
    seal::SecretKey secretKey;
    if (debug_skey) {
      std::ifstream secretKeyStream{*debug_skey};
      if (!secretKeyStream) {
        spdlog::error("Failed to open the secret key", strerror(errno));
        exit(1);
      }
      secretKey.load(context, secretKeyStream);
    }
    ArithHomFA::CKKSNoEmbedEncoder encoder(context);
    ArithHomFA::SizedCipherReader reader(istream);
    ArithHomFA::SizedTLWEWriter<TFHEpp::lvl1param> writer(ostream);

    std::vector<seal::Ciphertext> valuations;
    valuations.resize(ArithHomFA::CKKSPredicate::getSignalSize());
    spdlog::debug("Start monitoring with signal size: {}", ArithHomFA::CKKSPredicate::getSignalSize());

    while (istream.good()) {
      // get the content
      for (auto &valuation: valuations) {
        if (!reader.read(context, valuation)) {
          runner->printTime();
          return;
        }
        if (debug_skey) {
          seal::Plaintext plain;
          seal::Decryptor decryptor(context, secretKey);
          decryptor.decrypt(valuation, plain);
          spdlog::debug("valuation (encrypted): {}", encoder.decode(plain));
        }
      }
      // Evaluate
      writer.write(runner->feed(valuations));
    }

    runner->printTime();
  }

  void do_reverse(const ArithHomFA::SealConfig &config, const std::string &spec_filename,
                  const std::string &bkey_filename, const std::string &relinKeysPath, std::istream &istream,
                  std::ostream &ostream, int boot_interval, bool reversed,
                  const std::optional<std::string> &debug_skey) {
    const seal::SEALContext context = config.makeContext();
    spdlog::debug("Parameters:");
    spdlog::debug("\tscale: {}", config.scale);
    spdlog::debug("\tspec_filename: {}", spec_filename);
    spdlog::debug("\tbkey_filename: {}", bkey_filename);
    spdlog::debug("\trelinKeysPath: {}", relinKeysPath);
    spdlog::debug("\tboot_interval: {}", boot_interval);
    auto bkey = read_from_archive<ArithHomFA::BootstrappingKey>(bkey_filename);
    assert(bkey.ekey && bkey.tlwel1_trlwel1_ikskey && bkey.bkfft && bkey.kskh2m && bkey.kskm2l);
    seal::RelinKeys relinKeys;
    {
      std::ifstream relinKeysStream(relinKeysPath);
      if (!relinKeysStream) {
        spdlog::error("Failed to open the relinearization key", strerror(errno));
        exit(1);
      }
      relinKeys.load(context, relinKeysStream);
    }

    ArithHomFA::ReverseRunner runner(context, config.scale, spec_filename, boot_interval, bkey,
                                     ArithHomFA::CKKSPredicate::getReferences(), reversed);
    spdlog::debug("Constructed the reverse runner");
    runner.setRelinKeys(relinKeys);
    run_online(context, &runner, istream, ostream, debug_skey);
  }

  void do_block(const ArithHomFA::SealConfig &config, const std::string &spec_filename,
                const std::string &bkey_filename, const std::string &relinKeysPath, std::istream &istream,
                std::ostream &ostream, int blockSize, const std::optional<std::string> &debug_skey) {
    const seal::SEALContext context = config.makeContext();
    spdlog::debug("Parameters:");
    spdlog::debug("\tscale: {}", config.scale);
    spdlog::debug("\tspec_filename: {}", spec_filename);
    spdlog::debug("\tbkey_filename: {}", bkey_filename);
    spdlog::debug("\trelinKeysPath: {}", relinKeysPath);
    spdlog::debug("\tblockSize: {}", blockSize);
    auto bkey = read_from_archive<ArithHomFA::BootstrappingKey>(bkey_filename);
    assert(bkey.ekey && bkey.tlwel1_trlwel1_ikskey && bkey.bkfft && bkey.kskh2m && bkey.kskm2l);
    seal::RelinKeys relinKeys;
    {
      std::ifstream relinKeysStream(relinKeysPath);
      if (!relinKeysStream) {
        spdlog::error("Failed to open the relinearization key", strerror(errno));
        exit(1);
      }
      relinKeys.load(context, relinKeysStream);
    }

    ArithHomFA::BlockRunner runner(context, config.scale, spec_filename, blockSize, bkey,
                                   ArithHomFA::CKKSPredicate::getReferences());
    spdlog::debug("Constructed the block runner");
    runner.setRelinKeys(relinKeys);
    run_online(context, &runner, istream, ostream, debug_skey);
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
  CLI::App app{"Arith HomFA -- Oblivious Online STL Monitor via Fully "
               "Homomorphic Encryption"};
  register_general_options(app, args);
  register_pointwise(app, args);
  register_pointwise_tfhe(app, args);
  register_plain(app, args);
  register_offline(app, args);
  register_reverse(app, args);
  register_block(app, args);

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
    case TYPE::POINTWISE: {
      ArithHomFA::PointwiseRunner runner(*args.sealConfig, *args.relKey, *args.input, *args.output);
      runner.runPointwise();
      break;
    }
    case TYPE::POINTWISE_TFHE: {
      ArithHomFA::PointwiseRunner runner(*args.sealConfig, *args.bkey, *args.relKey, *args.input, *args.output);
      runner.runPointwiseTFHE();
      break;
    }
    case TYPE::PLAIN: {
      do_plain(*args.sealConfig, *args.spec, *args.input, *args.output);
      break;
    }
    case TYPE::OFFLINE: {
      do_offline(*args.sealConfig, *args.spec, *args.bkey, *args.relKey, *args.input, *args.output,
                 *args.bootstrapping_freq);
      break;
    }
    case TYPE::REVERSE: {
      do_reverse(*args.sealConfig, *args.spec, *args.bkey, *args.relKey, *args.input, *args.output,
                 *args.bootstrapping_freq, args.reversed, args.debug_skey);
      break;
    }
    case TYPE::BLOCK: {
      do_block(*args.sealConfig, *args.spec, *args.bkey, *args.relKey, *args.input, *args.output, *args.output_freq,
               args.debug_skey);
      break;
    }
    case TYPE::UNSPECIFIED: {
      spdlog::info("No mode is specified");
      spdlog::info(app.help());
    }
  }

  return 0;
}
