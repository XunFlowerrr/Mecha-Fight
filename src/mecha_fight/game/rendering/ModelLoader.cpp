#include "ModelLoader.h"
#include <iostream>
#include <iomanip>

namespace mecha
{

  Model *ModelLoader::LoadModel(const std::string &name, const std::string &path)
  {
    // Check if already loaded
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
      std::cout << "[ModelLoader] Model '" << name << "' already loaded, returning cached version" << std::endl;
      return it->second.model.get();
    }

    // Load new model
    try
    {
      std::cout << "[ModelLoader] Loading model '" << name << "' from: " << path << std::endl;

      ModelInfo info;
      info.model = std::make_unique<Model>(path);
      CalculateModelInfo(info);

      // Activate default animation if available
      if (info.model->HasAnimations())
      {
        info.model->SetActiveAnimation(0);
        std::cout << "[ModelLoader] Activated default animation for '" << name << "'" << std::endl;
      }

      Model *ptr = info.model.get();
      m_models[name] = std::move(info);

      std::cout << "[ModelLoader] Successfully loaded model '" << name << "'" << std::endl;
      return ptr;
    }
    catch (const std::exception &e)
    {
      std::cout << "[ModelLoader] Failed to load model '" << name << "': " << e.what() << std::endl;
      return nullptr;
    }
  }

  Model *ModelLoader::GetModel(const std::string &name)
  {
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
      return it->second.model.get();
    }

    std::cout << "[ModelLoader] Model '" << name << "' not found" << std::endl;
    return nullptr;
  }

  const ModelLoader::ModelInfo *ModelLoader::GetModelInfo(const std::string &name) const
  {
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
      return &it->second;
    }
    return nullptr;
  }

  bool ModelLoader::HasModel(const std::string &name) const
  {
    return m_models.find(name) != m_models.end();
  }

  void ModelLoader::UnloadModel(const std::string &name)
  {
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
      m_models.erase(it);
      std::cout << "[ModelLoader] Unloaded model '" << name << "'" << std::endl;
    }
  }

  void ModelLoader::Clear()
  {
    m_models.clear();
    std::cout << "[ModelLoader] Cleared all models" << std::endl;
  }

  void ModelLoader::CalculateModelInfo(ModelInfo &info)
  {
    info.boundingMin = info.model->GetBoundingMin();
    info.boundingMax = info.model->GetBoundingMax();
    info.dimensions = info.model->GetDimensions();
    info.center = (info.boundingMin + info.boundingMax) * 0.5f;

    std::cout << std::fixed << std::setprecision(3)
              << "[ModelLoader] Bounding box: min(" << info.boundingMin.x << ", " << info.boundingMin.y << ", " << info.boundingMin.z
              << ") max(" << info.boundingMax.x << ", " << info.boundingMax.y << ", " << info.boundingMax.z << ")\n"
              << "              Dimensions(" << info.dimensions.x << ", " << info.dimensions.y << ", " << info.dimensions.z
              << ") Center(" << info.center.x << ", " << info.center.y << ", " << info.center.z << ")"
              << std::endl;
  }

} // namespace mecha
