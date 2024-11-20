#include "BethesdaGame.hpp"
#include "BethesdaDirectory.hpp"
#include "CommonTests.hpp"
#include "ParallaxGenPlugin.hpp"
#include "ParallaxGenUtil.hpp"

#include <gtest/gtest.h>

#include <boost/algorithm/string/predicate.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <cwctype>

using namespace std;

class BethesdaDirectoryTest : public ::testing::TestWithParam<PGTesting::TestEnvGameParams> {
protected:
  // Set up code for each test
  void SetUp() override {
    auto &Params = GetParam();

    BG = make_unique<BethesdaGame>(Params.GameType, false, Params.GamePath, Params.AppDataPath, Params.DocumentPath); // no logging
    BD = make_unique<BethesdaDirectory>(*BG, "", nullptr, false); // no logging
  }

  // Tear down code for each test
  void TearDown() override {
    // Clean up after each test
  }
  std::unique_ptr<BethesdaGame> BG;
  std::unique_ptr<BethesdaDirectory> BD;
};

TEST_P(BethesdaDirectoryTest, Initialization) {
  auto &Params = GetParam();
  EXPECT_TRUE(std::filesystem::equivalent(BD->getDataPath(), Params.GamePath / "Data"));
}

TEST_P(BethesdaDirectoryTest, Path) {
  EXPECT_FALSE(BD->isPathAscii({L"textures\\texture\u00a1"}));
  EXPECT_TRUE(BD->isPathAscii({L"textures\\texture~"}));
  EXPECT_FALSE(BD->isPathAscii({L"textures\\texture�~"}));

  EXPECT_FALSE(BD->isPathAscii({"textures\\texture�~"}));
  EXPECT_TRUE(BD->isPathAscii({"textures\\texture~"}));

  EXPECT_TRUE(BD->getAsciiPathLower({L"Textures\\Texture"}) == L"textures\\texture");

  EXPECT_TRUE(BD->pathEqualityIgnoreCase({L"Textures\\Texture"}, {L"textures\\texture"}));
  EXPECT_FALSE(BD->pathEqualityIgnoreCase({L"Textures\\Texture1"}, {L"textures\\texture2"}));
  EXPECT_FALSE(BD->pathEqualityIgnoreCase({L"textures\\texture1"}, {L"texture\\texture2"}));

  // path separator
  EXPECT_TRUE(BD->pathEqualityIgnoreCase({L"Textures\\Texture"}, {L"textures/texture"}));
  EXPECT_FALSE(BD->pathEqualityIgnoreCase({L"Textures\\Texture1"}, {L"textures/texture2"}));
  EXPECT_FALSE(BD->pathEqualityIgnoreCase({L"textures\\texture1"}, {L"textures/texture2"}));

  EXPECT_TRUE(BD->checkIfAnyComponentIs(L"meshes\\landscape\\bridges\\bridge01.nif", {L"landscape", L"architecture"}));
  EXPECT_FALSE(BD->checkIfAnyComponentIs(L"meshes\\landscape\\bridges\\bridge01.nif", {L"architecture"}));
}

TEST_P(BethesdaDirectoryTest, Glob) {
    std::vector<wstring> GlobList{L"textures\\texture*.dds", L"meshes\\*.nif"};
    EXPECT_TRUE(BD->checkGlob(L"textures\\texture0.dds", GlobList));
    EXPECT_TRUE(BD->checkGlob(L"meshes\\mesh0.nif", GlobList));
    EXPECT_TRUE(BD->checkGlob(L"meshes\\architecture/mesh1.nif", GlobList));
    EXPECT_FALSE(BD->checkGlob(L"textures\\architecture\\texture1.dds", GlobList));
    EXPECT_FALSE(BD->checkGlob(L"scripts\\script1.pex", GlobList));
    EXPECT_FALSE(BD->checkGlob(L"textures\\texture0.png", GlobList));

    std::vector<wstring> GlobListCfg {L"*\\cameras\\*" };
    EXPECT_TRUE(BD->checkGlob(L"meshes\\cameras\\camera1.nif", GlobListCfg));
    EXPECT_TRUE(BD->checkGlob(L"meshes\\submeshes\\cameras\\camera1.nif", GlobListCfg));
    EXPECT_TRUE(BD->checkGlob(L"meshes\\Cameras\\camera1.nif", GlobListCfg));
    EXPECT_FALSE(BD->checkGlob(L"cameras\\camera1.nif", GlobListCfg));
    EXPECT_FALSE(BD->checkGlob(L"camera1.nif", GlobListCfg));
    EXPECT_FALSE(BD->checkGlob(L"cameras1.nif", GlobListCfg));

    // upper case
    std::vector<wstring> GlobListCfgU{L"*\\cameras\\*"};
    EXPECT_TRUE(BD->checkGlob(L"meshes\\cameras\\camera1.nif", GlobListCfgU));
    EXPECT_TRUE(BD->checkGlob(L"meshes\\submeshes\\cameras\\camera1.nif", GlobListCfgU));
    EXPECT_TRUE(BD->checkGlob(L"meshes\\Cameras\\camera1.nif", GlobListCfgU));

    // TODO: really needed on windows? forward slashes in glob
    std::vector<wstring> GlobListCfgSL{L"*/Cameras/*"};
    EXPECT_TRUE(BD->checkGlob(L"meshes/cameras/camera1.nif", GlobListCfgSL));
    EXPECT_TRUE(BD->checkGlob(L"meshes/submeshes/cameras/camera1.nif", GlobListCfgSL));
    EXPECT_TRUE(BD->checkGlob(L"meshes/Cameras/camera1.nif", GlobListCfgSL));
}

TEST_P(BethesdaDirectoryTest, FindFiles) {
  BD->populateFileMap(true);

  // find all meshes and make sure they are contained in the file map
  std::vector<wstring> GlobListCfg{L"meshes\\*"};
  auto Files = BD->findFiles(false, GlobListCfg);
  auto &FileMap = BD->getFileMap();

  EXPECT_FALSE(Files.empty());
  for (auto &File : Files) {
    ASSERT_TRUE(BD->isFile(File)) << File;
    if (!FileMap.contains(File)) {
      ASSERT_TRUE(FileMap.contains(BD->getAsciiPathLower(File))); // if path has upper characters it must still be in the file map
    }
  }

  // make sure file from BSA is found
  EXPECT_TRUE(std::find(Files.begin(), Files.end(), L"meshes\\landscape\\bridges\\bridge01.nif") != Files.end());

  // find loose file, default filenames
  Files = BD->findFiles(false, {L"textures\\smim\\clutter\\common\\*"});
  EXPECT_FALSE(Files.empty());
  const auto ClutterCommonTextures = std::vector<std::filesystem::path>{L"textures\\smim\\clutter\\common\\Stump_Bottom_for_Furniture.dds",
                                                                        L"textures\\smim\\clutter\\common\\Stump_Bottom_for_Furniture_n.dds",
                                                                        L"textures\\smim\\clutter\\common\\Stump_Bottom_for_Furniture_p.dds"};
  EXPECT_TRUE(Files == ClutterCommonTextures);

  // find loose files, lower filenames
  Files = BD->findFiles(true, {L"textures\\smim\\clutter\\common\\*"});
  EXPECT_FALSE(Files.empty());
  const auto ClutterCommonTexturesL = std::vector<std::filesystem::path>{L"textures\\smim\\clutter\\common\\stump_bottom_for_furniture.dds",
                                                                         L"textures\\smim\\clutter\\common\\stump_bottom_for_furniture_n.dds",
                                                                         L"textures\\smim\\clutter\\common\\stump_bottom_for_furniture_p.dds"};
  EXPECT_TRUE(Files == ClutterCommonTexturesL);

  // test file exclusion
  Files = BD->findFiles(false, { L"meshes\\*" } , {L"meshes\\landscape\\*"});
  EXPECT_TRUE(std::find(Files.begin(), Files.end(), L"meshes\\landscape\\bridges\\bridge01.nif") == Files.end());

  Files = BD->findFiles(false, {L"meshes\\*"}, {L"meshes\\landscape\\bridge*.nif"});
  EXPECT_TRUE(std::find(Files.begin(), Files.end(), L"meshes\\landscape\\bridges\\bridge01.nif") == Files.end());

  Files = BD->findFiles(false, {L"meshes\\*"}, {L"meshes\\furniture\\*"});
  EXPECT_TRUE(std::find(Files.begin(), Files.end(), L"meshes\\landscape\\bridges\\bridge01.nif") != Files.end());

  // test archive exclusion
  Files = BD->findFiles(false, {L"meshes\\*"}, {L"meshes\\furniture\\*"}, {L"Skyrim - Meshes1.bsa"});
  EXPECT_TRUE(std::find(Files.begin(), Files.end(), L"meshes\\landscape\\bridges\\bridge01.nif") == Files.end());

  Files = BD->findFiles(false, {L"meshes\\*"}, {L"meshes\\furniture\\*"}, {L"skyrim - meshes1.bsa"});
  EXPECT_TRUE(std::find(Files.begin(), Files.end(), L"meshes\\landscape\\bridges\\bridge01.nif") == Files.end());

  // corner cases
  Files = BD->findFiles(false, {L"meshes\\*"}, {L"meshes\\*"});
  EXPECT_TRUE(Files.empty());

  Files = BD->findFiles(false, {L"*"}, {L"*"});
  EXPECT_TRUE(Files.empty());
};

TEST_P(BethesdaDirectoryTest, Files) {
  const std::filesystem::path Bridge01Path{"textures\\landscape\\roads\\bridge01.dds"};

  const std::unordered_set<wstring> LoadOrderBSAs{L"Skyrim - Textures5.bsa", L"Skyrim - Misc.bsa",
                                                  L"unofficial skyrim special edition patch - textures.bsa",
                                                  L"Skyrim - Meshes1.bsa"};

  const std::unordered_set<wstring> NotLoadOrderBSAs{L"Skyrim - Textures0.bsa"};

  // functions throw exceptions if called before populating
  EXPECT_THROW(BD->isBSAFile(Bridge01Path), runtime_error);
  EXPECT_THROW(BD->isFile(Bridge01Path), runtime_error);
  EXPECT_THROW(BD->isLooseFile(Bridge01Path), runtime_error);
  EXPECT_THROW(BD->isBSAFile(Bridge01Path), runtime_error);

  // loose files only
  BD->populateFileMap(false);
  auto &FileMapLoose = BD->getFileMap();
  const std::filesystem::path WRCarpet01Path{L"textures\\architecture\\whiterun\\wrcarpet01.dds"};
  EXPECT_TRUE(BD->isFile(WRCarpet01Path));
  EXPECT_TRUE(BD->isLooseFile(WRCarpet01Path));
  EXPECT_FALSE(BD->isBSAFile(WRCarpet01Path));

  EXPECT_TRUE(FileMapLoose.at(WRCarpet01Path).BSAFile == nullptr);

  EXPECT_TRUE(!BD->getFile(WRCarpet01Path, false).empty());
  EXPECT_TRUE(!BD->getFile(WRCarpet01Path, true).empty());

  EXPECT_FALSE(BD->isFileInBSA(WRCarpet01Path, LoadOrderBSAs)); // no BSA was loaded, so function must return false

  // Skyrim - Textures5.bsa from env
  EXPECT_FALSE(BD->isFile(Bridge01Path));
  EXPECT_FALSE(BD->isLooseFile(Bridge01Path));
  EXPECT_FALSE(BD->isBSAFile(Bridge01Path));

  EXPECT_THROW(BD->getFile(Bridge01Path, false), runtime_error);
  EXPECT_THROW(BD->getFile(Bridge01Path, true), runtime_error);

  EXPECT_FALSE(BD->isFileInBSA(Bridge01Path, LoadOrderBSAs));
  EXPECT_FALSE(BD->isFileInBSA(Bridge01Path, NotLoadOrderBSAs));

  // all files
  BD->populateFileMap(true);
  auto &FileMap = BD->getFileMap();

  // Sycerscote.bsa
  EXPECT_FALSE(BD->isFile(L"meshes\\syerscote\\goldpot01Ã,Â°.nif"));

  // Skyrim - Textures5.bsa from env
  EXPECT_TRUE(BD->isFile(Bridge01Path));
  EXPECT_FALSE(BD->isLooseFile(Bridge01Path));
  EXPECT_TRUE(BD->isBSAFile(Bridge01Path));

  EXPECT_TRUE(FileMap.at(Bridge01Path).BSAFile != nullptr);
  EXPECT_TRUE(FileMap.at(Bridge01Path).BSAFile->Path.filename() == std::filesystem::path{L"Skyrim - Textures5.bsa"});
  EXPECT_TRUE(FileMap.at(Bridge01Path).Path == Bridge01Path);

  EXPECT_TRUE(!BD->getFile(Bridge01Path, false).empty());
  EXPECT_TRUE(!BD->getFile(Bridge01Path,true).empty());

  EXPECT_TRUE(BD->isFileInBSA(Bridge01Path, LoadOrderBSAs));
  EXPECT_FALSE(BD->isFileInBSA(Bridge01Path, NotLoadOrderBSAs));

  // Skyrim - Misc.bsa
  const std::filesystem::path AbForSwornScriptPath{L"scripts\\abforswornbriarheartscript.pex"};
  EXPECT_TRUE(BD->isFile(AbForSwornScriptPath));
  EXPECT_FALSE(BD->isLooseFile(AbForSwornScriptPath));
  EXPECT_TRUE(BD->isBSAFile(AbForSwornScriptPath));

  EXPECT_TRUE(FileMap.at(AbForSwornScriptPath).BSAFile != nullptr);
  EXPECT_TRUE(FileMap.at(AbForSwornScriptPath).BSAFile->Path.filename() == std::filesystem::path{L"Skyrim - Misc.bsa"});
  EXPECT_TRUE(FileMap.at(AbForSwornScriptPath).Path == AbForSwornScriptPath);

  EXPECT_TRUE(!BD->getFile(AbForSwornScriptPath, false).empty());
  EXPECT_TRUE(!BD->getFile(AbForSwornScriptPath, true).empty());

  EXPECT_TRUE(BD->isFileInBSA(AbForSwornScriptPath, LoadOrderBSAs));
  EXPECT_FALSE(BD->isFileInBSA(AbForSwornScriptPath, NotLoadOrderBSAs));

  // unofficial skyrim special edition patch - textures.bsa
  const std::filesystem::path StoneQuarry01Path{L"textures\\_byoh\\clutter\\resources/stonequarry01.dds"};
  EXPECT_TRUE(BD->isFile(StoneQuarry01Path));
  EXPECT_FALSE(BD->isLooseFile(StoneQuarry01Path));
  EXPECT_TRUE(BD->isBSAFile(StoneQuarry01Path));

  EXPECT_TRUE(FileMap.at(StoneQuarry01Path).BSAFile != nullptr);
  EXPECT_TRUE(FileMap.at(StoneQuarry01Path).BSAFile->Path.filename() == std::filesystem::path{L"unofficial skyrim special edition patch - textures.bsa"});
  EXPECT_TRUE(FileMap.at(StoneQuarry01Path).Path == StoneQuarry01Path);

  EXPECT_TRUE(!BD->getFile(StoneQuarry01Path, false).empty());
  EXPECT_TRUE(!BD->getFile(StoneQuarry01Path, true).empty());

  EXPECT_TRUE(BD->isFileInBSA(StoneQuarry01Path, LoadOrderBSAs));
  EXPECT_FALSE(BD->isFileInBSA(StoneQuarry01Path, NotLoadOrderBSAs));

  // Skyrim - Meshes1.bsa
  const std::filesystem::path Road3way01Path{L"meshes\\landscape\\roads\\road3way01.nif"};
  EXPECT_TRUE(BD->isFile(Road3way01Path));
  EXPECT_FALSE(BD->isLooseFile(Road3way01Path));
  EXPECT_TRUE(BD->isBSAFile(Road3way01Path));

  EXPECT_TRUE(FileMap.at(Road3way01Path).BSAFile != nullptr);
  EXPECT_TRUE(FileMap.at(Road3way01Path).BSAFile->Path.filename() == std::filesystem::path{L"Skyrim - Meshes1.bsa"});
  EXPECT_TRUE(FileMap.at(Road3way01Path).Path == Road3way01Path);

  EXPECT_TRUE(!BD->getFile(Road3way01Path).empty());
  EXPECT_TRUE(!BD->getFile(Road3way01Path, true).empty());

  EXPECT_TRUE(BD->isFileInBSA(StoneQuarry01Path, LoadOrderBSAs));
  EXPECT_FALSE(BD->isFileInBSA(StoneQuarry01Path, NotLoadOrderBSAs));

  // loose file overwriting BSA file
  EXPECT_TRUE(BD->isFile(WRCarpet01Path));
  EXPECT_TRUE(BD->isLooseFile(WRCarpet01Path));
  EXPECT_FALSE(BD->isBSAFile(WRCarpet01Path));

  EXPECT_TRUE(FileMap.at(WRCarpet01Path).BSAFile == nullptr);

  EXPECT_TRUE(!BD->getFile(WRCarpet01Path, false).empty());
  EXPECT_TRUE(!BD->getFile(WRCarpet01Path, true).empty());

  EXPECT_TRUE(BD->isFileInBSA(Bridge01Path, LoadOrderBSAs));

  // check if files can still be retrieved after clearing cache
  BD->clearCache();
  EXPECT_TRUE(!BD->getFile(Bridge01Path).empty());
  EXPECT_TRUE(!BD->getFile(AbForSwornScriptPath).empty());
  EXPECT_TRUE(!BD->getFile(StoneQuarry01Path).empty());
  EXPECT_TRUE(!BD->getFile(Road3way01Path).empty());
  EXPECT_TRUE(!BD->getFile(WRCarpet01Path).empty());

  // file from Skyrim - Textures0, not in env
  const std::filesystem::path AlduinPath{L"textures\\actors\\alduin\\alduin.dds"};
  EXPECT_FALSE(BD->isBSAFile(AlduinPath));
  EXPECT_FALSE(BD->isFile(AlduinPath));
  EXPECT_FALSE(BD->isLooseFile(AlduinPath));

  // upper case path
  //  Skyrim - Textures5.bsa from env
  const std::filesystem::path Bridge01PathUpper{L"TEXTURES\\LANDSCAPE\\ROADS\\bridge01.dds"};
  EXPECT_THROW(FileMap.at(Bridge01PathUpper), out_of_range);
  auto Bridge01PathLower = BD->getAsciiPathLower(Bridge01Path);
  EXPECT_TRUE(FileMap.at(Bridge01PathLower).Path == Bridge01PathLower); // original path from BSA is lower case

  // file is same with and without cache
  auto BridgeFileWOCache = BD->getFile(Bridge01Path, false);
  auto BridgeFileWCache = BD->getFile(Bridge01Path, true);
  EXPECT_TRUE(BridgeFileWOCache == BridgeFileWCache);
  BD->clearCache();
  BridgeFileWOCache = BD->getFile(Bridge01Path, false);
  EXPECT_TRUE(BridgeFileWOCache == BridgeFileWCache);
}

TEST_P(BethesdaDirectoryTest, LoadOrder) {
  auto &Params = GetParam();

  auto LoadOrder = BD->getBSALoadOrder();
  EXPECT_TRUE(LoadOrder.size() == 6);

  EXPECT_TRUE(std::find(LoadOrder.begin(), LoadOrder.end(), L"Skyrim - Meshes1.bsa") != LoadOrder.end());
  EXPECT_TRUE(std::find(LoadOrder.begin(), LoadOrder.end(), L"Skyrim - Textures5.bsa") != LoadOrder.end());
  EXPECT_TRUE(std::find(LoadOrder.begin(), LoadOrder.end(), L"Skyrim - Misc.bsa") != LoadOrder.end());
  EXPECT_TRUE(std::find(LoadOrder.begin(), LoadOrder.end(), L"ccBGSSSE037-Curios.bsa") != LoadOrder.end());
  EXPECT_TRUE(std::find(LoadOrder.begin(), LoadOrder.end(), L"unofficial skyrim special edition patch - textures.bsa") != LoadOrder.end());
  EXPECT_TRUE(std::find(LoadOrder.begin(), LoadOrder.end(), L"unofficial skyrim special edition patch.bsa") != LoadOrder.end());
}

TEST_P(BethesdaDirectoryTest, FileMap) {
  auto &Params = GetParam();

  // only loose files
  BD->populateFileMap(false);
  auto& FileMapNoBSA = BD->getFileMap();
  EXPECT_TRUE(!FileMapNoBSA.empty());

  // all files
  BD->populateFileMap(true);
  auto& FileMap = BD->getFileMap();

  ASSERT_TRUE(!FileMap.empty());
  auto File = FileMap.begin();
  cout << FileMap.size();

  // ASSERT_ is used here to stop on the first error and prevent the log spammed with thousands of error messages
  for (auto &MapEntry : FileMap) {
    auto PathStr = MapEntry.first.wstring();

    const int upperChars = std::count_if(PathStr.begin(), PathStr.end(), [](wchar_t c) { return std::iswupper(c); });
    ASSERT_TRUE(upperChars == 0);

    ASSERT_TRUE(boost::iequals(PathStr, MapEntry.second.Path.wstring()));

    auto BSAFile = MapEntry.second.BSAFile;

    if ( !(BSAFile == nullptr) ) {
        ASSERT_TRUE(std::filesystem::exists(BSAFile->Path));
        ASSERT_FALSE(BD->isLooseFile(MapEntry.first));
        ASSERT_TRUE(BD->isBSAFile(MapEntry.first));
    } else {
        ASSERT_TRUE(BD->isLooseFile(MapEntry.first));
    }
  }

  // clear file cache
  BD->clearCache();
  auto &FileMapAfterClearCache = BD->getFileMap();
  EXPECT_FALSE(FileMapAfterClearCache.empty());
}

INSTANTIATE_TEST_SUITE_P(GameParametersSE, BethesdaDirectoryTest, ::testing::Values(PGTestEnvs::TESTENVSkyrimSE));
