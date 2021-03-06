#include "EditorObject.hpp"
#include "Animatable.hpp"
#include "Editor.hpp"

namespace dgame {

EditorObject::EditorObject(Editor& editor, const glm::vec2& positionOnMap, const glm::ivec2& size, const std::string& sprite,
                           LinkedObject linkedObject)
   : m_editor(editor)
{
   m_globalPosition = m_editor.GetLevel().GetGlobalVec(positionOnMap);
   m_localPosition = positionOnMap;
   m_sprite.SetSpriteTextured(m_globalPosition, size, sprite);
   m_centeredGlobalPosition = m_sprite.GetPosition();

   m_centeredLocalPosition = m_editor.GetLevel().GetLocalVec(m_centeredGlobalPosition);
   m_linkedObject = linkedObject;
   m_objectID = m_linkedObject.first ? m_linkedObject.first->GetID() : -1;
}

bool
EditorObject::CheckIfCollidedScreenPosion(const glm::vec2& screenPosition) const
{
   bool collided = false;

   Camera camera = m_editor.GetCamera();
   camera.Rotate(m_sprite.GetRotation(), false);

   const auto boundingRectangle = m_sprite.GetTransformedRectangle();

   const auto transformed0 = camera.GetViewMatrix() * glm::vec4(boundingRectangle[0], 0.0f, 1.0f);
   const auto transformed1 = camera.GetViewMatrix() * glm::vec4(boundingRectangle[1], 0.0f, 1.0f);
   const auto transformed3 = camera.GetViewMatrix() * glm::vec4(boundingRectangle[3], 0.0f, 1.0f);

   const auto minX = transformed1.x;
   const auto maxX = transformed0.x;
   const auto minY = transformed3.y;
   const auto maxY = transformed0.y;

   const auto globalPosition = camera.GetViewMatrix() * glm::vec4(m_editor.ScreenToGlobal(screenPosition), 0.0f, 1.0f);

   // If 'screenPosition' is inside 'object' sprite (rectangle)
   if (globalPosition.x >= minX && globalPosition.x <= maxX && globalPosition.y <= maxY && globalPosition.y >= minY)
   {
      collided = true;
   }

   return collided;
}

glm::vec2
EditorObject::GetScreenPositionPixels() const
{
   return m_editor.GlobalToScreen(m_centeredGlobalPosition);
}

bool
EditorObject::Visible() const
{
   return m_visible;
}

void
EditorObject::SetColor(const glm::vec3& color)
{
   m_sprite.SetColor(color);
}

glm::ivec2
EditorObject::GetSize() const
{
   return m_sprite.GetSize();
}

glm::ivec2
EditorObject::GetCenteredLocalPosition() const
{
   return m_centeredLocalPosition;
}

void
EditorObject::SetCenteredLocalPosition(const glm::ivec2& pos)
{
   m_centeredLocalPosition = pos;
}

void
EditorObject::SetLocalPosition(const glm::ivec2& position)
{
   m_localPosition = position;
}

void
EditorObject::SetGlobalPosition(const glm::vec2& position)
{
   m_globalPosition = position;
}

glm::vec2
EditorObject::GetGlobalPosition() const
{
   return m_globalPosition;
}

glm::ivec2
EditorObject::GetLocalPosition() const
{
   return m_localPosition;
}

glm::vec2
EditorObject::GetCenteredGlobalPosition() const
{
   return m_centeredGlobalPosition;
}

const Sprite&
EditorObject::GetSprite() const
{
   return m_sprite;
}

Sprite&
EditorObject::GetSprite()
{
   return m_sprite;
}

void
EditorObject::SetShaders(const Shader& /*program*/)
{
   // m_program = program;
}

void
EditorObject::CreateSprite(const glm::vec2& globalPosition, const glm::ivec2& size)
{
   m_sprite.SetSprite(globalPosition, size);
   m_globalPosition = m_sprite.GetPosition();
}

void
EditorObject::CreateSpriteTextured(const glm::vec2& /*position*/, const glm::ivec2& /*size*/, const std::string& /*fileName*/)
{
   m_globalPosition = m_sprite.GetPosition();
}

void
EditorObject::SetObjectSelected()
{
   SetColor({1.0f, 0.0f, 0.0f});
   m_selected = true;
}

void
EditorObject::SetObjectUnselected()
{
   m_selected = false;
   SetColor({1.0f, 1.0f, 1.0f});
}

void
EditorObject::SetName(const std::string& name)
{
   m_name = name;
}

std::string
EditorObject::GetName() const
{
   return m_name;
}

std::shared_ptr< Object >
EditorObject::GetLinkedObject()
{
   return m_linkedObject.first;
}

void
EditorObject::DeleteLinkedObject()
{
   if (m_linkedObject.first)
   {
      switch (m_linkedObject.first->GetType())
      {
         case Object::TYPE::ANIMATION_POINT: {
            auto animatablePtr = std::dynamic_pointer_cast< Animatable >(m_linkedObject.second);
            animatablePtr->DeleteAnimationNode(std::dynamic_pointer_cast< AnimationPoint >(m_linkedObject.first));
            animatablePtr->ResetAnimation();

            m_linkedObject.second->Move(animatablePtr->GetAnimationStartLocation() - m_linkedObject.second->GetGlobalPosition());
         }
         break;

         case Object::TYPE::PATHFINDER_NODE: {
            m_editor.GetLevel().GetPathfinder().DeleteNode(std::dynamic_pointer_cast< Node >(m_linkedObject.first));
         }
         break;

         default: {
         }
      }
   }
}

void
EditorObject::Move(const glm::vec2& moveBy, bool /*isCameraMovement*/)
{
   m_sprite.Translate(moveBy);
   m_globalPosition += moveBy;
   m_centeredGlobalPosition += moveBy;

   m_localPosition += moveBy;
   m_centeredLocalPosition += moveBy;

   if (m_linkedObject.first)
   {
      switch (m_linkedObject.first->GetType())
      {
         case Object::TYPE::ANIMATION_POINT: {
            std::dynamic_pointer_cast< AnimationPoint >(m_linkedObject.first)->m_end += moveBy;
            m_editor.UpdateAnimationData();
         }
         break;
         case Object::TYPE::PATHFINDER_NODE: {
            std::dynamic_pointer_cast< Node >(m_linkedObject.first)->m_position += moveBy;
         }
         default: {
         }
      }
   }
}

void
EditorObject::Scale(const glm::vec2& scaleVal, bool cumulative)
{
   cumulative ? m_sprite.ScaleCumulative(scaleVal) : m_sprite.Scale(scaleVal);
}

void
EditorObject::Rotate(float angle, bool cumulative)
{
   cumulative ? m_sprite.RotateCumulative(angle) : m_sprite.Rotate(angle);

   auto rotate = m_sprite.GetRotation(Sprite::RotationType::DEGREES);
   if (m_linkedObject.first)
   {
      switch (m_linkedObject.first->GetType())
      {
         case Object::TYPE::ANIMATION_POINT: {
            std::dynamic_pointer_cast< AnimationPoint >(m_linkedObject.first)->m_rotation = rotate;
            m_editor.UpdateAnimationData();
         }
         default: {
         }
      }
   }
}

void
EditorObject::Update(bool /*isReverse*/)
{
}

void
EditorObject::Render()
{
   m_sprite.Render();
}

void
EditorObject::SetVisible(bool visible)
{
   m_visible = visible;
}

bool
EditorObject::IsVisible()
{
   return m_visible;
}

} // namespace dgame
