/**
 * @author Masaki Waga
 * @date 2023/05/02
 */

#include <iostream>
#include <optional>
#include <unordered_map>

#include <CLI/CLI.hpp>
#include <seal/seal.h>
#include <tfhe++.hpp>

#include "archive.hpp"
#include "graph.hpp"
#include "tfhepp_util.hpp"
#include "utility.hpp"

#include "ckks_no_embed.hh"
#include "ckks_predicate.hh"
#include "plain_runner.hh"
#include "seal_config.hh"
#include "sized_cipher_reader.hh"
#include "sized_cipher_writer.hh"

namespace {
  enum class VERBOSITY { VERBOSE, NORMAL, QUIET };

  enum class TYPE {
    UNSPECIFIED,

    POINTWISE,
    PLAIN,
    REVERSE,
    BLOCK,
    OFFLINE
  };

  struct Args {
    VERBOSITY verbosity = VERBOSITY::NORMAL;
    TYPE type = TYPE::UNSPECIFIED;

    bool minimized = false, reversed = false, negated = false,
         make_all_live_states_final = false, is_spec_reversed = false,
         sanitize_result = false;
    std::optional<ArithHomFA::SealConfig> sealConfig;
    std::optional<std::string> spec, skey, bkey, output_dir, debug_skey,
        formula, online_method;
    std::istream *input = &std::cin;
    std::ostream *output = &std::cout;
    seal::RelinKeys relKey;
    std::optional<size_t> num_vars, queue_size, bootstrapping_freq,
        max_second_lut_depth, num_ap, output_freq;
  };

  void register_general_options(CLI::App &app, Args &args) {
    app.add_flag_callback("--verbose",
                          [&] { args.verbosity = VERBOSITY::VERBOSE; });
    app.add_flag_callback("--quiet",
                          [&] { args.verbosity = VERBOSITY::QUIET; });
  }

  void add_common_flags(CLI::App &app, Args &args) {
    std::function<void(const std::string &)> configCallback =
        [&args](const std::string &path) {
          std::ifstream istream(path);
          cereal::JSONInputArchive archive(istream);
          args.sealConfig = ArithHomFA::SealConfig::load(archive);
        };
    app.add_option_function("-c,--config", configCallback,
                            "Configuration file of SEAL")
        ->required();

    std::function<void(const std::string &)> callback =
        [&args](const std::string &path) {
          args.input = new std::ifstream(path);
        };
    app.add_option_function("-i,--input", callback,
                            "The file to load the input");

    std::function<void(const std::string &)> output_callback =
        [&args](const std::string &path) {
          args.output = new std::ofstream(path);
        };
    app.add_option_function("-o,--output", output_callback,
                            "The file to write the result");

    std::function<void(const std::string &)> keyCallback =
        [&args](const std::string &path) {
          std::ifstream istream(path);
          args.relKey.load(args.sealConfig->makeContext(), istream);
        };
    app.add_option_function("-s,--seal-key", keyCallback,
                            "The relinearization key of SEAL");
  }
  void add_spec_flag(CLI::App &app, Args &args) {
    std::function<void(const std::string &)> keyCallback =
        [&args](const std::string &path) {
          std::ifstream istream(path);
          args.relKey.load(args.sealConfig->makeContext(), istream);
        };
    app.add_option("-f,--specification", args.spec,
                   "The specification to be monitored")
        ->required();
  }

  void register_pointwise(CLI::App &app, Args &args) {
    CLI::App *pointwise = app.add_subcommand(
        "pointwise", "Evaluate the given signal point-wise (for debugging)");
    add_common_flags(*pointwise, args);
    pointwise->parse_complete_callback(
        [&args] { args.type = TYPE::POINTWISE; });
  }

  void register_plain(CLI::App &app, Args &args) {
    CLI::App *plain = app.add_subcommand(
        "plain", "Execute a monitor with plaintext (for debugging)");
    add_common_flags(*plain, args);
    add_spec_flag(*plain, args);
    plain->parse_complete_callback([&args] { args.type = TYPE::PLAIN; });
  }

  void do_pointwise(const ArithHomFA::SealConfig &config,
                    const seal::RelinKeys &relinKeys, std::istream &istream,
                    std::ostream &ostream) {
    const auto context = config.makeContext();
    ArithHomFA::CKKSPredicate predicate{context, config.scale};
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

  void do_plain(const ArithHomFA::SealConfig &config,
                const std::string &graphFilename, std::istream &istream,
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

  void dumpBasicInfo(int argc, char **argv) {
    spdlog::info(
        R"(============================================================)");

    // Logo: thanks to:
    // https://patorjk.com/software/taag/#p=display&f=Standard&t=ArithHomFA
    spdlog::info(R"(     _         _ _   _     _   _                 _____ _)");
    spdlog::info(
        R"(    / \   _ __(_) |_| |__ | | | | ___  _ __ ___ |  ___/ \)");
    spdlog::info(
        R"(   / _ \ | '__| | __| '_ \| |_| |/ _ \| '_ ` _ \| |_ / _ \)");
    spdlog::info(
        R"(  / ___ \| |  | | |_| | | |  _  | (_) | | | | | |  _/ ___ \)");
    spdlog::info(
        R"( /_/   \_\_|  |_|\__|_| |_|_| |_|\___/|_| |_| |_|_|/_/   \_\)");
    spdlog::info(
        R"(                                                            )");

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

    spdlog::info(
        R"(============================================================)");
  }

} // namespace

int main(int argc, char **argv) {
  Args args;
  CLI::App app{"Arith HomFA -- Oblivious Online STL Monitor via Fully "
               "Homomorphic Encryption"};
  register_general_options(app, args);
  register_pointwise(app, args);
  register_plain(app, args);

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
      do_pointwise(*args.sealConfig, args.relKey, *args.input, *args.output);
      break;
    }
    case TYPE::PLAIN: {
      do_plain(*args.sealConfig, *args.spec, *args.input, *args.output);
      break;
    }
    case TYPE::UNSPECIFIED: {
      spdlog::info("No mode is specified");
      spdlog::info(app.help());
    }
  }

  return 0;
}
