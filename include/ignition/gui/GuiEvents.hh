/*
 * Copyright (C) 2020 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#ifndef IGNITION_GUI_GUIEVENTS_HH_
#define IGNITION_GUI_GUIEVENTS_HH_

#include <QEvent>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ignition/common/KeyEvent.hh>
#include <ignition/common/MouseEvent.hh>
#include <ignition/math/Vector2.hh>
#include <ignition/math/Vector3.hh>
#include <ignition/utils/ImplPtr.hh>

#include "ignition/gui/Export.hh"

namespace ignition
{
  namespace gui
  {
    /// \brief Namespace for all events.
    namespace events
    {
      /// User defined events should start from QEvent::MaxUser and
      /// count down to avoid collision with ign-gazebo events

      /// \brief Event called in the render thread of a 3D scene.
      /// It's safe to make rendering calls in this event's callback.
      class Render : public QEvent
      {
        public: Render()
            : QEvent(kType)
        {
        }
        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser);
      };

      /// \brief The class for sending and receiving custom snap value events.
      /// This event is used in the Transform Control plugin tool when the
      /// user manually alters their snapping values.
      class SnapIntervals : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _xyz XYZ snapping values.
        /// \param[in] _rpy RPY snapping values.
        /// \param[in] _scale Scale snapping values.
        public: SnapIntervals(
                    const math::Vector3d &_xyz,
                    const math::Vector3d &_rpy,
                    const math::Vector3d &_scale)
        : QEvent(kType), xyz(_xyz), rpy(_rpy), scale(_scale)
        {
        }

        /// \brief Get the XYZ snapping values.
        /// \return The XYZ snapping values.
        public: math::Vector3d Position() const
        {
          return this->xyz;
        }

        /// \brief Get the RPY snapping values.
        /// \return The RPY snapping values.
        public: math::Vector3d Rotation() const
        {
          return this->rpy;
        }

        /// \brief Get the scale snapping values.
        /// \return The scale snapping values.
        public: math::Vector3d Scale() const
        {
          return this->scale;
        }

        /// \brief The QEvent representing a snap event occurrence.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 1);

        /// \brief XYZ snapping values in meters, these values must be positive.
        private: math::Vector3d xyz;

        /// \brief RPY snapping values in degrees, these values must be
        /// positive.
        private: math::Vector3d rpy;

        /// \brief Scale snapping values - a multiplier of the current size,
        /// these values must be positive.
        private: math::Vector3d scale;
      };

      /// \brief Event called to spawn a resource, given its description as a
      /// string.
      class SpawnFromDescription : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _string The resource's description as a string, such
        /// as an SDF file.
        public: explicit SpawnFromDescription(const std::string &_description)
            : QEvent(kType), description(_description)
        {
        }

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 2);

        /// \brief Get the string description of the resource.
        /// \return The resource string
        public: const std::string &Description() const
        {
          return this->description;
        }

        /// \brief The string of the resource to be spawned.
        std::string description;
      };

      /// \brief Event called to spawn a resource, which takes the path
      /// to its file.
      class SpawnFromPath : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _filePath The path to a file.
        public: explicit SpawnFromPath(const std::string &_filePath)
            : QEvent(kType), filePath(_filePath)
        {
        }

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 3);

        /// \brief Get the path of the file.
        /// \return The file path.
        public: const std::string &FilePath() const
        {
          return this->filePath;
        }

        /// \brief The path of file to be previewed.
        std::string filePath;
      };

      /// \brief Event which is called to broadcast the 3D coordinates of a
      /// user's mouse hover within the scene.
      class HoverToScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _point The point at which the mouse is hovering within
        /// the scene
        public: explicit HoverToScene(const math::Vector3d &_point)
            : QEvent(kType), point(_point)
        {
        }

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 4);

        /// \brief Get the point within the scene over which the user is
        /// hovering.
        /// \return The 3D point
        public: math::Vector3d Point() const
        {
          return this->point;
        }

        /// \brief The 3D point over which the user is hovering.
        private: math::Vector3d point;
      };

      /// \brief Event which is called to broadcast the 3D coordinates of a
      /// user's left click within the scene.
      /// \sa LeftClickOnScene
      class LeftClickToScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _point The point which the user has left clicked within
        /// the scene
        public: explicit LeftClickToScene(const math::Vector3d &_point)
            : QEvent(kType), point(_point)
        {
        }

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 5);

        /// \brief Get the point within the scene that the user clicked.
        /// \return The 3D point.
        public: math::Vector3d Point() const
        {
          return this->point;
        }

        /// \brief The 3D point that the user clicked within the scene.
        private: math::Vector3d point;
      };

      /// \brief Event which is called to broadcast the 3D coordinates of a
      /// user's right click within the scene.
      /// \sa RightClickOnScene
      class RightClickToScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _point The point which the user has right clicked
        /// within the scene
        public: explicit RightClickToScene(const math::Vector3d &_point)
            : QEvent(kType), point(_point)
        {
        }

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 6);

        /// \brief Get the point within the scene that the user clicked.
        /// \return The 3D point.
        public: math::Vector3d Point() const
        {
          return this->point;
        }

        /// \brief The 3D point that the user clicked within the scene.
        private: math::Vector3d point;
      };

      /// \brief Event which is called to enable or disable the dropdown menu.
      /// This is primarily used by plugins which also use the right click
      /// mouse event to cancel any actions currently in progress.
      class DropdownMenuEnabled : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _menuEnabled The boolean indicating whether the dropdown
        /// menu should be enabled or disabled.
        public: explicit DropdownMenuEnabled(bool _menuEnabled)
            : QEvent(kType), menuEnabled(_menuEnabled)
        {
        }

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 7);

        /// \brief Gets whether the menu is enabled or not for this event.
        /// \return True if enabling the menu, false if disabling the menu
        public: bool MenuEnabled() const
        {
          return this->menuEnabled;
        }

        /// \brief The boolean indicating whether the menu is disabled or not
        /// for this event.
        private: bool menuEnabled;
      };

      /// \brief Event which is called to broadcast information about left
      /// mouse clicks on the scene.
      /// For the 3D coordinates of that point on the scene, see
      /// `LeftClickToScene`.
      /// \sa LeftClickToScene
      class IGNITION_GUI_VISIBLE LeftClickOnScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _mouse The left mouse event on the scene
        public: explicit LeftClickOnScene(
          const common::MouseEvent &_mouse);

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 10);

        /// \brief Return the left mouse event
        public: const common::MouseEvent &Mouse() const;

        /// \internal
        /// \brief Private data pointer
        IGN_UTILS_IMPL_PTR(dataPtr)
      };

      /// \brief Event which is called to broadcast information about right
      /// mouse clicks on the scene.
      /// For the 3D coordinates of that point on the scene, see
      /// `RightClickToScene`.
      /// \sa RightClickToScene
      class IGNITION_GUI_VISIBLE RightClickOnScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _mouse The right mouse event on the scene
        public: RightClickOnScene(
          const common::MouseEvent &_mouse);

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 11);

        /// \brief Return the right mouse event
        public: const common::MouseEvent &Mouse() const;

        /// \internal
        /// \brief Private data pointer
        IGN_UTILS_IMPL_PTR(dataPtr)
      };

      /// \brief Event which is called to broadcast the key release within
      /// the scene.
      class IGNITION_GUI_VISIBLE KeyReleaseOnScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _key The key released event within the scene
        public: explicit KeyReleaseOnScene(const common::KeyEvent &_key);

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 8);

        /// \brief Get the released key within the scene that the user released.
        /// \return The key code.
        public: common::KeyEvent Key() const;

        /// \internal
        /// \brief Private data pointer
        IGN_UTILS_IMPL_PTR(dataPtr)
      };

      /// \brief Event which is called to broadcast the key press within
      /// the scene.
      class IGNITION_GUI_VISIBLE KeyPressOnScene : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _key The pressed key within the scene
        public: explicit KeyPressOnScene(const common::KeyEvent &_key);

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 9);

        /// \brief Get the key within the scene that the user pressed
        /// \return The key code.
        public: common::KeyEvent Key() const;

        /// \internal
        /// \brief Private data pointer
        IGN_UTILS_IMPL_PTR(dataPtr)
      };

      /// \brief Event that block the Interactive View control when some of the
      /// other plugins require it. For example: When the transform control is
      /// active we should block the movements of the camera.
      class IGNITION_GUI_VISIBLE BlockOrbit : public QEvent
      {
        /// \brief Constructor
        /// \param[in] _block True to block otherwise False
        public: explicit BlockOrbit(const bool &_block);

        /// \brief Unique type for this event.
        static const QEvent::Type kType = QEvent::Type(QEvent::MaxUser - 12);

        /// \brief Get the if the event should block the Interactive view
        /// controller
        /// \return True to block otherwise False.
        public: bool Block() const;

        /// \internal
        /// \brief Private data pointer
        IGN_UTILS_IMPL_PTR(dataPtr)
      };
    }
  }
}

#endif  // IGNITION_GUI_GUIEVENTS_HH_
