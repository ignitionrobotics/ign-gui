/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#include <stdlib.h>
#include <gtest/gtest.h>
#include <ignition/common/Console.hh>
#include <ignition/utilities/ExtraTestMacros.hh>

#include "test_config.h"  // NOLINT(build/include)
#include "ignition/gui/Application.hh"
#include "ignition/gui/Dialog.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/Plugin.hh"

int g_argc = 1;
char **g_argv = new char *[g_argc];

using namespace ignition;
using namespace gui;

// See https://github.com/ignitionrobotics/ign-gui/issues/75
//////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(Constructor))
{
  ignition::common::Console::SetVerbosity(4);

  // No Qt app
  EXPECT_EQ(nullptr, qGuiApp);
  EXPECT_EQ(nullptr, App());

  // One app construct - destruct
  {
    Application app(g_argc, g_argv);

    EXPECT_NE(nullptr, qGuiApp);
    ASSERT_NE(nullptr, App());
    EXPECT_NE(nullptr, App()->Engine());

    // No crash if argc and argv were correctly set
    QCoreApplication::arguments();
  }

  // No Qt app
  EXPECT_EQ(nullptr, qGuiApp);
  EXPECT_EQ(nullptr, App());
}

//////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(LoadPlugin))
{
  ignition::common::Console::SetVerbosity(4);

  // No Qt app
  EXPECT_EQ(nullptr, qGuiApp);

  // Official plugin
  {
    Application app(g_argc, g_argv);

    EXPECT_TRUE(app.LoadPlugin("Publisher"));
  }

  // Inexistent plugin
  {
    Application app(g_argc, g_argv);

    EXPECT_FALSE(app.LoadPlugin("_doesnt_exist"));
  }

  // Plugin path added programmatically
  {
    Application app(g_argc, g_argv);
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

    EXPECT_TRUE(app.LoadPlugin("TestPlugin"));
  }

  // Plugin path added by env var
  {
    auto env_var = std::string(PROJECT_BINARY_PATH);

    setenv("TEST_ENV_VAR", env_var.c_str(), 1);
    Application app(g_argc, g_argv);
    app.SetPluginPathEnv("TEST_ENV_VAR");

    EXPECT_TRUE(app.LoadPlugin("TestPlugin"));
  }

  // Plugin which doesn't inherit from ignition::gui::Plugin
  {
    Application app(g_argc, g_argv);
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

    EXPECT_FALSE(app.LoadPlugin("TestBadInheritancePlugin"));
  }

  // Plugin which is not registered
  {
    Application app(g_argc, g_argv);
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH) + "/lib");

    EXPECT_FALSE(app.LoadPlugin("TestNotRegisteredPlugin"));
  }
}

//////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(LoadConfig))
{
  ignition::common::Console::SetVerbosity(4);

  EXPECT_EQ(nullptr, qGuiApp);

  // Empty string
  {
    Application app(g_argc, g_argv);

    EXPECT_FALSE(app.LoadConfig(""));
  }

  // Test config file
  {
    Application app(g_argc, g_argv);

    // Add test plugin to path (referenced in config)
    auto testBuildPath = std::string(PROJECT_BINARY_PATH) + "/lib/";
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
    app.AddPluginPath(testBuildPath);

    // Load test config file
    auto testSourcePath = std::string(PROJECT_SOURCE_PATH) + "/test/";
    EXPECT_TRUE(app.LoadConfig(testSourcePath + "config/test.config"));
  }
}

//////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(LoadDefaultConfig))
{
  ignition::common::Console::SetVerbosity(4);

  EXPECT_EQ(nullptr, qGuiApp);

  // Test config file
  {
    Application app(g_argc, g_argv);

    // Add test plugin to path (referenced in config)
    auto testBuildPath = ignition::common::joinPaths(
      std::string(PROJECT_BINARY_PATH), "lib");
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
    app.AddPluginPath(testBuildPath);

    // Set default config file
    auto configPath = ignition::common::joinPaths(
      std::string(PROJECT_SOURCE_PATH), "test", "config", "test.config");
    app.SetDefaultConfigPath(configPath);

    EXPECT_EQ(app.DefaultConfigPath(), configPath);
  }
}

//////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(InitializeMainWindow))
{
  ignition::common::Console::SetVerbosity(4);

  EXPECT_EQ(nullptr, qGuiApp);

  // No plugins
  {
    Application app(g_argc, g_argv);

    auto wins = app.allWindows();
    ASSERT_EQ(wins.size(), 1);

    // Close window after some time
    QTimer::singleShot(300, wins[0], SLOT(close()));

    // Show window
    app.exec();
  }

  // Load plugin
  {
    Application app(g_argc, g_argv);

    EXPECT_TRUE(app.LoadPlugin("Publisher"));

    auto win = App()->findChild<MainWindow *>();
    ASSERT_NE(nullptr, win);

    // Check plugin count
    auto plugins = win->findChildren<Plugin *>();
    EXPECT_EQ(1, plugins.count());

    // Close window after some time
    QTimer::singleShot(300, win->QuickWindow(), SLOT(close()));

    // Show window
    app.exec();
  }

  // Test config
  auto testBuildPath = std::string(PROJECT_BINARY_PATH) + "/lib/";
  auto testSourcePath = std::string(PROJECT_SOURCE_PATH) + "/test/";

  // Load config
  {
    Application app(g_argc, g_argv);

    // Add test plugin to path (referenced in config)
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));
    app.AddPluginPath(testBuildPath);

    // Load test config file
    EXPECT_TRUE(app.LoadConfig(testSourcePath + "config/test.config"));

    auto win = App()->findChild<MainWindow *>();
    ASSERT_NE(nullptr, win);

    // Check plugin count
    auto plugins = win->findChildren<Plugin *>();
    EXPECT_EQ(1, plugins.count());

    // Close window after some time
    QTimer::singleShot(300, win->QuickWindow(), SLOT(close()));

    // Show window
    app.exec();
  }
}

//////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(Dialog))
{
  ignition::common::Console::SetVerbosity(4);

  EXPECT_EQ(nullptr, qGuiApp);

  // Single dialog
  {
    Application app(g_argc, g_argv, WindowType::kDialog);
    EXPECT_EQ(app.allWindows().size(), 0);

    // Add test plugin to path
    auto testBuildPath = std::string(PROJECT_BINARY_PATH) + "/lib/";
    app.AddPluginPath(testBuildPath);
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));

    // Load plugin
    EXPECT_TRUE(app.LoadPlugin("TestPlugin"));

    // Close dialog after some time
    auto closed = false;
    QTimer::singleShot(300, [&] {
      auto ds = app.allWindows();

      // The main dialog - some systems return more, not sure why
      ASSERT_GE(ds.size(), 1);

      EXPECT_TRUE(qobject_cast<QQuickWindow *>(ds[0]));

      // Close
      ds[0]->close();
      closed = true;
    });

    // Exec dialog
    app.exec();

    // Make sure timer was triggered
    EXPECT_TRUE(closed);
  }

  // Multiple dialogs
  {
    Application app(g_argc, g_argv, WindowType::kDialog);
    EXPECT_EQ(app.allWindows().size(), 0);

    // Add test plugin to path
    auto testBuildPath = std::string(PROJECT_BINARY_PATH) + "/lib/";
    app.AddPluginPath(testBuildPath);
    app.AddPluginPath(std::string(PROJECT_BINARY_PATH));

    // Load plugins
    EXPECT_TRUE(app.LoadPlugin("TestPlugin"));
    EXPECT_TRUE(app.LoadPlugin("TestPlugin"));

    // Close dialogs after some time
    auto closed = false;
    QTimer::singleShot(300, [&] {
      auto ds = app.allWindows();

      // 2 dialog - some systems return more, not sure why
      EXPECT_GE(ds.size(), 2);

      for (auto dialog : ds)
        dialog->close();
      closed = true;
    });

    // Exec dialog
    app.exec();

    // Make sure timer was triggered
    EXPECT_TRUE(closed);
  }
}

/////////////////////////////////////////////////
TEST(ApplicationTest, IGN_UTILS_TEST_DISABLED_ON_WIN32(messageHandler))
{
  ignition::common::Console::SetVerbosity(4);

  EXPECT_EQ(nullptr, qGuiApp);

  Application app(g_argc, g_argv);

  // \todo Verify output, see ignition::commmon::Console_TEST for example
  qDebug("This came from qDebug");
  qInfo("This came from qInfo");
  qWarning("This came from qWarning");
  qCritical("This came from qCritical");
}
