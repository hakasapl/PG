#include "patchers/base/PatcherMeshShader.hpp"

#include "NIFUtil.hpp"
#include "ParallaxGenUtil.hpp"
#include <BasicTypes.hpp>
#include <Shaders.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

using namespace std;

// statics

unordered_map<tuple<filesystem::path, uint32_t>, PatcherMeshShader::PatchedTextureSet,
    PatcherMeshShader::PatchedTextureSetsHash>
    PatcherMeshShader::s_patchedTextureSets;
mutex PatcherMeshShader::s_patchedTextureSetsMutex;

// Constructor
PatcherMeshShader::PatcherMeshShader(filesystem::path nifPath, nifly::NifFile* nif, string patcherName)
    : PatcherMesh(std::move(nifPath), nif, std::move(patcherName))
{
}

auto PatcherMeshShader::getTextureSet(nifly::NiShape& nifShape) -> array<wstring, NUM_TEXTURE_SLOTS>
{
    const lock_guard<mutex> lock(s_patchedTextureSetsMutex);

    auto* const nifShader = getNIF()->GetShader(&nifShape);
    const auto texturesetBlockID = getNIF()->GetBlockID(getNIF()->GetHeader().GetBlock(nifShader->TextureSetRef()));
    const auto nifShapeKey = make_tuple(getNIFPath(), texturesetBlockID);

    // check if in patchedtexturesets
    if (s_patchedTextureSets.find(nifShapeKey) != s_patchedTextureSets.end()) {
        return s_patchedTextureSets[nifShapeKey].original;
    }

    // get the texture slots
    return NIFUtil::getTextureSlots(getNIF(), &nifShape);
}

auto PatcherMeshShader::setTextureSet(nifly::NiShape& nifShape, const array<wstring, NUM_TEXTURE_SLOTS>& textures)
    -> bool
{
    // TODO this can be local not static
    const lock_guard<mutex> lock(s_patchedTextureSetsMutex);

    auto* const nifShader = getNIF()->GetShader(&nifShape);
    const auto textureSetBlockID = getNIF()->GetBlockID(getNIF()->GetHeader().GetBlock(nifShader->TextureSetRef()));
    const auto nifShapeKey = make_tuple(getNIFPath(), textureSetBlockID);

    if (s_patchedTextureSets.find(nifShapeKey) != s_patchedTextureSets.end()) {
        // This texture set has been patched before
        uint32_t newBlockID = 0;

        // already been patched, check if it is the same
        for (const auto& [possibleTexRecordID, possibleTextures] : s_patchedTextureSets[nifShapeKey].patchResults) {
            if (possibleTextures == textures) {
                newBlockID = possibleTexRecordID;

                if (newBlockID == textureSetBlockID) {
                    return false;
                }

                break;
            }
        }

        // Add a new texture set to the NIF
        if (newBlockID == 0) {
            auto newTextureSet = make_unique<nifly::BSShaderTextureSet>();
            newTextureSet->textures.resize(NUM_TEXTURE_SLOTS);
            for (uint32_t i = 0; i < textures.size(); i++) {
                newTextureSet->textures[i] = ParallaxGenUtil::utf16toASCII(textures.at(i));
            }

            // Set shader reference
            newBlockID = getNIF()->GetHeader().AddBlock(std::move(newTextureSet));
        }

        auto* const nifShaderBSLSP = dynamic_cast<nifly::BSLightingShaderProperty*>(nifShader);
        const NiBlockRef<BSShaderTextureSet> newBlockRef(newBlockID);
        nifShaderBSLSP->textureSetRef = newBlockRef;

        s_patchedTextureSets[nifShapeKey].patchResults[newBlockID] = textures;
        return true;
    }

    // set original for future use
    const auto slots = NIFUtil::getTextureSlots(getNIF(), &nifShape);
    s_patchedTextureSets[nifShapeKey].original = slots;

    // set the texture slots for the shape like normal
    const bool changed = NIFUtil::setTextureSlots(getNIF(), &nifShape, textures);

    // update the patchedtexturesets
    s_patchedTextureSets[nifShapeKey].patchResults[textureSetBlockID] = textures;

    return changed;
}
