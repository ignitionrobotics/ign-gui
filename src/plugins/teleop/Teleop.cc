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

#include <iostream>
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <ignition/msgs/twist.pb.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Teleop.hh"

#include <string>

#include <ignition/plugin/Register.hh>

#include <ignition/gui/Application.hh>
#include <ignition/gui/MainWindow.hh>

namespace ignition
{
namespace gui
{
namespace plugins
{
  enum class KeyLinear{
    kForward,
    kBackward,
    kStop,
  };

  enum class KeyAngular{
    kLeft,
    kRight,
    kStop,
  };

  class TeleopPrivate
  {
    /// \brief Node for communication.
    public: ignition::transport::Node node;

    /// \brief Topic. Set '/cmd_vel' as default.
    public: std::string topic = "/cmd_vel";

    /// \brief Publisher.
    public: ignition::transport::Node::Publisher cmdVelPub;

    /// \brief Linear velocity.
    public: double linearVel = 0;
    /// \brief Angular velocity.
    public: double angularVel = 0;

    /// \brief Linear direction.
    public: int linearDir = 0;
    /// \brief Angular direction.
    public: int angularDir = 0;

    /// \brief Linear state setted by keyboard input.
    public: KeyLinear linearState = KeyLinear::kStop;
    /// \brief Angular state setted by keyboard input.
    public: KeyAngular angularState = KeyAngular::kStop;

    /// \brief Indicates if the keyboard is enabled or
    /// disabled.
    public: bool keyEnable = false;
  };
}
}
}

using namespace ignition;
using namespace gui;
using namespace plugins;

/////////////////////////////////////////////////
Teleop::Teleop(): Plugin(), dataPtr(std::make_unique<TeleopPrivate>())
{
  // Initialize publisher using default topic.
  this->dataPtr->cmdVelPub = ignition::transport::Node::Publisher();
  this->dataPtr->cmdVelPub =
      this->dataPtr->node.Advertise<ignition::msgs::Twist>
      (this->dataPtr->topic);
}

/////////////////////////////////////////////////
Teleop::~Teleop() = default;

/////////////////////////////////////////////////
void Teleop::LoadConfig(const tinyxml2::XMLElement *)
{
  if (this->title.empty())
    this->title = "Teleop";

  ignition::gui::App()->findChild
    <ignition::gui::MainWindow *>()->QuickWindow()->installEventFilter(this);
}

/////////////////////////////////////////////////
void Teleop::OnTeleopTwist()
{
  ignition::msgs::Twist cmdVelMsg;

  cmdVelMsg.mutable_linear()->set_x(
      this->dataPtr->linearDir * this->dataPtr->linearVel);
  cmdVelMsg.mutable_angular()->set_z(
      this->dataPtr->angularDir * this->dataPtr->angularVel);

  if (!this->dataPtr->cmdVelPub.Publish(cmdVelMsg))
    ignerr << "ignition::msgs::Twist message couldn't be published at topic: "
      << this->dataPtr->topic << std::endl;
}

/////////////////////////////////////////////////
void Teleop::OnTopicSelection(const QString &_topic)
{
  this->dataPtr->topic = _topic.toStdString();
  ignmsg << "A new topic has been entered: '" <<
      this->dataPtr->topic << " ' " <<std::endl;

  // Update publisher with new topic.
  this->dataPtr->cmdVelPub = ignition::transport::Node::Publisher();
  this->dataPtr->cmdVelPub =
      this->dataPtr->node.Advertise<ignition::msgs::Twist>
      (this->dataPtr->topic);
  if(!this->dataPtr->cmdVelPub)
      ignerr << "Error when advertising topic: " <<
        this->dataPtr->topic << std::endl;
}

/////////////////////////////////////////////////
void Teleop::OnLinearVelSelection(double _velocity)
{
  this->dataPtr->linearVel = _velocity;
}

/////////////////////////////////////////////////
void Teleop::OnAngularVelSelection(double _velocity)
{
  this->dataPtr->angularVel = _velocity;
}

/////////////////////////////////////////////////
void Teleop::OnKeySwitch(bool _checked)
{
  this->dataPtr->linearDir = 0;
  this->dataPtr->angularDir = 0;
  this->dataPtr->keyEnable = _checked;
}

/////////////////////////////////////////////////
void Teleop::OnSlidersSwitch(bool _checked)
{
  if(_checked)
  {
    this->dataPtr->linearDir = 1;
    this->dataPtr->angularDir = 1;
    this->OnTeleopTwist();
  }
}

/////////////////////////////////////////////////
bool Teleop::eventFilter(QObject *_obj, QEvent *_event)
{
  if(this->dataPtr->keyEnable == true)
  {
    if(_event->type() == QEvent::KeyPress)
    {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(_event);
      switch(keyEvent->key())
      {
        case Qt::Key_W:
          this->dataPtr->linearState = KeyLinear::kForward;
          break;
        case Qt::Key_A:
          this->dataPtr->angularState = KeyAngular::kLeft;
          break;
        case Qt::Key_D:
          this->dataPtr->angularState = KeyAngular::kRight;
          break;
        case Qt::Key_S:
          this->dataPtr->linearState = KeyLinear::kBackward;
          break;
        default:
          break;
      }
      this->SetKeyDirection();
      this->OnTeleopTwist();
    }

    if(_event->type() == QEvent::KeyRelease)
    {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(_event);
      switch(keyEvent->key())
      {
        case Qt::Key_W:
          this->dataPtr->linearState = KeyLinear::kStop;
          break;
        case Qt::Key_A:
          this->dataPtr->angularState = KeyAngular::kStop;
          break;
        case Qt::Key_D:
          this->dataPtr->angularState = KeyAngular::kStop;
          break;
        case Qt::Key_S:
          this->dataPtr->linearState = KeyLinear::kStop;
          break;
        default:
          break;
      }
      this->SetKeyDirection();
      this->OnTeleopTwist();
    }
  }
  return QObject::eventFilter(_obj, _event);
}

/////////////////////////////////////////////////
void Teleop::SetKeyDirection()
{
  this->dataPtr->linearDir = this->dataPtr->linearState ==
      KeyLinear::kForward ? 1 : this->dataPtr->linearState ==
      KeyLinear::kBackward ? -1 : 0;

  this->dataPtr->angularDir = this->dataPtr->angularState ==
      KeyAngular::kLeft ? 1 : this->dataPtr->angularState ==
      KeyAngular::kRight ? -1 : 0;
}

/////////////////////////////////////////////////
int Teleop::LinearDirection() const
{
  return this->dataPtr->linearDir;
}

/////////////////////////////////////////////////
void Teleop::setLinearDirection(int _linearDir)
{
  this->dataPtr->linearDir = _linearDir;
  this->LinearDirectionChanged();
}

/////////////////////////////////////////////////
int Teleop::AngularDirection() const
{
  return this->dataPtr->angularDir;
}

/////////////////////////////////////////////////
void Teleop::setAngularDirection(int _angularDir)
{
  this->dataPtr->angularDir = _angularDir;
  this->AngularDirectionChanged();
}

// Register this plugin
IGNITION_ADD_PLUGIN(ignition::gui::plugins::Teleop,
                    ignition::gui::Plugin)
