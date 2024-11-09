#pragma once

#include <mutex>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

#include "BethesdaGame.hpp"
#include "ModManagerDirectory.hpp"
#include "NIFUtil.hpp"

/**
 * @class ParallaxGenConfig
 * @brief Class responsible for ParallaxGen runtime configuration and JSON file interaction
 */
class ParallaxGenConfig {
public:
  /**
   * @struct PGParams
   * @brief Struct that holds all the user-configurable parameters for ParallaxGen
   */
  struct PGParams {
    // CLI Only
    bool Autostart = false;

    // Game
    struct Game {
      std::filesystem::path Dir;
      BethesdaGame::GameType Type = BethesdaGame::GameType::SKYRIM_SE;
    } Game;

    // Mod Manager
    struct ModManager {
      ModManagerDirectory::ModManagerType Type = ModManagerDirectory::ModManagerType::None;
      std::filesystem::path MO2InstanceDir;
      std::wstring MO2Profile = L"Default";
    } ModManager;

    // Output
    struct Output {
      std::filesystem::path Dir;
      bool Zip = true;
    } Output;

    // Processing
    struct Processing {
      bool Multithread = true;
      bool HighMem = false;
      bool GPUAcceleration = true;
      bool BSA = true;
      bool PluginPatching = true;
      bool MapFromMeshes = true;
    } Processing;

    // Pre-Patchers
    struct PrePatcher {
      bool DisableMLP = false;
    } PrePatcher;

    // Shader Patchers
    struct ShaderPatcher {
      bool Parallax = true;
      bool ComplexMaterial = true;
      bool TruePBR = true;
    } ShaderPatcher;

    // Shader Transforms
    struct ShaderTransforms {
      bool ParallaxToCM = false;
    } ShaderTransforms;

    // Post-Patchers
    struct PostPatcher {
      bool OptimizeMeshes = false;
    } PostPatcher;

    [[nodiscard]] auto getString() const -> std::wstring;
  };

private:
  std::filesystem::path ExePath; /** Stores the ExePath of ParallaxGen.exe */

  // Config Structures
  std::unordered_set<std::wstring> NIFBlocklist;        /** Stores the nif blocklist configuration */
  std::unordered_set<std::wstring> DynCubemapBlocklist; /** Stores the dynamic cubemap blocklist configuration */
  std::unordered_map<std::filesystem::path, NIFUtil::TextureType>
      ManualTextureMaps;                           /** Stores the manual texture maps configuration */
  std::unordered_set<std::wstring> VanillaBSAList; /** Stores the vanilla bsa list configuration */
  PGParams Params;                                 /** Stores the configured parameters */

  std::mutex ModOrderMutex;           /** Mutex for locking mod order */
  std::vector<std::wstring> ModOrder; /** Stores the mod order configuration */
  std::unordered_map<std::wstring, int>
      ModPriority; /** Stores the priority numbering for mods for constant time lookups */

  nlohmann::json_schema::json_validator Validator; /** Stores the validator JSON object */

public:
  /**
   * @brief Construct a new Parallax Gen Config object
   *
   * @param ExePath Path to ParallaxGen.exe
   */
  ParallaxGenConfig(std::filesystem::path ExePath);

  /**
   * @brief Get the Config Validation object
   *
   * @return nlohmann::json config validation object
   */
  static auto getConfigValidation() -> nlohmann::json;

  /**
   * @brief Get the User Config File object
   *
   * @return std::filesystem::path Path to user config file
   */
  [[nodiscard]] auto getUserConfigFile() const -> std::filesystem::path;

  /**
   * @brief Loads the config files in the `cfg` folder
   */
  void loadConfig();

  /**
   * @brief Gets the NIF blocklist
   *
   * @return const std::unordered_set<std::wstring>& NIF blocklist
   */
  [[nodiscard]] auto getNIFBlocklist() const -> const std::unordered_set<std::wstring> &;

  /**
   * @brief Get the Dyn Cubemap Blocklist object
   *
   * @return const std::unordered_set<std::wstring>& DynCubemapBlocklist
   */
  [[nodiscard]] auto getDynCubemapBlocklist() const -> const std::unordered_set<std::wstring> &;

  /**
   * @brief Get the Manual Texture Maps object
   *
   * @return const std::unordered_map<std::filesystem::path, NIFUtil::TextureType>& Manual texture maps
   */
  [[nodiscard]] auto
  getManualTextureMaps() const -> const std::unordered_map<std::filesystem::path, NIFUtil::TextureType> &;

  /**
   * @brief Get the Vanilla BSA List object
   *
   * @return const std::unordered_set<std::wstring>& vanilla bsa list
   */
  [[nodiscard]] auto getVanillaBSAList() const -> const std::unordered_set<std::wstring> &;

  /**
   * @brief Get the Mod Order object
   *
   * @return std::vector<std::wstring> Mod order
   */
  [[nodiscard]] auto getModOrder() -> std::vector<std::wstring>;

  /**
   * @brief Get mod priority for a given mod
   *
   * @param Mod mod to check
   * @return int priority of mod (-1 if not found)
   */
  [[nodiscard]] auto getModPriority(const std::wstring &Mod) -> int;

  /**
   * @brief Get the Mod Priority Map object
   *
   * @return std::unordered_map<std::wstring, int> Mod priority map
   */
  [[nodiscard]] auto getModPriorityMap() -> std::unordered_map<std::wstring, int>;

  /**
   * @brief Set the Mod Order object (also saves to user json)
   *
   * @param ModOrder new mod order to set
   */
  void setModOrder(const std::vector<std::wstring> &ModOrder);

  /**
   * @brief Get the current Params
   *
   * @return PGParams params
   */
  [[nodiscard]] auto getParams() const -> PGParams;

  /**
   * @brief Set params (also saves to user json)
   *
   * @param Params new params to set
   */
  void setParams(const PGParams &Params);

  /**
   * @brief Validates a given param struct
   *
   * @param Params Params to validate
   * @param Errors Error messages
   * @return true no validation errors
   * @return false validation errors
   */
  [[nodiscard]] static auto validateParams(const PGParams &Params, std::vector<std::string> &Errors) -> bool;

private:
  /**
   * @brief Parses JSON from a file
   *
   * @param JSONFile File to parse
   * @param Bytes Bytes to parse
   * @param J parsed JSON object
   * @return true no json errors
   * @return false unable to parse
   */
  static auto parseJSON(const std::filesystem::path &JSONFile, const std::vector<std::byte> &Bytes,
                        nlohmann::json &J) -> bool;

  /**
   * @brief Validates JSON file
   *
   * @param JSONFile File to validate
   * @param J JSON schema to validate against
   * @return true no validation errors
   * @return false validation errors
   */
  auto validateJSON(const std::filesystem::path &JSONFile, const nlohmann::json &J) -> bool;

  /**
   * @brief Adds a JSON config to the current config
   *
   * @param J JSON object to add
   */
  auto addConfigJSON(const nlohmann::json &J) -> void;

  /**
   * @brief Replaces any / with \\ in a JSON object
   *
   * @param JSON JSON object to change
   */
  static void replaceForwardSlashes(nlohmann::json &JSON);

  /**
   * @brief Saves user config to the user json file
   */
  void saveUserConfig() const;
};
