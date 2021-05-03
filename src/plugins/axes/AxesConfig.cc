/*
 * Copyright (C) 2020 Open Source Robotics Foundation
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

#include <map>
#include <set>
#include <utility>
#include <vector>

#include <ignition/common/Console.hh>
#include <ignition/gui/Application.hh>
#include <ignition/gui/MainWindow.hh>
#include <ignition/plugin/Register.hh>
#include <ignition/math/Color.hh>
#include <ignition/math/Pose3.hh>
#include <ignition/rendering.hh>
#include <ignition/rendering/AxisVisual.hh>

#include "ignition/gui/GuiEvents.hh"
#include "AxesConfig.hh"

namespace ignition::gazebo
{
  /// \brief struct to save some data about the axes
  struct AxesData_t {
    math::Pose3d pose;
    bool showAxes;
    bool showArrow;
  };

  class AxesConfigPrivate
  {
    /// \brief axes ptr in a scene
    public: rendering::AxisVisualPtr axes = nullptr;

    /// \brief Default visible state
    bool visible{true};

    /// \brief length of axes
    double length{1};

    /// \brief length of axes
    bool isArrow{true};

    /// \brief Default pose of grid
    math::Pose3d pose{math::Pose3d::Zero};

    /// \brief Scene pointer
    rendering::ScenePtr scene;

    /// \brief name of the axes
    public: std::string nameAxes;

    /// \brief structure to save data of the active axes
    public: std::unordered_map<std::string, AxesData_t> activeAxesMap;

    /// \brief Flag that indicates whether there are new updates to be rendered.
    public: bool dirty{true};
  };
}

using namespace ignition;
using namespace gazebo;

/////////////////////////////////////////////////
AxesConfig::AxesConfig()
  : ignition::gui::Plugin(), dataPtr(std::make_unique<AxesConfigPrivate>())
{
  this->dataPtr->nameAxes = "";
}

/////////////////////////////////////////////////
AxesConfig::~AxesConfig() = default;

/////////////////////////////////////////////////
void AxesConfig::Initialize()
{
  if (this->dataPtr->scene != nullptr)
    return;

  auto loadedEngNames = rendering::loadedEngines();
  if (loadedEngNames.empty())
    return;

  // assume there is only one engine loaded
  auto engineName = loadedEngNames[0];
  if (loadedEngNames.size() > 1)
  {
    igndbg << "More than one engine is available. "
      << "Origin axes config plugin will use engine ["
        << engineName << "]" << std::endl;
  }
  auto engine = rendering::engine(engineName);
  if (!engine)
  {
    ignerr << "Internal error: failed to load engine [" << engineName
      << "]. Origin axes plugin won't work." << std::endl;
    return;
  }

  if (engine->SceneCount() == 0)
    return;

  // assume there is only one scene
  // load scene
  this->dataPtr->scene = engine->SceneByIndex(0);
  if (!this->dataPtr->scene)
  {
    ignerr << "Internal error: scene is null." << std::endl;
    return;
  }

  if (!this->dataPtr->scene->IsInitialized() ||
    this->dataPtr->scene->VisualCount() == 0)
  {
    return;
  }

  this->EntitiesInScene();
  this->RefreshComboBox();
}

/////////////////////////////////////////////////
void AxesConfig::LoadConfig(const tinyxml2::XMLElement *)
{
  if (this->title.empty())
    this->title = "Axes";

  ignition::gui::App()->findChild<
      ignition::gui::MainWindow *>()->installEventFilter(this);
}

/////////////////////////////////////////////////
bool AxesConfig::eventFilter(QObject *_obj, QEvent *_event)
{
  if (_event->type() == ignition::gui::events::Render::kType)
  {
    this->Initialize();
    // This event is called in Scene3d's RenderThread, so it's safe to make
    // rendering calls here
    this->UpdateOriginArrows();
  }

  // Standard event processing
  return QObject::eventFilter(_obj, _event);
}

/////////////////////////////////////////////////
void AxesConfig::UpdateActiveAxes()
{
  for (auto vis : this->dataPtr->activeAxesMap)
  {
    auto visAxes = std::dynamic_pointer_cast<rendering::AxisVisual>(
          this->dataPtr->scene->VisualByName(vis.first + "Axes"));

    if (visAxes)
    {
      visAxes->SetLocalPose(vis.second.pose);
    }
  }
}

/////////////////////////////////////////////////
void AxesConfig::UpdateOriginArrows()
{
  if (this->dataPtr->nameAxes.empty())
    return;

  // Load axes if they don't already exist
  this->LoadAxesbyName(this->dataPtr->nameAxes);

  // If axes were not loaded successfully, don't update
  if (!this->dataPtr->axes)
    return;

  // Updating the poses
  this->UpdateActiveAxes();

  if (!this->dataPtr->dirty)
    return;

  // Save the axesVisual in the structure if it doesn't exist or
  // update the pose
  auto it = this->dataPtr->activeAxesMap.find(this->dataPtr->nameAxes);
  if (it == this->dataPtr->activeAxesMap.end())
  {
    ignition::gazebo::AxesData_t data;
    data.pose = this->dataPtr->pose;
    data.showAxes = true;
    data.showArrow = true;
    this->dataPtr->visible = true;
    this->dataPtr->isArrow = true;
    this->dataPtr->activeAxesMap.insert(
      std::pair<std::string, ignition::gazebo::AxesData_t>(
        this->dataPtr->nameAxes,
        data));
  }
  else
  {
    it->second.pose = this->dataPtr->pose;
    it->second.showAxes = this->dataPtr->visible;
    it->second.showArrow = this->dataPtr->isArrow;
  }

  // update visibility
  this->dataPtr->axes->SetVisible(this->dataPtr->visible);
  // Update type: arrow or line
  if (this->dataPtr->visible)
  {
    this->dataPtr->axes->ShowAxisHead(this->dataPtr->isArrow);
  }
  // update scale
  this->dataPtr->axes->SetLocalScale(1, 1, this->dataPtr->length * 2);

  this->dataPtr->dirty = false;
}

/////////////////////////////////////////////////
const QStringList AxesConfig::comboList()
{
  return itemComboList;
}

void AxesConfig::RefreshComboBox()
{
  this->EntitiesInScene();
  emit ComboListChanged();
  onCurrentIndexChanged(0);
}

/////////////////////////////////////////////////
void AxesConfig::SetComboList(const QStringList &comboList)
{
  if (this->itemComboList != comboList)
  {
    itemComboList = comboList;
    if (itemComboList.size() > 0 && this->dataPtr->nameAxes.empty())
    {
      this->dataPtr->nameAxes = itemComboList[0].toStdString();
    }
  }
}

/////////////////////////////////////////////////
void AxesConfig::LoadAxesbyName(const std::string &_name)
{
  if ((this->dataPtr->nameAxes.compare(_name) == 0 && this->dataPtr->axes)
      && !this->dataPtr->dirty)
    return;

  this->dataPtr->nameAxes = _name;
  this->dataPtr->axes = std::dynamic_pointer_cast<rendering::AxisVisual>(
    this->dataPtr->scene->VisualByName(this->dataPtr->nameAxes + "Axes"));

  if (!this->dataPtr->axes)
  {
    rendering::VisualPtr parentVisual =
      std::dynamic_pointer_cast<rendering::Visual>(
        this->dataPtr->scene->VisualByName(this->dataPtr->nameAxes));

    auto axes = this->dataPtr->scene->CreateAxisVisual(
      this->dataPtr->nameAxes + "Axes");
    parentVisual->AddChild(axes);
  }
}

/////////////////////////////////////////////////
void AxesConfig::onCurrentIndexChanged(int _index)
{
  this->dataPtr->nameAxes = itemComboList[_index].toStdString();
  this->dataPtr->dirty = true;
  auto axes = std::dynamic_pointer_cast<rendering::AxisVisual>(
    this->dataPtr->scene->VisualByName(this->dataPtr->nameAxes + "Axes"));
  if (axes)
  {
    this->dataPtr->length = axes->LocalScale().Z() / 2.0;
    // Save the axesVisual in the structure if it doesn't exist or update
    // the pose
    auto it = this->dataPtr->activeAxesMap.find(this->dataPtr->nameAxes);
    if (it != this->dataPtr->activeAxesMap.end())
    {
      this->dataPtr->pose = it->second.pose;
      this->dataPtr->visible = it->second.showAxes;
      this->dataPtr->isArrow = it->second.showArrow;
    }
    else
    {
      this->dataPtr->pose = math::Pose3d(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      this->dataPtr->visible = true;
      this->dataPtr->isArrow = true;
    }
  }
  else
  {
    this->dataPtr->length = 1.0;
    this->dataPtr->visible = true;
    this->dataPtr->isArrow = true;
    this->dataPtr->pose = math::Pose3d(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  }
}

/////////////////////////////////////////////////
double AxesConfig::length() const
{
  return this->dataPtr->length;
}

/////////////////////////////////////////////////
double AxesConfig::axesX() const
{
  return this->dataPtr->pose.Pos().X();
}

/////////////////////////////////////////////////
double AxesConfig::axesY() const
{
  return this->dataPtr->pose.Pos().Y();
}

/////////////////////////////////////////////////
double AxesConfig::axesZ() const
{
  return this->dataPtr->pose.Pos().Z();
}

/////////////////////////////////////////////////
double AxesConfig::axesRoll() const
{
  return this->dataPtr->pose.Rot().Euler().X();
}

/////////////////////////////////////////////////
double AxesConfig::axesPitch() const
{
  return this->dataPtr->pose.Rot().Euler().Y();
}

/////////////////////////////////////////////////
double AxesConfig::axesYaw() const
{
  return this->dataPtr->pose.Rot().Euler().Z();
}

/////////////////////////////////////////////////
int AxesConfig::showAxes() const
{
  return this->dataPtr->visible ? Qt::Checked : Qt::Unchecked;
}

/////////////////////////////////////////////////
int AxesConfig::showArrow() const
{
  return this->dataPtr->isArrow ? Qt::Checked : Qt::Unchecked;
}

/////////////////////////////////////////////////
void AxesConfig::EntitiesInScene()
{
  std::set<std::string> setEntities;

  for (unsigned int i = 0; i < this->dataPtr->scene->VisualCount(); ++i)
  {
    auto vis = this->dataPtr->scene->VisualByIndex(i);
    if (!vis)
      continue;
    std::string vis_name = vis->Name();
    std::vector<std::string> tokens = ignition::common::split(vis_name, "::");
    if (tokens.size() == 1)
    {
      std::size_t found = tokens[0].find("Axes");
      if (found == std::string::npos)
      {
        setEntities.insert(tokens[0]);
      }
    }
  }

  QStringList localCombolist;
  for (auto s : setEntities)
  {
    localCombolist << s.c_str();
  }

  SetComboList(localCombolist);
}

/////////////////////////////////////////////////
void AxesConfig::UpdateLength(double _length)
{
  this->dataPtr->length = _length;
  this->dataPtr->dirty = true;
}

/////////////////////////////////////////////////
void AxesConfig::SetPose(
  double _x, double _y, double _z,
  double _roll, double _pitch, double _yaw)
{
  this->dataPtr->pose = math::Pose3d(_x, _y, _z, _roll, _pitch, _yaw);
  this->dataPtr->dirty = true;
}

/////////////////////////////////////////////////
void AxesConfig::OnTypeAxes(bool _checked)
{
  this->dataPtr->isArrow = _checked;
  this->dataPtr->dirty = true;
}

/////////////////////////////////////////////////
void AxesConfig::OnShow(bool _checked)
{
  this->dataPtr->visible = _checked;
  this->dataPtr->dirty = true;
}

// Register this plugin
IGNITION_ADD_PLUGIN(ignition::gazebo::AxesConfig,
                    ignition::gui::Plugin)
