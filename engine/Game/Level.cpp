#include "Level.hpp"
#include "Enemy.hpp"
#include "FileManager.hpp"
#include "Game.hpp"
#include "Player.hpp"
#include "Shader.hpp"
#include "Timer.hpp"
#include "Window.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>

namespace dgame {

void
Level::Create(Application* context, const glm::ivec2& size)
{
   m_levelSize = size;
   m_background.SetSprite(glm::vec2(0.0f, 0.0f), m_levelSize);
   m_contextPointer = context;
}

void
Level::Load(Application* context, const std::string& pathToLevel)
{
   const auto json = FileManager::LoadJsonFile(pathToLevel);

   for (auto& [key, value] : json.items())
   {
      if (key == "BACKGROUND")
      {
         const auto background = json[key]["texture"];
         const auto size = json[key]["size"];

         LoadPremade(background, glm::ivec2(size[0], size[1]));

         // Temporary solution
         // This should be moved to Level class soon
         auto gameHandle = static_cast< Game* >(context);
         if (gameHandle)
         {
            const auto collision = json[key]["collision"];
            m_collision.LoadTextureFromFile(collision);
            gameHandle->SetCollisionMap(m_collision.GetVec4Data());
         }

         m_contextPointer = context;
      }
      else if (key == "PATHFINDER")
      {
         for (auto& nodeJson : json[key]["nodes"])
         {
            m_pathinder.AddNode(
               std::make_shared< Node >(glm::ivec2(nodeJson["position"][0], nodeJson["position"][1]), nodeJson["id"],
                                        std::vector< Node::NodeID >(nodeJson["connected to"].begin(), nodeJson["connected to"].end())));
         }
      }
      else if (key == "SHADER")
      {
         const auto shaderName = json[key]["name"];
         LoadShaders(shaderName);
      }
      else if (key == "PLAYER")
      {
         const auto position = json[key]["position"];
         const auto size = json[key]["size"];
         const auto texture = json[key]["texture"];
         const auto weapons = json[key]["weapons"];
         const auto name = json[key]["name"];

         m_player = std::make_shared< Player >(*context, glm::vec2(position[0], position[1]), glm::ivec2(size[0], size[1]), texture, name);
         m_player->GetSprite().Scale(glm::vec2(json[key]["scale"][0], json[key]["scale"][1]));
         m_player->GetSprite().Rotate(json[key]["rotation"]);
         m_objects.emplace_back(m_player);
      }
      else if (key == "ENEMIES")
      {
         for (auto& enemy : json[key])
         {
            const auto position = enemy["position"];
            const auto size = enemy["size"];
            const auto texture = enemy["texture"];
            const auto weapons = enemy["weapons"];
            const auto name = enemy["name"];

            AnimationPoint::vectorPtr keypointsPositions = {};
            glm::vec2 beginPoint = glm::vec2(position[0], position[1]);

            for (auto& point : enemy["animate positions"])
            {
               auto animationPoint = std::make_shared< AnimationPoint >();
               animationPoint->m_end = glm::vec2(point["end position"][0], point["end position"][1]);
               animationPoint->m_timeDuration = Timer::seconds(point["time duration"]);

               keypointsPositions.emplace_back(animationPoint);

               beginPoint = animationPoint->m_end;
            }

            auto object = std::make_shared< Enemy >(*context, glm::vec2(position[0], position[1]), glm::ivec2(size[0], size[1]), texture,
                                                    keypointsPositions);
            object->SetName(name);
            object->GetSprite().Scale(glm::vec2(enemy["scale"][0], enemy["scale"][1]));
            object->GetSprite().Rotate(enemy["rotation"]);

            m_objects.emplace_back(object);
         }
      }
      else
      {
         m_logger.Log(Logger::TYPE::WARNING, "Level::Load -> Unspecified type " + key + " during level loading");
      }
   }
}

void
Level::Save(const std::string& pathToLevel)
{
   nlohmann::json json;

   const auto nodes = m_pathinder.GetAllNodes();
   for (const auto& node : nodes)
   {
      nlohmann::json nodeJson;
      nodeJson["id"] = node->m_ID;
      nodeJson["position"] = {node->m_position.x, node->m_position.y};
      nodeJson["connected to"] = node->m_connectedNodes;

      json["PATHFINDER"]["nodes"].emplace_back(nodeJson);
   }

   // Serialize shader
   // json["SHADER"]["name"] = m_shaders.GetName();

   // Serialize background
   json["BACKGROUND"]["texture"] = m_background.GetTextureName();
   json["BACKGROUND"]["size"] = {m_background.GetSize().x, m_background.GetSize().y};
   json["BACKGROUND"]["collision"] = m_collision.GetName();

   // Serialize player
   json["PLAYER"]["name"] = m_player->GetName();
   json["PLAYER"]["position"] = {m_player->GetLocalPosition().x, m_player->GetLocalPosition().y};
   json["PLAYER"]["scale"] = {m_player->GetSprite().GetScale().x, m_player->GetSprite().GetScale().y};
   json["PLAYER"]["rotation"] = m_player->GetSprite().GetRotation();
   json["PLAYER"]["size"] = {m_player->GetSprite().GetOriginalSize().x, m_player->GetSprite().GetOriginalSize().y};
   json["PLAYER"]["texture"] = m_player->GetSprite().GetTextureName();
   json["PLAYER"]["weapons"] = m_player->GetWeapons();

   // Serialize game objects
   for (const auto& object : m_objects)
   {
      if (object->GetType() == Object::TYPE::PLAYER)
      {
         continue;
      }

      nlohmann::json enemyJson;

      enemyJson["name"] = object->GetName();
      enemyJson["position"] = {object->GetLocalPosition().x, object->GetLocalPosition().y};
      enemyJson["size"] = {object->GetSprite().GetOriginalSize().x, object->GetSprite().GetOriginalSize().y};
      enemyJson["scale"] = {object->GetSprite().GetScale().x, object->GetSprite().GetScale().y};
      enemyJson["rotation"] = object->GetSprite().GetRotation();
      enemyJson["texture"] = object->GetSprite().GetTextureName();

      auto enemyPtr = dynamic_cast< Enemy* >(object.get());

      enemyJson["weapons"] = enemyPtr->GetWeapon();
      enemyJson["animation type"] = enemyPtr->GetAnimationType() == Animatable::ANIMATION_TYPE::LOOP ? "Loop" : "Reversable";

      const auto keypoints = enemyPtr->GetAnimationKeypoints();
      for (const auto& point : keypoints)
      {
         nlohmann::json animationPoint;
         animationPoint["end position"] = {point->m_end.x, point->m_end.y};
         animationPoint["time duration"] = point->m_timeDuration.count();

         enemyJson["animate positions"].emplace_back(animationPoint);
      }

      json["ENEMIES"].emplace_back(enemyJson);
   }

   FileManager::SaveJsonFile(pathToLevel, json);
}

void
Level::Quit()
{
   m_objects.clear();
   m_player.reset();
}

std::shared_ptr< GameObject >
Level::AddGameObject(GameObject::TYPE objectType)
{
   const auto defaultPosition = GetLocalVec(m_contextPointer->GetCamera().GetPosition());
   const auto defaultSize = glm::ivec2(128, 128);
   const auto defaultTexture = std::string("Default128.png");

   std::shared_ptr< GameObject > newObject;

   switch (objectType)
   {
      case GameObject::TYPE::ENEMY: {
         newObject =
            std::make_shared< Enemy >(*m_contextPointer, defaultPosition, defaultSize, defaultTexture, AnimationPoint::vectorPtr{});
         m_objects.push_back(newObject);
      }
      break;

      case GameObject::TYPE::PLAYER: {
         newObject = std::make_shared< Player >(*m_contextPointer, defaultPosition, defaultSize, defaultTexture);
         m_player = std::dynamic_pointer_cast< Player >(newObject);
         m_objects.push_back(newObject);
      }
      break;

      default: {

      }
   }

   return newObject;
}

glm::vec2
Level::GetLocalVec(const glm::vec2& global) const
{
   // get the vector relative to map's position
   // glm::vec2 returnVal{m_background.GetPosition() - global};

   //// change 'y' to originate in top left
   // returnVal.y -= m_levelSize.y;
   // returnVal *= -1;

   return global;
}

glm::vec2
Level::GetGlobalVec(const glm::vec2& local) const
{
   // glm::vec2 returnVal = local;

   // returnVal *= -1;
   // returnVal.y += m_levelSize.y;
   // returnVal = m_background.GetPosition() - returnVal;

   return local;
}

bool
Level::CheckCollision(const glm::ivec2& localPos, const Player& player)
{
   for (auto& obj : m_objects)
   {
      auto length = glm::length(glm::vec2(localPos - obj->GetCenteredLocalPosition()));
      auto objSize = obj->GetSize();

      if (length < static_cast<float>(objSize.x) / 2.5f)
      {
         obj->Hit(player.GetWeaponDmg());

         return false;
      }

      obj->SetColor({1.0f, 1.0f, 1.0f});
   }
   return true;
}

void
Level::LoadPremade(const std::string& fileName, const glm::ivec2& size)
{
   m_locked = false;
   m_levelSize = size;

   m_background.SetSpriteTextured(glm::vec2(static_cast< float >(m_levelSize.x) / 2.0f, static_cast< float >(m_levelSize.y) / 2.0f), size,
                                  fileName);
   // m_shaders.LoadDefault();
}

void
Level::LoadShaders(const std::string&)
{
   // m_shaders.LoadShaders(shaderName);
}

void
Level::AddGameObject(Game& game, const glm::vec2& pos, const glm::ivec2& size, const std::string& sprite)
{
   std::unique_ptr< GameObject > object = std::make_unique< Enemy >(game, pos, size, sprite);
   m_objects.push_back(std::move(object));
}

void
Level::DeleteObject(std::shared_ptr< Object > deletedObject)
{
   m_objects.erase(std::find(m_objects.begin(), m_objects.end(), deletedObject));
}

void
Level::Move(const glm::vec2& moveBy)
{
   for (auto& obj : m_objects)
   {
      obj->Move(moveBy);
   }

   m_background.Translate(moveBy);
   // MoveCamera(moveBy);
}

void
Level::Scale(const glm::vec2& scaleVal)
{
   for (auto& obj : m_objects)
   {
      obj->Scale(scaleVal);
   }

   m_background.Scale(scaleVal);
}

void
Level::Rotate(float angle, bool cumulative)
{
   // Move objects to center of level
   // Rotate them
   // Move them back on position

   for (auto& obj : m_objects)
   {
      // obj->Move(m_background.GetCenteredPosition());
      obj->Rotate(angle, cumulative);
      // obj->Move(-m_background.GetCenteredPosition());
   }

   cumulative ? m_background.RotateCumulative(angle) : m_background.Rotate(angle);
}

void
Level::Update(bool isReverse)
{
   m_background.Update(isReverse);

   for (auto& obj : m_objects)
   {
      if (obj->Visible())
      {
         if (obj->GetType() == Object::TYPE::ENEMY)
         {
            std::dynamic_pointer_cast< Enemy >(obj)->DealWithPlayer();
         }

         obj->Update(isReverse);
      }
   }
}

void
Level::Render()
{
   m_background.Render();

   for (auto& obj : m_objects)
   {
      if (obj->Visible())
      {
         obj->Render();
      }
   }
}

void
Level::MoveObjs(const glm::vec2& moveBy, bool isCameraMovement)
{
   for (auto& obj : m_objects)
   {
      obj->Move(moveBy, isCameraMovement);
   }
}

std::vector< std::shared_ptr< GameObject > >
Level::GetObjects(bool)
{
   return m_objects;
}

void
Level::SetPlayersPosition(const glm::vec2&)
{
   // m_playerPos = position;
   // m_playerPos /= m_tileSize;
}

std::shared_ptr< GameObject >
Level::GetGameObjectOnLocation(const glm::vec2& screenPosition)
{
   // std::shared_ptr< GameObject > foundObject = nullptr;

   // if (m_player)
   //{
   //   foundObject = m_player->CheckIfCollidedScreenPosion(screenPosition) ? m_player : nullptr;

   //   if (foundObject)
   //   {
   //      return foundObject;
   //   }
   //}

   auto objectOnLocation = std::find_if(m_objects.begin(), m_objects.end(), [screenPosition](const auto& object) {
      return object->CheckIfCollidedScreenPosion(screenPosition);
   });

   return objectOnLocation != m_objects.end() ? *objectOnLocation : nullptr;
}

Sprite&
Level::GetSprite()
{
   return m_background;
}

void
Level::SetSize(const glm::ivec2& newSize)
{
   m_levelSize = newSize;
   m_background.SetSize(newSize);
}

} // namespace dgame
