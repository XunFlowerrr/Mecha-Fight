#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // Added for glm::make_vec3/vec2
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// #include <assimp/Importer.hpp>
// #include <assimp/scene.h>
// #include <assimp/postprocess.h>
#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#endif
#include "tiny_gltf.h" // ADDED for GLTF support (includes stb_image.h)
#include "stb_image.h" // Ensure stbi_* functions are visible in global namespace

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cfloat>
#include <iomanip>
#include <functional>
#include <utility>
using namespace std;

inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
    // model data
    vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;
    glm::vec3 boundingMin;
    glm::vec3 boundingMax;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        boundingMin = glm::vec3(FLT_MAX);
        boundingMax = glm::vec3(-FLT_MAX);
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        int lastSkinIndex = std::numeric_limits<int>::min();
        static bool loggedNoSkin = false;
        for (unsigned int i = 0; i < meshes.size(); i++)
        {
            const Mesh &mesh = meshes[i];
            if (mesh.skinIndex != lastSkinIndex)
            {
                applySkinningUniforms(shader, mesh.skinIndex);
                lastSkinIndex = mesh.skinIndex;
            }
            meshes[i].Draw(shader);
            if (mesh.skinIndex < 0 && !loggedNoSkin)
            {
                std::cout << "[GLTF] Draw mesh without skin (" << mesh.materialName << ")" << std::endl;
                loggedNoSkin = true;
            }
        }

        shader.setBool("useSkinning", false);
        shader.setInt("bonesCount", 0);
    }

    glm::vec3 GetBoundingMin() const { return boundingMin; }
    glm::vec3 GetBoundingMax() const { return boundingMax; }
    glm::vec3 GetDimensions() const { return boundingMax - boundingMin; }
    bool HasSkins() const { return !skins.empty(); }
    bool HasAnimations() const { return !animationClips.empty(); }
    int GetAnimationClipCount() const { return static_cast<int>(animationClips.size()); }
    int GetActiveAnimationIndex() const { return activeAnimation; }
    void SetActiveAnimation(int animationIndex);
    void UpdateAnimation(float deltaTime);
    void SetAnimationPlaybackWindow(float startTimeSeconds, float endTimeSeconds);
    void ClearAnimationPlaybackWindow();
    float GetAnimationClipDuration(int animationIndex) const;
    float GetActiveAnimationDuration() const;
    void StartAnimationBlend(int targetAnimationIndex, float durationSeconds, bool targetWindowEnabled,
                             float targetWindowStartSeconds, float targetWindowEndSeconds);
    bool IsAnimationBlendActive() const { return animationBlendActive; }

private:
    static constexpr int MAX_BONES = 100;

    struct SkinData
    {
        std::string name;
        std::vector<int> joints;
        std::vector<glm::mat4> inverseBindMatrices;
        int skeletonRoot = -1;
    };

    struct NodeInfo
    {
        std::string name;
        int parent = -1;
        std::vector<int> children;
    };

    enum class ChannelPath
    {
        Translation,
        Rotation,
        Scale
    };

    struct AnimationSampler
    {
        std::vector<float> inputs;
        std::vector<glm::vec4> outputs;
        std::string interpolation = "LINEAR";
    };

    struct AnimationChannel
    {
        int samplerIndex = -1;
        int targetNode = -1;
        ChannelPath path = ChannelPath::Translation;
    };

    struct AnimationClip
    {
        std::string name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float duration = 0.0f;
    };

    std::vector<SkinData> skins;
    std::vector<int> nodeSkinBindings;
    std::vector<NodeInfo> nodes;
    std::vector<int> sceneRootNodes;

    std::vector<glm::vec3> nodeDefaultTranslations;
    std::vector<glm::quat> nodeDefaultRotations;
    std::vector<glm::vec3> nodeDefaultScales;

    std::vector<glm::vec3> nodeTranslations;
    std::vector<glm::quat> nodeRotations;
    std::vector<glm::vec3> nodeScales;

    std::vector<glm::mat4> nodeLocalMatrices;
    std::vector<glm::mat4> nodeGlobalMatrices;

    std::vector<AnimationClip> animationClips;
    int activeAnimation = -1;
    float currentAnimationTime = 0.0f;
    bool animationWindowEnabled = false;
    float animationWindowStart = 0.0f;
    float animationWindowEnd = 0.0f;

    std::vector<std::vector<glm::mat4>> skinMatrices;

    bool animationBlendActive = false;
    int blendAnimationIndex = -1;
    float blendAnimationTime = 0.0f;
    float blendDuration = 0.0f;
    float blendElapsed = 0.0f;
    bool blendWindowEnabled = false;
    float blendWindowStart = 0.0f;
    float blendWindowEnd = 0.0f;
    std::vector<glm::vec3> blendBaseTranslations;
    std::vector<glm::quat> blendBaseRotations;
    std::vector<glm::vec3> blendBaseScales;
    std::vector<glm::vec3> blendTargetTranslations;
    std::vector<glm::quat> blendTargetRotations;
    std::vector<glm::vec3> blendTargetScales;

    std::pair<float, float> getPlaybackWindow(const AnimationClip &clip) const;
    void ensureBlendPoseBuffers();
    float advanceAnimationTime(float currentTime, float deltaTime, float windowStart, float windowEnd) const;

    void loadModel(string const &path)
    {
        // --- OLD ASSIMP IMPLEMENTATION ---
        /*
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        size_t lastSlash = path.find_last_of("/\\");
        directory = (lastSlash != string::npos) ? path.substr(0, lastSlash) : "";

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
        */

        // --- NEW TINYGLTF IMPLEMENTATION ---
        boundingMin = glm::vec3(FLT_MAX);
        boundingMax = glm::vec3(-FLT_MAX);

        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        // retrieve the directory path of the filepath (same as assimp)
        size_t lastSlash = path.find_last_of("/\\");
        directory = (lastSlash != string::npos) ? path.substr(0, lastSlash) : "";

        bool ret;
        std::string ext = path.substr(path.find_last_of(".") + 1);
        if (ext == "glb")
        {
            std::cout << "Loading binary GLTF: " << path << std::endl;
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, path); // for .glb
        }
        else
        {
            std::cout << "Loading ASCII GLTF: " << path << std::endl;
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, path); // for .gltf
        }

        if (!warn.empty())
        {
            cout << "WARN: " << warn << endl;
        }

        if (!err.empty())
        {
            cout << "ERR: " << err << endl;
        }

        if (!ret)
        {
            cout << "Failed to parse glTF" << endl;
            return;
        }

        loadSkins(model);
        initializeNodeData(model);

        // A GLTF scene can have multiple "scenes". We'll just load the default one.
        const tinygltf::Scene &scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
            // We are processing the root nodes here.
            processNode(scene.nodes[i], model);
        }
    }

    void loadSkins(const tinygltf::Model &model)
    {
        skins.clear();
        skins.reserve(model.skins.size());
        nodeSkinBindings.assign(model.nodes.size(), -1);

        for (size_t nodeIdx = 0; nodeIdx < model.nodes.size(); ++nodeIdx)
        {
            nodeSkinBindings[nodeIdx] = model.nodes[nodeIdx].skin;
        }

        for (size_t skinIdx = 0; skinIdx < model.skins.size(); ++skinIdx)
        {
            const tinygltf::Skin &src = model.skins[skinIdx];
            SkinData skinData;
            skinData.name = src.name;
            skinData.skeletonRoot = src.skeleton;
            skinData.joints.assign(src.joints.begin(), src.joints.end());

            const size_t jointCount = skinData.joints.size();
            skinData.inverseBindMatrices.reserve(jointCount);

            if (src.inverseBindMatrices > -1)
            {
                const tinygltf::Accessor &accessor = model.accessors[src.inverseBindMatrices];
                if (accessor.bufferView > -1)
                {
                    const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
                    const unsigned char *dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

                    size_t stride = accessor.ByteStride(bufferView);
                    if (stride == 0)
                    {
                        stride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                 tinygltf::GetComponentSizeInBytes(accessor.componentType);
                    }

                    for (size_t i = 0; i < accessor.count; ++i)
                    {
                        const unsigned char *matrixPtr = dataPtr + i * stride;
                        glm::mat4 mat(1.0f);

                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float *floats = reinterpret_cast<const float *>(matrixPtr);
                            for (int col = 0; col < 4; ++col)
                            {
                                for (int row = 0; row < 4; ++row)
                                {
                                    mat[col][row] = floats[col * 4 + row];
                                }
                            }
                        }
                        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE)
                        {
                            const double *doubles = reinterpret_cast<const double *>(matrixPtr);
                            for (int col = 0; col < 4; ++col)
                            {
                                for (int row = 0; row < 4; ++row)
                                {
                                    mat[col][row] = static_cast<float>(doubles[col * 4 + row]);
                                }
                            }
                        }

                        skinData.inverseBindMatrices.push_back(mat);
                    }
                }
            }

            // Ensure the inverse bind list matches joint count.
            if (skinData.inverseBindMatrices.size() < jointCount)
            {
                for (size_t i = skinData.inverseBindMatrices.size(); i < jointCount; ++i)
                {
                    skinData.inverseBindMatrices.push_back(glm::mat4(1.0f));
                }
            }

            skins.push_back(std::move(skinData));
        }

        skinMatrices.clear();
        skinMatrices.resize(skins.size());
    }

    void initializeNodeData(const tinygltf::Model &model)
    {
        nodes.clear();
        nodes.resize(model.nodes.size());

        nodeDefaultTranslations.assign(model.nodes.size(), glm::vec3(0.0f));
        nodeDefaultRotations.assign(model.nodes.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        nodeDefaultScales.assign(model.nodes.size(), glm::vec3(1.0f));

        nodeTranslations = nodeDefaultTranslations;
        nodeRotations = nodeDefaultRotations;
        nodeScales = nodeDefaultScales;

        nodeLocalMatrices.assign(model.nodes.size(), glm::mat4(1.0f));
        nodeGlobalMatrices.assign(model.nodes.size(), glm::mat4(1.0f));

        for (size_t nodeIdx = 0; nodeIdx < model.nodes.size(); ++nodeIdx)
        {
            const tinygltf::Node &node = model.nodes[nodeIdx];
            NodeInfo &info = nodes[nodeIdx];
            info.name = node.name;
            info.children.assign(node.children.begin(), node.children.end());

            glm::vec3 translation(0.0f);
            if (node.translation.size() == 3)
            {
                translation = glm::vec3(static_cast<float>(node.translation[0]),
                                        static_cast<float>(node.translation[1]),
                                        static_cast<float>(node.translation[2]));
            }

            glm::vec3 scale(1.0f);
            if (node.scale.size() == 3)
            {
                scale = glm::vec3(static_cast<float>(node.scale[0]),
                                  static_cast<float>(node.scale[1]),
                                  static_cast<float>(node.scale[2]));
            }

            glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            if (node.rotation.size() == 4)
            {
                rotation = glm::quat(static_cast<float>(node.rotation[3]),
                                     static_cast<float>(node.rotation[0]),
                                     static_cast<float>(node.rotation[1]),
                                     static_cast<float>(node.rotation[2]));
                rotation = glm::normalize(rotation);
            }

            glm::mat4 localMatrix(1.0f);
            if (node.matrix.size() == 16)
            {
                for (int col = 0; col < 4; ++col)
                {
                    for (int row = 0; row < 4; ++row)
                    {
                        localMatrix[col][row] = static_cast<float>(node.matrix[col * 4 + row]);
                    }
                }

                glm::vec3 skew;
                glm::vec4 perspective;
                glm::vec3 translationVec;
                glm::vec3 scaleVec;
                glm::quat rotationQuat;
                if (glm::decompose(localMatrix, scaleVec, rotationQuat, translationVec, skew, perspective))
                {
                    translation = translationVec;
                    rotation = glm::normalize(glm::conjugate(rotationQuat));
                    scale = scaleVec;
                }
            }
            else
            {
                localMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
            }

            nodeDefaultTranslations[nodeIdx] = translation;
            nodeDefaultRotations[nodeIdx] = rotation;
            nodeDefaultScales[nodeIdx] = scale;
            nodeLocalMatrices[nodeIdx] = localMatrix;
        }

        for (size_t nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx)
        {
            for (int child : nodes[nodeIdx].children)
            {
                if (child >= 0 && child < static_cast<int>(nodes.size()))
                {
                    nodes[child].parent = static_cast<int>(nodeIdx);
                }
            }
        }

        buildSceneRoots(model);
        loadAnimations(model);
        resetAnimationPose();
        updateNodeMatrices();
        updateSkinMatrices();
    }

    void buildSceneRoots(const tinygltf::Model &model)
    {
        sceneRootNodes.clear();
        int sceneIndex = model.defaultScene;
        if (sceneIndex < 0 && !model.scenes.empty())
        {
            sceneIndex = 0;
        }

        if (sceneIndex >= 0 && sceneIndex < static_cast<int>(model.scenes.size()))
        {
            const tinygltf::Scene &scene = model.scenes[sceneIndex];
            sceneRootNodes.assign(scene.nodes.begin(), scene.nodes.end());
        }

        if (sceneRootNodes.empty())
        {
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                if (nodes[i].parent == -1)
                {
                    sceneRootNodes.push_back(static_cast<int>(i));
                }
            }
        }
    }

    void loadAnimations(const tinygltf::Model &model)
    {
        animationClips.clear();
        animationClips.reserve(model.animations.size());

        for (size_t animIdx = 0; animIdx < model.animations.size(); ++animIdx)
        {
            const tinygltf::Animation &anim = model.animations[animIdx];
            AnimationClip clip;
            clip.name = anim.name.empty() ? ("Animation_" + std::to_string(animIdx)) : anim.name;
            clip.samplers.reserve(anim.samplers.size());

            for (const auto &sampler : anim.samplers)
            {
                AnimationSampler samplerData;
                samplerData.interpolation = sampler.interpolation.empty() ? "LINEAR" : sampler.interpolation;

                if (sampler.input > -1)
                {
                    const tinygltf::Accessor &accessor = model.accessors[sampler.input];
                    if (accessor.bufferView > -1)
                    {
                        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
                        const unsigned char *dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        size_t stride = accessor.ByteStride(bufferView);
                        if (stride == 0)
                        {
                            stride = tinygltf::GetComponentSizeInBytes(accessor.componentType);
                        }

                        for (size_t i = 0; i < accessor.count; ++i)
                        {
                            float value = 0.0f;
                            const unsigned char *ptr = dataPtr + i * stride;
                            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                            {
                                value = reinterpret_cast<const float *>(ptr)[0];
                            }
                            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE)
                            {
                                value = static_cast<float>(reinterpret_cast<const double *>(ptr)[0]);
                            }

                            samplerData.inputs.push_back(value);
                        }
                    }
                }

                if (!samplerData.inputs.empty())
                {
                    clip.duration = std::max(clip.duration, samplerData.inputs.back());
                }

                if (sampler.output > -1)
                {
                    const tinygltf::Accessor &accessor = model.accessors[sampler.output];
                    if (accessor.bufferView > -1)
                    {
                        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
                        const unsigned char *dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        size_t stride = accessor.ByteStride(bufferView);
                        if (stride == 0)
                        {
                            stride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                     tinygltf::GetComponentSizeInBytes(accessor.componentType);
                        }

                        for (size_t i = 0; i < accessor.count; ++i)
                        {
                            const unsigned char *ptr = dataPtr + i * stride;
                            glm::vec4 value(0.0f);

                            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                            {
                                const float *components = reinterpret_cast<const float *>(ptr);
                                for (int c = 0; c < tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)); ++c)
                                {
                                    if (c < 4)
                                    {
                                        value[c] = components[c];
                                    }
                                }
                            }
                            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE)
                            {
                                const double *components = reinterpret_cast<const double *>(ptr);
                                for (int c = 0; c < tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)); ++c)
                                {
                                    if (c < 4)
                                    {
                                        value[c] = static_cast<float>(components[c]);
                                    }
                                }
                            }

                            samplerData.outputs.push_back(value);
                        }
                    }
                }

                clip.samplers.push_back(std::move(samplerData));
            }

            clip.channels.reserve(anim.channels.size());
            for (const auto &channel : anim.channels)
            {
                if (channel.target_node < 0)
                {
                    continue;
                }
                if (channel.sampler < 0 || channel.sampler >= static_cast<int>(clip.samplers.size()))
                {
                    continue;
                }

                AnimationChannel channelData;
                channelData.samplerIndex = channel.sampler;
                channelData.targetNode = channel.target_node;

                const std::string &path = channel.target_path;
                if (path == "translation")
                {
                    channelData.path = ChannelPath::Translation;
                }
                else if (path == "rotation")
                {
                    channelData.path = ChannelPath::Rotation;
                }
                else if (path == "scale")
                {
                    channelData.path = ChannelPath::Scale;
                }
                else
                {
                    continue; // Skip morph target weights for now
                }

                clip.channels.push_back(channelData);
            }

            animationClips.push_back(std::move(clip));
        }

        if (!animationClips.empty())
        {
            activeAnimation = 0;
            currentAnimationTime = 0.0f;
        }
    }

    void resetAnimationPose()
    {
        nodeTranslations = nodeDefaultTranslations;
        nodeRotations = nodeDefaultRotations;
        nodeScales = nodeDefaultScales;
    }

    void applyAnimationClip(const AnimationClip &clip, float time)
    {
        for (const auto &channel : clip.channels)
        {
            if (channel.targetNode < 0 || channel.targetNode >= static_cast<int>(nodeTranslations.size()))
            {
                continue;
            }
            if (channel.samplerIndex < 0 || channel.samplerIndex >= static_cast<int>(clip.samplers.size()))
            {
                continue;
            }

            const AnimationSampler &sampler = clip.samplers[channel.samplerIndex];
            switch (channel.path)
            {
            case ChannelPath::Translation:
                nodeTranslations[channel.targetNode] = sampleVec3(sampler, time);
                break;
            case ChannelPath::Rotation:
                nodeRotations[channel.targetNode] = sampleQuat(sampler, time);
                break;
            case ChannelPath::Scale:
                nodeScales[channel.targetNode] = sampleVec3(sampler, time);
                break;
            }
        }
    }

    glm::vec4 sampleVec4(const AnimationSampler &sampler, float time) const
    {
        if (sampler.outputs.empty())
        {
            return glm::vec4(0.0f);
        }
        if (sampler.inputs.empty() || sampler.inputs.size() == 1)
        {
            return sampler.outputs[0];
        }

        if (time <= sampler.inputs.front())
        {
            return sampler.outputs.front();
        }
        if (time >= sampler.inputs.back())
        {
            return sampler.outputs.back();
        }

        size_t upperIndex = 0;
        while (upperIndex < sampler.inputs.size() && sampler.inputs[upperIndex] < time)
        {
            ++upperIndex;
        }

        size_t lowerIndex = upperIndex > 0 ? upperIndex - 1 : 0;
        if (upperIndex >= sampler.inputs.size())
        {
            return sampler.outputs.back();
        }

        if (sampler.interpolation == "STEP")
        {
            return sampler.outputs[lowerIndex];
        }

        float t0 = sampler.inputs[lowerIndex];
        float t1 = sampler.inputs[upperIndex];
        float factor = (t1 - t0) <= std::numeric_limits<float>::epsilon() ? 0.0f : (time - t0) / (t1 - t0);
        return glm::mix(sampler.outputs[lowerIndex], sampler.outputs[upperIndex], factor);
    }

    glm::vec3 sampleVec3(const AnimationSampler &sampler, float time) const
    {
        glm::vec4 value = sampleVec4(sampler, time);
        return glm::vec3(value.x, value.y, value.z);
    }

    glm::quat sampleQuat(const AnimationSampler &sampler, float time) const
    {
        if (sampler.outputs.empty())
        {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        if (sampler.inputs.empty() || sampler.inputs.size() == 1)
        {
            glm::vec4 v = sampler.outputs[0];
            return glm::normalize(glm::quat(v.w, v.x, v.y, v.z));
        }

        if (time <= sampler.inputs.front())
        {
            glm::vec4 v = sampler.outputs.front();
            return glm::normalize(glm::quat(v.w, v.x, v.y, v.z));
        }
        if (time >= sampler.inputs.back())
        {
            glm::vec4 v = sampler.outputs.back();
            return glm::normalize(glm::quat(v.w, v.x, v.y, v.z));
        }

        size_t upperIndex = 0;
        while (upperIndex < sampler.inputs.size() && sampler.inputs[upperIndex] < time)
        {
            ++upperIndex;
        }
        size_t lowerIndex = upperIndex > 0 ? upperIndex - 1 : 0;
        if (upperIndex >= sampler.inputs.size())
        {
            glm::vec4 v = sampler.outputs.back();
            return glm::normalize(glm::quat(v.w, v.x, v.y, v.z));
        }

        if (sampler.interpolation == "STEP")
        {
            glm::vec4 v = sampler.outputs[lowerIndex];
            return glm::normalize(glm::quat(v.w, v.x, v.y, v.z));
        }

        float t0 = sampler.inputs[lowerIndex];
        float t1 = sampler.inputs[upperIndex];
        float factor = (t1 - t0) <= std::numeric_limits<float>::epsilon() ? 0.0f : (time - t0) / (t1 - t0);

        glm::vec4 v0 = sampler.outputs[lowerIndex];
        glm::vec4 v1 = sampler.outputs[upperIndex];
        glm::quat q0 = glm::normalize(glm::quat(v0.w, v0.x, v0.y, v0.z));
        glm::quat q1 = glm::normalize(glm::quat(v1.w, v1.x, v1.y, v1.z));
        return glm::normalize(glm::slerp(q0, q1, factor));
    }

    void updateNodeMatrices()
    {
        nodeLocalMatrices.resize(nodes.size(), glm::mat4(1.0f));
        nodeGlobalMatrices.resize(nodes.size(), glm::mat4(1.0f));

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), nodeTranslations[i]);
            glm::mat4 rotationMat = glm::mat4_cast(nodeRotations[i]);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), nodeScales[i]);
            nodeLocalMatrices[i] = translationMat * rotationMat * scaleMat;
        }

        std::function<void(int, const glm::mat4 &)> traverse = [&](int nodeIndex, const glm::mat4 &parentMatrix)
        {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodeLocalMatrices.size()))
            {
                return;
            }

            glm::mat4 global = parentMatrix * nodeLocalMatrices[nodeIndex];
            nodeGlobalMatrices[nodeIndex] = global;
            for (int child : nodes[nodeIndex].children)
            {
                traverse(child, global);
            }
        };

        if (sceneRootNodes.empty())
        {
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                if (nodes[i].parent == -1)
                {
                    traverse(static_cast<int>(i), glm::mat4(1.0f));
                }
            }
        }
        else
        {
            for (int root : sceneRootNodes)
            {
                traverse(root, glm::mat4(1.0f));
            }
        }
    }

    void updateSkinMatrices()
    {
        if (skins.empty())
        {
            return;
        }

        if (skinMatrices.size() != skins.size())
        {
            skinMatrices.resize(skins.size());
        }

        for (size_t skinIdx = 0; skinIdx < skins.size(); ++skinIdx)
        {
            const SkinData &skin = skins[skinIdx];
            auto &palette = skinMatrices[skinIdx];
            palette.resize(skin.joints.size(), glm::mat4(1.0f));

            for (size_t jointIdx = 0; jointIdx < skin.joints.size(); ++jointIdx)
            {
                glm::mat4 jointMatrix = glm::mat4(1.0f);
                int nodeIndex = skin.joints[jointIdx];
                if (nodeIndex >= 0 && nodeIndex < static_cast<int>(nodeGlobalMatrices.size()))
                {
                    jointMatrix = nodeGlobalMatrices[nodeIndex] * skin.inverseBindMatrices[jointIdx];
                }
                palette[jointIdx] = jointMatrix;
            }
        }
    }

    void applySkinningUniforms(Shader &shader, int skinIndex)
    {
        if (skinIndex < 0 || skinIndex >= static_cast<int>(skinMatrices.size()))
        {
            shader.setBool("useSkinning", false);
            shader.setInt("bonesCount", 0);
            return;
        }

        const auto &palette = skinMatrices[skinIndex];
        int boneCount = static_cast<int>(palette.size());
        int uploadCount = std::min(boneCount, MAX_BONES);
        shader.setBool("useSkinning", uploadCount > 0);
        shader.setInt("bonesCount", uploadCount);
        static bool paletteLogged = false;
        if (!paletteLogged)
        {
            std::cout << "[GLTF] applySkinningUniforms skinIndex=" << skinIndex << " bones=" << uploadCount << std::endl;
            paletteLogged = true;
        }

        if (uploadCount == 0)
        {
            return;
        }

        for (int i = 0; i < uploadCount; ++i)
        {
            shader.setMat4("bones[" + std::to_string(i) + "]", palette[i]);
        }
    }

    // --- OLD ASSIMP IMPLEMENTATION ---
    /*
    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

            // Filter out unwanted container/box meshes by material name
            if (mesh->mMaterialIndex < scene->mNumMaterials) {
                aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
                aiString matName;
                if (AI_SUCCESS == mat->Get(AI_MATKEY_NAME, matName)) {
                    std::string name = std::string(matName.C_Str());
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
                    if (name.find("box") != std::string::npos) {
                        std::cout << "Skipping mesh with material: " << name << std::endl;
                        continue; // don't add this mesh
                    }
                }
            }

            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }
    */

    // --- NEW TINYGLTF IMPLEMENTATION ---
    void processNode(int node_idx, const tinygltf::Model &model, const glm::mat4 &parentTransform = glm::mat4(1.0f))
    {
        const tinygltf::Node &node = model.nodes[node_idx];

        glm::mat4 nodeTransform = getNodeTransform(node);
        glm::mat4 globalTransform = parentTransform * nodeTransform;

        // Check if this node contains a mesh
        if (node.mesh > -1)
        {
            const tinygltf::Mesh &mesh = model.meshes[node.mesh];

            // A GLTF mesh is made of "primitives". Each primitive is a drawable
            // entity, which maps directly to our 'Mesh' class.
            for (size_t i = 0; i < mesh.primitives.size(); i++)
            {
                const tinygltf::Primitive &primitive = mesh.primitives[i];
                // Skip non-triangular primitives
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                {
                    continue;
                }
                meshes.push_back(processPrimitive(primitive, model, globalTransform, node.skin));
            }
        }

        // Recursively process all children
        for (size_t i = 0; i < node.children.size(); i++)
        {
            processNode(node.children[i], model, globalTransform);
        }
    }

    // --- OLD ASSIMP IMPLEMENTATION ---
    /*
    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::string materialName;
        if (material) {
            aiString matName;
            if (AI_SUCCESS == material->Get(AI_MATKEY_NAME, matName)) {
                materialName = std::string(matName.C_Str());
            }
        }
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        if (!diffuseMaps.empty()) {
            std::cout << "Loaded " << diffuseMaps.size() << " diffuse texture(s) for a mesh" << std::endl;
        } else {
            std::cout << "No diffuse textures found for a mesh" << std::endl;
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, materialName, skinIndex);
    }
    */

    // --- NEW TINYGLTF IMPLEMENTATION ---
    // Replaces Assimp's `processMesh`
    Mesh processPrimitive(const tinygltf::Primitive &primitive, const tinygltf::Model &model, const glm::mat4 &transform, int skinIndex)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;
        std::string materialName = "default";

        // --- 1. Load Indices ---
        if (primitive.indices > -1)
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

            const unsigned char *bufferData = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
            size_t count = accessor.count;

            switch (accessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                const uint8_t *p = reinterpret_cast<const uint8_t *>(bufferData);
                for (size_t i = 0; i < count; i++)
                {
                    indices.push_back(p[i]);
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                const uint16_t *p = reinterpret_cast<const uint16_t *>(bufferData);
                for (size_t i = 0; i < count; i++)
                {
                    indices.push_back(p[i]);
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            {
                const uint32_t *p = reinterpret_cast<const uint32_t *>(bufferData);
                for (size_t i = 0; i < count; i++)
                {
                    indices.push_back(p[i]);
                }
                break;
            }
            default:
                std::cerr << "Unsupported index component type: " << accessor.componentType << std::endl;
            }
        }

        // --- 2. Load Vertex Attributes ---
        const unsigned char *positionBuffer = nullptr;
        size_t positionStride = 0;
        const unsigned char *normalBuffer = nullptr;
        size_t normalStride = 0;
        const unsigned char *texCoordBuffer = nullptr;
        size_t texCoordStride = 0;
        const unsigned char *tangentBuffer = nullptr;
        size_t tangentStride = 0;
        const unsigned char *jointBuffer = nullptr;
        size_t jointStride = 0;
        int jointComponentType = 0;
        const unsigned char *weightBuffer = nullptr;
        size_t weightStride = 0;
        int weightComponentType = 0;
        bool hasJointAttribute = false;
        bool hasWeightAttribute = false;
        size_t vertexCount = 0;

        // Get Position data
        if (primitive.attributes.find("POSITION") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
            positionBuffer = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
            positionStride = accessor.ByteStride(bufferView);
            if (positionStride == 0)
            {
                positionStride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                 tinygltf::GetComponentSizeInBytes(accessor.componentType);
            }
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                std::cerr << "Unsupported POSITION component type: " << accessor.componentType << std::endl;
                positionBuffer = nullptr;
            }
            vertexCount = accessor.count;
        }

        // Get Normal data
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("NORMAL")];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
            normalBuffer = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
            normalStride = accessor.ByteStride(bufferView);
            if (normalStride == 0)
            {
                normalStride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                               tinygltf::GetComponentSizeInBytes(accessor.componentType);
            }
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                normalBuffer = nullptr;
            }
        }

        // Get Texture Coordinate data
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
            texCoordBuffer = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
            texCoordStride = accessor.ByteStride(bufferView);
            if (texCoordStride == 0)
            {
                texCoordStride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                 tinygltf::GetComponentSizeInBytes(accessor.componentType);
            }
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                texCoordBuffer = nullptr;
            }
        }

        // Get Tangent data
        if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("TANGENT")];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
            tangentBuffer = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
            tangentStride = accessor.ByteStride(bufferView);
            if (tangentStride == 0)
            {
                tangentStride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                tinygltf::GetComponentSizeInBytes(accessor.componentType);
            }
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                tangentBuffer = nullptr;
            }
        }

        // Get Joint data
        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("JOINTS_0")];
            if (accessor.bufferView > -1)
            {
                const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
                jointBuffer = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                jointStride = accessor.ByteStride(bufferView);
                if (jointStride == 0)
                {
                    jointStride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                  tinygltf::GetComponentSizeInBytes(accessor.componentType);
                }
                jointComponentType = accessor.componentType;
                hasJointAttribute = true;
            }
        }

        // Get Weight data
        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("WEIGHTS_0")];
            if (accessor.bufferView > -1)
            {
                const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
                weightBuffer = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                weightStride = accessor.ByteStride(bufferView);
                if (weightStride == 0)
                {
                    weightStride = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(accessor.type)) *
                                   tinygltf::GetComponentSizeInBytes(accessor.componentType);
                }
                weightComponentType = accessor.componentType;
                hasWeightAttribute = true;
            }
        }

        // --- 3. Assemble the Vertex data ---
        const bool applyTransform = (skinIndex < 0);
        glm::mat3 transformMat3 = glm::mat3(1.0f);
        glm::mat3 normalMatrix = glm::mat3(1.0f);
        if (applyTransform)
        {
            transformMat3 = glm::mat3(transform);
            float determinant = glm::determinant(transformMat3);
            if (std::abs(determinant) > std::numeric_limits<float>::epsilon())
            {
                normalMatrix = glm::transpose(glm::inverse(transformMat3));
            }
        }

        for (size_t i = 0; i < vertexCount; i++)
        {
            Vertex vertex;
            std::fill(std::begin(vertex.m_BoneIDs), std::end(vertex.m_BoneIDs), -1);
            std::fill(std::begin(vertex.m_Weights), std::end(vertex.m_Weights), 0.0f);

            glm::vec3 position = glm::vec3(0.0f);
            if (positionBuffer)
            {
                const float *pos = reinterpret_cast<const float *>(positionBuffer + i * positionStride);
                position = glm::vec3(pos[0], pos[1], pos[2]);
            }

            glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
            if (normalBuffer)
            {
                const float *n = reinterpret_cast<const float *>(normalBuffer + i * normalStride);
                normal = glm::vec3(n[0], n[1], n[2]);
            }

            glm::vec2 texCoord = glm::vec2(0.0f);
            if (texCoordBuffer)
            {
                const float *uv = reinterpret_cast<const float *>(texCoordBuffer + i * texCoordStride);
                texCoord = glm::vec2(uv[0], uv[1]);
            }

            glm::vec3 tangent = glm::vec3(0.0f);
            if (tangentBuffer)
            {
                const float *tan = reinterpret_cast<const float *>(tangentBuffer + i * tangentStride);
                tangent = glm::vec3(tan[0], tan[1], tan[2]);
            }

            glm::vec3 finalPosition = position;
            if (applyTransform)
            {
                glm::vec4 transformedPos = transform * glm::vec4(position, 1.0f);
                finalPosition = glm::vec3(transformedPos);
            }
            vertex.Position = finalPosition;
            updateBounds(vertex.Position);

            if (applyTransform)
            {
                glm::vec3 transformedNormal = normalMatrix * normal;
                if (glm::dot(transformedNormal, transformedNormal) > 0.0f)
                {
                    vertex.Normal = glm::normalize(transformedNormal);
                }
                else
                {
                    vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
                }
            }
            else
            {
                if (glm::dot(normal, normal) > 0.0f)
                {
                    vertex.Normal = glm::normalize(normal);
                }
                else
                {
                    vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
                }
            }
            vertex.TexCoords = texCoord;

            // GLTF Tangents are VEC4 (w component for handedness), our Vertex struct is VEC3
            if (applyTransform)
            {
                glm::vec3 transformedTangent = normalMatrix * tangent;
                if (glm::dot(transformedTangent, transformedTangent) > 0.0f)
                {
                    vertex.Tangent = glm::normalize(transformedTangent);
                }
                else
                {
                    vertex.Tangent = glm::vec3(0.0f);
                }
            }
            else
            {
                if (glm::dot(tangent, tangent) > 0.0f)
                {
                    vertex.Tangent = glm::normalize(tangent);
                }
                else
                {
                    vertex.Tangent = glm::vec3(0.0f);
                }
            }

            // GLTF doesn't store Bitangents, they are meant to be calculated.
            // We can calculate them from Position, Normal, and TexCoords if needed,
            // or just set them to zero. For now, set to zero.
            vertex.Bitangent = glm::vec3(0.0f); // Placeholder

            if (hasJointAttribute)
            {
                auto readJointVec4 = [&](size_t index) -> glm::ivec4
                {
                    glm::ivec4 values(0);
                    if (!jointBuffer)
                    {
                        return values;
                    }
                    const unsigned char *ptr = jointBuffer + index * jointStride;
                    switch (jointComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    {
                        const uint8_t *data = reinterpret_cast<const uint8_t *>(ptr);
                        values = glm::ivec4(data[0], data[1], data[2], data[3]);
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t *data = reinterpret_cast<const uint16_t *>(ptr);
                        values = glm::ivec4(data[0], data[1], data[2], data[3]);
                        break;
                    }
                    default:
                        break;
                    }
                    return values;
                };

                glm::ivec4 jointIds = readJointVec4(i);

                glm::vec4 weights(0.0f);
                if (hasWeightAttribute)
                {
                    auto readWeightVec4 = [&](size_t index) -> glm::vec4
                    {
                        glm::vec4 values(0.0f);
                        if (!weightBuffer)
                        {
                            return values;
                        }
                        const unsigned char *ptr = weightBuffer + index * weightStride;
                        switch (weightComponentType)
                        {
                        case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        {
                            const float *data = reinterpret_cast<const float *>(ptr);
                            values = glm::vec4(data[0], data[1], data[2], data[3]);
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        {
                            const uint16_t *data = reinterpret_cast<const uint16_t *>(ptr);
                            const float inv = 1.0f / 65535.0f;
                            values = glm::vec4(data[0], data[1], data[2], data[3]) * inv;
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        {
                            const uint8_t *data = reinterpret_cast<const uint8_t *>(ptr);
                            const float inv = 1.0f / 255.0f;
                            values = glm::vec4(data[0], data[1], data[2], data[3]) * inv;
                            break;
                        }
                        default:
                            break;
                        }
                        return values;
                    };

                    weights = readWeightVec4(i);
                }
                else
                {
                    weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                }

                float weightSum = weights.x + weights.y + weights.z + weights.w;
                if (weightSum > 0.0f)
                {
                    weights /= weightSum;
                }

                const int jointArray[4] = {jointIds.x, jointIds.y, jointIds.z, jointIds.w};
                const float weightArray[4] = {weights.x, weights.y, weights.z, weights.w};

                for (int influence = 0; influence < MAX_BONE_INFLUENCE; ++influence)
                {
                    if (influence < 4)
                    {
                        vertex.m_BoneIDs[influence] = jointArray[influence];
                        vertex.m_Weights[influence] = weightArray[influence];
                    }
                    else
                    {
                        vertex.m_BoneIDs[influence] = -1;
                        vertex.m_Weights[influence] = 0.0f;
                    }
                }
            }

            vertices.push_back(vertex);
        }

        // --- 4. Process Materials (Textures) ---
        if (primitive.material > -1)
        {
            const tinygltf::Material &mat = model.materials[primitive.material];
            materialName = mat.name;

            // 1. Diffuse (Base Color)
            if (mat.pbrMetallicRoughness.baseColorTexture.index > -1)
            {
                const tinygltf::Texture &tex = model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
                if (tex.source > -1)
                {
                    const tinygltf::Image &img = model.images[tex.source];
                    textures.push_back(loadTextureFromImage(img, "texture_diffuse"));
                }
            }

            // 2. Specular (Metallic-Roughness)
            // We map the PBR metallic-roughness map to "texture_specular"
            if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index > -1)
            {
                const tinygltf::Texture &tex = model.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
                if (tex.source > -1)
                {
                    const tinygltf::Image &img = model.images[tex.source];
                    textures.push_back(loadTextureFromImage(img, "texture_specular"));
                }
            }

            // 3. Normal Map
            if (mat.normalTexture.index > -1)
            {
                const tinygltf::Texture &tex = model.textures[mat.normalTexture.index];
                if (tex.source > -1)
                {
                    const tinygltf::Image &img = model.images[tex.source];
                    textures.push_back(loadTextureFromImage(img, "texture_normal"));
                }
            }

            // 4. Height (Occlusion)
            // We map the GLTF occlusion map to "texture_height"
            if (mat.occlusionTexture.index > -1)
            {
                const tinygltf::Texture &tex = model.textures[mat.occlusionTexture.index];
                if (tex.source > -1)
                {
                    const tinygltf::Image &img = model.images[tex.source];
                    textures.push_back(loadTextureFromImage(img, "texture_height"));
                }
            }
        }

        if (indices.empty())
        {
            for (size_t i = 0; i < vertexCount; ++i)
            {
                indices.push_back(static_cast<unsigned int>(i));
            }
        }

        static int debugPrimitiveCount = 0;
        if (debugPrimitiveCount < 5)
        {
            glm::vec3 minPos(FLT_MAX);
            glm::vec3 maxPos(-FLT_MAX);
            for (const auto &v : vertices)
            {
                minPos = glm::min(minPos, v.Position);
                maxPos = glm::max(maxPos, v.Position);
            }

            std::cout << std::fixed << std::setprecision(3)
                      << "[GLTF] Primitive " << debugPrimitiveCount
                      << " vertices=" << vertexCount
                      << " indices=" << indices.size()
                      << " bounds min(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")"
                      << " max(" << maxPos.x << ", " << maxPos.y << ", " << maxPos.z << ")"
                      << std::endl;
            ++debugPrimitiveCount;
        }

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, materialName, skinIndex);
    }

    void updateBounds(const glm::vec3 &position)
    {
        boundingMin = glm::min(boundingMin, position);
        boundingMax = glm::max(boundingMax, position);
    }

    glm::mat4 getNodeTransform(const tinygltf::Node &node)
    {
        if (node.matrix.size() == 16)
        {
            glm::mat4 m(1.0f);
            for (int col = 0; col < 4; ++col)
            {
                for (int row = 0; row < 4; ++row)
                {
                    m[col][row] = static_cast<float>(node.matrix[col * 4 + row]);
                }
            }

            return m;
        }

        glm::vec3 translation(0.0f);
        if (node.translation.size() == 3)
        {
            translation = glm::vec3(static_cast<float>(node.translation[0]),
                                    static_cast<float>(node.translation[1]),
                                    static_cast<float>(node.translation[2]));
        }

        glm::vec3 scale(1.0f);
        if (node.scale.size() == 3)
        {
            scale = glm::vec3(static_cast<float>(node.scale[0]),
                              static_cast<float>(node.scale[1]),
                              static_cast<float>(node.scale[2]));
        }

        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (node.rotation.size() == 4)
        {
            rotation = glm::quat(static_cast<float>(node.rotation[3]),
                                 static_cast<float>(node.rotation[0]),
                                 static_cast<float>(node.rotation[1]),
                                 static_cast<float>(node.rotation[2]));
        }

        glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 rotationMat = glm::mat4_cast(rotation);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

        return translationMat * rotationMat * scaleMat;
    }

    // --- OLD ASSIMP IMPLEMENTATION ---
    /*
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            { // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                std::cout << "Request to load " << typeName << ": '" << str.C_Str() << "' from directory '" << this->directory << "'" << std::endl;
                textures.push_back(texture);
                textures_loaded.push_back(texture); // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }
    */

    // --- NEW TINYGLTF HELPER ---
    // This is a new helper function that mimics the caching behavior of the original
    // loadMaterialTextures, but is simpler and designed to be called from processPrimitive.
    Texture loadTextureFromImage(const tinygltf::Image &image, const string &typeName)
    {
        const bool isDataUri = (!image.uri.empty() && image.uri.rfind("data:", 0) == 0);

        std::string cacheKey;
        if (!image.uri.empty() && !isDataUri)
        {
            cacheKey = image.uri;
        }
        else if (!image.name.empty())
        {
            cacheKey = "embedded:" + image.name;
        }
        else
        {
            std::ostringstream oss;
            oss << "embedded_ptr_" << static_cast<const void *>(&image);
            cacheKey = oss.str();
        }

        // Re-use previously loaded textures when possible
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (textures_loaded[j].path == cacheKey)
            {
                return textures_loaded[j];
            }
        }

        Texture texture;
        texture.type = typeName;
        texture.path = cacheKey;

        // If the image references an external URI, fall back to the existing file loader.
        if (!image.uri.empty() && !isDataUri)
        {
            texture.id = TextureFromFile(image.uri.c_str(), this->directory, gammaCorrection);
            textures_loaded.push_back(texture);
            return texture;
        }

        if (image.image.empty())
        {
            std::cout << "GLTF image has no pixel data for type: " << typeName << std::endl;
            texture.id = 0;
            textures_loaded.push_back(texture);
            return texture;
        }

        const int width = image.width;
        const int height = image.height;
        const int components = image.component;
        if (width <= 0 || height <= 0 || components <= 0)
        {
            std::cout << "Invalid image dimensions for embedded texture: " << cacheKey << std::endl;
            texture.id = 0;
            textures_loaded.push_back(texture);
            return texture;
        }

        GLenum format = GL_RGB;
        switch (components)
        {
        case 1:
            format = GL_RED;
            break;
        case 2:
            format = GL_RG;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
        default:
            format = GL_RGBA;
            break;
        }

        GLenum internalFormat = format;
        if (components == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
        }
        else if (components == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        }

        auto resolvePixelType = [](int tinyType, int bits)
        {
            switch (tinyType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                return GL_UNSIGNED_BYTE;
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                return GL_BYTE;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                return GL_UNSIGNED_SHORT;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                return GL_SHORT;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                return GL_UNSIGNED_INT;
            case TINYGLTF_COMPONENT_TYPE_INT:
                return GL_INT;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                return GL_FLOAT;
            default:
                break;
            }

            // Fallback based on bit depth if pixel_type is not recognized.
            if (bits == 16)
            {
                return GL_UNSIGNED_SHORT;
            }
            return GL_UNSIGNED_BYTE;
        };

        GLenum pixelType = resolvePixelType(image.pixel_type, image.bits);

        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        GLint previousUnpackAlignment = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, pixelType, image.image.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

        textures_loaded.push_back(texture);
        return texture;
    }
};

inline void Model::SetActiveAnimation(int animationIndex)
{
    if (animationClips.empty())
    {
        activeAnimation = -1;
        currentAnimationTime = 0.0f;
        return;
    }

    if (animationIndex < 0 || animationIndex >= static_cast<int>(animationClips.size()))
    {
        animationIndex = 0;
    }

    activeAnimation = animationIndex;
    const AnimationClip &clip = animationClips[activeAnimation];
    auto window = getPlaybackWindow(clip);
    currentAnimationTime = window.first;
    resetAnimationPose();
    applyAnimationClip(clip, currentAnimationTime);
    updateNodeMatrices();
    updateSkinMatrices();
}

inline std::pair<float, float> Model::getPlaybackWindow(const AnimationClip &clip) const
{
    float clipDuration = std::max(clip.duration, 0.0f);
    if (!animationWindowEnabled || clipDuration <= 0.0f)
    {
        return {0.0f, clipDuration};
    }

    float start = std::clamp(animationWindowStart, 0.0f, clipDuration);
    float endCandidate = animationWindowEnd > 0.0f ? animationWindowEnd : clipDuration;
    float end = std::clamp(endCandidate, start, clipDuration);
    return {start, end};
}

inline void Model::SetAnimationPlaybackWindow(float startTimeSeconds, float endTimeSeconds)
{
    if (endTimeSeconds <= startTimeSeconds)
    {
        ClearAnimationPlaybackWindow();
        return;
    }

    animationWindowEnabled = true;
    animationWindowStart = std::max(0.0f, startTimeSeconds);
    animationWindowEnd = endTimeSeconds;

    if (activeAnimation >= 0 && activeAnimation < static_cast<int>(animationClips.size()))
    {
        const AnimationClip &clip = animationClips[activeAnimation];
        auto window = getPlaybackWindow(clip);
        currentAnimationTime = window.first;
        resetAnimationPose();
        applyAnimationClip(clip, currentAnimationTime);
        updateNodeMatrices();
        updateSkinMatrices();
    }
}

inline void Model::ClearAnimationPlaybackWindow()
{
    animationWindowEnabled = false;
    animationWindowStart = 0.0f;
    animationWindowEnd = 0.0f;

    if (activeAnimation >= 0 && activeAnimation < static_cast<int>(animationClips.size()))
    {
        const AnimationClip &clip = animationClips[activeAnimation];
        currentAnimationTime = 0.0f;
        resetAnimationPose();
        applyAnimationClip(clip, currentAnimationTime);
        updateNodeMatrices();
        updateSkinMatrices();
    }
}

inline float Model::GetAnimationClipDuration(int animationIndex) const
{
    if (animationIndex < 0 || animationIndex >= static_cast<int>(animationClips.size()))
    {
        return 0.0f;
    }
    return animationClips[animationIndex].duration;
}

inline float Model::GetActiveAnimationDuration() const
{
    return GetAnimationClipDuration(activeAnimation);
}

inline void Model::StartAnimationBlend(int targetAnimationIndex, float durationSeconds, bool targetWindowEnabled,
                                       float targetWindowStartSeconds, float targetWindowEndSeconds)
{
    if (targetAnimationIndex < 0 || targetAnimationIndex >= static_cast<int>(animationClips.size()) ||
        activeAnimation < 0 || activeAnimation >= static_cast<int>(animationClips.size()))
    {
        animationBlendActive = false;
        return;
    }

    const AnimationClip &targetClip = animationClips[targetAnimationIndex];
    float clipDuration = std::max(targetClip.duration, 0.0f);

    auto clampWindow = [clipDuration](float start, float end)
    {
        float clampedStart = std::clamp(start, 0.0f, clipDuration);
        float minEnd = clampedStart + 0.0001f;
        float clampedEnd = std::clamp(end, minEnd, clipDuration > 0.0f ? clipDuration : minEnd);
        if (clipDuration <= 0.0f)
        {
            clampedEnd = clampedStart;
        }
        return std::pair<float, float>{clampedStart, clampedEnd};
    };

    std::pair<float, float> targetWindow{0.0f, clipDuration};
    if (targetWindowEnabled)
    {
        targetWindow = clampWindow(targetWindowStartSeconds, targetWindowEndSeconds);
    }

    if (durationSeconds <= 0.0f)
    {
        animationBlendActive = false;
        SetActiveAnimation(targetAnimationIndex);
        if (targetWindowEnabled)
        {
            SetAnimationPlaybackWindow(targetWindow.first, targetWindow.second);
        }
        else
        {
            ClearAnimationPlaybackWindow();
        }
        return;
    }

    blendAnimationIndex = targetAnimationIndex;
    blendDuration = durationSeconds;
    blendElapsed = 0.0f;
    blendWindowEnabled = targetWindowEnabled;
    blendWindowStart = targetWindow.first;
    blendWindowEnd = targetWindow.second;
    blendAnimationTime = blendWindowStart;
    animationBlendActive = true;
    ensureBlendPoseBuffers();
}

inline void Model::ensureBlendPoseBuffers()
{
    size_t count = nodeTranslations.size();
    if (blendBaseTranslations.size() != count)
    {
        blendBaseTranslations.resize(count, glm::vec3(0.0f));
        blendTargetTranslations.resize(count, glm::vec3(0.0f));
        blendBaseRotations.resize(count, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        blendTargetRotations.resize(count, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        blendBaseScales.resize(count, glm::vec3(1.0f));
        blendTargetScales.resize(count, glm::vec3(1.0f));
    }
}

inline float Model::advanceAnimationTime(float currentTime, float deltaTime, float windowStart, float windowEnd) const
{
    float windowLength = std::max(windowEnd - windowStart, 0.0f);
    if (windowLength <= 0.0f)
    {
        return windowStart;
    }

    float relativeTime = (currentTime - windowStart) + deltaTime;
    relativeTime = std::fmod(relativeTime, windowLength);
    if (relativeTime < 0.0f)
    {
        relativeTime += windowLength;
    }
    return windowStart + relativeTime;
}

inline void Model::UpdateAnimation(float deltaTime)
{
    if (animationClips.empty())
    {
        return;
    }

    if (activeAnimation < 0 || activeAnimation >= static_cast<int>(animationClips.size()))
    {
        activeAnimation = 0;
    }

    AnimationClip &clip = animationClips[activeAnimation];
    auto window = getPlaybackWindow(clip);
    currentAnimationTime = advanceAnimationTime(currentAnimationTime, deltaTime, window.first, window.second);

    bool blending = animationBlendActive && blendAnimationIndex >= 0 &&
                    blendAnimationIndex < static_cast<int>(animationClips.size());

    if (blending)
    {
        ensureBlendPoseBuffers();

        AnimationClip &targetClip = animationClips[blendAnimationIndex];
        float targetWindowStart = blendWindowEnabled ? blendWindowStart : 0.0f;
        float targetWindowEnd = blendWindowEnabled ? blendWindowEnd : targetClip.duration;
        if (targetWindowEnd <= targetWindowStart)
        {
            targetWindowEnd = std::max(targetWindowStart + 0.0001f, targetWindowStart + targetClip.duration);
        }

        blendAnimationTime = advanceAnimationTime(blendAnimationTime, deltaTime, targetWindowStart, targetWindowEnd);
        blendElapsed = std::min(blendElapsed + deltaTime, blendDuration);
        float blendFactor = blendDuration <= 0.0f ? 1.0f : std::clamp(blendElapsed / blendDuration, 0.0f, 1.0f);

        resetAnimationPose();
        applyAnimationClip(clip, currentAnimationTime);
        for (size_t i = 0; i < nodeTranslations.size(); ++i)
        {
            blendBaseTranslations[i] = nodeTranslations[i];
            blendBaseRotations[i] = nodeRotations[i];
            blendBaseScales[i] = nodeScales[i];
        }

        resetAnimationPose();
        applyAnimationClip(targetClip, blendAnimationTime);
        for (size_t i = 0; i < nodeTranslations.size(); ++i)
        {
            blendTargetTranslations[i] = nodeTranslations[i];
            blendTargetRotations[i] = nodeRotations[i];
            blendTargetScales[i] = nodeScales[i];
        }

        for (size_t i = 0; i < nodeTranslations.size(); ++i)
        {
            nodeTranslations[i] = glm::mix(blendBaseTranslations[i], blendTargetTranslations[i], blendFactor);
            nodeRotations[i] = glm::normalize(glm::slerp(blendBaseRotations[i], blendTargetRotations[i], blendFactor));
            nodeScales[i] = glm::mix(blendBaseScales[i], blendTargetScales[i], blendFactor);
        }

        if (blendFactor >= 0.999f)
        {
            animationBlendActive = false;
            activeAnimation = blendAnimationIndex;
            currentAnimationTime = blendAnimationTime;
            animationWindowEnabled = blendWindowEnabled;
            animationWindowStart = targetWindowStart;
            animationWindowEnd = targetWindowEnd;
        }
    }
    else
    {
        animationBlendActive = false;
        resetAnimationPose();
        applyAnimationClip(clip, currentAnimationTime);
    }

    updateNodeMatrices();
    updateSkinMatrices();

    static bool logged = false;
    if (!logged && !skins.empty())
    {
        const auto &joints = skins[0].joints;
        if (!joints.empty())
        {
            int nodeIndex = joints[0];
            glm::vec3 t = nodeTranslations[nodeIndex];
            std::cout << "[GLTF] UpdateAnimation node " << nodeIndex << " translation " << t.x << ", " << t.y << ", " << t.z << std::endl;
        }
        if (joints.size() > 1)
        {
            int nodeIndex = joints[1];
            glm::vec3 t = nodeTranslations[nodeIndex];
            std::cout << "[GLTF] UpdateAnimation node " << nodeIndex << " translation " << t.x << ", " << t.y << ", " << t.z << std::endl;
        }
        logged = true;
    }
}

inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    if (filename.empty())
    {
        std::cout << "TextureFromFile: Empty path given." << std::endl;
        return 0; // Return 0 to indicate failure
    }

    filename.erase(std::remove_if(filename.begin(), filename.end(), [](char c)
                                  { return c == '\r' || c == '\n'; }),
                   filename.end());
    std::replace(filename.begin(), filename.end(), '\\', '/');

    // GLTF paths are often relative to the model file.
    // Assimp's 'directory' logic is perfect here.
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        // std::cout << "Loaded texture OK: " << filename << std::endl;
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gamma ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path
                  << " (resolved: " << filename << ") reason: "
                  << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
        glDeleteTextures(1, &textureID); // Clean up failed texture
        return 0;                        // Return 0 to indicate failure
    }

    return textureID;
}
#endif
