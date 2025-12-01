# Enemy Model Swap Guide

This guide outlines the steps to replace the current procedural sphere placeholder for the enemy with a proper 3D model (e.g., GLTF format).

## Current Implementation

- The enemy is rendered as a procedural UV sphere created in `EnemyPlaceholder.cpp`.
- The sphere mesh is generated via `CreateEnemyPlaceholderSphere()` and used in `game.cpp` for both enemy and projectiles.
- `EnemyDrone` class handles enemy logic but delegates rendering to the shared sphere VAO.

## Steps to Swap to a Model

### 1. Prepare the Model File

- Obtain or create a GLTF model file for the enemy (e.g., `resources/objects/enemy/enemy.gltf`).
- Ensure the model has appropriate scale, animations (if needed), and materials.
- Place the model in `resources/objects/` directory, similar to the mecha model.

### 2. Update EnemyDrone Class

- Add a `Model` member to `EnemyDrone.h`:

  ```cpp
  #include <learnopengl/model.h>
  // ...
  private:
      Model m_enemyModel;
  ```

- In `EnemyDrone.cpp` constructor or initialization, load the model:

  ```cpp
  std::string modelPath = FileSystem::getPath("resources/objects/enemy/enemy.gltf");
  m_enemyModel = Model(modelPath);
  ```

- Modify `EnemyDrone::Render()` to draw the model instead of using the sphere:
  - Remove sphere rendering code.
  - Add model rendering similar to mecha:

    ```cpp
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, m_position);
    // Add rotations/scaling as needed
    shader.setMat4("model", modelMatrix);
    m_enemyModel.Draw(shader);
    ```

- Ensure the shader used supports the model's materials (e.g., use `carShader` or a dedicated enemy shader).

### 3. Handle Model Scaling and Pivoting

- Compute bounding box and scale similar to mecha:

  ```cpp
  glm::vec3 minBounds = m_enemyModel.GetBoundingMin();
  glm::vec3 maxBounds = m_enemyModel.GetBoundingMax();
  glm::vec3 dimensions = m_enemyModel.GetDimensions();
  glm::vec3 center = (minBounds + maxBounds) * 0.5f;
  float scale = desiredHeight / dimensions.y; // e.g., 2.0f for enemy height
  m_pivotOffset = center;
  m_modelScale = scale;
  ```

- Apply scale and pivot in render matrix.

### 4. Update CMakeLists.txt (if needed)

- If adding new shaders or files, include them in `MECHA_FIGHT_SOURCES`.
- Ensure `TINYGLTF` library is linked (already is for mecha).

### 5. Remove Placeholder Usage for Enemy

- In `game.cpp`, the sphere is still used for projectiles, so keep `EnemyPlaceholder` for now.
- Call `EnemyDrone::SetRenderResources` during initialization to provide the shader and loaded model.
- Modify `EnemyDrone::Render` to accept a shader and use the model.

### 6. Testing

- Build and run the game.
- Verify enemy renders correctly, collides properly, and animations play if applicable.
- Check performance; models may require more resources than spheres.

### 7. Optional Enhancements

- Add enemy-specific shaders if needed.
- Implement LOD or simpler model for distant enemies.
- Update collision detection if model has complex geometry (currently sphere-based).

## Notes

- The procedural sphere remains for projectiles until a bullet model is desired.
- Follow the mecha loading pattern for consistency.
- If animations are present, call `m_enemyModel.UpdateAnimation(deltaTime)` in `Update()`.

## Files to Modify

- `src/mecha_fight/game/EnemyDrone.h`
- `src/mecha_fight/game/EnemyDrone.cpp`
- `src/mecha_fight/game.cpp` (render context updates)
- `CMakeLists.txt` (if adding shaders)
