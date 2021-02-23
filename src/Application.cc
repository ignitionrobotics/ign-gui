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

#include <tinyxml2.h>
#include <queue>

#include <ignition/common/Console.hh>
#include <ignition/common/SignalHandler.hh>
#include <ignition/common/SystemPaths.hh>
#include <ignition/common/Util.hh>

#include <ignition/plugin/Loader.hh>

#include "ignition/gui/Application.hh"
#include "ignition/gui/config.hh"
#include "ignition/gui/Dialog.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/Plugin.hh"

namespace ignition
{
  namespace gui
  {
    class ApplicationPrivate
    {
      /// \brief QML engine
      public: QQmlApplicationEngine *engine{nullptr};

      /// \brief Pointer to main window
      public: MainWindow *mainWin{nullptr};

      /// \brief Vector of pointers to dialogs
      public: std::vector<Dialog *> dialogs;

      /// \brief Queue of plugins which should be added to the window
      public: std::queue<std::shared_ptr<Plugin>> pluginsToAdd;

      /// \brief Vector of pointers to plugins already added, we hang on to
      /// these until it is ok to unload the plugin's shared library.
      public: std::vector<std::shared_ptr<Plugin>> pluginsAdded;

      /// \brief Environment variable which holds paths to look for plugins
      public: std::string pluginPathEnv = "IGN_GUI_PLUGIN_PATH";

      /// \brief Vector of paths to look for plugins
      public: std::vector<std::string> pluginPaths;

      /// \brief Holds a configuration which may be applied to mainWin once it
      /// is created by calling applyConfig(). It's no longer needed and
      /// should not be used after that.
      public: WindowConfig windowConfig;

      /// \brief The path containing the default configuration file.
      public: std::string defaultConfigPath;

      public: common::SignalHandler signalHandler;

      /// \brief QT message handler that pipes qt messages into our console
      /// system.
      public: static void MessageHandler(QtMsgType _type,
          const QMessageLogContext &_context, const QString &_msg);
    };
  }
}

using namespace ignition;
using namespace gui;

/////////////////////////////////////////////////
Application::Application(int &_argc, char **_argv, const WindowType _type)
  : QApplication(_argc, _argv), dataPtr(new ApplicationPrivate)
{
  igndbg << "Initializing application." << std::endl;

  // Configure console
  common::Console::SetPrefix("[GUI] ");

  // QML engine
  this->dataPtr->engine = new QQmlApplicationEngine();

  // Install signal handler for graceful shutdown
  this->dataPtr->signalHandler.AddCallback(
      [](int)  // NOLINT(readability/casting)
      {
          App()->quit();
      });

  // Handle qt console messages
  qInstallMessageHandler(this->dataPtr->MessageHandler);

  // Default config path
  std::string home;
  common::env(IGN_HOMEDIR, home);
  this->dataPtr->defaultConfigPath = common::joinPaths(
        home, ".ignition", "gui", "default.config");

  // If it's a main window, initialize it
  if (_type == WindowType::kMainWindow)
  {
    if (!this->InitializeMainWindow())
      ignerr << "Failed to initialize main window." << std::endl;
  }
  else if (_type == WindowType::kDialog)
  {
    // Do nothing, dialogs are initialized as plugins are loaded
  }
  else
  {
    ignerr << "Unknown WindowType [" << static_cast<int>(_type) << "]\n";
  }
}

/////////////////////////////////////////////////
Application::~Application()
{
  igndbg << "Terminating application." << std::endl;

  if (this->dataPtr->mainWin && this->dataPtr->mainWin->QuickWindow())
  {
    // Detach object from main window and leave libraries for ign-common
    auto plugins = this->dataPtr->mainWin->findChildren<Plugin *>();
    for (auto plugin : plugins)
    {
      auto pluginName = plugin->CardItem()->objectName();
      this->RemovePlugin(pluginName.toStdString());
    }
    if (this->dataPtr->mainWin->QuickWindow()->isVisible())
      this->dataPtr->mainWin->QuickWindow()->close();
    delete this->dataPtr->mainWin;
    this->dataPtr->mainWin = nullptr;
  }

  for (auto dialog : this->dataPtr->dialogs)
  {
    if (dialog->QuickWindow())
      dialog->QuickWindow()->close();
    dialog->deleteLater();
  }
  this->dataPtr->dialogs.clear();

  if (this->dataPtr->engine)
  {
    this->dataPtr->engine->deleteLater();
  }

  std::queue<std::shared_ptr<Plugin>> empty;
  std::swap(this->dataPtr->pluginsToAdd, empty);
  this->dataPtr->pluginsAdded.clear();
  this->dataPtr->pluginPaths.clear();
  this->dataPtr->pluginPathEnv = "IGN_GUI_PLUGIN_PATH";
}

/////////////////////////////////////////////////
QQmlApplicationEngine *Application::Engine() const
{
  return this->dataPtr->engine;
}

/////////////////////////////////////////////////
Application *ignition::gui::App()
{
  return qobject_cast<Application *>(qGuiApp);
}

/////////////////////////////////////////////////
bool Application::RemovePlugin(const std::string &_pluginName)
{
  bool found{false};
  for (auto plugin : this->dataPtr->pluginsAdded)
  {
    auto cardItem = plugin->CardItem();
    if (!cardItem)
      continue;

    if (cardItem->objectName().toStdString() == _pluginName)
    {
      // Remove on QML
      cardItem->deleteLater();

      // Remove split on QML
      auto bgItem = this->dataPtr->mainWin->QuickWindow()
          ->findChild<QQuickItem *>("background");
      if (bgItem)
      {
        QMetaObject::invokeMethod(bgItem, "removeSplitItem",
            Q_ARG(QVariant, cardItem->parentItem()->objectName()));
      }

      // Unload shared library
      this->RemovePlugin(plugin);

      found = true;
      break;
    }
  }

  return found;
}

/////////////////////////////////////////////////
bool Application::LoadConfig(const std::string &_config)
{
  if (_config.empty())
  {
    ignerr << "Missing config file" << std::endl;
    return false;
  }

  // Use tinyxml to read config
  tinyxml2::XMLDocument doc;
  auto success = !doc.LoadFile(_config.c_str());
  if (!success)
  {
    // We do not show an error message if the default config path doesn't exist
    // yet. It's expected behavior, it will be created the first time the user
    // presses "Save configuration".
    if (_config != this->DefaultConfigPath())
    {
      ignerr << "Failed to load file [" << _config << "]: XMLError"
             << std::endl;
    }

    return false;
  }

  ignmsg << "Loading config [" << _config << "]" << std::endl;

  // Clear all previous plugins
  auto plugins = this->dataPtr->mainWin->findChildren<Plugin *>();
  for (auto plugin : plugins)
  {
    auto pluginName = plugin->CardItem()->objectName();
    this->RemovePlugin(pluginName.toStdString());
  }
  if (this->dataPtr->pluginsAdded.size() > 0)
  {
    ignerr << "The plugin list was not properly cleaned up." << std::endl;
  }
  this->dataPtr->pluginsAdded.clear();

  // Process each plugin
  for (auto pluginElem = doc.FirstChildElement("plugin"); pluginElem != nullptr;
      pluginElem = pluginElem->NextSiblingElement("plugin"))
  {
    auto filename = pluginElem->Attribute("filename");
    this->LoadPlugin(filename, pluginElem);
  }

  // Process window properties
  if (auto winElem = doc.FirstChildElement("window"))
  {
    igndbg << "Loading window config" << std::endl;

    tinyxml2::XMLPrinter printer;
    if (!winElem->Accept(&printer))
    {
      ignwarn << "There was an error parsing the <window> element"
              << std::endl;
      return false;
    }
    this->dataPtr->windowConfig.MergeFromXML(std::string(printer.CStr()));
  }

  this->ApplyConfig();

  return true;
}

/////////////////////////////////////////////////
bool Application::LoadDefaultConfig()
{
  return this->LoadConfig(this->dataPtr->defaultConfigPath);
}

/////////////////////////////////////////////////
void Application::SetDefaultConfigPath(const std::string &_path)
{
  this->dataPtr->defaultConfigPath = _path;
}

/////////////////////////////////////////////////
std::string Application::DefaultConfigPath()
{
  return this->dataPtr->defaultConfigPath;
}

/////////////////////////////////////////////////
bool Application::LoadPlugin(const std::string &_filename,
    const tinyxml2::XMLElement *_pluginElem)
{
  igndbg << "Loading plugin [" << _filename << "]" << std::endl;

  common::SystemPaths systemPaths;
  systemPaths.SetPluginPathEnv(this->dataPtr->pluginPathEnv);
  systemPaths.AddPluginPaths("./" + std::string(IGN_GUI_PLUGIN_PATH));

  for (const auto &path : this->dataPtr->pluginPaths)
    systemPaths.AddPluginPaths(path);

  // Add default folder and install folder
  std::string home;
  common::env(IGN_HOMEDIR, home);
  systemPaths.AddPluginPaths(home + "/.ignition/gui/plugins:" +
                            IGN_GUI_PLUGIN_INSTALL_DIR);

  auto pathToLib = systemPaths.FindSharedLibrary(_filename);
  if (pathToLib.empty())
  {
    ignerr << "Failed to load plugin [" << _filename <<
              "] : couldn't find shared library." << std::endl;
    return false;
  }

  // Load plugin
  plugin::Loader pluginLoader;

  auto pluginNames = pluginLoader.LoadLib(pathToLib);
  if (pluginNames.empty())
  {
    ignerr << "Failed to load plugin [" << _filename <<
              "] : couldn't load library on path [" << pathToLib <<
              "]." << std::endl;
    return false;
  }
  // Go over all plugin names and get the first one that implements the
  // ignition::gui::Plugin interface
  plugin::PluginPtr commonPlugin;
  std::shared_ptr<gui::Plugin> plugin{nullptr};
  for (auto pluginName : pluginNames)
  {
    commonPlugin = pluginLoader.Instantiate(pluginName);
    if (!commonPlugin)
      continue;

    plugin = commonPlugin->QueryInterfaceSharedPtr<ignition::gui::Plugin>();
    if (plugin)
      break;
  }

  if (!commonPlugin)
  {
    ignerr << "Failed to load plugin [" << _filename <<
              "] : couldn't instantiate plugin on path [" << pathToLib <<
              "]. Tried plugin names: " << std::endl;

    for (auto pluginName : pluginNames)
    {
      ignerr << " * " << pluginName << std::endl;
    }
    return false;
  }

  if (!plugin)
  {
    ignerr << "Failed to load plugin [" << _filename <<
              "] : couldn't get [ignition::gui::Plugin] interface."
           << std::endl;
    return false;
  }

  // Basic config in case there is none
  if (!_pluginElem)
  {
    std::string pluginStr = "<plugin filename=\"" + _filename + "\"></plugin>";

    tinyxml2::XMLDocument pluginDoc;
    pluginDoc.Parse(pluginStr.c_str());

    plugin->Load(pluginDoc.FirstChildElement("plugin"));
  }
  else
    plugin->Load(_pluginElem);

  // Store plugin in queue to be added to the window
  this->dataPtr->pluginsToAdd.push(plugin);

  // Add to window or dialog
  if (this->dataPtr->mainWin)
    this->AddPluginsToWindow();
  else
    this->InitializeDialogs();

  this->PluginAdded(plugin->objectName());
  ignmsg << "Loaded plugin [" << _filename << "] from path [" << pathToLib
         << "]" << std::endl;

  return true;
}

/////////////////////////////////////////////////
bool Application::InitializeMainWindow()
{
  igndbg << "Create main window" << std::endl;

  this->dataPtr->mainWin = new MainWindow();
  if (!this->dataPtr->mainWin->QuickWindow())
    return false;

  this->dataPtr->mainWin->setParent(this);

  return true;
}

/////////////////////////////////////////////////
bool Application::ApplyConfig()
{
  igndbg << "Applying config" << std::endl;

  if (!this->dataPtr->mainWin)
    return false;

  return this->dataPtr->mainWin->ApplyConfig(this->dataPtr->windowConfig);
}

/////////////////////////////////////////////////
bool Application::AddPluginsToWindow()
{
  if (!this->dataPtr->mainWin || !this->dataPtr->mainWin->QuickWindow())
    return false;

  // Get main window background item
  auto bgItem = this->dataPtr->mainWin->QuickWindow()
      ->findChild<QQuickItem *>("background");
  if (!this->dataPtr->pluginsToAdd.empty() && !bgItem)
  {
    ignerr << "Null background QQuickItem!" << std::endl;
    return false;
  }

  // Create a widget for each plugin
  while (!this->dataPtr->pluginsToAdd.empty())
  {
    auto plugin = this->dataPtr->pluginsToAdd.front();

    this->dataPtr->pluginsAdded.push_back(plugin);
    this->dataPtr->pluginsToAdd.pop();

    if (plugin->DeleteLaterRequested())
    {
      this->RemovePlugin(plugin);
      continue;
    }

    auto cardItem = plugin->CardItem();
    if (!cardItem)
      continue;

    // Add split item
    QVariant splitName;
    QMetaObject::invokeMethod(bgItem, "addSplitItem",
        Q_RETURN_ARG(QVariant, splitName));

    auto splitItem = bgItem->findChild<QQuickItem *>(
        splitName.toString());
    if (!splitItem)
    {
      ignerr << "Internal error: failed to create split ["
             << splitName.toString().toStdString() << "]" << std::endl;
      return false;
    }

    // Add card to main window
    cardItem->setParentItem(splitItem);
    cardItem->setParent(this->dataPtr->engine);
    plugin->setParent(this->dataPtr->mainWin);

    // Apply anchors and state changes now that it's attached to window
    plugin->PostParentChanges();

    // Signals
    this->dataPtr->mainWin->connect(cardItem, SIGNAL(close()),
        this, SLOT(OnPluginClose()));

    ignmsg << "Added plugin [" << plugin->Title() << "] to main window" <<
        std::endl;
  }

  this->dataPtr->mainWin->SetPluginCount(this->dataPtr->pluginsAdded.size());

  return true;
}

/////////////////////////////////////////////////
bool Application::InitializeDialogs()
{
  igndbg << "Initialize dialogs" << std::endl;

  while (!this->dataPtr->pluginsToAdd.empty())
  {
    auto plugin = this->dataPtr->pluginsToAdd.front();
    this->dataPtr->pluginsToAdd.pop();

    // Create card
    auto cardItem = plugin->CardItem();
    if (!cardItem)
      continue;

    // Create dialog
    auto dialog = new Dialog();
    if (!dialog || !dialog->QuickWindow())
      continue;

    this->dataPtr->dialogs.push_back(dialog);

    // Configure card
    cardItem->setProperty("standalone", true);

    // Configure dialog
    auto cardWidth = cardItem->property("width").toInt();
    auto cardHeight = cardItem->property("height").toInt();
    dialog->QuickWindow()->setProperty("minimumWidth", cardWidth);
    dialog->QuickWindow()->setProperty("minimumHeight", cardHeight);

    cardItem->setParentItem(dialog->RootItem());

    // Signals
    this->dataPtr->mainWin->connect(cardItem, SIGNAL(close()),
        this, SLOT(OnPluginClose()));

    this->dataPtr->pluginsAdded.push_back(plugin);

    auto title = QString::fromStdString(plugin->Title());
    igndbg << "Initialized dialog [" << title.toStdString() << "]" << std::endl;
  }

  if (this->dataPtr->pluginsAdded.empty())
    return false;

  return true;
}

/////////////////////////////////////////////////
void Application::SetPluginPathEnv(const std::string &_env)
{
  this->dataPtr->pluginPathEnv = _env;
}

/////////////////////////////////////////////////
void Application::AddPluginPath(const std::string &_path)
{
  this->dataPtr->pluginPaths.push_back(_path);
}

/////////////////////////////////////////////////
std::vector<std::pair<std::string, std::vector<std::string>>>
    Application::PluginList()
{
  // 1. Paths from env variable
  auto paths = common::SystemPaths::PathsFromEnv(this->dataPtr->pluginPathEnv);

  // 2. Paths added by calling addPluginPath
  for (auto const &path : this->dataPtr->pluginPaths)
    paths.push_back(path);

  // 3. ~/.ignition/gui/plugins
  std::string home;
  common::env(IGN_HOMEDIR, home);
  paths.push_back(home + "/.ignition/gui/plugins");
  paths.push_back(common::cwd() + "/" + std::string(IGN_GUI_PLUGIN_PATH));
  
  // 4. Install path
  paths.push_back(IGN_GUI_PLUGIN_INSTALL_DIR);

  // Populate map
  std::vector<std::pair<std::string, std::vector<std::string>>> plugins;

  for (auto const &path : paths)
  {
    std::vector<std::string> ps;

    common::DirIter endIter;
    for (common::DirIter dirIter(path);
        dirIter != endIter; ++dirIter)
    {
      auto plugin = common::basename(*dirIter);

      // All we verify is that the file starts with "lib", any further
      // checks would require loading the plugin.

      if (plugin.find("lib") == 0)
        ps.push_back(plugin);
    }

    plugins.push_back(std::make_pair(path, ps));
  }

  return plugins;
}

/////////////////////////////////////////////////
void Application::OnPluginClose()
{
  auto pluginName = this->sender()->objectName();
  this->RemovePlugin(pluginName.toStdString());
}

/////////////////////////////////////////////////
void Application::RemovePlugin(std::shared_ptr<Plugin> _plugin)
{
  this->dataPtr->pluginsAdded.erase(std::remove(
      this->dataPtr->pluginsAdded.begin(),
      this->dataPtr->pluginsAdded.end(), _plugin),
      this->dataPtr->pluginsAdded.end());

  auto pluginCount = this->dataPtr->pluginsAdded.size();

  // Update main window count
  if (this->dataPtr->mainWin)
  {
    this->dataPtr->mainWin->SetPluginCount(pluginCount);
  }
  // Or close app if it's the last dialog
  else if (pluginCount == 0)
  {
    this->exit();
  }
}

//////////////////////////////////////////////////
void ApplicationPrivate::MessageHandler(QtMsgType _type,
    const QMessageLogContext &_context, const QString &_msg)
{
  std::string msg = "[QT] " + _msg.toStdString();
  if (_context.function)
    msg += std::string("(") + _context.function + ")";

  switch (_type)
  {
    case QtDebugMsg:
      igndbg << msg << std::endl;
      break;
    case QtInfoMsg:
      ignmsg << msg << std::endl;
      break;
    case QtWarningMsg:
      ignwarn << msg << std::endl;
      break;
    case QtFatalMsg:
    case QtCriticalMsg:
      ignerr << msg << std::endl;
      break;
    default:
      ignwarn << "Unknown QT Message type[" << _type << "]: "
        << msg << std::endl;
      break;
  }
}
