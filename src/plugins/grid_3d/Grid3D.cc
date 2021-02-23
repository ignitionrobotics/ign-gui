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
#include <ignition/math/Color.hh>
#include <ignition/math/Pose3.hh>
#include <ignition/plugin/Register.hh>

// TODO(louise) Remove these pragmas once ign-rendering is disabling the
// warnings
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

#include <ignition/rendering/Grid.hh>
#include <ignition/rendering/RenderEngine.hh>
#include <ignition/rendering/RenderingIface.hh>
#include <ignition/rendering/Scene.hh>
#include <ignition/rendering/Visual.hh>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Grid3D.hh"

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
    ignition::math::Color(0.7f, 0.7f, 0.7f, 1.0f)};

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

  class Grid3DPrivate
  {
    /// brief Parent window
    public: QQuickWindow *quickWindow;

    /// \brief We keep a pointer to the engine and rely on it not being
    /// destroyed, since it is a singleton.
    public: rendering::RenderEngine *engine;

    /// \brief We keep the scene name rather than a shared pointer because we
    /// don't want to share ownership.
    public: std::string sceneName{"scene"};

    /// \brief Engine name received at startup
    public: std::string engineName{"ogre2"};

    /// \brief Grids received from config file on startup
    public: std::vector<GridInfo> startupGrids;

    /// \brief Keep track of grids we currently found on the scene
    public: std::vector<rendering::GridPtr> grids;
  };
}
}
}

using namespace ignition;
using namespace gui;
using namespace plugins;

/////////////////////////////////////////////////
Grid3D::Grid3D()
  : Plugin(), dataPtr(new Grid3DPrivate)
{
}

/////////////////////////////////////////////////
Grid3D::~Grid3D()
{
}

/////////////////////////////////////////////////
void Grid3D::LoadConfig(const tinyxml2::XMLElement *_pluginElem)
{
  if (this->title.empty())
    this->title = "3D Grid";

  // Configuration
  if (_pluginElem)
  {
    // All grids managed belong to the same engine and scene
    auto elem = _pluginElem->FirstChildElement("engine");
    if (nullptr != elem && nullptr != elem->GetText())
      this->dataPtr->engineName = elem->GetText();

    elem = _pluginElem->FirstChildElement("scene");
    if (nullptr != elem && nullptr != elem->GetText())
      this->dataPtr->sceneName = elem->GetText();

    // For grids to be inserted at startup
    for (auto insertElem = _pluginElem->FirstChildElement("insert");
         insertElem != nullptr;
         insertElem = insertElem->NextSiblingElement("insert"))
    {
      GridInfo gridInfo;

      if (auto cellCountElem = insertElem->FirstChildElement("cell_count"))
        cellCountElem->QueryIntText(&gridInfo.cellCount);

      if (auto vElem = insertElem->FirstChildElement("vertical_cell_count"))
        vElem->QueryIntText(&gridInfo.vertCellCount);

      if (auto lengthElem = insertElem->FirstChildElement("cell_length"))
        lengthElem->QueryDoubleText(&gridInfo.cellLength);

      elem = insertElem->FirstChildElement("pose");
      if (nullptr != elem && nullptr != elem->GetText())
      {
        std::stringstream poseStr;
        poseStr << std::string(elem->GetText());
        poseStr >> gridInfo.pose;
      }

      elem = insertElem->FirstChildElement("color");
      if (nullptr != elem && nullptr != elem->GetText())
      {
        std::stringstream colorStr;
        colorStr << std::string(elem->GetText());
        colorStr >> gridInfo.color;
      }

      this->dataPtr->startupGrids.push_back(gridInfo);
    }
  }

  // TODO(anyone): remove - just for testing when inserting plugin
  GridInfo gridInfo;
  this->dataPtr->startupGrids.push_back(gridInfo);

  this->connect(this->PluginItem(), &QQuickItem::windowChanged,
    [=](QQuickWindow *_window)
    {
      if (!_window)
      {
        igndbg << "Changed to null window" << std::endl;
        return;
      }

      this->dataPtr->quickWindow = _window;

      // Initialize after Scene3D plugins
      this->connect(this->dataPtr->quickWindow, &QQuickWindow::beforeRendering,
          this, &Grid3D::Initialize, Qt::DirectConnection);
    });
}

/////////////////////////////////////////////////
void Grid3D::Initialize()
{
  this->disconnect(this->dataPtr->quickWindow, &QQuickWindow::beforeRendering,
      this, &Grid3D::Initialize);

  std::string error{""};
  rendering::ScenePtr scene;

  // Render engine
  this->dataPtr->engine = rendering::engine(this->dataPtr->engineName);
  if (!this->dataPtr->engine)
  {
    error = "Engine \"" + this->dataPtr->engineName
           + "\" not supported, Grid plugin won't work.";
    ignwarn << error << std::endl;
  }
  else
  {
    // Scene
    scene = this->dataPtr->engine->SceneByName(this->dataPtr->sceneName);
    if (!scene)
    {
      error = "Scene \"" + this->dataPtr->sceneName
             + "\" not found, Grid plugin won't work.";
      ignwarn << error << std::endl;
    }
    else
    {
      auto root = scene->RootVisual();

      // Initial grids
      for (const auto &g : this->dataPtr->startupGrids)
      {
        auto grid = scene->CreateGrid();
        grid->SetCellCount(g.cellCount);
        grid->SetVerticalCellCount(g.vertCellCount);
        grid->SetCellLength(g.cellLength);

        auto gridVis = scene->CreateVisual();
        root->AddChild(gridVis);
        gridVis->SetLocalPose(g.pose);
        gridVis->AddGeometry(grid);

        auto mat = scene->CreateMaterial();
        mat->SetAmbient(g.color);
        gridVis->SetMaterial(mat);
      }
    }
  }

  // Don't waste time loading widgets if this will be deleted anyway
  if (this->DeleteLaterRequested())
    return;

  if (!error.empty())
  {
//    // Add message
//    auto msg = new QLabel(QString::fromStdString(error));
//
//    auto mainLayout = new QVBoxLayout();
//    mainLayout->addWidget(msg);
//    mainLayout->setAlignment(msg, Qt::AlignCenter);
//    this->setLayout(mainLayout);

    return;
  }

  this->Refresh();
}

/////////////////////////////////////////////////
void Grid3D::Refresh()
{
//  auto mainLayout = this->layout();
//  // Clear previous layout
//  if (mainLayout)
//  {
//    while (mainLayout->count() != 1)
//    {
//      auto item = mainLayout->takeAt(1);
//      if (qobject_cast<CollapsibleWidget *>(item->widget()))
//      {
//        delete item->widget();
//        delete item;
//      }
//    }
//  }
//  // Creating layout for the first time
//  else
//  {
//    mainLayout = new QVBoxLayout();
//    mainLayout->setContentsMargins(0, 0, 0, 0);
//    mainLayout->setSpacing(0);
//    this->setLayout(mainLayout);
//
//    auto addButton = new QPushButton("New grid");
//    addButton->setObjectName("addGridButton");
//    addButton->setToolTip("Add a new grid with default values");
//    this->connect(addButton, SIGNAL(clicked()), this, SLOT(OnAdd()));
//
//    auto refreshButton = new QPushButton("Refresh");
//    refreshButton->setObjectName("refreshGridButton");
//    refreshButton->setToolTip("Refresh the list of grids");
//    this->connect(refreshButton, SIGNAL(clicked()), this, SLOT(Refresh()));
//
//    auto buttonsLayout = new QHBoxLayout();
//    buttonsLayout->addWidget(addButton);
//    buttonsLayout->addWidget(refreshButton);
//
//    auto buttonsWidget = new QWidget();
//    buttonsWidget->setLayout(buttonsLayout);
//
//    mainLayout->addWidget(buttonsWidget);
//  }
//
//  auto scene = this->dataPtr->engine->SceneByName(this->dataPtr->sceneName);
//  // Scene has been destroyed
//  if (!scene)
//  {
//    // Delete buttons
//    auto item = mainLayout->takeAt(0);
//    if (item)
//    {
//      delete item->widget();
//      delete item;
//    }
//
//    // Add message
//    auto msg = new QLabel(QString::fromStdString(
//        "Scene \"" + this->dataPtr->sceneName + "\" has been destroyed.\n"
//        + "Create a new scene and then open a new Grid plugin."));
//    mainLayout->addWidget(msg);
//    mainLayout->setAlignment(msg, Qt::AlignCenter);
//    return;
//  }
//
//
//  // Search for all grids currently in the scene
//  this->dataPtr->grids.clear();
//  for (unsigned int i = 0; i < scene->VisualCount(); ++i)
//  {
//    auto vis = scene->VisualByIndex(i);
//    if (!vis || vis->GeometryCount() == 0)
//      continue;
//
//    rendering::GridPtr grid;
//    for (unsigned int j = 0; j < vis->GeometryCount(); ++j)
//    {
//      grid = std::dynamic_pointer_cast<rendering::Grid>(
//          vis->GeometryByIndex(j));
//      if (grid)
//        break;
//    }
//    if (!grid)
//      continue;
//
//    this->dataPtr->grids.push_back(grid);
//    auto gridName = QString::fromStdString(grid->Name());
//
//    auto cellCountWidget = new NumberWidget("Horizontal cell count",
//        NumberType::UINT);
//    cellCountWidget->SetValue(QVariant::fromValue(grid->CellCount()));
//    cellCountWidget->setProperty("gridName", gridName);
//    cellCountWidget->setObjectName("cellCountWidget");
//    this->connect(cellCountWidget, SIGNAL(ValueChanged(QVariant)), this,
//        SLOT(OnChange(QVariant)));
//
//    auto vertCellCountWidget = new NumberWidget("Vertical cell count",
//        NumberType::UINT);
//    vertCellCountWidget->SetValue(
//        QVariant::fromValue(grid->VerticalCellCount()));
//    vertCellCountWidget->setProperty("gridName", gridName);
//    vertCellCountWidget->setObjectName("vertCellCountWidget");
//    this->connect(vertCellCountWidget, SIGNAL(ValueChanged(QVariant)), this,
//        SLOT(OnChange(QVariant)));
//
//    auto cellLengthWidget = new NumberWidget("Cell length",
//        NumberType::DOUBLE);
//    cellLengthWidget->SetValue(QVariant::fromValue(grid->CellLength()));
//    cellLengthWidget->setProperty("gridName", gridName);
//    cellLengthWidget->setObjectName("cellLengthWidget");
//    this->connect(cellLengthWidget, SIGNAL(ValueChanged(QVariant)), this,
//        SLOT(OnChange(QVariant)));
//
//    auto poseWidget = new Pose3dWidget();
//    poseWidget->SetValue(QVariant::fromValue(grid->Parent()->WorldPose()));
//    poseWidget->setProperty("gridName", gridName);
//    poseWidget->setObjectName("poseWidget");
//    this->connect(poseWidget, SIGNAL(ValueChanged(QVariant)), this,
//        SLOT(OnChange(QVariant)));
//
//    auto colorWidget = new ColorWidget();
//    colorWidget->SetValue(QVariant::fromValue(grid->Material()->Ambient()));
//    colorWidget->setProperty("gridName", gridName);
//    colorWidget->setObjectName("colorWidget");
//    this->connect(colorWidget, SIGNAL(ValueChanged(QVariant)), this,
//        SLOT(OnChange(QVariant)));
//
//    auto deleteButton = new QPushButton("Delete grid");
//    deleteButton->setToolTip("Delete grid " + gridName);
//    deleteButton->setProperty("gridName", gridName);
//    deleteButton->setObjectName("deleteButton");
//    this->connect(deleteButton, SIGNAL(clicked()), this, SLOT(OnDelete()));
//
//    auto collapsible = new CollapsibleWidget(grid->Name());
//    collapsible->AppendContent(cellCountWidget);
//    collapsible->AppendContent(vertCellCountWidget);
//    collapsible->AppendContent(cellLengthWidget);
//    collapsible->AppendContent(poseWidget);
//    collapsible->AppendContent(colorWidget);
//    collapsible->AppendContent(deleteButton);
//
//    mainLayout->addWidget(collapsible);
//  }
//
//  auto spacer = new QWidget();
//  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//  mainLayout->addWidget(spacer);
}

/////////////////////////////////////////////////
void Grid3D::OnChange(const QVariant & /*_value*/)
{
//  auto gridName =
//      this->sender()->property("gridName").toString().toStdString();
//  auto type = this->sender()->objectName().toStdString();
//
//  for (auto grid : this->dataPtr->grids)
//  {
//    if (grid->Name() != gridName)
//      continue;
//
//    if (type == "cellCountWidget")
//      grid->SetCellCount(_value.toInt());
//    else if (type == "vertCellCountWidget")
//      grid->SetVerticalCellCount(_value.toInt());
//    else if (type == "cellLengthWidget")
//      grid->SetCellLength(_value.toDouble());
//    else if (type == "poseWidget")
//      grid->Parent()->SetWorldPose(_value.value<math::Pose3d>());
//    else if (type == "colorWidget")
//      grid->Material()->SetAmbient(_value.value<math::Color>());
//
//    break;
//  }
}

/////////////////////////////////////////////////
void Grid3D::OnDelete()
{
//  auto gridName =
//      this->sender()->property("gridName").toString().toStdString();
//
//  for (auto grid : this->dataPtr->grids)
//  {
//    if (grid->Name() != gridName)
//      continue;
//
//    grid->Scene()->DestroyVisual(grid->Parent());
//    this->dataPtr->grids.erase(std::remove(this->dataPtr->grids.begin(),
//                                           this->dataPtr->grids.end(), grid),
//                                           this->dataPtr->grids.end());
//
//    this->Refresh();
//    break;
//  }
}

/////////////////////////////////////////////////
void Grid3D::OnAdd()
{
//  auto scene = this->dataPtr->engine->SceneByName(this->dataPtr->sceneName);
//  if (!scene)
//  {
//    return;
//  }
//
//  auto root = scene->RootVisual();
//
//  auto grid = scene->CreateGrid();
//  grid->SetCellCount(kDefaultCellCount);
//  grid->SetVerticalCellCount(kDefaultVertCellCount);
//  grid->SetCellLength(kDefaultCellLength);
//
//  auto gridVis = scene->CreateVisual();
//  root->AddChild(gridVis);
//  gridVis->SetLocalPose(kDefaultPose);
//  gridVis->AddGeometry(grid);
//
//  auto mat = scene->CreateMaterial();
//  mat->SetAmbient(kDefaultColor);
//  gridVis->SetMaterial(mat);
//
//  this->Refresh();
}

// Register this plugin
IGNITION_ADD_PLUGIN(ignition::gui::plugins::Grid3D,
                    ignition::gui::Plugin)

