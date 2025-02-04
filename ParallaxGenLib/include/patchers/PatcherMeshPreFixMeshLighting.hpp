#pragma once

#include "patchers/base/PatcherMeshPre.hpp"

class PatcherMeshPreFixMeshLighting : public PatcherMeshPre {
private:
    constexpr static float SOFTLIGHTING_MAX = 0.6F;

public:
    /**
     * @brief Get the Factory object
     *
     * @return PatcherShaderTransform::PatcherShaderTransformFactory
     */
    static auto getFactory() -> PatcherMeshPre::PatcherMeshPreFactory;

    /**
     * @brief Construct a new PrePatcher Particle Lights To LP patcher
     *
     * @param nifPath NIF path to be patched
     * @param nif NIF object to be patched
     */
    PatcherMeshPreFixMeshLighting(std::filesystem::path nifPath, nifly::NifFile* nif);

    /**
     * @brief Apply this patcher to shape
     *
     * @param nifShape Shape to patch
     * @param nifModified Whether NIF was modified
     * @param shapeDeleted Whether shape was deleted
     * @return true Shape was patched
     * @return false Shape was not patched
     */
    void applyPatch(nifly::NiShape& nifShape, bool& nifModified) override;
};
