/*
 * Copyright (C) 2021 Open Source Robotics Foundation
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

#ifndef IGNITION_GUI_PLUGINS_TELEOP_HH_
#define IGNITION_GUI_PLUGINS_TELEOP_HH_

#include <memory>

#include <ignition/transport/Node.hh>

#include <ignition/gui/Plugin.hh>
#include <ignition/gui/qt.h>

namespace ignition
{
namespace gui
{
namespace plugins
{
  class TeleopPrivate;

  /// \brief Publish teleop stokes to a user selected topic,
  /// or to '/cmd_vel' if no topic is selected.
  /// Buttons, the keyboard or sliders can be used to move a
  /// vehicle load to the world.
  /// ## Configuration
  /// This plugin doesn't accept any custom configuration.
  class Teleop : public Plugin
  {
    Q_OBJECT

    /// \brief Linear direction
    Q_PROPERTY(
      int linearDir
      READ LinearDirection
      WRITE setLinearDirection
      NOTIFY LinearDirectionChanged
    )

    /// \brief Angular direction
    Q_PROPERTY(
      int angularDir
      READ AngularDirection
      WRITE setAngularDirection
      NOTIFY AngularDirectionChanged
    )

    /// \brief Constructor
    public: Teleop();

    /// \brief Destructor
    public: virtual ~Teleop();

    // Documentation inherited.
    public: virtual void LoadConfig(const tinyxml2::XMLElement *) override;

    /// \brief Filters events of type 'keypress' and 'keyrelease'.
    protected: bool eventFilter(QObject *_obj, QEvent *_event) override;

    /// \brief Publish the twist message to the selected command velocity topic.
    public slots: void OnTeleopTwist();

    /// \brief Returns the linear direction variable value.
    ///  When the movement is forward it takes the value 1, when
    /// is backward it takes the value -1, and when it's 0 the
    /// movement stops.
    public: Q_INVOKABLE int LinearDirection() const;

    /// \brief Set the linear direction of the movement.
    /// \param[in] _linearDir Modifier of the velocity for setting
    /// the movement direction.
    public: Q_INVOKABLE void setLinearDirection(int _linearDir);

    /// \brief Notify that the linear direction changed.
    signals: void LinearDirectionChanged();

    /// \brief Returns the angular direction variable value.
    /// When the turn is counterclockwise it takes the value 1, when
    /// is clockwise it takes the value -1, and when it's 0 the
    /// movement stops.
    public: Q_INVOKABLE int AngularDirection() const;

    /// \brief Set the angular direction of the movement.
    /// \param[in] _angularDir Modifier of the velocity for setting
    /// the direction of the rotation.
    public: Q_INVOKABLE void setAngularDirection(int _angularDir);

    /// \brief Notify that the angular direction changed.
    signals: void AngularDirectionChanged();

    /// \brief Callback in Qt thread when the topic changes.
    /// \param[in] _topic variable to indicate the topic to
    /// publish the Twist commands.
    public slots: void OnTopicSelection(const QString &_topic);

    /// \brief Callback in Qt thread when the linear velocity changes.
    /// \param[in] _velocity variable to indicate the linear velocity.
    public slots: void OnLinearVelSelection(double _velocity);

    /// \brief Callback in Qt thread when the angular velocity changes.
    /// \param[in] _velocity variable to indicate the angular velocity.
    public slots: void OnAngularVelSelection(double _velocity);

    /// \brief Callback in Qt thread when the keyboard is enabled or disabled.
    /// \param[in] _checked variable to indicate the state of the switch.
    public slots: void OnKeySwitch(bool _checked);

    /// \brief Callback in Qt thread when the sliders is enabled or disabled.
    /// \param[in] _checked variable to indicate the state of the switch.
    public slots: void OnSlidersSwitch(bool _checked);

    /// \brief Sets the movement direction when the keyboard is used.
    public: void SetKeyDirection();

    /// \internal
    /// \brief Pointer to private data.
    private: std::unique_ptr<TeleopPrivate> dataPtr;

  };
}
}
}

#endif
