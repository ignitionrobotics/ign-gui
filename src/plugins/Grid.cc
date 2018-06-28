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

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include <ignition/common/Console.hh>
#include <ignition/common/PluginMacros.hh>
#include <ignition/math/Color.hh>
#include <ignition/math/Pose3.hh>
#include <ignition/rendering.hh>

#include "ignition/gui/CollapsibleWidget.hh"
#include "ignition/gui/ColorWidget.hh"
#include "ignition/gui/NumberWidget.hh"
#include "ignition/gui/Pose3dWidget.hh"
#include "ignition/gui/QtMetatypes.hh"
#include "ignition/gui/plugins/Grid.hh"

// Default cell count
static const int kDefaultCellCount{20};

// Default vertical cell count
static const int kDefaultVertCellCount{0};

// Default cell length
static const double kDefaultCellLength{1.0};

// Default pose
static const ignition::math::Pose3d kDefaultPose{ignition::math::Pose3d::Zero};

// Default color
static const ignition::math::Color kDefaultColor{
    ignition::math::Color(0.7, 0.7, 0.7, 1.0)};

namespace ignition
{
namespace gui
{
namespace plugins
{
  /// \brief Holds configuration for a grid
  struct GridInfo
  {
    /// \brief Number of cells in the horizontal
    int cellCount{kDefaultCellCount};

    /// \brief Number of cells in the vertical
    int vertCellCount{kDefaultVertCellCount};

    /// \brief Cell length, both horizontal and vertical
    double cellLength{kDefaultCellLength};

    /// \brief Grid pose in the world
    math::Pose3d pose{kDefaultPose};

    /// \brief Grid ambient color
    math::Color color{kDefaultColor};
  };

  class GridPrivate
  {
    /// \brief Keep track of this grid.
    public: rendering::GridPtr grid;
  };
}
}
}

using namespace ignition;
using namespace gui;
using namespace plugins;

/////////////////////////////////////////////////
Grid::Grid()
  : DisplayPlugin(), dataPtr(new GridPrivate)
{
}

/////////////////////////////////////////////////
Grid::~Grid()
{
}

/////////////////////////////////////////////////
void Grid::Initialize(const tinyxml2::XMLElement *_pluginElem)
{
  if (this->title.empty())
    this->title = "3D Grid";

  // Configuration
  GridInfo gridInfo;
  if (_pluginElem)
  {
    if (auto elem = _pluginElem->FirstChildElement("cell_count"))
      elem->QueryIntText(&gridInfo.cellCount);

    if (auto elem = _pluginElem->FirstChildElement("vertical_cell_count"))
      elem->QueryIntText(&gridInfo.vertCellCount);

    if (auto elem = _pluginElem->FirstChildElement("cell_length"))
      elem->QueryDoubleText(&gridInfo.cellLength);

    if (auto elem = _pluginElem->FirstChildElement("pose"))
    {
      std::stringstream poseStr;
      poseStr << std::string(elem->GetText());
      poseStr >> gridInfo.pose;
    }

    if (auto elem = _pluginElem->FirstChildElement("color"))
    {
      std::stringstream colorStr;
      colorStr << std::string(elem->GetText());
      colorStr >> gridInfo.color;
    }

  }

  this->dataPtr->grid = this->Scene()->CreateGrid();
  this->dataPtr->grid->SetCellCount(gridInfo.cellCount);
  this->dataPtr->grid->SetVerticalCellCount(gridInfo.vertCellCount);
  this->dataPtr->grid->SetCellLength(gridInfo.cellLength);

  this->Visual()->SetLocalPose(gridInfo.pose);
  this->Visual()->AddGeometry(this->dataPtr->grid);

  auto mat = this->Scene()->CreateMaterial();
  mat->SetAmbient(gridInfo.color);
  this->Visual()->SetMaterial(mat);
}

/////////////////////////////////////////////////
QWidget* Grid::CreateProperties()
{
  auto gridName = QString::fromStdString(this->dataPtr->grid->Name());

  auto cellCountWidget = new NumberWidget("Horizontal cell count",
      NumberType::UINT);
  cellCountWidget->SetValue(QVariant::fromValue(this->dataPtr->grid->CellCount()));
  cellCountWidget->setProperty("gridName", gridName);
  cellCountWidget->setObjectName("cellCountWidget");
  this->connect(cellCountWidget, SIGNAL(ValueChanged(QVariant)), this,
      SLOT(OnChange(QVariant)));

  auto vertCellCountWidget = new NumberWidget("Vertical cell count",
      NumberType::UINT);
  vertCellCountWidget->SetValue(
      QVariant::fromValue(this->dataPtr->grid->VerticalCellCount()));
  vertCellCountWidget->setProperty("gridName", gridName);
  vertCellCountWidget->setObjectName("vertCellCountWidget");
  this->connect(vertCellCountWidget, SIGNAL(ValueChanged(QVariant)), this,
      SLOT(OnChange(QVariant)));

  auto cellLengthWidget = new NumberWidget("Cell length", NumberType::DOUBLE);
  cellLengthWidget->SetValue(QVariant::fromValue(this->dataPtr->grid->CellLength()));
  cellLengthWidget->setProperty("gridName", gridName);
  cellLengthWidget->setObjectName("cellLengthWidget");
  this->connect(cellLengthWidget, SIGNAL(ValueChanged(QVariant)), this,
      SLOT(OnChange(QVariant)));

  auto poseWidget = new Pose3dWidget();
  poseWidget->SetValue(QVariant::fromValue(this->dataPtr->grid->Parent()->WorldPose()));
  poseWidget->setProperty("gridName", gridName);
  poseWidget->setObjectName("poseWidget");
  this->connect(poseWidget, SIGNAL(ValueChanged(QVariant)), this,
      SLOT(OnChange(QVariant)));

  auto colorWidget = new ColorWidget();
  colorWidget->SetValue(QVariant::fromValue(this->dataPtr->grid->Material()->Ambient()));
  colorWidget->setProperty("gridName", gridName);
  colorWidget->setObjectName("colorWidget");
  this->connect(colorWidget, SIGNAL(ValueChanged(QVariant)), this,
      SLOT(OnChange(QVariant)));

  auto collapsible = new CollapsibleWidget(this->dataPtr->grid->Name());
  collapsible->AppendContent(cellCountWidget);
  collapsible->AppendContent(vertCellCountWidget);
  collapsible->AppendContent(cellLengthWidget);
  collapsible->AppendContent(poseWidget);
  collapsible->AppendContent(colorWidget);

  return collapsible;
}

/////////////////////////////////////////////////
void Grid::OnVisibilityChange(bool _value)
{
  // TODO(dhood): remove this once parent visual has setVisible
  if (_value)
  {
    this->dataPtr->grid->Material()->SetTransparency(0.);
  } else {
    this->dataPtr->grid->Material()->SetTransparency(1.);
  }
}

/////////////////////////////////////////////////
void Grid::OnChange(const QVariant &_value)
{
  auto gridName = this->sender()->property("gridName").toString().toStdString();
  auto type = this->sender()->objectName().toStdString();

  if (type == "cellCountWidget")
    this->dataPtr->grid->SetCellCount(_value.toInt());
  else if (type == "vertCellCountWidget")
    this->dataPtr->grid->SetVerticalCellCount(_value.toInt());
  else if (type == "cellLengthWidget")
    this->dataPtr->grid->SetCellLength(_value.toDouble());
  else if (type == "poseWidget")
    this->dataPtr->grid->Parent()->SetWorldPose(_value.value<math::Pose3d>());
  else if (type == "colorWidget")
    this->dataPtr->grid->Material()->SetAmbient(_value.value<math::Color>());
}

// Register this plugin
IGN_COMMON_REGISTER_SINGLE_PLUGIN(ignition::gui::plugins::Grid,
                                  ignition::gui::Plugin)
