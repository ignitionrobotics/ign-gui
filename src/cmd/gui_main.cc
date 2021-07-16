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

#include <ignition/common/Console.hh>
#include <ignition/common/Util.hh>

#include <ignition/utils/cli/CLI.hpp>

#include "ignition/gui/config.hh"
#include "ign.hh"

//////////////////////////////////////////////////
/// \brief Enumeration of available commands
enum class GUICommand
{
  kNone,
  kGUIConfig,
  kGUIHelp,
  kGUIList,
  kGUIStandalone
};

//////////////////////////////////////////////////
/// \brief Structure to hold all available topic options
struct GUIOptions
{
  /// \brief Command to execute
  GUICommand command{GUICommand::kNone};

  /// \brief Plugin name to open
  std::string pluginName;

  /// \brief Config file to load
  std::string configFile;

  /// Verbosity level
  int verboseLevel = 1;
};

void runGUICommand(const GUIOptions &_opt)
{
  cmdVerbose(_opt.verboseLevel);

  switch(_opt.command)
  {
    case GUICommand::kGUIList:
      cmdPluginList();
      break;
    case GUICommand::kGUIConfig:
      cmdConfig(_opt.configFile.c_str());
      break;
    case GUICommand::kGUIStandalone:
      cmdStandalone(_opt.pluginName.c_str());
      break;
    case GUICommand::kNone:
    default:
      cmdEmptyWindow();
  }
}

void addGUIFlags(CLI::App &_app)
{
  auto opt = std::make_shared<GUIOptions>();

  _app.add_option("-v,--verbose",
                  opt->verboseLevel,
                  "Adjust the level of console output (0~4).\n"
                  "The default verbosity is 1, use -v without \n"
                  "arguments for level 3.");

  _app.add_option("-s,--standalone",
                   opt->pluginName,
                   "Run a plugin as a standalone window.\n"
                   "Give the plugin filename as an argument.");

   _app.add_option("-c,--config",
                    opt->configFile,
                    "Open the main window with a configuration file.\n"
                    "Give the configuration file path as an argument.");

  _app.add_flag_callback("-l,--list",
      [opt](){
        opt->command = GUICommand::kGUIList;
      }, "List available GUI plugins");

  _app.callback([&_app, opt](){
    if (!opt->pluginName.empty())
    {
      opt->command = GUICommand::kGUIStandalone;
    }
    if (!opt->configFile.empty())
    {
      opt->command = GUICommand::kGUIConfig;
    }
    runGUICommand(*opt);
  });
}

//////////////////////////////////////////////////
int main(int argc, char** argv)
{
  CLI::App app{"Introspect Ignition GUI"};

  app.set_help_all_flag("--help-all", "Show all help");

  app.add_flag_callback("--version", [](){
      std::cout << ignitionVersion() << std::endl;
      throw CLI::Success();
  });

  addGUIFlags(app);
  CLI11_PARSE(app, argc, argv);
}
