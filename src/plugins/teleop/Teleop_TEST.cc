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

#include <gtest/gtest.h>
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <ignition/msgs/stringmsg.pb.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <ignition/utilities/ExtraTestMacros.hh>

#include "test_config.h"  // NOLINT(build/include)
#include "ignition/gui/Application.hh"
#include "ignition/gui/Plugin.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/qt.h"

#include "Teleop.hh"

int g_argc = 1;
char* g_argv[] =
{
  reinterpret_cast<char*>(const_cast<char*>("./Teleop_TEST")),
};

using namespace ignition;
using namespace gui;

class TeleopTest : public ::testing::Test
{
  // Set up function.
  protected: void SetUp() override
    {
      common::Console::SetVerbosity(4);

      this->app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

      // Load plugin
      const char *pluginStr =
        "<plugin filename=\"Teleop\">"
          "<ignition-gui>"
            "<title>Teleop!</title>"
          "</ignition-gui>"
        "</plugin>";

      tinyxml2::XMLDocument pluginDoc;
      EXPECT_EQ(tinyxml2::XML_SUCCESS, pluginDoc.Parse(pluginStr));
      EXPECT_TRUE(this->app.LoadPlugin("Teleop",
          pluginDoc.FirstChildElement("plugin")));

      // Get main window
      win = this->app.findChild<MainWindow *>();
      ASSERT_NE(nullptr, win);

      // Show, but don't exec, so we don't block
      win->QuickWindow()->show();

      // Get plugin
      plugins = win->findChildren<plugins::Teleop *>();
      EXPECT_EQ(plugins.size(), 1);

      plugin = plugins[0];
      EXPECT_EQ(plugin->Title(), "Teleop!");

      // Subscribes to the command velocity topic.
      node.Subscribe("/model/vehicle_blue/cmd_vel",
          &TeleopTest::VerifyTwistMsgCb, this);

      // Sets topic. This must be the same as the
      // one the node is subscribed to.
      plugin->OnTopicSelection(
          QString::fromStdString("/model/vehicle_blue/cmd_vel"));

      // Checks if the directions of the movement are set
      // with the default value '0'.
      EXPECT_EQ(plugin->LinearDirection(), 0);
      EXPECT_EQ(plugin->AngularDirection(), 0);

      // Set velocity value and movement direction.
      plugin->OnLinearVelSelection(linearVel);
      plugin->OnAngularVelSelection(angularVel);
    }

  // Subscriber call back function. Verifies if the Twist message is
  // sent correctly.
  protected: void VerifyTwistMsgCb(const msgs::Twist &_msg)
    {
      EXPECT_DOUBLE_EQ(_msg.linear().x(),
          plugin->LinearDirection() * linearVel);
      EXPECT_DOUBLE_EQ(_msg.angular().z(),
          plugin->AngularDirection() * angularVel);
      received = true;
    }

  // Provides an API to load plugins and configuration files.
  protected: Application app{g_argc, g_argv};

  // List of plugins.
  protected: QList<plugins::Teleop *> plugins;
  protected: plugins::Teleop * plugin;

  // Instance of the main window.
  protected: MainWindow * win;

  // Checks if a new command has been received.
  protected: bool received = false;
  protected: transport::Node node;

  // Define velocity values.
  protected: const double linearVel = 1.0;
  protected: const double angularVel = 0.5;
};

/////////////////////////////////////////////////
TEST_F(TeleopTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(ButtonCommand))
{
  // Forward movement.
  plugin->setLinearDirection(1);
  // Counterclockwise movement.
  plugin->setAngularDirection(1);
  plugin->OnTeleopTwist();

  int sleep = 0;
  const int maxSleep = 30;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Change movement direction.
  // Backward movement.
  plugin->setLinearDirection(-1);
  // Clockwise direction.
  plugin->setAngularDirection(-1);
  plugin->OnTeleopTwist();

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Stops angular movement.
  plugin->setAngularDirection(0);
  plugin->OnTeleopTwist();

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }
  EXPECT_TRUE(received);
  received = false;

  // Stops linear movement.
  // Starts angular movement.
  plugin->setLinearDirection(0);
  plugin->setAngularDirection(1);
  plugin->OnTeleopTwist();

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Stops movement.
  plugin->setAngularDirection(0);
  plugin->setLinearDirection(0);
  plugin->OnTeleopTwist();

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);

  // Cleanup
  plugins.clear();
}

/////////////////////////////////////////////////
TEST_F(TeleopTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(KeyboardCommand))
{
  // Generates a key press event on the main window.
  QKeyEvent *keypress_W = new QKeyEvent(QKeyEvent::KeyPress,
    Qt::Key_W, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keypress_W);
  QCoreApplication::processEvents();

  EXPECT_FALSE(received);

  // Enables key input.
  plugin->OnKeySwitch(true);
  app.sendEvent(win->QuickWindow(), keypress_W);

  int sleep = 0;
  const int maxSleep = 30;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Generates a key press event on the main window.
  QKeyEvent *keypress_D = new QKeyEvent(QKeyEvent::KeyPress,
    Qt::Key_D, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keypress_D);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Generates a key release event on the main window.
  QKeyEvent *keyrelease_D = new QKeyEvent(QKeyEvent::KeyRelease,
    Qt::Key_D, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keyrelease_D);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Generates a key press event on the main window.
  QKeyEvent *keypress_A = new QKeyEvent(QKeyEvent::KeyPress,
    Qt::Key_A, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keypress_A);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;

  // Generates a key release event on the main window.
  QKeyEvent *keyrelease_A = new QKeyEvent(QKeyEvent::KeyRelease,
    Qt::Key_A, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keyrelease_A);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;
  // Generates a key release event on the main window.
  QKeyEvent *keyrelease_W = new QKeyEvent(QKeyEvent::KeyRelease,
    Qt::Key_W, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keyrelease_W);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;
  // Generates a key press event on the main window.
  QKeyEvent *keypress_X = new QKeyEvent(QKeyEvent::KeyPress,
    Qt::Key_X, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keypress_X);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  EXPECT_TRUE(received);
  received = false;
  // Generates a key release event on the main window.
  QKeyEvent *keyrelease_X = new QKeyEvent(QKeyEvent::KeyRelease,
    Qt::Key_X, Qt::NoModifier);
  app.sendEvent(win->QuickWindow(), keyrelease_X);

  sleep = 0;
  while (!received && sleep < maxSleep)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QCoreApplication::processEvents();
    sleep++;
  }

  // Cleanup
  plugins.clear();
}
