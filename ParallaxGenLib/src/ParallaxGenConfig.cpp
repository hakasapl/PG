#include "ParallaxGenConfig.hpp"

#include "NIFUtil.hpp"
#include "ParallaxGenUtil.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <mutex>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <boost/algorithm/string/case_conv.hpp>

#include <exception>
#include <filesystem>
#include <string>

#include <cstdlib>

using namespace std;
using namespace ParallaxGenUtil;

ParallaxGenConfig::ParallaxGenConfig(ParallaxGenDirectory *PGD, std::filesystem::path ExePath)
    : PGD(PGD), ExePath(std::move(ExePath)) {}

auto ParallaxGenConfig::getConfigValidation() -> nlohmann::json {
  const static nlohmann::json PGConfigSchema = R"(
  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "parallaxgen-cfg",
    "type": "object",
    "properties": {
      "dyncubemap_blocklist": {
        "type": "array",
        "items": {
          "type": "string"
        }
      },
      "nif_blocklist": {
        "type": "array",
        "items": {
          "type": "string"
        }
      },
      "texture_map": {
        "type": "object",
        "additionalProperties": {
          "type": "string"
        }
      },
      "vanilla_bsas" : {
        "type" : "array",
        "items": {
            "type": "string"
        }
      },
      "mod_rules": {
        "type": "object",
        "additionalProperties": {
          "type": "string"
        }
      }
    }
  }
  )"_json;

  return PGConfigSchema;
}

void ParallaxGenConfig::loadConfig(const bool &LoadNative) {
  // Initialize Validator
  try {
    Validator.set_root_schema(getConfigValidation());
  } catch (const std::exception &E) {
    spdlog::critical("Unable to validate JSON validation: {}", E.what());
    exit(1);
  }

  size_t NumConfigs = 0;

  if (LoadNative) {
    filesystem::path DefConfPath = ExePath / "cfg";
    // Loop through all files in DefConfPath recursively directoryiterator
    for (const auto &Entry : filesystem::recursive_directory_iterator(DefConfPath)) {
      if (filesystem::is_regular_file(Entry) && Entry.path().filename().extension() == ".json") {
        spdlog::debug(L"Loading ParallaxGen Config: {}", Entry.path().wstring());

        nlohmann::json J;
        if (!parseJSON(Entry.path(), getFileBytes(Entry.path()), J)) {
          continue;
        }

        if (!validateJSON(Entry.path(), J)) {
          continue;
        }

        if (!J.empty()) {
          replaceForwardSlashes(J);
          addConfigJSON(J);
          NumConfigs++;
        }
      }
    }
  }

  // loop through PGConfigPaths (in load order)
  for (const auto &PGConfigFile : PGD->getPGJSONs()) {
    spdlog::debug(L"Loading ParallaxGen Config: {}", PGConfigFile.wstring());

    nlohmann::json J;
    if (!parseJSON(PGConfigFile, PGD->getFile(PGConfigFile), J)) {
      continue;
    }

    if (!validateJSON(PGConfigFile, J)) {
      continue;
    }

    if (!J.empty()) {
      replaceForwardSlashes(J);
      addConfigJSON(J);
      NumConfigs++;
    }
  }

  // Print number of configs loaded
  spdlog::info("Loaded {} ParallaxGen configs successfully", NumConfigs);
}

auto ParallaxGenConfig::addConfigJSON(const nlohmann::json &J) -> void {
  // "dyncubemap_blocklist" field
  if (J.contains("dyncubemap_blocklist")) {
    for (const auto &Item : J["dyncubemap_blocklist"]) {
      DynCubemapBlocklist.insert(strToWstr(boost::to_lower_copy(Item.get<string>())));
    }
  }

  // "nif_blocklist" field
  if (J.contains("nif_blocklist")) {
    for (const auto &Item : J["nif_blocklist"]) {
      NIFBlocklist.insert(strToWstr(boost::to_lower_copy(Item.get<string>())));
    }
  }

  // "texture_map" field
  if (J.contains("texture_maps")) {
    for (const auto &Item : J["texture_maps"].items()) {
      ManualTextureMaps[boost::to_lower_copy(Item.key())] = NIFUtil::getTexTypeFromStr(Item.value().get<string>());
    }
  }

  // "vanilla_bsas" field
  if (J.contains("vanilla_bsas")) {
    for (const auto &Item : J["vanilla_bsas"]) {
      VanillaBSAList.insert(strToWstr(boost::to_lower_copy(Item.get<string>())));
    }
  }

  // "mod_rules" field
  if (J.contains("mod_rules")) {
    for (const auto &Item : J["mod_rules"].items()) {
      const auto ModSetStr = strToWstr(Item.key());
      const auto DecisionModStr = Item.value().get<string>();

      ConflictRule ModsetRule;
      boost::split(ModsetRule.Mods, ModSetStr, boost::is_any_of(","));
      ModsetRule.DecisionMod = strToWstr(DecisionModStr);

      ModsetRules.push_back(ModsetRule);
    }
  }
}

auto ParallaxGenConfig::parseJSON(const std::filesystem::path &JSONFile, const vector<std::byte> &Bytes,
                                  nlohmann::json &J) -> bool {
  // Parse JSON
  try {
    J = nlohmann::json::parse(Bytes);
  } catch (const nlohmann::json::parse_error &E) {
    spdlog::error(L"Error parsing JSON file {}: {}", JSONFile.wstring(), strToWstr(E.what()));
    J = {};
    return false;
  }

  return true;
}

auto ParallaxGenConfig::validateJSON(const std::filesystem::path &JSONFile, const nlohmann::json &J) -> bool {
  // Validate JSON
  try {
    Validator.validate(J);
  } catch (const std::exception &E) {
    spdlog::error(L"Invalid JSON file {}: {}", JSONFile.wstring(), strToWstr(E.what()));
    return false;
  }

  return true;
}

void ParallaxGenConfig::replaceForwardSlashes(nlohmann::json &JSON) {
  if (JSON.is_string()) {
    auto &Str = JSON.get_ref<string &>();
    for (auto &Ch : Str) {
      if (Ch == '/') {
        Ch = '\\';
      }
    }
  } else if (JSON.is_object()) {
    for (const auto &Item : JSON.items()) {
      replaceForwardSlashes(Item.value());
    }
  } else if (JSON.is_array()) {
    for (auto &Element : JSON) {
      replaceForwardSlashes(Element);
    }
  }
}

auto ParallaxGenConfig::getNIFBlocklist() const -> const unordered_set<wstring> & { return NIFBlocklist; }

auto ParallaxGenConfig::getDynCubemapBlocklist() const -> const unordered_set<wstring> & { return DynCubemapBlocklist; }

auto ParallaxGenConfig::getManualTextureMaps() const -> const unordered_map<filesystem::path, NIFUtil::TextureType> & {
  return ManualTextureMaps;
}

auto ParallaxGenConfig::getVanillaBSAList() const -> const std::unordered_set<std::wstring> & { return VanillaBSAList; }

auto ParallaxGenConfig::getModsetRule(const unordered_set<wstring> &PossibleMods) -> wstring {
  lock_guard<mutex> Lock(ModsetRulesMutex);

  // loop through each ModsetRule
  for (const auto &ModsetRule : ModsetRules) {
    // check if decision mod is in possible mods
    if (!PossibleMods.contains(ModsetRule.DecisionMod)) {
      continue;
    }

    if (std::all_of(PossibleMods.begin(), PossibleMods.end(),
                    [&](const auto &Mod) { return ModsetRule.Mods.contains(Mod); })) {
      return ModsetRule.DecisionMod;
    }
  }

  return {};
}

void ParallaxGenConfig::setModsetRule(const unordered_set<wstring> &PossibleMods, const wstring &DecisionMod) {
  lock_guard<mutex> Lock(ModsetRulesMutex);

  ConflictRule ModsetRule;
  ModsetRule.Mods = PossibleMods;
  ModsetRule.DecisionMod = DecisionMod;

  // check if this already exists
  auto It = find_if(ModsetRules.begin(), ModsetRules.end(),
                    [&](const auto &Rule) { return Rule.Mods == PossibleMods; });

  if (It == ModsetRules.end()) {
    ModsetRules.push_back(ModsetRule);
  }

  // TODO Save userdata
}
