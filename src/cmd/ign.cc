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

#include <string.h>

#include <iostream>
#include <string>

#include <ignition/common/Console.hh>

#include "ignition/gui/Application.hh"
#include "ignition/gui/config.hh"
#include "ign.hh"
#include "ignition/gui/MainWindow.hh"

int g_argc = 1;
char* g_argv[] =
{
  reinterpret_cast<char*>(const_cast<char*>("./ign_TEST")),
};

//////////////////////////////////////////////////
extern "C" char *ignitionVersion()
{
  return strdup(IGNITION_GUI_VERSION_FULL);
}

//////////////////////////////////////////////////
extern "C" void cmdPluginList()
{
  ignition::gui::Application app(g_argc, g_argv);

  auto pluginsList = app.PluginList();
  for (auto const &path : pluginsList)
  {
    std::cout << path.first << std::endl;

    for (unsigned int i = 0; i < path.second.size(); ++i)
    {
      if (i == path.second.size() - 1)
#ifndef _WIN32
        std::cout << "└── " << path.second[i] << std::endl;
#else
        std::cout << " -- " << path.second[i] << std::endl;
#endif
      else
#ifndef _WIN32
        std::cout << "├── " << path.second[i] << std::endl;
#else
        std::cout << "|-- " << path.second[i] << std::endl;
#endif
    }

    if (path.second.empty())
    {
#ifndef _WIN32
      std::cout << "└── No plugins" << std::endl;
#else
      std::cout << " -- No plugins" << std::endl;
#endif
    }
  }
}

//////////////////////////////////////////////////
extern "C" void cmdStandalone(const char *_filename)
{
  ignition::gui::Application app(g_argc, g_argv,
      ignition::gui::WindowType::kDialog);

  if (!app.LoadPlugin(_filename))
  {
    return;
  }

  app.exec();
}

//////////////////////////////////////////////////
extern "C" void cmdConfig(const char *_config)
{
  ignition::gui::Application app(g_argc, g_argv);

  if (!app.findChild<ignition::gui::MainWindow *>())
  {
    return;
  }

  if (!app.LoadConfig(std::string(_config)))
  {
    return;
  }

  app.exec();
}

//////////////////////////////////////////////////
extern "C" void cmdVerbose(int _verbosity)
{
  ignition::common::Console::SetVerbosity(_verbosity);
}

//////////////////////////////////////////////////
extern "C" void cmdEmptyWindow()
{
  ignition::gui::Application app(g_argc, g_argv);

  if (!app.findChild<ignition::gui::MainWindow *>())
  {
    return;
  }

  app.LoadDefaultConfig();

  app.exec();
}

//////////////////////////////////////////////////
extern "C" void cmdSetStyleFromFile(
    const char * /*_filename*/)
{
//  ignition::gui::setStyleFromFile(std::string(_filename));
}
