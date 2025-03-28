#include "patchers/base/PatcherMeshPre.hpp"

using namespace std;

PatcherMeshPre::PatcherMeshPre(
    std::filesystem::path nifPath, nifly::NifFile* nif, std::string patcherName, const bool& triggerSave)
    : PatcherMesh(std::move(nifPath), nif, std::move(patcherName), triggerSave)
{
}
