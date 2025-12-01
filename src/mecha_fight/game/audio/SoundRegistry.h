#pragma once

namespace mecha
{

  /**
   * @brief Central registry for sound file paths
   *
   * Provides constants for all sound files used in the game,
   * avoiding magic strings throughout the codebase.
   */
  namespace SoundRegistry
  {
    // Player sounds
    constexpr const char *PLAYER_SHOOT = "resources/audio/scifi-gun-shoot-1-266417.mp3";
    constexpr const char *PLAYER_DASH = "resources/audio/long-whoosh-194554.mp3";
    constexpr const char *PLAYER_MELEE = "resources/audio/big-robot-footstep-010-426482.mp3";
    constexpr const char *PLAYER_MELEE_CONTINUE = "resources/audio/big-robot-footstep-004-346797.mp3";
    constexpr const char *PLAYER_DAMAGE = "resources/audio/metal-knock-2-103060.mp3";
    constexpr const char *PLAYER_FLIGHT = "resources/audio/large-rocket-engine-86240.mp3"; // Looping thruster sound
    constexpr const char *PLAYER_WALKING = "resources/audio/3-legged-robot-walker-86726.mp3";
    constexpr const char *PLAYER_LASER = "resources/audio/laser-gun-105918.mp3";

    // Enemy sounds
    constexpr const char *ENEMY_SHOOT = "resources/audio/scifi-gun-shoot-1-266417.mp3";
    constexpr const char *ENEMY_DEATH = "resources/audio/solid.wav";
    constexpr const char *ENEMY_DRONE_MOVEMENT = "resources/audio/little-robot-sound-84657.mp3"; // Looping movement sound for drone
    constexpr const char *ENEMY_TURRET_LASER = "resources/audio/laser-gun-105918.mp3";

    // Projectile sounds
    constexpr const char *PROJECTILE_IMPACT = "resources/audio/solid.wav";
    constexpr const char *PROJECTILE_SHOOT = "resources/audio/scifi-gun-shoot-1-266417.mp3";
    constexpr const char *MISSILE_LAUNCH = "resources/audio/large-rocket-engine-86240.mp3";
    constexpr const char *MISSILE_EXPLOSION = "resources/audio/medium-explosion-40472.mp3";

    // Boss sounds
    constexpr const char *BOSS_DEATH = "resources/audio/big-cine-boom-sound-effect-245851.mp3";
    constexpr const char *BOSS_MOVEMENT = "resources/audio/robot-tank-34600.mp3";
    constexpr const char *BOSS_PROJECTILE = "resources/audio/scifi-gun-shoot-1-266417.mp3";
    constexpr const char *BOSS_SHOCKWAVE = "resources/audio/sound-design-elements-impact-sfx-ps-093-369575.mp3";

    // Environment sounds
    constexpr const char *PORTAL_ACTIVE = "resources/audio/breakout.mp3"; // Looping portal sound
    constexpr const char *PORTAL_DESTROY = "resources/audio/powerup.wav";
    constexpr const char *GATE_COLLAPSE = "resources/audio/exploding-building-1-185114.mp3";

    // UI sounds
    constexpr const char *UI_CLICK = "resources/audio/bleep.wav";
    constexpr const char *UI_SELECT = "resources/audio/powerup.wav";
  }

} // namespace mecha
