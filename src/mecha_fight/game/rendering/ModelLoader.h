#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <learnopengl/model.h>

namespace mecha
{

  /**
   * @brief Factory for loading and caching 3D models
   *
   * Handles GLTF/GLB model loading, bounding box calculation, and animation setup.
   * Provides caching to avoid redundant loads.
   */
  class ModelLoader
  {
  public:
    struct ModelInfo
    {
      std::unique_ptr<Model> model;
      glm::vec3 dimensions;
      glm::vec3 center;
      glm::vec3 boundingMin;
      glm::vec3 boundingMax;
    };

    /**
     * @brief Load or retrieve cached model
     * @param name Unique identifier for the model
     * @param path Path to model file (GLTF/GLB)
     * @return Pointer to loaded model, nullptr on failure
     */
    Model *LoadModel(const std::string &name, const std::string &path);

    /**
     * @brief Get previously loaded model
     * @param name Model identifier
     * @return Pointer to model, nullptr if not found
     */
    Model *GetModel(const std::string &name);

    /**
     * @brief Get model information including bounding box
     * @param name Model identifier
     * @return Pointer to model info, nullptr if not found
     */
    const ModelInfo *GetModelInfo(const std::string &name) const;

    /**
     * @brief Check if model is cached
     */
    bool HasModel(const std::string &name) const;

    /**
     * @brief Remove model from cache
     */
    void UnloadModel(const std::string &name);

    /**
     * @brief Clear all cached models
     */
    void Clear();

    /**
     * @brief Get number of loaded models
     */
    size_t GetLoadedCount() const { return m_models.size(); }

  private:
    std::unordered_map<std::string, ModelInfo> m_models;

    void CalculateModelInfo(ModelInfo &info);
  };

} // namespace mecha
