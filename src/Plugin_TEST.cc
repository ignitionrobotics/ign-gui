/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include <ignition/common/Console.hh>
#include <ignition/utilities/ExtraTestMacros.hh>

#include "test_config.h"  // NOLINT(build/include)
#include "ignition/gui/Application.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/Plugin.hh"

int g_argc = 1;
char **g_argv = new char *[g_argc];

using namespace ignition;
using namespace gui;

// See https://github.com/ignitionrobotics/ign-gui/issues/75
TEST(PluginTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(DeleteLater))
{
  ignition::common::Console::SetVerbosity(4);

  Application app(g_argc, g_argv);
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

  // Load normal plugin
  const char *pluginStr =
    "<plugin filename=\"TestPlugin\">"
    "</plugin>";

  tinyxml2::XMLDocument pluginDoc;
  pluginDoc.Parse(pluginStr);
  EXPECT_TRUE(app.LoadPlugin("TestPlugin",
      pluginDoc.FirstChildElement("plugin")));

  // Load plugin to be deleted
  pluginStr =
    "<plugin filename=\"TestPlugin\">"
      "<ignition-gui>"
        "<delete_later>true</delete_later>"
      "</ignition-gui>"
    "</plugin>";

  pluginDoc.Parse(pluginStr);

  // Create main window
  EXPECT_TRUE(app.LoadPlugin("TestPlugin",
      pluginDoc.FirstChildElement("plugin")));;

  auto win = app.findChild<MainWindow *>();
  ASSERT_NE(nullptr, win);

  // Check plugin count
  EXPECT_EQ(1, win->findChildren<Plugin *>().size());
}

/////////////////////////////////////////////////
TEST(PluginTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(InvalidXmlText))
{
  ignition::common::Console::SetVerbosity(4);

  Application app(g_argc, g_argv);
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

  // Load plugin config that returns null GetText
  const char *pluginStr =
    "<plugin filename=\"TestPlugin\">"
      "<ignition-gui>"
        "<title><null>This results in null titleElem->GetText</null></title>"
      "</ignition-gui>"
    "</plugin>";

  tinyxml2::XMLDocument pluginDoc;
  pluginDoc.Parse(pluginStr);
  EXPECT_TRUE(app.LoadPlugin("TestPlugin",
      pluginDoc.FirstChildElement("plugin")));

  auto win = app.findChild<MainWindow *>();
  ASSERT_NE(nullptr, win);

  // Check plugin was loaded and has empty title
  auto plugins = win->findChildren<Plugin *>();
  ASSERT_EQ(1, plugins.size());
  EXPECT_TRUE(plugins[0]->Title().empty());
}

/////////////////////////////////////////////////
TEST(PluginTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(Getters))
{
  ignition::common::Console::SetVerbosity(4);

  Application app(g_argc, g_argv);
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
  app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

  // Load normal plugin
  const char *pluginStr =
    "<plugin filename=\"TestPlugin\">"
    "</plugin>";

  tinyxml2::XMLDocument pluginDoc;
  pluginDoc.Parse(pluginStr);
  EXPECT_TRUE(app.LoadPlugin("TestPlugin",
      pluginDoc.FirstChildElement("plugin")));

  // Create main window
  auto win = app.findChild<MainWindow *>();
  ASSERT_NE(nullptr, win);

  // Check plugin count
  EXPECT_EQ(1, win->findChildren<Plugin *>().size());

  // Get pointers
  auto plugin = win->findChildren<Plugin *>()[0];
  ASSERT_NE(nullptr, plugin->PluginItem());
  ASSERT_NE(nullptr, plugin->CardItem());
  ASSERT_NE(nullptr, plugin->Context());
}
