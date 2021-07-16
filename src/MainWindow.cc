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

#include <tinyxml2.h>
#include <regex>
#include <string>

#include <ignition/common/Console.hh>
#include <ignition/common/Filesystem.hh>
#include "ignition/gui/Application.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/Plugin.hh"
#include "ignition/gui/qt.h"

namespace ignition
{
  namespace gui
  {
    class MainWindowPrivate
    {
      /// \brief Number of plugins on the window
      public: int pluginCount{0};

      /// \brief Pointer to quick window
      public: QQuickWindow *quickWindow{nullptr};

      /// \brief Configuration for this window.
      public: WindowConfig windowConfig;

      /// \brief Counts the times the window has been painted
      public: unsigned int paintCount{0};

      /// \brief Minimum number of paint events to consider the window to be
      /// fully initialized.
      public: const unsigned int paintCountMin{20};

      /// \brief Show the confirmation dialog on exit
      public: bool showDialogOnExit{false};
    };
  }
}

using namespace ignition;
using namespace gui;

/// \brief Strip last component from a path.
/// \return Original path without its last component.
/// \ToDo: Move this function to ignition::common::Filesystem
std::string dirName(const std::string &_path)
{
  std::size_t found = _path.find_last_of("/\\");
  return _path.substr(0, found);
}

/////////////////////////////////////////////////
MainWindow::MainWindow()
  : dataPtr(new MainWindowPrivate)
{
  // Make MainWindow functions available from all QML files (using root)
  App()->Engine()->rootContext()->setContextProperty("MainWindow", this);

  // Load QML and keep pointer to generated QQuickWindow
  std::string qmlFile("qrc:qml/Main.qml");
  App()->Engine()->load(QUrl(QString::fromStdString(qmlFile)));

  this->dataPtr->quickWindow = qobject_cast<QQuickWindow *>(
      App()->Engine()->rootObjects().value(0));
  if (!this->dataPtr->quickWindow)
  {
    ignerr << "Internal error: Failed to instantiate QML file [" << qmlFile
           << "]" << std::endl;
    return;
  }

  App()->setWindowIcon(QIcon(
    common::joinPaths(
      ":", "qml", "images", "ignition_logo_50x50.png").c_str()));
}

/////////////////////////////////////////////////
MainWindow::~MainWindow()
{
}

/////////////////////////////////////////////////
QStringList MainWindow::PluginListModel() const
{
  QStringList pluginNames;
  auto plugins = App()->PluginList();
  for (auto const &path : plugins)
  {
    for (auto const &plugin : path.second)
    {
#ifndef _WIN32
      // Remove lib and .so
      auto pluginName = plugin.substr(3, plugin.find(".") - 3);
#else
      // Remove lib and .dll
      auto pluginName = plugin.substr(0, plugin.find("."));
#endif

      // Split WWWCamelCase3D -> WWW Camel Case 3D
      std::regex reg("(\\B[A-Z][a-z])|(\\B[0-9])");
      pluginName = std::regex_replace(pluginName, reg, " $&");

      // Show?
      if (this->dataPtr->windowConfig.pluginsFromPaths ||
          std::find(this->dataPtr->windowConfig.showPlugins.begin(),
                    this->dataPtr->windowConfig.showPlugins.end(),
                    pluginName) !=
                    this->dataPtr->windowConfig.showPlugins.end())
      {
        pluginNames.append(QString::fromStdString(pluginName));
      }
    }
  }

  // Error
  for (auto plugin : this->dataPtr->windowConfig.showPlugins)
  {
    if (!pluginNames.contains(QString::fromStdString(plugin)))
    {
      ignwarn << "Requested to show plugin [" << plugin <<
          "] but it doesn't exist." << std::endl;
    }
  }

  pluginNames.sort();
  return pluginNames;
}

//////////////////////////////////////////////////
void MainWindow::OnLoadConfig(const QString &_path)
{
  auto localPath = QUrl(_path).toLocalFile();
  if (localPath.isEmpty())
    localPath = _path;

  App()->LoadConfig(localPath.toStdString());
}

/////////////////////////////////////////////////
void MainWindow::OnSaveConfig()
{
  this->SaveConfig(App()->DefaultConfigPath());
}

/////////////////////////////////////////////////
void MainWindow::OnSaveConfigAs(const QString &_path)
{
  auto localPath = QUrl(_path).toLocalFile();
  if (localPath.isEmpty())
    localPath = _path;
  this->SaveConfig(localPath.toStdString());
}

/////////////////////////////////////////////////
void MainWindow::SaveConfig(const std::string &_path)
{
  this->dataPtr->windowConfig = this->CurrentWindowConfig();

  // Create the intermediate directories if needed.
  // We check for errors when we try to open the file.
  auto dirname = dirName(_path);
  ignition::common::createDirectories(dirname);

  // Open the file
  std::ofstream out(_path.c_str(), std::ios::out);
  if (!out)
  {
    std::string str = "Unable to open file: " + _path;
    str += ".\nCheck file permissions.";
    this->notify(QString::fromStdString(str));
  }
  else
    out << this->dataPtr->windowConfig.XMLString();

  std::string msg("Saved configuration to <b>" + _path + "</b>");

  this->notify(QString::fromStdString(msg));
  ignmsg << msg << std::endl;
}

/////////////////////////////////////////////////
void MainWindow::OnAddPlugin(QString _plugin)
{
  auto plugin = _plugin.toStdString();

  // Remove spaces
  plugin.erase(remove_if(plugin.begin(), plugin.end(), isspace), plugin.end());

  ignlog << "Add [" << plugin << "] via menu" << std::endl;

  App()->LoadPlugin(plugin);
}

///////////////////////////////////////////////////
bool MainWindow::ApplyConfig(const WindowConfig &_config)
{
  if (!this->dataPtr->quickWindow)
    return false;

  // Window position
  if (!_config.IsIgnoring("position_x") &&
      !_config.IsIgnoring("position_y") &&
      !_config.IsIgnoring("position") &&
      _config.posX >= 0 && _config.posY >= 0)
  {
//    this->move(_config.posX, _config.posY);
  }

  // Window size
  if (!_config.IsIgnoring("width") &&
      !_config.IsIgnoring("height") &&
      !_config.IsIgnoring("size") &&
      _config.width >= 0 && _config.height >= 0)
  {
    this->QuickWindow()->resize(_config.width, _config.height);
  }

  // Docks state
  if (!_config.IsIgnoring("state") && !_config.state.isEmpty())
  {
//    if (!this->restoreState(_config.state))
//      ignwarn << "Failed to restore state" << std::endl;
  }

  // Style
  if (!_config.IsIgnoring("style"))
  {
    this->SetMaterialTheme(QString::fromStdString(_config.materialTheme));
    this->SetMaterialPrimary(QString::fromStdString(_config.materialPrimary));
    this->SetMaterialAccent(QString::fromStdString(_config.materialAccent));

    this->SetToolBarColorLight(QString::fromStdString(
        _config.toolBarColorLight));
    this->SetToolBarTextColorLight(QString::fromStdString(
        _config.toolBarTextColorLight));
    this->SetToolBarColorDark(QString::fromStdString(_config.toolBarColorDark));
    this->SetToolBarTextColorDark(QString::fromStdString(
        _config.toolBarTextColorDark));

    this->SetPluginToolBarColorLight(QString::fromStdString(
        _config.pluginToolBarColorLight));
    this->SetPluginToolBarTextColorLight(QString::fromStdString(
        _config.pluginToolBarTextColorLight));
    this->SetPluginToolBarColorDark(QString::fromStdString(
        _config.pluginToolBarColorDark));
    this->SetPluginToolBarTextColorDark(QString::fromStdString(
        _config.pluginToolBarTextColorDark));
  }

  // Menus
  this->SetShowDrawer(_config.showDrawer);
  this->SetShowDefaultDrawerOpts(_config.showDefaultDrawerOpts);
  this->SetShowPluginMenu(_config.showPluginMenu);

  // Keep a copy
  this->dataPtr->windowConfig = _config;

  // Notify view
  this->configChanged();

  return true;
}

/////////////////////////////////////////////////
WindowConfig MainWindow::CurrentWindowConfig() const
{
  if (!this->dataPtr->quickWindow)
    return this->dataPtr->windowConfig;

  WindowConfig config;

  // Position
//  config.posX = this->pos().x();
//  config.posY = this->pos().y();

  // Size
  config.width = this->QuickWindow()->width();
  config.height = this->QuickWindow()->height();

  // Docks state
//  config.state = this->saveState();

  // Style
  config.materialTheme = this->QuickWindow()->property("materialTheme")
    .toString().toStdString() == "0" ? "Light" : "Dark";
  config.materialPrimary = this->QuickWindow()->property("materialPrimary")
    .toString().toStdString();
  config.materialAccent =
    this->QuickWindow()->property("materialAccent").toString().toStdString();
  config.toolBarColorLight =
    this->QuickWindow()->property("toolBarColorLight").toString().toStdString();
  config.toolBarTextColorLight = this->QuickWindow()->property(
    "toolBarTextColorLight").toString().toStdString();
  config.toolBarColorDark =
    this->QuickWindow()->property("toolBarColorDark").toString().toStdString();
  config.toolBarTextColorDark = this->QuickWindow()->property(
    "toolBarTextColorDark").toString().toStdString();

  // Menus configuration and ignored properties are kept the same as the
  // initial ones. They might have been changed programatically but we
  // don't guarantee that will be saved.
  config.showDrawer = this->dataPtr->windowConfig.showDrawer;
  config.showDefaultDrawerOpts =
      this->dataPtr->windowConfig.showDefaultDrawerOpts;
  config.showPluginMenu = this->dataPtr->windowConfig.showPluginMenu;
  config.pluginsFromPaths = this->dataPtr->windowConfig.pluginsFromPaths;
  config.showPlugins = this->dataPtr->windowConfig.showPlugins;
  config.ignoredProps = this->dataPtr->windowConfig.ignoredProps;

  // Plugins
  auto plugins = this->findChildren<Plugin *>();
  for (const auto plugin : plugins)
    config.plugins += plugin->ConfigStr();

  return config;
}

/////////////////////////////////////////////////
bool WindowConfig::MergeFromXML(const std::string &_windowXml)
{
  // TinyXml element from string
  tinyxml2::XMLDocument doc;
  doc.Parse(_windowXml.c_str());
  auto winElem = doc.FirstChildElement("window");

  if (!winElem)
    return false;

  // Position
  if (auto xElem = winElem->FirstChildElement("position_x"))
    xElem->QueryIntText(&this->posX);

  if (auto yElem = winElem->FirstChildElement("position_y"))
    yElem->QueryIntText(&this->posY);

  // Size
  if (auto widthElem = winElem->FirstChildElement("width"))
    widthElem->QueryIntText(&this->width);

  if (auto heightElem = winElem->FirstChildElement("height"))
    heightElem->QueryIntText(&this->height);

  // Docks state
  if (auto stateElem = winElem->FirstChildElement("state"))
  {
    auto text = stateElem->GetText();
    if (nullptr != text)
      this->state = QByteArray::fromBase64(text);
  }

  // Style
  if (auto styleElem = winElem->FirstChildElement("style"))
  {
    auto mTheme = styleElem->Attribute("material_theme");
    if (mTheme)
    {
      this->materialTheme = mTheme;
    }
    auto mPrimary = styleElem->Attribute("material_primary");
    if (mPrimary)
    {
      this->materialPrimary = mPrimary;
    }
    auto mAccent = styleElem->Attribute("material_accent");
    if (mAccent)
    {
      this->materialAccent = mAccent;
    }
    auto tbColorLight = styleElem->Attribute("toolbar_color_light");
    if (tbColorLight)
    {
      this->toolBarColorLight = tbColorLight;
    }
    auto tbTextColorLight = styleElem->Attribute("toolbar_text_color_light");
    if (tbTextColorLight)
    {
      this->toolBarTextColorLight = tbTextColorLight;
    }
    auto tbColorDark = styleElem->Attribute("toolbar_color_dark");
    if (tbColorDark)
    {
      this->toolBarColorDark = tbColorDark;
    }
    auto tbTextColorDark = styleElem->Attribute("toolbar_text_color_dark");
    if (tbTextColorDark)
    {
      this->toolBarTextColorDark = tbTextColorDark;
    }
    auto pluginTBColorLight =
        styleElem->Attribute("plugin_toolbar_color_light");
    if (pluginTBColorLight)
    {
      this->pluginToolBarColorLight = pluginTBColorLight;
    }
    auto pluginTBTextColorLight =
        styleElem->Attribute("plugin_toolbar_text_color_light");
    if (pluginTBTextColorLight)
    {
      this->pluginToolBarTextColorLight = pluginTBTextColorLight;
    }
    auto pluginTBColorDark = styleElem->Attribute("plugin_toolbar_color_dark");
    if (pluginTBColorDark)
    {
      this->pluginToolBarColorDark = pluginTBColorDark;
    }
    auto pluginTBTextColorDark =
        styleElem->Attribute("plugin_toolbar_text_color_dark");
    if (pluginTBTextColorDark)
    {
      this->pluginToolBarTextColorDark = pluginTBTextColorDark;
    }
  }

  // Menus
  if (auto menusElem = winElem->FirstChildElement("menus"))
  {
    // Drawer
    if (auto drawerElem = menusElem->FirstChildElement("drawer"))
    {
      // Visible
      if (drawerElem->Attribute("visible"))
      {
        bool visible = true;
        drawerElem->QueryBoolAttribute("visible", &visible);
        this->showDrawer = visible;
      }
      // Default
      if (drawerElem->Attribute("default"))
      {
        bool def = true;
        drawerElem->QueryBoolAttribute("default", &def);
        this->showDefaultDrawerOpts = def;
      }
    }

    // Plugins
    if (auto pluginsElem = menusElem->FirstChildElement("plugins"))
    {
      // Visible
      if (pluginsElem->Attribute("visible"))
      {
        bool visible = true;
        pluginsElem->QueryBoolAttribute("visible", &visible);
        this->showPluginMenu = visible;
      }

      // From paths
      if (pluginsElem->Attribute("from_paths"))
      {
        bool fromPaths = false;
        pluginsElem->QueryBoolAttribute("from_paths", &fromPaths);
        this->pluginsFromPaths = fromPaths;
      }

      // Show individual plugins
      for (auto showElem = pluginsElem->FirstChildElement("show");
          showElem != nullptr;
          showElem = showElem->NextSiblingElement("show"))
      {
        if (auto pluginName = showElem->GetText())
          this->showPlugins.push_back(pluginName);
      }
    }
  }

  // Ignore
  for (auto ignoreElem = winElem->FirstChildElement("ignore");
      ignoreElem != nullptr;
      ignoreElem = ignoreElem->NextSiblingElement("ignore"))
  {
    if (auto prop = ignoreElem->GetText())
      this->ignoredProps.insert(prop);
  }

  return true;
}

/////////////////////////////////////////////////
std::string WindowConfig::XMLString() const
{
  tinyxml2::XMLDocument doc;

  // Window
  auto windowElem = doc.NewElement("window");
  doc.InsertEndChild(windowElem);

  // Position
  if (!this->IsIgnoring("position") && !this->IsIgnoring("position_x"))
  {
    auto elem = doc.NewElement("position_x");
    elem->SetText(std::to_string(this->posX).c_str());
    windowElem->InsertEndChild(elem);
  }

  if (!this->IsIgnoring("position") && !this->IsIgnoring("position_y"))
  {
    auto elem = doc.NewElement("position_y");
    elem->SetText(std::to_string(this->posY).c_str());
    windowElem->InsertEndChild(elem);
  }

  // Docks state
  if (!this->IsIgnoring("state"))
  {
    auto elem = doc.NewElement("state");
    elem->SetText(this->state.toBase64().toStdString().c_str());
    windowElem->InsertEndChild(elem);
  }

  // Size
  if (!this->IsIgnoring("size") && !this->IsIgnoring("width"))
  {
    auto elem = doc.NewElement("width");
    elem->SetText(std::to_string(this->width).c_str());
    windowElem->InsertEndChild(elem);
  }

  if (!this->IsIgnoring("size") && !this->IsIgnoring("height"))
  {
    auto elem = doc.NewElement("height");
    elem->SetText(std::to_string(this->height).c_str());
    windowElem->InsertEndChild(elem);
  }

  // Style
  if (!this->IsIgnoring("style"))
  {
    auto elem = doc.NewElement("style");
    elem->SetAttribute("material_theme", this->materialTheme.c_str());
    elem->SetAttribute("material_primary", this->materialPrimary.c_str());
    elem->SetAttribute("material_accent", this->materialAccent.c_str());

    elem->SetAttribute("toolbar_color_light", this->toolBarColorLight.c_str());
    elem->SetAttribute("toolbar_text_color_light",
        this->toolBarTextColorLight.c_str());
    elem->SetAttribute("toolbar_color_dark", this->toolBarColorDark.c_str());
    elem->SetAttribute("toolbar_text_color_dark",
        this->toolBarTextColorDark.c_str());

    elem->SetAttribute("plugin_toolbar_color_light",
        this->pluginToolBarColorLight.c_str());
    elem->SetAttribute("plugin_toolbar_text_color_light",
        this->pluginToolBarTextColorLight.c_str());
    elem->SetAttribute("plugin_toolbar_color_dark",
        this->pluginToolBarColorDark.c_str());
    elem->SetAttribute("plugin_toolbar_text_color_dark",
        this->pluginToolBarTextColorDark.c_str());

    windowElem->InsertEndChild(elem);
  }

  // Menus
  {
    auto menusElem = doc.NewElement("menus");
    windowElem->InsertEndChild(menusElem);

    // Drawer
    {
      auto elem = doc.NewElement("drawer");

      // Visible
      elem->SetAttribute("visible", this->showDrawer);

      // Default
      elem->SetAttribute("default", this->showDefaultDrawerOpts);

      menusElem->InsertEndChild(elem);
    }

    // Plugins
    {
      auto elem = doc.NewElement("plugins");

      // Visible
      elem->SetAttribute("visible", this->showPluginMenu);

      // From paths
      elem->SetAttribute("from_paths", this->pluginsFromPaths);

      // Show
      for (const auto &show : this->showPlugins)
      {
        auto showElem = doc.NewElement("show");
        showElem->SetText(show.c_str());
        elem->InsertEndChild(showElem);
      }

      menusElem->InsertEndChild(elem);
    }

    windowElem->InsertEndChild(menusElem);
  }

  // Ignored properties
  {
    for (const auto &ignore : this->ignoredProps)
    {
      auto ignoreElem = doc.NewElement("ignore");
      ignoreElem->SetText(ignore.c_str());
      windowElem->InsertEndChild(ignoreElem);
    }
  }

  tinyxml2::XMLPrinter printer;
  doc.Print(&printer);

  std::string config = "<?xml version=\"1.0\"?>\n\n";
  config += printer.CStr();
  config += this->plugins;

  return config;
}

/////////////////////////////////////////////////
bool WindowConfig::IsIgnoring(const std::string &_prop) const
{
  return this->ignoredProps.find(_prop) != this->ignoredProps.end();
}

/////////////////////////////////////////////////
int MainWindow::PluginCount() const
{
  return this->dataPtr->pluginCount;
}

/////////////////////////////////////////////////
void MainWindow::SetPluginCount(const int _pluginCount)
{
  this->dataPtr->pluginCount = _pluginCount;
  this->PluginCountChanged();
}

/////////////////////////////////////////////////
QString MainWindow::MaterialTheme() const
{
  return QString::fromStdString(this->dataPtr->windowConfig.materialTheme);
}

/////////////////////////////////////////////////
void MainWindow::SetMaterialTheme(const QString &_materialTheme)
{
  this->dataPtr->windowConfig.materialTheme = _materialTheme.toStdString();
  this->MaterialThemeChanged();
}

/////////////////////////////////////////////////
QString MainWindow::MaterialPrimary() const
{
  return QString::fromStdString(this->dataPtr->windowConfig.materialPrimary);
}

/////////////////////////////////////////////////
void MainWindow::SetMaterialPrimary(const QString &_materialPrimary)
{
  this->dataPtr->windowConfig.materialPrimary = _materialPrimary.toStdString();
  this->MaterialPrimaryChanged();
}

/////////////////////////////////////////////////
QString MainWindow::MaterialAccent() const
{
  return QString::fromStdString(this->dataPtr->windowConfig.materialAccent);
}

/////////////////////////////////////////////////
void MainWindow::SetMaterialAccent(const QString &_materialAccent)
{
  this->dataPtr->windowConfig.materialAccent = _materialAccent.toStdString();
  this->MaterialAccentChanged();
}

/////////////////////////////////////////////////
QString MainWindow::ToolBarColorLight() const
{
  return QString::fromStdString(this->dataPtr->windowConfig.toolBarColorLight);
}

/////////////////////////////////////////////////
void MainWindow::SetToolBarColorLight(const QString &_toolBarColorLight)
{
  this->dataPtr->windowConfig.toolBarColorLight =
      _toolBarColorLight.toStdString();
  this->ToolBarColorLightChanged();
}

/////////////////////////////////////////////////
QString MainWindow::ToolBarTextColorLight() const
{
  return QString::fromStdString(
      this->dataPtr->windowConfig.toolBarTextColorLight);
}

/////////////////////////////////////////////////
void MainWindow::SetToolBarTextColorLight(const QString &_toolBarTextColorLight)
{
  this->dataPtr->windowConfig.toolBarTextColorLight =
      _toolBarTextColorLight.toStdString();
  this->ToolBarTextColorLightChanged();
}

/////////////////////////////////////////////////
QString MainWindow::ToolBarColorDark() const
{
  return QString::fromStdString(this->dataPtr->windowConfig.toolBarColorDark);
}

/////////////////////////////////////////////////
void MainWindow::SetToolBarColorDark(const QString &_toolBarColorDark)
{
  this->dataPtr->windowConfig.toolBarColorDark =
      _toolBarColorDark.toStdString();
  this->ToolBarColorDarkChanged();
}

/////////////////////////////////////////////////
QString MainWindow::ToolBarTextColorDark() const
{
  return QString::fromStdString(
      this->dataPtr->windowConfig.toolBarTextColorDark);
}

/////////////////////////////////////////////////
void MainWindow::SetToolBarTextColorDark(const QString &_toolBarTextColorDark)
{
  this->dataPtr->windowConfig.toolBarTextColorDark =
      _toolBarTextColorDark.toStdString();
  this->ToolBarTextColorDarkChanged();
}

/////////////////////////////////////////////////
QString MainWindow::PluginToolBarColorLight() const
{
  return QString::fromStdString(
      this->dataPtr->windowConfig.pluginToolBarColorLight);
}

/////////////////////////////////////////////////
void MainWindow::SetPluginToolBarColorLight(
    const QString &_pluginToolBarColorLight)
{
  this->dataPtr->windowConfig.pluginToolBarColorLight =
      _pluginToolBarColorLight.toStdString();
  this->PluginToolBarColorLightChanged();
}

/////////////////////////////////////////////////
QString MainWindow::PluginToolBarTextColorLight() const
{
  return QString::fromStdString(
      this->dataPtr->windowConfig.pluginToolBarTextColorLight);
}

/////////////////////////////////////////////////
void MainWindow::SetPluginToolBarTextColorLight(
    const QString &_pluginToolBarTextColorLight)
{
  this->dataPtr->windowConfig.pluginToolBarTextColorLight =
      _pluginToolBarTextColorLight.toStdString();
  this->PluginToolBarTextColorLightChanged();
}

/////////////////////////////////////////////////
QString MainWindow::PluginToolBarColorDark() const
{
  return QString::fromStdString(
      this->dataPtr->windowConfig.pluginToolBarColorDark);
}

/////////////////////////////////////////////////
void MainWindow::SetPluginToolBarColorDark(
    const QString &_pluginToolBarColorDark)
{
  this->dataPtr->windowConfig.pluginToolBarColorDark =
      _pluginToolBarColorDark.toStdString();
  this->PluginToolBarColorDarkChanged();
}

/////////////////////////////////////////////////
QString MainWindow::PluginToolBarTextColorDark() const
{
  return QString::fromStdString(
      this->dataPtr->windowConfig.pluginToolBarTextColorDark);
}

/////////////////////////////////////////////////
void MainWindow::SetPluginToolBarTextColorDark(
    const QString &_pluginToolBarTextColorDark)
{
  this->dataPtr->windowConfig.pluginToolBarTextColorDark =
      _pluginToolBarTextColorDark.toStdString();
  this->PluginToolBarTextColorDarkChanged();
}

/////////////////////////////////////////////////
QQuickWindow *MainWindow::QuickWindow() const
{
  return this->dataPtr->quickWindow;
}

/////////////////////////////////////////////////
bool MainWindow::ShowDrawer() const
{
  return this->dataPtr->windowConfig.showDrawer;
}

/////////////////////////////////////////////////
void MainWindow::SetShowDrawer(const bool _showDrawer)
{
  this->dataPtr->windowConfig.showDrawer = _showDrawer;
  this->ShowDrawerChanged();
}

/////////////////////////////////////////////////
bool MainWindow::ShowDefaultDrawerOpts() const
{
  return this->dataPtr->windowConfig.showDefaultDrawerOpts;
}

/////////////////////////////////////////////////
void MainWindow::SetShowDefaultDrawerOpts(const bool _showDefaultDrawerOpts)
{
  this->dataPtr->windowConfig.showDefaultDrawerOpts =
      _showDefaultDrawerOpts;
  this->ShowDefaultDrawerOptsChanged();
}

/////////////////////////////////////////////////
bool MainWindow::ShowPluginMenu() const
{
  return this->dataPtr->windowConfig.showPluginMenu;
}

/////////////////////////////////////////////////
void MainWindow::SetShowPluginMenu(const bool _showPluginMenu)
{
  this->dataPtr->windowConfig.showPluginMenu = _showPluginMenu;
  this->ShowPluginMenuChanged();
}

/////////////////////////////////////////////////
bool MainWindow::ShowDialogOnExit() const
{
  return this->dataPtr->showDialogOnExit;
}

/////////////////////////////////////////////////
void MainWindow::SetShowDialogOnExit(bool _showDialogOnExit)
{
  this->dataPtr->showDialogOnExit = _showDialogOnExit;
  this->ShowDialogOnExitChanged();
}
