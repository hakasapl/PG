#pragma once

#include <windows.h>

#include <filesystem>

// Steam game ID definitions
enum {
  STEAMGAMEID_SKYRIM_SE = 489830,
  STEAMGAMEID_SKYRIM_VR = 611670,
  STEAMGAMEID_SKYRIM = 72850,
  STEAMGAMEID_ENDERAL = 933480,
  STEAMGAMEID_ENDERAL_SE = 976620
};

constexpr unsigned REG_BUFFER_SIZE = 1024;

class BethesdaGame {
public:
  // GameType enum
  enum class GameType { SKYRIM_SE, SKYRIM_GOG, SKYRIM_VR, SKYRIM, ENDERAL, ENDERAL_SE };

  // StoreType enum (for now only Steam is used)
  enum class StoreType { STEAM, WINDOWS_STORE, EPIC_GAMES_STORE, GOG };

  // struct that stores location of ini and custom ini file for a game
  struct ININame {
    std::filesystem::path INI;
    std::filesystem::path INIPrefs;
    std::filesystem::path INICustom;
  };

private:
  [[nodiscard]] auto getINILocations() const -> ININame;
  [[nodiscard]] auto getDocumentLocation() const -> std::filesystem::path;
  [[nodiscard]] auto getAppDataLocation() const -> std::filesystem::path;
  [[nodiscard]] auto getSteamGameID() const -> int;
  [[nodiscard]] static auto getDataCheckFile(const GameType &Type) -> std::filesystem::path;

  // stores the game type
  GameType ObjGameType;

  // stores game path and game data path (game path / data)
  std::filesystem::path GamePath;
  std::filesystem::path GameDataPath;
  std::filesystem::path GameAppDataPath;
  std::filesystem::path GameDocumentPath;

  // stores whether logging is enabled
  bool Logging;

public:
  // constructor
  BethesdaGame(GameType GameType, const bool &Logging = false, const std::filesystem::path &GamePath = "", const std::filesystem::path &AppDataPath = "", const std::filesystem::path &DocumentPath = "");

  // get functions
  [[nodiscard]] auto getGameType() const -> GameType;
  [[nodiscard]] auto getGamePath() const -> std::filesystem::path;
  [[nodiscard]] auto getGameDataPath() const -> std::filesystem::path;

  [[nodiscard]] auto getINIPaths() const -> ININame;
  [[nodiscard]] auto getLoadOrderFile() const -> std::filesystem::path;
  [[nodiscard]] auto getPluginsFile() const -> std::filesystem::path;

  // Get number of active plugins including Bethesda master files
  [[nodiscard]] auto getActivePlugins(const bool &TrimExtension = false, const bool &Lowercase = false) const -> std::vector<std::wstring>;

  // Helpers
  [[nodiscard]] static auto getGameTypes() -> std::vector<GameType>;
  [[nodiscard]] static auto getStrFromGameType(const GameType &Type) -> std::string;
  [[nodiscard]] static auto getGameTypeFromStr(const std::string &Type) -> GameType;
  [[nodiscard]] static auto isGamePathValid(const std::filesystem::path &GamePath, const GameType &Type) -> bool;

  // locates the steam install locatino of steam
  [[nodiscard]] static auto findGamePathFromSteam(const GameType &Type) -> std::filesystem::path;

private:
  [[nodiscard]] auto getGameDocumentSystemPath() const -> std::filesystem::path;
  [[nodiscard]] auto getGameAppdataSystemPath() const -> std::filesystem::path;

  // gets the system path for a folder (from windows.h)
  static auto getSystemPath(const GUID &FolderID) -> std::filesystem::path;

  [[nodiscard]] static auto getGameRegistryPath(const GameType &Type) -> std::string;
};
