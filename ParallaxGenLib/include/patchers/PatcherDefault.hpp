#pragma once

#include <NifFile.hpp>
#include <filesystem>
#include <string>

#include "NIFUtil.hpp"
#include "Patchers/PatcherShader.hpp"

/**
 * @class PatcherVanilla
 * @brief Patcher for vanilla
 */
class PatcherDefault : public PatcherShader {
public:
    /**
     * @brief Get the Factory object for parallax patcher
     *
     * @return PatcherShader::PatcherShaderFactory Factory object
     */
    static auto getFactory() -> PatcherShader::PatcherShaderFactory;

    /**
     * @brief Get the Shader Type for this patcher (Parallax)
     *
     * @return NIFUtil::ShapeShader Parallax
     */
    static auto getShaderType() -> NIFUtil::ShapeShader;

    /**
     * @brief Construct a new Patcher Vanilla Parallax object
     *
     * @param nifPath NIF path to patch
     * @param nif NIF object to patch
     */
    PatcherDefault(std::filesystem::path nifPath, nifly::NifFile* nif);

    /**
     * @brief Check if a shape can be patched by this patcher (without looking at slots)
     *
     * @param nifShape Shape to check
     * @return true Shape can be patched
     * @return false Shape cannot be patched
     */
    auto canApply(nifly::NiShape& nifShape) -> bool override;

    /**
     * @brief Check if a shape can be patched by this patcher (with slots)
     *
     * @param nifShape Shape to check
     * @param[out] matches Matches found
     * @return true Found matches
     * @return false No matches found
     */
    auto shouldApply(nifly::NiShape& nifShape, std::vector<PatcherMatch>& matches) -> bool override;

    /**
     * @brief Check if slots can accomodate parallax
     *
     * @param oldSlots Slots to check
     * @param[out] matches Matches found
     * @return true Found matches
     * @return false No matches found
     */
    auto shouldApply(const std::array<std::wstring, NUM_TEXTURE_SLOTS>& oldSlots, std::vector<PatcherMatch>& matches)
        -> bool override;

    /**
     * @brief Apply a match to a shape for parallax
     *
     * @param nifShape Shape to patch
     * @param match Match to apply
     * @param[out] nifModified Whether the NIF was modified
     * @param[out] shapeDeleted Whether the shape was deleted (always false)
     * @return std::array<std::wstring, NUM_TEXTURE_SLOTS> New slots of shape
     */
    auto applyPatch(nifly::NiShape& nifShape, const PatcherMatch& match, bool& nifModified)
        -> std::array<std::wstring, NUM_TEXTURE_SLOTS> override;

    /**
     * @brief Apply a match to slots for parallax
     *
     * @param oldSlots Slots to patch
     * @param match Match to apply
     * @return std::array<std::wstring, NUM_TEXTURE_SLOTS> New slots
     */
    auto applyPatchSlots(const std::array<std::wstring, NUM_TEXTURE_SLOTS>& oldSlots, const PatcherMatch& match)
        -> std::array<std::wstring, NUM_TEXTURE_SLOTS> override;

    void processNewTXSTRecord(const PatcherMatch& match, const std::string& edid = {}) override;

    /**
     * @brief Apply default shader to a shape (does nothing)
     *
     * @param nifShape Shape to apply shader to
     * @param nifModified Whether the NIF was modified
     */
    void applyShader(nifly::NiShape& nifShape, bool& nifModified) override;
};
