#include <CLI/CLI.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <windows.h>

#include "BethesdaGame.hpp"
#include "ParallaxGen.hpp"
#include "ParallaxGenConfig.hpp"
#include "ParallaxGenD3D.hpp"
#include "ParallaxGenDirectory.hpp"
#include "ParallaxGenUtil.hpp"
#include "patchers/PatcherTruePBR.hpp"

using namespace std;
using namespace ParallaxGenUtil;

struct ParallaxGenCLIArgs {
  int Verbosity = 0;
  filesystem::path GameDir;
  string GameType = "skyrimse";
  filesystem::path OutputDir;
  bool Autostart = false;
  bool NoMultithread = false;
  bool NoGPU = false;
  bool NoBSA = false;
  bool UpgradeShaders = false;
  bool OptimizeMeshes = false;
  bool NoZip = false;
  bool NoCleanup = false;
  bool NoDefaultConfig = false;
  bool IgnoreParallax = false;
  bool IgnoreComplexMaterial = false;
  bool IgnoreTruePBR = false;

  [[nodiscard]] auto getString() const -> string {
    string OutStr;
    OutStr += "Verbosity: " + to_string(Verbosity) + "\n";
    OutStr += "GameDir: " + GameDir.string() + "\n";
    OutStr += "GameType: " + GameType + "\n";
    OutStr += "OutputDir: " + OutputDir.string() + "\n";
    OutStr += "Autostart: " + to_string(static_cast<int>(Autostart)) + "\n";
    OutStr += "NoMultithread: " + to_string(static_cast<int>(NoMultithread)) + "\n";
    OutStr += "NoGPU: " + to_string(static_cast<int>(NoGPU)) + "\n";
    OutStr += "NoBSA: " + to_string(static_cast<int>(NoBSA)) + "\n";
    OutStr += "UpgradeShaders: " + to_string(static_cast<int>(UpgradeShaders)) + "\n";
    OutStr += "OptimizeMeshes: " + to_string(static_cast<int>(OptimizeMeshes)) + "\n";
    OutStr += "NoZip: " + to_string(static_cast<int>(NoZip)) + "\n";
    OutStr += "NoCleanup: " + to_string(static_cast<int>(NoCleanup)) + "\n";
    OutStr += "NoDefaultConfig: " + to_string(static_cast<int>(NoDefaultConfig)) + "\n";
    OutStr += "IgnoreParallax: " + to_string(static_cast<int>(IgnoreParallax)) + "\n";
    OutStr += "IgnoreComplexMaterial: " + to_string(static_cast<int>(IgnoreComplexMaterial)) + "\n";
    OutStr += "IgnoreTruePBR: " + to_string(static_cast<int>(IgnoreTruePBR));

    return OutStr;
  }
};

// Store game type strings and their corresponding BethesdaGame::GameType enum
// values This also determines the CLI argument help text (the key values)
auto getGameTypeMap() -> unordered_map<string, BethesdaGame::GameType> {
  static unordered_map<string, BethesdaGame::GameType> GameTypeMap = {
      {"skyrimse", BethesdaGame::GameType::SKYRIM_SE}, {"skyrimgog", BethesdaGame::GameType::SKYRIM_GOG},
      {"skyrim", BethesdaGame::GameType::SKYRIM},      {"skyrimvr", BethesdaGame::GameType::SKYRIM_VR},
      {"enderal", BethesdaGame::GameType::ENDERAL},    {"enderalse", BethesdaGame::GameType::ENDERAL_SE}};
  return GameTypeMap;
}

auto getGameTypeMapStr() -> string {
  const auto GameTypeMap = getGameTypeMap();
  static vector<string> GameTypeStrs;
  GameTypeStrs.reserve(GameTypeMap.size());
  for (const auto &Pair : GameTypeMap) {
    GameTypeStrs.push_back(Pair.first);
  }

  static string GameTypeStr = boost::join(GameTypeStrs, ", ");
  return GameTypeStr;
}

void mainRunner(ParallaxGenCLIArgs &Args, const filesystem::path &ExePath) {
  // Welcome Message
  spdlog::info("Welcome to ParallaxGen version {}!", PARALLAXGEN_VERSION);

  // Print configuration parameters
  spdlog::debug("Configuration Parameters:\n{}", Args.getString());

  //
  // Init
  //

  // print output location
  spdlog::info(L"ParallaxGen output directory (the contents will be deleted if you "
               L"start generation!): {}",
               Args.OutputDir.wstring());

  // Create bethesda game type object
  BethesdaGame::GameType BGType = getGameTypeMap().at(Args.GameType);

  // Create relevant objects
  BethesdaGame BG = BethesdaGame(BGType, Args.GameDir, true);
  ParallaxGenDirectory PGD = ParallaxGenDirectory(BG);
  ParallaxGenConfig PGC = ParallaxGenConfig(&PGD, ExePath);
  ParallaxGenD3D PGD3D = ParallaxGenD3D(&PGD, Args.OutputDir, ExePath, !Args.NoGPU);
  ParallaxGen PG = ParallaxGen(Args.OutputDir, &PGD, &PGC, &PGD3D, Args.OptimizeMeshes, Args.IgnoreParallax,
                               Args.IgnoreComplexMaterial, Args.IgnoreTruePBR);

  // Check if GPU needs to be initialized
  if (!Args.NoGPU) {
    PGD3D.initGPU();
  }

  // Create output directory
  try {
    filesystem::create_directories(Args.OutputDir);
  } catch (const filesystem::filesystem_error &E) {
    spdlog::error("Failed to create output directory: {}", E.what());
    exit(1);
  }

  // If output dir is the same as data dir meshes might get overwritten
  if (filesystem::equivalent(Args.OutputDir, PGD.getDataPath())) {
    spdlog::critical("Output directory cannot be the same directory as your data folder. "
                     "Exiting.");
    exit(1);
  }

  //
  // Generation
  //

  // User Input to Continue
  if (!Args.Autostart) {
    cout << "Press ENTER to start ParallaxGen...";
    cin.get();
  }

  // delete existing output
  PG.deleteOutputDir();

  // Check if ParallaxGen output already exists in data directory
  const filesystem::path PGStateFilePath = BG.getGameDataPath() / ParallaxGen::getStateFileName();
  if (filesystem::exists(PGStateFilePath)) {
    spdlog::critical("ParallaxGen meshes exist in your data directory, please delete before "
                     "re-running.");
    exit(1);
  }

  PG.initOutputDir();

  // Populate file map from data directory
  PGD.populateFileMap(!Args.NoBSA);

  // Load configs
  PGD.findPGConfigs();
  PGC.loadConfig(!Args.NoDefaultConfig);

  // Install default cubemap file if needed
  if (!Args.IgnoreComplexMaterial) {
    // install default cubemap file if needed
    if (!PGD.defCubemapExists()) {
      spdlog::info("Installing default cubemap file");

      // Create Directory
      filesystem::path OutputCubemapPath = Args.OutputDir / ParallaxGenDirectory::getDefaultCubemapPath().parent_path();
      filesystem::create_directories(OutputCubemapPath);

      boost::filesystem::path AssetPath = boost::filesystem::path(ExePath) / L"assets/dynamic1pxcubemap_black_ENB.dds";
      boost::filesystem::path OutputPath =
          boost::filesystem::path(Args.OutputDir) / ParallaxGenDirectory::getDefaultCubemapPath();

      // Move File
      boost::filesystem::copy_file(AssetPath, OutputPath, boost::filesystem::copy_options::overwrite_existing);
    }
  }

  // Build file vectors
  if (!Args.IgnoreParallax || Args.UpgradeShaders) {
    PGD.findHeightMaps(
        stringVecToWstringVec(PGC.getConfig()["parallax_lookup"]["allowlist"].get<vector<string>>()),
        stringVecToWstringVec(PGC.getConfig()["parallax_lookup"]["blocklist"].get<vector<string>>()),
        stringVecToWstringVec(PGC.getConfig()["parallax_lookup"]["archive_blocklist"].get<vector<string>>()));
  }

  if (!Args.IgnoreComplexMaterial || Args.UpgradeShaders) {
    PGD.findComplexMaterialMaps(
        stringVecToWstringVec(PGC.getConfig()["complexmaterial_lookup"]["allowlist"].get<vector<string>>()),
        stringVecToWstringVec(PGC.getConfig()["complexmaterial_lookup"]["blocklist"].get<vector<string>>()),
        stringVecToWstringVec(PGC.getConfig()["complexmaterial_lookup"]["archive_blocklist"].get<vector<string>>()));
    PGD3D.findCMMaps();
  }

  if (!Args.IgnoreTruePBR) {
    PGD.findTruePBRConfigs(
        stringVecToWstringVec(PGC.getConfig()["truepbr_cfg_lookup"]["allowlist"].get<vector<string>>()),
        stringVecToWstringVec(PGC.getConfig()["truepbr_cfg_lookup"]["blocklist"].get<vector<string>>()),
        stringVecToWstringVec(PGC.getConfig()["truepbr_cfg_lookup"]["archive_blocklist"].get<vector<string>>()));
    PatcherTruePBR::loadPatcherBuffers(PGD.getTruePBRConfigs());
  }

  // Build base vectors
  PGD.buildBaseVectors(PGC.getConfig()["suffixes"].get<vector<vector<string>>>());

  // Upgrade shaders if requested
  if (Args.UpgradeShaders) {
    PG.upgradeShaders();
  }

  // Patch meshes if set
  if (!Args.IgnoreParallax || !Args.IgnoreComplexMaterial || !Args.IgnoreTruePBR) {
    PGD.findMeshes(stringVecToWstringVec(PGC.getConfig()["nif_lookup"]["allowlist"].get<vector<string>>()),
                   stringVecToWstringVec(PGC.getConfig()["nif_lookup"]["blocklist"].get<vector<string>>()),
                   stringVecToWstringVec(PGC.getConfig()["nif_lookup"]["archive_blocklist"].get<vector<string>>()));
    PG.patchMeshes(!Args.NoMultithread);
  }

  spdlog::info("ParallaxGen has finished patching meshes.");

  // archive
  if (!Args.NoZip) {
    PG.zipMeshes();
  }

  // cleanup
  if (!Args.NoCleanup) {
    PG.deleteMeshes();
  }
}

void exitBlocking() {
  cout << "Press ENTER to exit...";
  cin.get();
}

auto getExecutablePath() -> filesystem::path {
  char Buffer[MAX_PATH];                                   // NOLINT
  if (GetModuleFileName(nullptr, Buffer, MAX_PATH) == 0) { // NOLINT
    cerr << "Error getting executable path: " << GetLastError() << "\n";
    exit(1);
  }

  filesystem::path OutPath = filesystem::path(Buffer);

  if (filesystem::exists(OutPath)) {
    return OutPath;
  }

  cerr << "Error getting executable path: path does not exist\n";
  exit(1);

  return {};
}

void addArguments(CLI::App &App, ParallaxGenCLIArgs &Args, const filesystem::path &ExePath) {
  // Logging
  App.add_flag("-v", Args.Verbosity,
               "Verbosity level -v for DEBUG data or -vv for TRACE data "
               "(warning: TRACE data is very verbose)");
  // Game
  App.add_option("-d,--game-dir", Args.GameDir, "Manually specify game directory");
  App.add_option("-g,--game-type", Args.GameType, "Specify game type [" + getGameTypeMapStr() + "]");
  App.add_flag("--no-bsa", Args.NoBSA, "Don't load BSA files, only loose files");
  // App Options
  App.add_flag("--autostart", Args.Autostart, "Start generation without user input");
  App.add_flag("--no-multithread", Args.NoMultithread, "Don't use multithreading (Slower)");
  auto *FlagNoGpu = App.add_flag("--no-gpu", Args.NoGPU, "Don't use the GPU for any operations (Slower)");
  App.add_flag("--no-default-conifg", Args.NoDefaultConfig,
               "Don't load the default config file (You need to know what "
               "you're doing for this)");
  // Output
  App.add_option("-o,--output-dir", Args.OutputDir, "Manually specify output directory");
  App.add_flag("--optimize-meshes", Args.OptimizeMeshes, "Optimize meshes before saving them");
  App.add_flag("--no-zip", Args.NoZip, "Don't zip the output meshes (also enables --no-cleanup)");
  App.add_flag("--no-cleanup", Args.NoCleanup, "Don't delete generated meshes after zipping");
  // Patchers
  App.add_flag("--upgrade-shaders", Args.UpgradeShaders, "Upgrade shaders to a better version whenever possible")
      ->excludes(FlagNoGpu);
  App.add_flag("--ignore-parallax", Args.IgnoreParallax, "Don't generate any parallax meshes");
  App.add_flag("--ignore-complex-material", Args.IgnoreComplexMaterial, "Don't generate any complex material meshes");
  App.add_flag("--ignore-truepbr", Args.IgnoreTruePBR, "Don't apply any TruePBR configs in the load order");

  // Multi-argument Validation
  App.callback([&Args, &ExePath]() {
    // One action needs to be enabled
    if (Args.IgnoreParallax && Args.IgnoreComplexMaterial && Args.IgnoreTruePBR && !Args.UpgradeShaders) {
      throw CLI::ValidationError("No action items to do (check that you are not ignoring all patchers)");
    }

    // Validate Game Type
    const auto GameTypeMap = getGameTypeMap();
    if (GameTypeMap.find(Args.GameType) == GameTypeMap.end()) {
      throw CLI::ValidationError("Invalid game type (-g) specified: " + Args.GameType + ". Available options are [" +
                                 getGameTypeMapStr() + "]");
    }

    // Check if output dir is set, otherwise set default
    if (Args.OutputDir.empty()) {
      Args.OutputDir = ExePath / "ParallaxGen_Output";
    } else {
      // Check if output dir is a directory
      if (!filesystem::is_directory(Args.OutputDir) && filesystem::exists(Args.OutputDir)) {
        throw CLI::ValidationError("Output directory (-o) must be a directory or not exist");
      }
    }

    // If --no-zip is set, also set --no-cleanup
    if (Args.NoZip) {
      Args.NoCleanup = true;
    }
  });
}

void initLogger(const filesystem::path &LOGPATH, const ParallaxGenCLIArgs &Args) {
  // Create loggers
  vector<spdlog::sink_ptr> Sinks;
  Sinks.push_back(make_shared<spdlog::sinks::stdout_color_sink_mt>());
  Sinks.push_back(
      make_shared<spdlog::sinks::basic_file_sink_mt>(LOGPATH.string(), true)); // TODO wide string support here
  auto Logger = make_shared<spdlog::logger>("ParallaxGen", Sinks.begin(), Sinks.end());

  // register logger parameters
  spdlog::register_logger(Logger);
  spdlog::set_default_logger(Logger);
  spdlog::set_level(spdlog::level::info);
  spdlog::flush_on(spdlog::level::info);

  // Set logging mode
  if (Args.Verbosity == 1) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::debug);
    spdlog::debug("DEBUG logging enabled");
  }

  if (Args.Verbosity >= 2) {
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
    spdlog::trace("TRACE logging enabled");
  }
}

auto main(int ArgC, char **ArgV) -> int {
  // This is what keeps the console window open after the program exits until user input
  if (atexit(exitBlocking) != 0) {
    cerr << "Failed to register exitBlocking function\n";
    return 1;
  }

  // Find location of ParallaxGen.exe
  const filesystem::path ExePath = getExecutablePath().parent_path();

  // CLI Arguments
  ParallaxGenCLIArgs Args;
  CLI::App App{"ParallaxGen: Auto convert meshes to parallax meshes"};
  addArguments(App, Args, ExePath);

  // Parse CLI Arguments (this is what exits on any validation issues)
  CLI11_PARSE(App, ArgC, ArgV);

  // Initialize logger
  const filesystem::path LogPath = ExePath / "ParallaxGen.log";
  initLogger(LogPath, Args);

  // Main Runner (Catches all exceptions)
  try {
    mainRunner(Args, ExePath);
  } catch (const exception &E) {
    spdlog::critical("An unhandled exception occurred (Please provide this entire message "
                     "in your bug report).\n\nException type: {}\nMessage: {}\nStack Trace: "
                     "\n{}",
                     typeid(E).name(), E.what(), boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
    cout << "Press ENTER to abort...";
    cin.get();
    abort();
  }
}
