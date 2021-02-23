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

#include <unordered_set>

#include <ignition/common/Console.hh>
#include "ignition/gui/Application.hh"
#include "ignition/gui/Helpers.hh"
#include "ignition/gui/MainWindow.hh"
#include "ignition/gui/Plugin.hh"

/// \brief Used to store information about anchors set by the user.
struct Anchors
{
  /// \brief Name of target item, which can be "window" or the
  /// title of another plugin.
  std::string target;

  /// \brief Vector of anchor lines, where the first is the plugin's own line
  /// and the second is the line on the target.
  std::vector<std::pair<std::string, std::string>> lines;
};

/// \brief Set of all possible lines.
static const std::unordered_set<std::string> kAnchorLineSet{
    "top",
    "bottom",
    "left",
    "right",
    "horizontalCenter",
    "verticalCenter",
    "baseline"};

/// \brief Properties which shouldn't be saved or loaded
static const std::unordered_set<std::string> kIgnoredProps{
    "objectName",
    "pluginName",
    "anchored"};

class ignition::gui::PluginPrivate
{
  /// \brief Set this to true if the plugin should be deleted as soon as it has
  ///  a parent.
  public: bool deleteLater{false};

  /// \brief Holds the value of the `delete_later` attribute on the
  /// configuration. Subclasses can check this value for example to return
  /// before the end of LoadConfig.
  public: bool deleteLaterRequested{false};

  /// \brief Pointer to item generated with plugin's QML
  public: QQuickItem *pluginItem{nullptr};

  /// \brief Pointer to wrapping card item
  public: QQuickItem *cardItem{nullptr};

  /// \brief Context in which plugin item was cerated
  public: QQmlContext *context{nullptr};

  /// \brief Map of card properties to be passed to QML card object.
  /// Accepts all QML Pane properties plus custom Igntiion GUI properties.
  /// https://doc.qt.io/qt-5/qml-qtquick-controls2-pane-members.html
  public: std::map<std::string, QVariant> cardProperties;

  /// \brief Holds all anchor information
  public: Anchors anchors;
};

using namespace ignition;
using namespace gui;

/////////////////////////////////////////////////
Plugin::Plugin() : dataPtr(new PluginPrivate)
{
}

/////////////////////////////////////////////////
Plugin::~Plugin()
{
  delete this->dataPtr->pluginItem;
}

/////////////////////////////////////////////////
void Plugin::Load(const tinyxml2::XMLElement *_pluginElem)
{
  if (!_pluginElem)
  {
    ignerr << "Failed to load plugin with a NULL element." << std::endl;
    return;
  }

  // TODO(anyone): Too complicated to deep clone elements with tinyxml2, storing
  // string for now and consider moving away from tinyxml
  tinyxml2::XMLPrinter printer;
  if (!_pluginElem->Accept(&printer))
  {
    ignwarn << "There was an error parsing the plugin element for " <<
        "[" << this->title << "]." << std::endl;
  }
  else
  {
    this->configStr = std::string(printer.CStr());
  }

  // Qml file
  std::string filename = _pluginElem->Attribute("filename");

  // This let's <filename>.qml use <pluginclass> functions and properties
  this->dataPtr->context = new QQmlContext(App()->Engine()->rootContext());
  this->dataPtr->context->setContextProperty(QString::fromStdString(filename),
      this);

  // TODO perhaps find a more generic way of locating these resource files
  {
    std::string pathToRcc = ignition::common::cwd() + "/ign_gui/src/plugins/qrc_" + filename + ".rcc";
    QResource::registerResource(QString::fromStdString(pathToRcc));
  }
  {
    std::string pathToRcc = ignition::common::cwd() + "/ign_gui/test/plugins/qrc_" + filename + ".rcc";
    QResource::registerResource(QString::fromStdString(pathToRcc));
  }



  // Instantiate plugin QML file into a component
  std::string qmlFile("qrc:/" + filename + "/" + filename + ".qml");
  QQmlComponent component(App()->Engine(), QUrl(QString::fromStdString(qmlFile)));

  // Create an item for the plugin
  this->dataPtr->pluginItem =
      qobject_cast<QQuickItem *>(component.create(this->dataPtr->context));
  if (!this->dataPtr->pluginItem)
  {
    ignerr << "Failed to instantiate QML file [" << qmlFile << "]." << std::endl
           << "* Are you sure it's been added to the .qrc file?" << std::endl
           << "* Are you sure the file is valid QML? "
           << "You can check with the `qmlscene` tool" << std::endl;
    return;
  }

  // Load common configuration
  this->LoadCommonConfig(_pluginElem->FirstChildElement("ignition-gui"));

  // Load custom configuration
  this->LoadConfig(_pluginElem);
}

/////////////////////////////////////////////////
void Plugin::LoadCommonConfig(const tinyxml2::XMLElement *_ignGuiElem)
{
  if (nullptr == _ignGuiElem)
    return;

  auto elem = _ignGuiElem->FirstChildElement("title");
  if (nullptr != elem && nullptr != elem->GetText())
  {
    this->title = elem->GetText();
  }

  // Delete later
  elem = _ignGuiElem->FirstChildElement("delete_later");
  if (nullptr != elem)
  {
    // Store param
    elem->QueryBoolText(&this->dataPtr->deleteLaterRequested);

    // Use it
    if (this->dataPtr->deleteLaterRequested)
      this->DeleteLater();
  }

  // Properties
  for (auto propElem = _ignGuiElem->FirstChildElement("property");
      propElem != nullptr;
      propElem = propElem->NextSiblingElement("property"))
  {
    std::string key = propElem->Attribute("key");
    std::string type = propElem->Attribute("type");
    QVariant variant;

    if (type == "bool")
    {
      bool value;
      propElem->QueryBoolText(&value);
      variant = QVariant(value);
    }
    else if (type == "int")
    {
      int value;
      propElem->QueryIntText(&value);
      variant = QVariant(value);
    }
    else if (type == "double")
    {
      double value;
      propElem->QueryDoubleText(&value);
      variant = QVariant(value);
    }
    else if (type == "string")
    {
      if (nullptr == propElem->GetText())
      {
        ignerr << "Invalid string inside [" << key << "]" << std::endl;
        continue;
      }
      std::string value = propElem->GetText();
      variant = QVariant(QString::fromStdString(value));
    }
    else
    {
      ignwarn << "Property type [" << type << "] not supported." << std::endl;
      continue;
    }

    this->dataPtr->cardProperties[key] = variant;
  }

  // Anchors
  if (auto anchorElem = _ignGuiElem->FirstChildElement("anchors"))
  {
    this->dataPtr->anchors.target = anchorElem->Attribute("target");
    this->dataPtr->anchors.lines.clear();

    for (auto lineElem = anchorElem->FirstChildElement("line");
        lineElem != nullptr;
        lineElem = lineElem->NextSiblingElement("line"))
    {
      auto ownLine = lineElem->Attribute("own");
      auto targetLine = lineElem->Attribute("target");

      if (kAnchorLineSet.find(ownLine) == kAnchorLineSet.end())
      {
        ignwarn << "Invalid anchor line [" << ownLine << "]" << std::endl;
        continue;
      }

      if (kAnchorLineSet.find(targetLine) == kAnchorLineSet.end())
      {
        ignwarn << "Invalid anchor target line [" << targetLine << "]"
                << std::endl;
        continue;
      }

      this->dataPtr->anchors.lines.push_back(
          std::make_pair(ownLine, targetLine));
    }
  }
}

/////////////////////////////////////////////////
std::string Plugin::ConfigStr()
{
  // TODO(anyone): When plugins override this function they will lose the
  // card updates, must refactor config handling

  // Convert string to XML
  tinyxml2::XMLDocument doc;
  doc.Parse(this->configStr.c_str());

  // <plugin>
  auto pluginElem = doc.FirstChildElement("plugin");
  if (!pluginElem)
  {
    ignerr << "Missing <plugin> element, not updating config string."
           << std::endl;
    return this->configStr;
  }

  // <ignition-gui>
  auto ignGuiElem = pluginElem->FirstChildElement("ignition-gui");
  if (!ignGuiElem)
  {
    ignGuiElem = doc.NewElement("ignition-gui");
    pluginElem->InsertEndChild(ignGuiElem);
  }

  // Clean <property>s
  for (auto propElem = ignGuiElem->FirstChildElement("property");
      propElem != nullptr;)
  {
    auto nextProp = propElem->NextSiblingElement("property");
    ignGuiElem->DeleteChild(propElem);
    propElem = nextProp;
  }

  // Add <property>s
  auto meta = this->CardItem()->metaObject();
  for (int i = 0; i < meta->propertyCount(); ++i)
  {
    auto key = meta->property(i).name();
    auto type = std::string(meta->property(i).typeName());

    // Explicitly skip some keys
    if (kIgnoredProps.find(key) != kAnchorLineSet.end())
      continue;

    // When setting, it will need to be string
    if (type == "QString")
      type = "string";

    std::string value;
    if (type != "double" && type != "int" && type != "bool" && type != "string")
    {
      continue;
    }

    value = this->CardItem()->property(meta->property(i).name())
            .toString().toStdString();

    auto elem = doc.NewElement("property");
    elem->SetAttribute("key", key);
    elem->SetAttribute("type", type.c_str());
    elem->SetText(value.c_str());
    ignGuiElem->InsertEndChild(elem);
  }

  // Remove <anchors> if needed
  // TODO(louise) Support setting anchors from UI and then saving it.
  auto anchored = this->CardItem()->property("anchored").toBool();
  if (!anchored)
  {
    for (auto anchorElem = ignGuiElem->FirstChildElement("anchors");
        anchorElem != nullptr;)
    {
      auto nextAnchor = anchorElem->NextSiblingElement("anchors");
      ignGuiElem->DeleteChild(anchorElem);
      anchorElem = nextAnchor;
    }
  }

  // Then convert XML back to string
  tinyxml2::XMLPrinter printer;
  if (!pluginElem->Accept(&printer))
  {
    ignwarn << "There was an error parsing the plugin element for " <<
        "[" << this->title << "]." << std::endl;
  }
  else
  {
    this->configStr = std::string(printer.CStr());
  }

  return this->configStr;
}

/////////////////////////////////////////////////
void Plugin::DeleteLater()
{
  this->dataPtr->deleteLaterRequested = true;
//  if (this->parent())
//  {
//    qobject_cast<QWidget *>(this->parent())->close();
//  }
//  else
//  {
//    this->dataPtr->deleteLater = true;
//  }
}

/////////////////////////////////////////////////
bool Plugin::DeleteLaterRequested() const
{
  return this->dataPtr->deleteLaterRequested;
}

/////////////////////////////////////////////////
QQuickItem *Plugin::PluginItem() const
{
  return this->dataPtr->pluginItem;
}

/////////////////////////////////////////////////
QQuickItem *Plugin::CardItem() const
{
  if (!this->dataPtr->pluginItem)
    return nullptr;

  // If already created, just return it
  if (this->dataPtr->cardItem)
    return this->dataPtr->cardItem;

  // Instantiate a card
  std::string qmlFile(":qml/IgnCard.qml");
  QQmlComponent cardComp(App()->Engine(),
      QString(QString::fromStdString(qmlFile)));
  auto cardItem = qobject_cast<QQuickItem *>(cardComp.create());
  if (!cardItem)
  {
    ignerr << "Internal error: Failed to instantiate QML file [" << qmlFile
           << "]" << std::endl;
    return nullptr;
  }

  // C++ ownership
  QQmlEngine::setObjectOwnership(cardItem, QQmlEngine::CppOwnership);

  // Get card parts
  auto cardContentItem = cardItem->findChild<QQuickItem *>("content");
  if (!cardContentItem)
  {
    ignerr << "Null card content QQuickItem!" << std::endl;
    return nullptr;
  }

  auto cardToolbarItem = cardItem->findChild<QQuickItem *>("cardToolbar");
  if (!cardToolbarItem)
  {
    ignerr << "Null toolbar content QQuickItem!" << std::endl;
    return nullptr;
  }

  // Configure card
  cardItem->setProperty("pluginName",
      QString::fromStdString(this->Title()));

  for (auto prop : this->dataPtr->cardProperties)
  {
    // Skip and only apply once it's reparented
    if (prop.first == "state")
      continue;

    cardItem->setProperty(prop.first.c_str(), prop.second);
  }

  // Adjust size to accomodate plugin if not explicitly set in config
  if (this->dataPtr->cardProperties.find("width") ==
      this->dataPtr->cardProperties.end())
  {
    cardItem->setProperty("width",
        this->dataPtr->pluginItem->property("width").toInt());
  }

  if (this->dataPtr->cardProperties.find("height") ==
      this->dataPtr->cardProperties.end())
  {
    cardItem->setProperty("height",
        this->dataPtr->pluginItem->property("height").toInt());
  }

  // Add plugin to card content
  this->dataPtr->pluginItem->setParentItem(cardContentItem);

  this->dataPtr->cardItem = cardItem;

  return cardItem;
}

/////////////////////////////////////////////////
QQmlContext *Plugin::Context() const
{
  return this->dataPtr->context;
}

/////////////////////////////////////////////////
void Plugin::PostParentChanges()
{
  // TODO(louise) Test more configuration combinations, the order of operations
  // may be messing up some use cases.

  // State
  // Change state now that we have a parent
  if (this->dataPtr->cardProperties.find("state") !=
      this->dataPtr->cardProperties.end())
  {
    this->CardItem()->setProperty("state",
        this->dataPtr->cardProperties["state"]);
  }

  // Anchor
  this->ApplyAnchors();

  // Re-apply other properties like size and position if present
  for (auto prop : this->dataPtr->cardProperties)
  {
    if (prop.first == "state")
      continue;

    this->CardItem()->setProperty(prop.first.c_str(), prop.second);
  }
}

/////////////////////////////////////////////////
void Plugin::ApplyAnchors()
{
  if (this->dataPtr->anchors.target.empty() ||
      this->dataPtr->anchors.lines.empty())
  {
    return;
  }

  // Only floating plugins can be anchored
  if (this->CardItem()->property("state") != "floating")
  {
    ignwarn << "Anchors can only be applied on floating state." << std::endl;
    return;
  }

  // Get target
  QQuickItem *target = nullptr;

  if (this->dataPtr->anchors.target == "window")
  {
    auto win = App()->findChild<MainWindow *>();
    if (!win)
    {
      ignerr << "Internal error: missing window" << std::endl;
      return;
    }

    auto bgItem = win->QuickWindow()->findChild<QQuickItem *>("background");
    if (!bgItem)
    {
      ignerr << "Internal error: missing background item" << std::endl;
      return;
    }

    target = bgItem;
  }
  else
  {
    // See if there's a plugin with that name
    target =
        findFirstByProperty(App()->Engine()->findChildren<QQuickItem *>(),
        "pluginName", QString::fromStdString(this->dataPtr->anchors.target));
  }

  if (!target)
  {
    ignwarn << "Failed to find anchor target [" << this->dataPtr->anchors.target
            << "]" << std::endl;
    return;
  }

  // Reparent so it can be anchored
  this->CardItem()->setParentItem(target);

  // Clear previous anchors
  QMetaObject::invokeMethod(this->CardItem(), "clearAnchors");

  // Set anchors
  auto cardAnchors = qvariant_cast<QObject *>(
      this->CardItem()->property("anchors"));
  for (auto line : this->dataPtr->anchors.lines)
  {
    cardAnchors->setProperty(line.first.c_str(),
        target->property(line.second.c_str()));
  }
  this->CardItem()->setProperty("anchored", true);
}

