#include "SceneRenderer.h"
#include "RenderConstants.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>

namespace mecha
{

  SceneRenderer::SceneRenderer()
  {
  }

  SceneRenderer::~SceneRenderer()
  {
    DestroySkyboxGeometry();
  }

  bool SceneRenderer::Initialize(const RenderConfig &config)
  {
    m_config = config;

    if (m_config.enableSSAO)
    {
      SSAORenderer::Config ssaoConfig;
      ssaoConfig.width = config.screenWidth;
      ssaoConfig.height = config.screenHeight;
      ssaoConfig.kernelSize = kSSAOKernelSize;
      m_ssaoInitialized = m_ssaoRenderer.Init(ssaoConfig);
      if (!m_ssaoInitialized)
      {
        std::cerr << "[SceneRenderer] Failed to initialize SSAO renderer" << std::endl;
      }
    }

    CreateSkyboxGeometry();

    std::cout << "[SceneRenderer] Initialized with resolution " << config.screenWidth << "x" << config.screenHeight << std::endl;
    return true;
  }

  void SceneRenderer::SetDependencies(ResourceManager *resourceMgr, ShadowMapper *shadowMapper, GameWorld *world)
  {
    m_resourceMgr = resourceMgr;
    m_shadowMapper = shadowMapper;
    m_world = world;
  }

  void SceneRenderer::RenderFrame(const FrameData &frameData)
  {
    if (ShouldUseSSAO())
    {
      RenderSSAOGeometry(frameData);
      EvaluateSSAO(frameData);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_config.screenWidth, m_config.screenHeight);
    glEnable(GL_DEPTH_TEST);
    glClearColor(m_config.clearColor.r, m_config.clearColor.g, m_config.clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Shadow pass
    RenderShadowPass(frameData);

    // 2. Main scene
    RenderMainScene(frameData);

    // 3. World entities
    RenderEntities(frameData);
  }

  void SceneRenderer::RenderShadowPass(const FrameData &frameData)
  {
    if (!m_shadowMapper || !m_resourceMgr)
      return;

    glm::mat4 lightSpaceMatrix = m_shadowMapper->GetLightSpaceMatrix();

    Shader *shadowShader = m_resourceMgr->Shaders().GetShader("shadow");
    if (!shadowShader)
      return;

    shadowShader->use();
    shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    m_shadowMapper->BeginShadowPass();

    // Render terrain to depth map
    if (frameData.terrainConfig && frameData.terrainConfig->terrainModel)
    {
      glm::mat4 terrainModel = glm::mat4(1.0f);
      terrainModel = glm::translate(terrainModel, frameData.terrainConfig->modelTranslation);
      terrainModel = glm::scale(terrainModel, frameData.terrainConfig->modelScale);
      shadowShader->setMat4("model", terrainModel);
      shadowShader->setBool("useSkinning", false);
      shadowShader->setInt("bonesCount", 0);
      frameData.terrainConfig->terrainModel->Draw(*shadowShader);
    }

    if (m_world)
    {
      RenderContext shadowCtx{};
      shadowCtx.deltaTime = frameData.deltaTime;
      shadowCtx.lightSpaceMatrix = lightSpaceMatrix;
      shadowCtx.shadowPass = true;
      shadowCtx.overrideShader = shadowShader;
      m_world->Render(shadowCtx);
    }

    m_shadowMapper->EndShadowPass();
  }

  void SceneRenderer::RenderMainScene(const FrameData &frameData)
  {
    if (!m_shadowMapper || !m_resourceMgr)
      return;

    glViewport(0, 0, m_config.screenWidth, m_config.screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 lightSpaceMatrix = m_shadowMapper->GetLightSpaceMatrix();

    RenderSkybox(frameData);
    RenderTerrain(frameData, lightSpaceMatrix);
  }

  void SceneRenderer::RenderTerrain(const FrameData &frameData, const glm::mat4 &lightSpaceMatrix)
  {
    if (!frameData.terrainConfig)
      return;

    Shader *terrainShader = m_resourceMgr->Shaders().GetShader("terrain");
    if (!terrainShader)
      return;

    const TerrainConfig *terrainConfig = frameData.terrainConfig;
    if (!terrainConfig || !terrainConfig->terrainModel)
      return;

    terrainShader->use();
    terrainShader->setMat4("projection", frameData.projection);
    terrainShader->setMat4("view", frameData.view);
    terrainShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    terrainShader->setVec3("viewPos", frameData.viewPos);
    terrainShader->setVec3("lightPos", m_shadowMapper->GetLightPosition());
    terrainShader->setVec3("lightIntensity", m_config.lightIntensity);

    bool hasAlbedoTexture = false;
    for (const auto &mesh : terrainConfig->terrainModel->meshes)
    {
      if (!mesh.textures.empty())
      {
        hasAlbedoTexture = true;
        break;
      }
    }
    terrainShader->setBool("useAlbedoTexture", hasAlbedoTexture);
    terrainShader->setVec3("fallbackColor", glm::vec3(0.35f, 0.45f, 0.35f));

    glActiveTexture(GL_TEXTURE0 + kShadowMapTextureUnit);
    glBindTexture(GL_TEXTURE_2D, m_shadowMapper->GetDepthMapTexture());
    terrainShader->setInt("shadowMap", kShadowMapTextureUnit);

    terrainShader->setVec2("screenSize", glm::vec2(static_cast<float>(m_config.screenWidth), static_cast<float>(m_config.screenHeight)));
    bool useSSAO = ShouldUseSSAO();
    terrainShader->setBool("useSSAO", useSSAO);
    terrainShader->setFloat("aoStrength", m_config.ssaoStrength);
    if (useSSAO)
    {
      glActiveTexture(GL_TEXTURE0 + kSSAOTexUnit);
      glBindTexture(GL_TEXTURE_2D, m_ssaoRenderer.GetSSAOBlurTexture());
      terrainShader->setInt("ssaoMap", kSSAOTexUnit);
    }

    glm::mat4 terrainModel = glm::mat4(1.0f);
    terrainModel = glm::translate(terrainModel, terrainConfig->modelTranslation);
    terrainModel = glm::scale(terrainModel, terrainConfig->modelScale);
    terrainShader->setMat4("model", terrainModel);
    terrainConfig->terrainModel->Draw(*terrainShader);
  }

  void SceneRenderer::RenderSkybox(const FrameData &frameData)
  {
    if (!m_config.enableSkybox || !m_resourceMgr || m_skyboxVAO == 0)
    {
      return;
    }

    unsigned int cubemapTex = m_resourceMgr->GetSkyboxCubemap();
    if (cubemapTex == 0)
    {
      return;
    }

    Shader *skyboxShader = m_resourceMgr->Shaders().GetShader("skybox");
    if (!skyboxShader)
    {
      return;
    }

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(frameData.view));

    skyboxShader->use();
    skyboxShader->setMat4("projection", frameData.projection);
    skyboxShader->setMat4("view", viewNoTranslation);
    skyboxShader->setVec3("tint", m_config.skyboxTint);
    skyboxShader->setFloat("intensity", m_config.skyboxIntensity);
    skyboxShader->setInt("skybox", 0);

    glBindVertexArray(m_skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
  }

  void SceneRenderer::RenderEntities(const FrameData &frameData)
  {
    if (!m_world || !m_shadowMapper)
      return;

    RenderContext renderCtx{};
    renderCtx.deltaTime = frameData.deltaTime;
    renderCtx.projection = frameData.projection;
    renderCtx.view = frameData.view;
    renderCtx.viewPos = frameData.viewPos;
    renderCtx.lightSpaceMatrix = m_shadowMapper->GetLightSpaceMatrix();
    renderCtx.lightPos = m_shadowMapper->GetLightPosition();
    renderCtx.lightIntensity = m_config.lightIntensity;
    renderCtx.shadowMapTexture = m_shadowMapper->GetDepthMapTexture();
    renderCtx.screenSize = glm::vec2(static_cast<float>(m_config.screenWidth), static_cast<float>(m_config.screenHeight));
    renderCtx.ssaoEnabled = ShouldUseSSAO();
    renderCtx.ssaoStrength = m_config.ssaoStrength;
    renderCtx.ssaoTexture = renderCtx.ssaoEnabled ? m_ssaoRenderer.GetSSAOBlurTexture() : 0;
    m_world->Render(renderCtx);

    RenderLightDebug(frameData);
  }

  void SceneRenderer::RenderLightDebug(const FrameData &frameData)
  {
    if (!m_config.showLightDebug || !m_resourceMgr || !m_shadowMapper)
    {
      return;
    }

    Shader *colorShader = m_resourceMgr->Shaders().GetShader("color");
    const MeshHandle *sphere = m_resourceMgr->Meshes().GetSphere("enemy_sphere");
    if (!colorShader || !sphere)
    {
      return;
    }

    glm::vec3 lightPos = m_shadowMapper->GetLightPosition();

    colorShader->use();
    colorShader->setMat4("projection", frameData.projection);
    colorShader->setMat4("view", frameData.view);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(m_config.lightMarkerScale));
    colorShader->setMat4("model", model);
    colorShader->setVec4("color", glm::vec4(1.0f, 0.9f, 0.3f, 1.0f));

    glBindVertexArray(sphere->vao);
    glDrawElements(GL_TRIANGLES, sphere->indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }

  void SceneRenderer::RenderSSAOGeometry(const FrameData &frameData)
  {
    if (!ShouldUseSSAO() || !m_resourceMgr)
    {
      return;
    }

    Shader *ssaoInputShader = m_resourceMgr->Shaders().GetShader("ssao_input");
    if (!ssaoInputShader)
    {
      return;
    }

    glViewport(0, 0, m_config.screenWidth, m_config.screenHeight);
    glEnable(GL_DEPTH_TEST);
    m_ssaoRenderer.BeginGeometryPass();

    ssaoInputShader->use();
    ssaoInputShader->setMat4("projection", frameData.projection);
    ssaoInputShader->setMat4("view", frameData.view);

    if (frameData.terrainConfig && frameData.terrainConfig->terrainModel)
    {
      glm::mat4 terrainModel = glm::mat4(1.0f);
      terrainModel = glm::translate(terrainModel, frameData.terrainConfig->modelTranslation);
      terrainModel = glm::scale(terrainModel, frameData.terrainConfig->modelScale);
      ssaoInputShader->setMat4("model", terrainModel);
      ssaoInputShader->setBool("useSkinning", false);
      ssaoInputShader->setInt("bonesCount", 0);
      frameData.terrainConfig->terrainModel->Draw(*ssaoInputShader);
    }

    if (m_world)
    {
      RenderContext ctx{};
      ctx.deltaTime = frameData.deltaTime;
      ctx.projection = frameData.projection;
      ctx.view = frameData.view;
      ctx.shadowPass = true;
      ctx.overrideShader = ssaoInputShader;
      m_world->Render(ctx);
    }

    m_ssaoRenderer.EndGeometryPass();
  }

  void SceneRenderer::EvaluateSSAO(const FrameData &frameData)
  {
    if (!ShouldUseSSAO() || !m_resourceMgr)
    {
      return;
    }

    Shader *ssaoShader = m_resourceMgr->Shaders().GetShader("ssao");
    Shader *ssaoBlurShader = m_resourceMgr->Shaders().GetShader("ssao_blur");
    if (!ssaoShader || !ssaoBlurShader)
    {
      return;
    }

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, m_config.screenWidth, m_config.screenHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoRenderer.GetSSAOFBO());
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader->use();
    ssaoShader->setInt("gPosition", 0);
    ssaoShader->setInt("gNormal", 1);
    ssaoShader->setInt("texNoise", 2);
    ssaoShader->setMat4("projection", frameData.projection);
    ssaoShader->setFloat("radius", m_config.ssaoRadius);
    ssaoShader->setFloat("bias", m_config.ssaoBias);
    ssaoShader->setFloat("power", m_config.ssaoPower);
    const auto &kernel = m_ssaoRenderer.GetKernel();
    for (size_t i = 0; i < kernel.size(); ++i)
    {
      ssaoShader->setVec3("samples[" + std::to_string(i) + "]", kernel[i]);
    }
    float noiseScaleX = static_cast<float>(m_config.screenWidth) / static_cast<float>(kSSAONoiseDimension);
    float noiseScaleY = static_cast<float>(m_config.screenHeight) / static_cast<float>(kSSAONoiseDimension);
    ssaoShader->setVec2("noiseScale", glm::vec2(noiseScaleX, noiseScaleY));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ssaoRenderer.GetPositionTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_ssaoRenderer.GetNormalTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_ssaoRenderer.GetNoiseTexture());

    RenderFullscreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoRenderer.GetSSAOBlurFBO());
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoBlurShader->use();
    ssaoBlurShader->setInt("ssaoInput", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ssaoRenderer.GetSSAORawTexture());
    RenderFullscreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
  }

  void SceneRenderer::RenderFullscreenQuad()
  {
    unsigned int quadVAO = m_ssaoRenderer.GetQuadVAO();
    if (quadVAO == 0)
    {
      return;
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  }

  bool SceneRenderer::ShouldUseSSAO() const
  {
    return m_config.enableSSAO && m_ssaoInitialized;
  }

  void SceneRenderer::CreateSkyboxGeometry()
  {
    if (m_skyboxVAO != 0)
    {
      return;
    }

    const float skyboxVertices[] = {
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};

    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);
    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindVertexArray(0);
  }

  void SceneRenderer::DestroySkyboxGeometry()
  {
    if (m_skyboxVAO != 0)
    {
      glDeleteVertexArrays(1, &m_skyboxVAO);
      m_skyboxVAO = 0;
    }
    if (m_skyboxVBO != 0)
    {
      glDeleteBuffers(1, &m_skyboxVBO);
      m_skyboxVBO = 0;
    }
  }

} // namespace mecha
