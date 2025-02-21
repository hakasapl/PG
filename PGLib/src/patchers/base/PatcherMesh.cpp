#include "patchers/base/PatcherMesh.hpp"

using namespace std;

PatcherMesh::PatcherMesh(filesystem::path nifPath, nifly::NifFile* nif, string patcherName, const bool& triggerSave)
    : Patcher(std::move(patcherName), triggerSave)
    , m_nifPath(std::move(nifPath))
    , m_nif(nif)
{
}

auto PatcherMesh::getNIFPath() const -> filesystem::path { return m_nifPath; }
auto PatcherMesh::getNIF() const -> nifly::NifFile* { return m_nif; }
