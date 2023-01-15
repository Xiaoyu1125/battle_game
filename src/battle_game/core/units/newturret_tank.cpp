#include "newturret_tank.h"

#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"

namespace battle_game::unit {

namespace {
uint32_t newturret_tank_body_model_index = 0xffffffffu;
uint32_t newturret_tank_turret_model_index = 0xffffffffu;
}  // namespace

newturret_Tank::newturret_Tank(GameCore *game_core,
                               uint32_t id,
                               uint32_t player_id)
    : Unit(game_core, id, player_id) {
  if (!~newturret_tank_body_model_index) {
    auto mgr = AssetsManager::GetInstance();
    {
      /* Tank Body */
      newturret_tank_body_model_index = mgr->RegisterModel(
          {
              // red heart shape
              {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{-0.4f, 0.6f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{-0.6f, 0.6f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{-1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{0.0f, -1.2f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{0.6f, 0.6f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
              {{0.4f, 0.6f}, {0.0f, 0.0f}, {0.0f, 0.1f, 1.0f, 1.0f}},
          },
          {0, 1, 2, 1, 2, 3, 2, 3, 4, 0, 2, 4, 0, 3, 4, 3, 4,
           5, 0, 4, 5, 0, 6, 4, 4, 5, 6, 5, 6, 7, 0, 6, 7});
    }

    {
      /* Tank Turret */
      std::vector<ObjectVertex> turret_vertices;
      std::vector<uint32_t> turret_indices;
      const int precision = 60;
      const float inv_precision = 1.0f / float(precision);
      for (int i = 0; i < precision; i++) {
        auto theta = (float(i) + 0.5f) * inv_precision;
        theta *= glm::pi<float>() * 2.0f;
        auto sin_theta = std::sin(theta);
        auto cos_theta = std::cos(theta);
        turret_vertices.push_back({{sin_theta * 0.5f, cos_theta * 0.5f},
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_indices.push_back(i);
        turret_indices.push_back((i + 1) % precision);
        turret_indices.push_back(precision);
      }
      turret_vertices.push_back(
          {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{-0.1f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{0.1f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{-0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_indices.push_back(precision + 1 + 0);
      turret_indices.push_back(precision + 1 + 1);
      turret_indices.push_back(precision + 1 + 2);
      turret_indices.push_back(precision + 1 + 1);
      turret_indices.push_back(precision + 1 + 2);
      turret_indices.push_back(precision + 1 + 3);
      newturret_tank_turret_model_index =
          mgr->RegisterModel(turret_vertices, turret_indices);
    }
  }
}

void newturret_Tank::Render() {
  battle_game::SetTransformation(position_, rotation_);
  battle_game::SetTexture(0);
  battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
  battle_game::DrawModel(newturret_tank_body_model_index);
  battle_game::SetRotation(turret_rotation_);
  battle_game::DrawModel(newturret_tank_turret_model_index);
}

void newturret_Tank::Update() {
  TankMove(3.0f, glm::radians(180.0f));
  TurretRotate();
  Fire();
}
// use left,right,up,down to make tank move
void newturret_Tank::TankMove(float move_speed, float rotate_angular_speed) {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    glm::vec2 offset{0.0f};
    if (input_data.key_down[GLFW_KEY_UP]) {
      offset.y += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_DOWN]) {
      offset.y -= 1.0f;
    }
    float speed = move_speed * GetSpeedScale();
    offset *= kSecondPerTick * speed;
    auto new_position =
        position_ + glm::vec2{glm::rotate(glm::mat4{1.0f}, rotation_,
                                          glm::vec3{0.0f, 0.0f, 1.0f}) *
                              glm::vec4{offset, 0.0f, 0.0f}};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }
    float rotation_offset = 0.0f;
    if (input_data.key_down[GLFW_KEY_LEFT]) {
      rotation_offset += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_RIGHT]) {
      rotation_offset -= 1.0f;
    }
    rotation_offset *= kSecondPerTick * rotate_angular_speed * GetSpeedScale();
    game_core_->PushEventRotateUnit(id_, rotation_ + rotation_offset);
  }
}

void newturret_Tank::TurretRotate() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
     if (input_data.key_down[GLFW_KEY_LEFT]) {
     float rotation_offset = 0.0f;
     float rotate_angular_speed = 0.0f;
     rotation_offset += 1.0f;
      rotation_offset *=
          kSecondPerTick * rotate_angular_speed * GetSpeedScale();
      turret_rotation_ = rotation_offset + rotation_;
     }
     if (input_data.key_down[GLFW_KEY_RIGHT]) {
     float rotation_offset = 0.0f;
     float rotate_angular_speed = 0.0f;
     rotation_offset -= 1.0f;
     rotation_offset *=
       kSecondPerTick * rotate_angular_speed * GetSpeedScale();
     turret_rotation_ = rotation_offset + rotation_;
     } else {
         turret_rotation_ = glm::radians(0.0f);
       }
  }
}

void newturret_Tank::Fire() {
  if (fire_count_down_) {
    fire_count_down_--;
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.mouse_button_down[GLFW_MOUSE_BUTTON_LEFT]) {
        auto velocity = Rotate(glm::vec2{0.0f, 20.0f}, turret_rotation_);
        GenerateBullet<bullet::CannonBall>(
            position_ + Rotate({0.0f, 1.2f}, turret_rotation_),
            turret_rotation_, GetDamageScale(), velocity);
        fire_count_down_ = kTickPerSecond;  // Fire interval 1 second.
      }
    }
  }
}

bool newturret_Tank::IsHit(glm::vec2 position) const {
  position = WorldToLocal(position);
  return position.x > -0.8f && position.x < 0.8f && position.y > -1.0f &&
         position.y < 1.0f && position.x + position.y < 1.6f &&
         position.y - position.x < 1.6f;
}

const char *newturret_Tank::UnitName() const {
  return "newturret_tank";
}

const char *newturret_Tank::Author() const {
  return "Xiaoyu1125";
}
}  // namespace battle_game::unit