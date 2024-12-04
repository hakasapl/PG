#pragma once
#include "BethesdaGame.hpp"
#include "ModManagerDirectory.hpp"

#include <bsa/tes4.hpp>

#include <boost/algorithm/string.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

constexpr unsigned ASCII_UPPER_BOUND = 127;

class BethesdaDirectory {
private:
  /**
   * @struct BSAFile
   * @brief Stores data about an individual BSA file
   *
   * path stores the path to the BSA archive, preserving case from the original
   * path version stores the version of the BSA archive archive stores the BSA
   * archive object, which is where files can be accessed
   */
  struct BSAFile {
    std::filesystem::path Path;
    bsa::tes4::version Version;
    bsa::tes4::archive Archive;
  };

  /**
   * @struct BethesdaFile
   * @brief Structure which holds information about a specific file in the file
   * map
   *
   * path stores the path to the file, preserving case from the original path
   * bsa_file stores a shared pointer to a BSA file struct, or nullptr if the
   * file is a loose file
   */
  struct BethesdaFile {
    std::filesystem::path Path;
    std::shared_ptr<BSAFile> BSAFile;
    std::wstring Mod;
    bool Generated;
  };

  /**
   * @struct ModFile
   * @brief Structure which the path and the file size for a specific mod file
   *
   * Path stores the path to the file, preserving case from the original path
   * FileSize stores the size of the file in bytes
   * CRC32 stores the CRC32 checksum of the file
   */
  struct ModFile {
    std::filesystem::path Path;
    uintmax_t FileSize;
    unsigned int CRC32;
  };

  // Class member variables
  std::filesystem::path DataDir;                         /**< Stores the path to the game data directory */
  std::filesystem::path GeneratedDir;                    /**< Stores the path to the generated directory */
  std::map<std::filesystem::path, BethesdaFile> FileMap; /** < Stores the file map for every file found in the load
                                                            order. Key is a lowercase path, value is a BethesdaFile*/
  std::mutex FileMapMutex;                               /** < Mutex for the file map */
  std::vector<ModFile> ModFiles;  /** < Stores files in mod staging directory */

  std::unordered_map<std::filesystem::path, std::vector<std::byte>> FileCache; /** < Stores a cache of file bytes */
  std::mutex FileCacheMutex; /** < Mutex for the file cache map */

  bool Logging;  /** < Bool for whether logging is enabled or not */
  BethesdaGame BG; /** < BethesdaGame which stores a BethesdaGame object
                      corresponding to this load order */
  ModManagerDirectory *MMD; /** < ModManagerDirectory which stores a pointer to a
                               ModManagerDirectory object corresponding to this
                               load order */

  /**
   * @brief Returns a vector of strings that represent the fields in the INI
   * file that store information about BSA file loading
   *
   * @return std::vector<std::string>
   */
  static auto getINIBSAFields() -> std::vector<std::string>;
  /**
   * @brief Gets a list of extensions to ignore when populating the file map
   *
   * @return std::vector<std::wstring>
   */
  static auto getExtensionBlocklist() -> std::vector<std::wstring>;

public:
  /**
   * @brief Construct a new Bethesda Directory object
   *
   * @param BG BethesdaGame object corresponding to load order
   * @param Logging Whether to enable CLI logging
   */
  BethesdaDirectory(BethesdaGame &BG, std::filesystem::path GeneratedPath = "", ModManagerDirectory *MMD = nullptr, const bool &Logging = false);

  /**
   * @brief Populate file map with all files in the load order
   */
  void populateFileMap(bool IncludeBSAs = true);

  /**
   * @brief Get the file map vector, path of the files is is all lower case
   *
   * @return std::map<std::filesystem::path, BethesdaFile>
   */
  [[nodiscard]] auto getFileMap() const -> const std::map<std::filesystem::path, BethesdaFile> &;

  /**
   * @brief Get the data directory path
   *
   * @return std::filesystem::path absolute path to the data directory
   */
  [[nodiscard]] auto getDataPath() const -> std::filesystem::path;

  /**
   * @brief Get the Generated Path
   *
   * @return std::filesystem::path absolute path to the generated directory
   */
  [[nodiscard]] auto getGeneratedPath() const -> std::filesystem::path;

  /**
   * @brief Get bytes from a file in the load order. Throws runtime_error if file does not exist and logging is turned off
   *
   * @param RelPath path to the file relative to the data directory
   * @return std::vector<std::byte> vector of bytes of the file
   */
  [[nodiscard]] auto getFile(const std::filesystem::path &RelPath,
                             const bool &CacheFile = false) -> std::vector<std::byte>;

  /**
   * @brief Get the Mod that has the winning version of the file
   *
   * @param RelPath path to the file relative to the data directory
   * @return std::wstring mod label
   */
  [[nodiscard]] auto getMod(const std::filesystem::path &RelPath) -> std::wstring;

  /**
   * @brief Create a Generated file in the file map
   *
   * @param RelPath path of the generated file
   * @param Mod wstring mod label to assign
   */
  void addGeneratedFile(const std::filesystem::path &RelPath, const std::wstring &Mod);

  /**
   * @brief Clear the file cache
   */
  auto clearCache() -> void;

  /**
   * @brief Check if a file in the load order is a loose file
   *
   * @param RelPath path to the file relative to the data directory
   * @return true if file is a loose file
   * @return false if file is not a loose file or doesn't exist
   */
  [[nodiscard]] auto isLooseFile(const std::filesystem::path &RelPath) -> bool;

  /**
   * @brief Check if a file in the load order is a file from a BSA
   *
   * @param RelPath path to the file relative to the data directory
   * @return true if file is a BSA file
   * @return false if file is not a BSA file or doesn't exist
   */
  [[nodiscard]] auto isBSAFile(const std::filesystem::path &RelPath) -> bool;

  /**
   * @brief Check if a file exists in the load order
   *
   * @param RelPath path to the file relative to the data directory
   * @return true if file exists in the load order
   * @return false if file does not exist in the load order
   */
  [[nodiscard]] auto isFile(const std::filesystem::path &RelPath) -> bool;

  /**
   * @brief Check if a file is a generated file
   *
   * @param RelPath path to the file relative to the data directory
   * @return true if file is generated
   * @return false if file is not generated
   */
  [[nodiscard]] auto isGenerated(const std::filesystem::path &RelPath) -> bool;

  /**
   * @brief Check if file is a directory
   *
   * @param RelPath path to the file relative to the data directory
   * @return true if file is a directory
   * @return false if file is not a directory or doesn't exist
   */
  [[nodiscard]] auto isPrefix(const std::filesystem::path &RelPath) -> bool;

  /**
   * @brief Get the full path of a loose file in the load order
   *
   * @param RelPath path to the file relative to the data directory
   * @return std::filesystem::path absolute path to the file
   */
  [[nodiscard]] auto getLooseFileFullPath(const std::filesystem::path &RelPath) -> std::filesystem::path;

  /**
   * @brief Get the load order of BSAs
   *
   * @return std::vector<std::wstring> Names of BSA files ordered by load order.
   * First element is loaded first.
   */
  [[nodiscard]] auto getBSALoadOrder() const -> std::vector<std::wstring>;

  // Helpers

  /**
   * @brief Checks if fs::path object has only ascii characters
   *
   * @param Path Path to check
   * @return true When path only has ascii chars
   * @return false When path has other than ascii chars
   */
  [[nodiscard]] static auto isPathAscii(const std::filesystem::path &Path) -> bool;

  /*
  * @brief Checks if the given file is included in any of the given BSA files
  */
  [[nodiscard]] auto isFileInBSA(const std::filesystem::path& File, const std::vector<std::wstring>& BSAFiles) -> bool;

  /**
   * @brief Get the lowercase path of a path using the "C" locale, i.e. only ASCII characters are converted
   *
   * @param Path Path to be made lowercase
   * @return std::filesystem::path lowercase path of the input
   */
  static auto getAsciiPathLower(const std::filesystem::path &Path) -> std::filesystem::path;

  /**
   * @brief Check if two paths are equal, ignoring case
   *
   * @param Path1 1st path to check
   * @param Path2 2nd path to check
   * @return true if paths equal ignoring case
   * @return false if paths don't equal ignoring case
   */
  static auto pathEqualityIgnoreCase(const std::filesystem::path &Path1, const std::filesystem::path &Path2) -> bool;

  /**
   * @brief Check if any component on a path is in a list of components
   *
   * @param Path path to check
   * @param Components components to check against
   * @return true if any component is in the path
   * @return false if no component is in the path
   */
  static auto checkIfAnyComponentIs(const std::filesystem::path &Path,
                                    const std::vector<std::wstring> &Components) -> bool;

  /**
   * @brief Check if any glob in list matches string. Globs are basic MS-DOS wildcards, a * represents any number of any character including slashes
   *
   * @param Str String to check
   * @param GlobList Globs to check
   * @return true if any match
   * @return false if none match
   */
  static auto checkGlob(const std::wstring &Str, const std::vector<std::wstring> &GlobList) -> bool;

private:
  /**
   * @brief Looks through each BSA and adds files to the file map
   */
  void addBSAFilesToMap();

  /**
   * @brief Looks through all loose files in the load order and adds to the file
   * map
   */
  void addLooseFilesToMap();

  /**
   * @brief Add files in a BSA to the file map
   *
   * @param BSAName BSA name to read files from
   */
  void addBSAToFileMap(const std::wstring &BSAName);

  /**
   * @brief Check if a file being added to the file map should be added
   *
   * @param FilePath File being checked
   * @return true if file should be added
   * @return false if file should not be added
   */
  static auto isFileAllowed(const std::filesystem::path &FilePath) -> bool;

  /**
   * @brief Get BSA files defined in INI files
   *
   * @return std::vector<std::wstring> list of BSAs in the order the INI has
   * them
   */
  [[nodiscard]] auto getBSAFilesFromINIs() const -> std::vector<std::wstring>;

  /**
   * @brief Get BSA files in the data directory
   *
   * @return std::vector<std::wstring> list of BSAs in the data directory
   */
  [[nodiscard]] auto getBSAFilesInDirectory() const -> std::vector<std::wstring>;

  /**
   * @brief gets BSA files that are loaded with a plugin
   *
   * @param BSAFileList List of BSA files to check
   * @param PluginPrefix Plugin to check without extension
   * @return std::vector<std::wstring> list of BSAs loaded with plugin
   */
  [[nodiscard]] auto findBSAFilesFromPluginName(const std::vector<std::wstring> &BSAFileList,
                                                const std::wstring &PluginPrefix) const -> std::vector<std::wstring>;

  /**
   * @brief Get a file object from the file map
   *
   * @param FilePath Path to get the object for
   * @return BethesdaFile object of file in load order
   */
  [[nodiscard]] auto getFileFromMap(const std::filesystem::path &FilePath) -> BethesdaFile;

  /**
   * @brief Update the file map with
   *
   * @param FilePath path to update or add
   * @param BSAFile BSA file or nullptr if it doesn't exist
   */
  void updateFileMap(const std::filesystem::path &FilePath, std::shared_ptr<BSAFile> BSAFile, const std::wstring &Mod = L"", const bool &Generated = false);

  /**
   * @brief Convert a list of wstrings to a LPCWSTRs
   *
   * @param Original original list of wstrings to convert
   * @return std::vector<LPCWSTR> list of LPCWSTRs
   */
  [[nodiscard]] static auto
  convertWStringToLPCWSTRVector(const std::vector<std::wstring> &Original) -> std::vector<LPCWSTR>;

  /**
   * @brief Checks whether a glob matches a provided list of globs
   *
   * @param Str String to check
   * @param WinningGlob Last glob that one (for performance)
   * @param GlobList List of globs to check
   * @return true Any glob is the list mated
   * @return false No globs in the list matched
   */
  static auto checkGlob(const LPCWSTR &Str, LPCWSTR &WinningGlob, const std::vector<LPCWSTR> &GlobList) -> bool;

  static auto readINIValue(const std::filesystem::path &INIPath, const std::wstring &Section, const std::wstring &Key,
                           const bool &Logging, const bool &FirstINIRead) -> std::wstring;
};
