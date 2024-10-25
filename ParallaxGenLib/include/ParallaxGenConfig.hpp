#pragma once

#include <mutex>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

#include "NIFUtil.hpp"
#include "ParallaxGenDirectory.hpp"

// Forward declaration
class ParallaxGenDirectory;

class ParallaxGenConfig {
private:
  ParallaxGenDirectory *PGD;
  std::filesystem::path ExePath;

  // Config Structures
  std::unordered_set<std::wstring> NIFBlocklist {};
  std::unordered_set<std::wstring> DynCubemapBlocklist{};
  std::unordered_map<std::filesystem::path, NIFUtil::TextureType> ManualTextureMaps{};
  std::unordered_set<std::wstring> VanillaBSAList{};

  struct ConflictRule {
    std::unordered_set<std::wstring> Mods;
    std::wstring DecisionMod;
  };

  std::mutex ModsetRulesMutex;
  std::vector<ConflictRule> ModsetRules;

  // Validator
  nlohmann::json_schema::json_validator Validator;

public:
  ParallaxGenConfig(ParallaxGenDirectory *PGD, std::filesystem::path ExePath);
  static auto getConfigValidation() -> nlohmann::json;
  static auto getUserConfigFile() -> std::filesystem::path;

  void loadConfig(const bool &LoadNative = true);

  [[nodiscard]] auto getNIFBlocklist() const -> const std::unordered_set<std::wstring> &;

  [[nodiscard]] auto getDynCubemapBlocklist() const -> const std::unordered_set<std::wstring> &;

  [[nodiscard]] auto getManualTextureMaps() const -> const std::unordered_map<std::filesystem::path, NIFUtil::TextureType> &;

  [[nodiscard]] auto getVanillaBSAList() const -> const std::unordered_set<std::wstring> &;

  [[nodiscard]] auto getModsetRule(const std::unordered_set<std::wstring> &PossibleMods) -> std::wstring;
  void setModsetRule(const std::unordered_set<std::wstring> &PossibleMods, const std::wstring &DecisionMod);

private:
  static auto parseJSON(const std::filesystem::path &JSONFile, const std::vector<std::byte> &Bytes, nlohmann::json &J) -> bool;

  auto validateJSON(const std::filesystem::path &JSONFile, const nlohmann::json &J) -> bool;

  auto addConfigJSON(const nlohmann::json &J) -> void;

  static void replaceForwardSlashes(nlohmann::json &JSON);
};
