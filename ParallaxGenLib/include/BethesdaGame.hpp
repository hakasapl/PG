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
  [[nodiscard]] auto getDataCheckFile() const -> std::filesystem::path;

  // stores the game type
  GameType ObjGameType;

  // stores game path and game data path (game path / data)
  std::filesystem::path GamePath;
  std::filesystem::path GameDataPath;

  // stores whether logging is enabled
  bool Logging;

public:
  // constructor
  BethesdaGame(GameType GameType, const std::filesystem::path &GamePath, const bool &Logging = false);

  // get functions
  [[nodiscard]] auto getGameType() const -> GameType;
  [[nodiscard]] auto getGamePath() const -> std::filesystem::path;
  [[nodiscard]] auto getGameDataPath() const -> std::filesystem::path;

  [[nodiscard]] auto getGameDocumentPath() const -> std::filesystem::path;
  [[nodiscard]] auto getGameAppdataPath() const -> std::filesystem::path;

  [[nodiscard]] auto getINIPaths() const -> ININame;
  [[nodiscard]] auto getLoadOrderFile() const -> std::filesystem::path;

private:
  // locates the steam install locatino of steam
  [[nodiscard]] auto findGamePathFromSteam() const -> std::filesystem::path;

  // gets the system path for a folder (from windows.h)
  static auto getSystemPath(const GUID &FolderID) -> std::filesystem::path;

  [[nodiscard]] auto getGameRegistryPath() const -> std::string;
};
