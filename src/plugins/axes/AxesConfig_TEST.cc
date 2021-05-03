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
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <ignition/rendering.hh>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <gtest/gtest.h>

#include <ignition/common/Console.hh>
#include <ignition/utilities/ExtraTestMacros.hh>

#include "test_config.h"  // NOLINT(build/include)
#include "ignition/gui/Application.hh"
#include "ignition/gui/GuiEvents.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/Plugin.hh"

#include <QComboBox>

int g_argc = 1;
char **g_argv = new char *[g_argc];

#include "AxesConfig.hh"

using namespace ignition;
using namespace gui;

/////////////////////////////////////////////////
TEST(AxesConfigTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(Load))
{
  common::Console::SetVerbosity(4);

  // Load the plugin
  Application app(g_argc, g_argv);
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

  EXPECT_TRUE(app.LoadPlugin("AxesConfig"));

  // Get main window
  auto window = app.findChild<MainWindow *>();
  ASSERT_NE(window, nullptr);

  // Get plugin
  auto plugins = window->findChildren<Plugin *>();
  EXPECT_EQ(plugins.size(), 1);
  auto plugin = plugins[0];
  EXPECT_EQ(plugin->Title(), "Axes");

  // Cleanup
  plugins.clear();
}

/////////////////////////////////////////////////
TEST(AxesConfigTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(AxesConfig))
{
  common::Console::SetVerbosity(4);

  Application app(g_argc, g_argv);
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

  // Load plugin
  const char *pluginStr =
    "<plugin filename=\"AxesConfig\">"
      "<ignition-gui>"
        "<title>AxesConfig!</title>"
      "</ignition-gui>"
    "</plugin>";

  tinyxml2::XMLDocument pluginDoc;
  EXPECT_EQ(tinyxml2::XML_SUCCESS, pluginDoc.Parse(pluginStr));
  EXPECT_TRUE(app.LoadPlugin("AxesConfig",
      pluginDoc.FirstChildElement("plugin")));

  // Get main window
  auto win = app.findChild<MainWindow *>();
  ASSERT_NE(nullptr, win);

  // Show, but don't exec, so we don't block
  win->QuickWindow()->show();

  // Get plugin
  auto plugins = win->findChildren<ignition::gazebo::AxesConfig *>();
  EXPECT_EQ(plugins.size(), 1);

  auto plugin = plugins[0];
  EXPECT_EQ(plugin->Title(), "AxesConfig!");

  // Cleanup
  plugins.clear();
}
