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

#include "MinimalScene.hh"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <ignition/common/Console.hh>
#include <ignition/common/KeyEvent.hh>
#include <ignition/common/MouseEvent.hh>
#include <ignition/math/Vector2.hh>
#include <ignition/math/Vector3.hh>
#include <ignition/plugin/Register.hh>

// TODO(louise) Remove these pragmas once ign-rendering
// is disabling the warnings
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <ignition/rendering/Camera.hh>
#include <ignition/rendering/RayQuery.hh>
#include <ignition/rendering/RenderEngine.hh>
#include <ignition/rendering/RenderingIface.hh>
#include <ignition/rendering/Scene.hh>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "ignition/gui/Application.hh"
#include "ignition/gui/Conversions.hh"
#include "ignition/gui/GuiEvents.hh"
#include "ignition/gui/MainWindow.hh"

/// \brief Private data class for IgnRenderer
class ignition::gui::plugins::IgnRenderer::Implementation
{
  /// \brief Flag to indicate if mouse event is dirty
  public: bool mouseDirty{false};

  /// \brief Flag to indicate if hover event is dirty
  public: bool hoverDirty{false};

  /// \brief Mouse event
  public: common::MouseEvent mouseEvent;

  /// \brief Key event
  public: common::KeyEvent keyEvent;

  /// \brief Mutex to protect mouse events
  public: std::mutex mutex;

  /// \brief User camera
  public: rendering::CameraPtr camera{nullptr};

  /// \brief The currently hovered mouse position in screen coordinates
  public: math::Vector2i mouseHoverPos{math::Vector2i::Zero};

  /// \brief Ray query for mouse clicks
  public: rendering::RayQueryPtr rayQuery{nullptr};

  /// \brief View control focus target
  public: math::Vector3d target;
};

/// \brief Private data class for RenderWindowItem
class ignition::gui::plugins::RenderWindowItem::Implementation
{
  /// \brief Keep latest mouse event
  public: common::MouseEvent mouseEvent;

  /// \brief Render thread
  public : RenderThread *renderThread = nullptr;

  //// \brief List of threads
  public: static QList<QThread *> threads;
};

/// \brief Private data class for MinimalScene
class ignition::gui::plugins::MinimalScene::Implementation
{
};

using namespace ignition;
using namespace gui;
using namespace plugins;

QList<QThread *> RenderWindowItem::Implementation::threads;

/////////////////////////////////////////////////
IgnRenderer::IgnRenderer()
  : dataPtr(utils::MakeUniqueImpl<Implementation>())
{
}

/////////////////////////////////////////////////
void IgnRenderer::Render()
{
  if (this->textureDirty)
  {
    this->dataPtr->camera->SetImageWidth(this->textureSize.width());
    this->dataPtr->camera->SetImageHeight(this->textureSize.height());
    this->dataPtr->camera->SetAspectRatio(this->textureSize.width() /
        this->textureSize.height());
    // setting the size should cause the render texture to be rebuilt
    this->dataPtr->camera->PreRender();
    this->textureDirty = false;
  }

  this->textureId = this->dataPtr->camera->RenderTextureGLId();

  // view control
  this->HandleMouseEvent();

  // update and render to texture
  this->dataPtr->camera->Update();

  if (ignition::gui::App())
  {
    ignition::gui::App()->sendEvent(
        ignition::gui::App()->findChild<ignition::gui::MainWindow *>(),
        new gui::events::Render());
  }
}

/////////////////////////////////////////////////
void IgnRenderer::HandleMouseEvent()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->BroadcastHoverPos();
  this->BroadcastLeftClick();
  this->BroadcastRightClick();
  this->HandleMouseViewControl();
  this->dataPtr->mouseDirty = false;
}

/////////////////////////////////////////////////
void IgnRenderer::HandleMouseViewControl()
{
  if (!this->dataPtr->mouseDirty)
    return;

  auto pos = this->ScreenToScene(this->dataPtr->mouseEvent.Pos());

  events::LeftClickOnScene leftClickOnSceneEvent(this->dataPtr->mouseEvent);
  events::LeftClickToScene leftClickToSceneEvent(pos);
  App()->sendEvent(App()->findChild<MainWindow *>(), &leftClickOnSceneEvent);
  App()->sendEvent(App()->findChild<MainWindow *>(), &leftClickToSceneEvent);
}

////////////////////////////////////////////////
void IgnRenderer::HandleKeyPress(QKeyEvent *_e)
{
  if (_e->isAutoRepeat())
    return;

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  this->dataPtr->keyEvent.SetKey(_e->key());
  this->dataPtr->keyEvent.SetText(_e->text().toStdString());

  this->dataPtr->keyEvent.SetControl(
    (_e->modifiers() & Qt::ControlModifier));
  this->dataPtr->keyEvent.SetShift(
    (_e->modifiers() & Qt::ShiftModifier));
  this->dataPtr->keyEvent.SetAlt(
    (_e->modifiers() & Qt::AltModifier));

  this->dataPtr->mouseEvent.SetControl(this->dataPtr->keyEvent.Control());
  this->dataPtr->mouseEvent.SetShift(this->dataPtr->keyEvent.Shift());
  this->dataPtr->mouseEvent.SetAlt(this->dataPtr->keyEvent.Alt());
  this->dataPtr->keyEvent.SetType(common::KeyEvent::PRESS);
}

////////////////////////////////////////////////
void IgnRenderer::HandleKeyRelease(QKeyEvent *_e)
{
  if (_e->isAutoRepeat())
    return;

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  this->dataPtr->keyEvent.SetKey(0);

  this->dataPtr->keyEvent.SetControl(
    (_e->modifiers() & Qt::ControlModifier)
    && (_e->key() != Qt::Key_Control));
  this->dataPtr->keyEvent.SetShift(
    (_e->modifiers() & Qt::ShiftModifier)
    && (_e->key() != Qt::Key_Shift));
  this->dataPtr->keyEvent.SetAlt(
    (_e->modifiers() & Qt::AltModifier)
    && (_e->key() != Qt::Key_Alt));

  this->dataPtr->mouseEvent.SetControl(this->dataPtr->keyEvent.Control());
  this->dataPtr->mouseEvent.SetShift(this->dataPtr->keyEvent.Shift());
  this->dataPtr->mouseEvent.SetAlt(this->dataPtr->keyEvent.Alt());
  this->dataPtr->keyEvent.SetType(common::KeyEvent::RELEASE);
}

/////////////////////////////////////////////////
void IgnRenderer::BroadcastHoverPos()
{
  if (!this->dataPtr->hoverDirty)
    return;

  auto pos = this->ScreenToScene(this->dataPtr->mouseHoverPos);

  events::HoverToScene hoverToSceneEvent(pos);
  App()->sendEvent(App()->findChild<MainWindow *>(), &hoverToSceneEvent);
}

/////////////////////////////////////////////////
void IgnRenderer::BroadcastLeftClick()
{
  if (!this->dataPtr->mouseDirty)
    return;

  if (this->dataPtr->mouseEvent.Dragging())
    return;

  if (this->dataPtr->mouseEvent.Button() != common::MouseEvent::LEFT ||
      this->dataPtr->mouseEvent.Type() != common::MouseEvent::RELEASE)
    return;

  auto pos = this->ScreenToScene(this->dataPtr->mouseEvent.Pos());

  events::LeftClickToScene leftClickToSceneEvent(pos);
  App()->sendEvent(App()->findChild<MainWindow *>(), &leftClickToSceneEvent);
}

/////////////////////////////////////////////////
void IgnRenderer::BroadcastRightClick()
{
  if (!this->dataPtr->mouseDirty)
    return;

  if (this->dataPtr->mouseEvent.Dragging())
    return;

  if (this->dataPtr->mouseEvent.Button() != common::MouseEvent::RIGHT ||
      this->dataPtr->mouseEvent.Type() != common::MouseEvent::RELEASE)
    return;

  auto pos = this->ScreenToScene(this->dataPtr->mouseEvent.Pos());

  events::RightClickToScene rightClickToSceneEvent(pos);
  App()->sendEvent(App()->findChild<MainWindow *>(), &rightClickToSceneEvent);
}

/////////////////////////////////////////////////
void IgnRenderer::Initialize()
{
  if (this->initialized)
    return;

  std::map<std::string, std::string> params;
  params["useCurrentGLContext"] = "1";
  auto engine = rendering::engine(this->engineName, params);
  if (!engine)
  {
    ignerr << "Engine [" << this->engineName << "] is not supported"
           << std::endl;
    return;
  }

  // Scene
  auto scene = engine->SceneByName(this->sceneName);
  if (!scene)
  {
    igndbg << "Create scene [" << this->sceneName << "]" << std::endl;
    scene = engine->CreateScene(this->sceneName);
    scene->SetAmbientLight(this->ambientLight);
    scene->SetBackgroundColor(this->backgroundColor);
  }

  if (this->skyEnable)
  {
    scene->SetSkyEnabled(true);
  }

  auto root = scene->RootVisual();

  // Camera
  this->dataPtr->camera = scene->CreateCamera();
  this->dataPtr->camera->SetUserData("user-camera", true);
  root->AddChild(this->dataPtr->camera);
  this->dataPtr->camera->SetLocalPose(this->cameraPose);
  this->dataPtr->camera->SetImageWidth(this->textureSize.width());
  this->dataPtr->camera->SetImageHeight(this->textureSize.height());
  this->dataPtr->camera->SetAntiAliasing(8);
  this->dataPtr->camera->SetHFOV(M_PI * 0.5);
  // setting the size and calling PreRender should cause the render texture to
  // be rebuilt
  this->dataPtr->camera->PreRender();
  this->textureId = this->dataPtr->camera->RenderTextureGLId();

  // Ray Query
  this->dataPtr->rayQuery = this->dataPtr->camera->Scene()->CreateRayQuery();

  this->initialized = true;
}

/////////////////////////////////////////////////
void IgnRenderer::Destroy()
{
  auto engine = rendering::engine(this->engineName);
  if (!engine)
    return;
  auto scene = engine->SceneByName(this->sceneName);
  if (!scene)
    return;
  scene->DestroySensor(this->dataPtr->camera);

  // If that was the last sensor, destroy scene
  if (scene->SensorCount() == 0)
  {
    igndbg << "Destroy scene [" << scene->Name() << "]" << std::endl;
    engine->DestroyScene(scene);

    // TODO(anyone) If that was the last scene, terminate engine?
  }
}

/////////////////////////////////////////////////
void IgnRenderer::NewHoverEvent(const math::Vector2i &_hoverPos)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->mouseHoverPos = _hoverPos;
  this->dataPtr->hoverDirty = true;
}

/////////////////////////////////////////////////
void IgnRenderer::NewMouseEvent(const common::MouseEvent &_e)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->mouseEvent = _e;
  this->dataPtr->mouseDirty = true;
}

/////////////////////////////////////////////////
math::Vector3d IgnRenderer::ScreenToScene(
    const math::Vector2i &_screenPos) const
{
  // Normalize point on the image
  double width = this->dataPtr->camera->ImageWidth();
  double height = this->dataPtr->camera->ImageHeight();

  double nx = 2.0 * _screenPos.X() / width - 1.0;
  double ny = 1.0 - 2.0 * _screenPos.Y() / height;

  // Make a ray query
  this->dataPtr->rayQuery->SetFromCamera(
      this->dataPtr->camera, math::Vector2d(nx, ny));

  auto result = this->dataPtr->rayQuery->ClosestPoint();
  if (result)
    return result.point;

  // Set point to be 10m away if no intersection found
  return this->dataPtr->rayQuery->Origin() +
      this->dataPtr->rayQuery->Direction() * 10;
}

/////////////////////////////////////////////////
RenderThread::RenderThread()
{
  RenderWindowItem::Implementation::threads << this;
}

/////////////////////////////////////////////////
void RenderThread::RenderNext()
{
  this->context->makeCurrent(this->surface);

  if (!this->ignRenderer.initialized)
  {
    // Initialize renderer
    this->ignRenderer.Initialize();
  }

  // check if engine has been successfully initialized
  if (!this->ignRenderer.initialized)
  {
    ignerr << "Unable to initialize renderer" << std::endl;
    return;
  }

  this->ignRenderer.Render();

  emit TextureReady(this->ignRenderer.textureId, this->ignRenderer.textureSize);
}

/////////////////////////////////////////////////
void RenderThread::ShutDown()
{
  this->context->makeCurrent(this->surface);

  this->ignRenderer.Destroy();

  this->context->doneCurrent();
  delete this->context;

  // schedule this to be deleted only after we're done cleaning up
  this->surface->deleteLater();

  // Stop event processing, move the thread to GUI and make sure it is deleted.
  this->moveToThread(QGuiApplication::instance()->thread());
}

/////////////////////////////////////////////////
void RenderThread::SizeChanged()
{
  auto item = qobject_cast<QQuickItem *>(this->sender());
  if (!item)
  {
    ignerr << "Internal error, sender is not QQuickItem." << std::endl;
    return;
  }

  if (item->width() <= 0 || item->height() <= 0)
    return;

  this->ignRenderer.textureSize = QSize(item->width(), item->height());
  this->ignRenderer.textureDirty = true;
}

/////////////////////////////////////////////////
TextureNode::TextureNode(QQuickWindow *_window)
    : window(_window)
{
  // Our texture node must have a texture, so use the default 0 texture.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
  this->texture = this->window->createTextureFromId(0, QSize(1, 1));
#else
  void * nativeLayout;
  this->texture = this->window->createTextureFromNativeObject(
      QQuickWindow::NativeObjectTexture, &nativeLayout, 0, QSize(1, 1),
      QQuickWindow::TextureIsOpaque);
#endif
  this->setTexture(this->texture);
}

/////////////////////////////////////////////////
TextureNode::~TextureNode()
{
  delete this->texture;
}

/////////////////////////////////////////////////
void TextureNode::NewTexture(int _id, const QSize &_size)
{
  this->mutex.lock();
  this->id = _id;
  this->size = _size;
  this->mutex.unlock();

  // We cannot call QQuickWindow::update directly here, as this is only allowed
  // from the rendering thread or GUI thread.
  emit PendingNewTexture();
}

/////////////////////////////////////////////////
void TextureNode::PrepareNode()
{
  this->mutex.lock();
  int newId = this->id;
  QSize sz = this->size;
  this->id = 0;
  this->mutex.unlock();
  if (newId)
  {
    delete this->texture;
    // note: include QQuickWindow::TextureHasAlphaChannel if the rendered
    // content has alpha.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    this->texture = this->window->createTextureFromId(
        newId, sz, QQuickWindow::TextureIsOpaque);
#else
    // TODO(anyone) Use createTextureFromNativeObject
    // https://github.com/ignitionrobotics/ign-gui/issues/113
#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    this->texture = this->window->createTextureFromId(
        newId, sz, QQuickWindow::TextureIsOpaque);
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

#endif
    this->setTexture(this->texture);

    this->markDirty(DirtyMaterial);

    // This will notify the rendering thread that the texture is now being
    // rendered and it can start rendering to the other one.
    emit TextureInUse();
  }
}

/////////////////////////////////////////////////
RenderWindowItem::RenderWindowItem(QQuickItem *_parent)
  : QQuickItem(_parent), dataPtr(utils::MakeUniqueImpl<Implementation>())
{
  this->setAcceptedMouseButtons(Qt::AllButtons);
  this->setFlag(ItemHasContents);
  this->dataPtr->renderThread = new RenderThread();
}

/////////////////////////////////////////////////
void RenderWindowItem::Ready()
{
  this->dataPtr->renderThread->surface = new QOffscreenSurface();
  this->dataPtr->renderThread->surface->setFormat(
      this->dataPtr->renderThread->context->format());
  this->dataPtr->renderThread->surface->create();

  this->dataPtr->renderThread->ignRenderer.textureSize =
      QSize(std::max({this->width(), 1.0}), std::max({this->height(), 1.0}));

  this->dataPtr->renderThread->moveToThread(this->dataPtr->renderThread);

  this->connect(this, &QObject::destroyed,
      this->dataPtr->renderThread, &RenderThread::ShutDown,
      Qt::QueuedConnection);

  this->connect(this, &QQuickItem::widthChanged,
      this->dataPtr->renderThread, &RenderThread::SizeChanged);
  this->connect(this, &QQuickItem::heightChanged,
      this->dataPtr->renderThread, &RenderThread::SizeChanged);

  this->dataPtr->renderThread->start();
  this->update();
}

/////////////////////////////////////////////////
QSGNode *RenderWindowItem::updatePaintNode(QSGNode *_node,
    QQuickItem::UpdatePaintNodeData * /*_data*/)
{
  TextureNode *node = static_cast<TextureNode *>(_node);

  if (!this->dataPtr->renderThread->context)
  {
    QOpenGLContext *current = this->window()->openglContext();
    // Some GL implementations require that the currently bound context is
    // made non-current before we set up sharing, so we doneCurrent here
    // and makeCurrent down below while setting up our own context.
    current->doneCurrent();

    this->dataPtr->renderThread->context = new QOpenGLContext();
    this->dataPtr->renderThread->context->setFormat(current->format());
    this->dataPtr->renderThread->context->setShareContext(current);
    this->dataPtr->renderThread->context->create();
    this->dataPtr->renderThread->context->moveToThread(
        this->dataPtr->renderThread);

    current->makeCurrent(this->window());

    QMetaObject::invokeMethod(this, "Ready");
    return nullptr;
  }

  if (!node)
  {
    node = new TextureNode(this->window());

    // Set up connections to get the production of render texture in sync with
    // vsync on the rendering thread.
    //
    // When a new texture is ready on the rendering thread, we use a direct
    // connection to the texture node to let it know a new texture can be used.
    // The node will then emit PendingNewTexture which we bind to
    // QQuickWindow::update to schedule a redraw.
    //
    // When the scene graph starts rendering the next frame, the PrepareNode()
    // function is used to update the node with the new texture. Once it
    // completes, it emits TextureInUse() which we connect to the rendering
    // thread's RenderNext() to have it start producing content into its render
    // texture.
    //
    // This rendering pipeline is throttled by vsync on the scene graph
    // rendering thread.

    this->connect(this->dataPtr->renderThread, &RenderThread::TextureReady,
        node, &TextureNode::NewTexture, Qt::DirectConnection);
    this->connect(node, &TextureNode::PendingNewTexture, this->window(),
        &QQuickWindow::update, Qt::QueuedConnection);
    this->connect(this->window(), &QQuickWindow::beforeRendering, node,
        &TextureNode::PrepareNode, Qt::DirectConnection);
    this->connect(node, &TextureNode::TextureInUse, this->dataPtr->renderThread,
        &RenderThread::RenderNext, Qt::QueuedConnection);

    // Get the production of FBO textures started..
    QMetaObject::invokeMethod(this->dataPtr->renderThread, "RenderNext",
        Qt::QueuedConnection);
  }

  node->setRect(this->boundingRect());

  return node;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetBackgroundColor(const math::Color &_color)
{
  this->dataPtr->renderThread->ignRenderer.backgroundColor = _color;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetAmbientLight(const math::Color &_ambient)
{
  this->dataPtr->renderThread->ignRenderer.ambientLight = _ambient;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetEngineName(const std::string &_name)
{
  this->dataPtr->renderThread->ignRenderer.engineName = _name;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetSceneName(const std::string &_name)
{
  this->dataPtr->renderThread->ignRenderer.sceneName = _name;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetCameraPose(const math::Pose3d &_pose)
{
  this->dataPtr->renderThread->ignRenderer.cameraPose = _pose;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetSceneService(const std::string &_service)
{
  this->dataPtr->renderThread->ignRenderer.sceneService = _service;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetPoseTopic(const std::string &_topic)
{
  this->dataPtr->renderThread->ignRenderer.poseTopic = _topic;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetDeletionTopic(const std::string &_topic)
{
  this->dataPtr->renderThread->ignRenderer.deletionTopic = _topic;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetSceneTopic(const std::string &_topic)
{
  this->dataPtr->renderThread->ignRenderer.sceneTopic = _topic;
}

/////////////////////////////////////////////////
void RenderWindowItem::SetSkyEnabled(const bool &_sky)
{
  this->dataPtr->renderThread->ignRenderer.skyEnable = _sky;
}

/////////////////////////////////////////////////
MinimalScene::MinimalScene()
  : Plugin(), dataPtr(utils::MakeUniqueImpl<Implementation>())
{
  qmlRegisterType<RenderWindowItem>("RenderWindow", 1, 0, "RenderWindow");
}

/////////////////////////////////////////////////
void MinimalScene::LoadConfig(const tinyxml2::XMLElement *_pluginElem)
{
  RenderWindowItem *renderWindow =
      this->PluginItem()->findChild<RenderWindowItem *>();
  if (!renderWindow)
  {
    ignerr << "Unable to find Render Window item. "
           << "Render window will not be created" << std::endl;
    return;
  }

  if (this->title.empty())
    this->title = "3D Scene";

  // Custom parameters
  if (_pluginElem)
  {
    auto elem = _pluginElem->FirstChildElement("engine");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      renderWindow->SetEngineName(elem->GetText());
      // there is a problem with displaying ogre2 render textures that are in
      // sRGB format. Workaround for now is to apply gamma correction manually.
      // There maybe a better way to solve the problem by making OpenGL calls..
      if (elem->GetText() == std::string("ogre2"))
        this->PluginItem()->setProperty("gammaCorrect", true);
    }

    elem = _pluginElem->FirstChildElement("scene");
    if (nullptr != elem && nullptr != elem->GetText())
      renderWindow->SetSceneName(elem->GetText());

    elem = _pluginElem->FirstChildElement("ambient_light");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      math::Color ambient;
      std::stringstream colorStr;
      colorStr << std::string(elem->GetText());
      colorStr >> ambient;
      renderWindow->SetAmbientLight(ambient);
    }

    elem = _pluginElem->FirstChildElement("background_color");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      math::Color bgColor;
      std::stringstream colorStr;
      colorStr << std::string(elem->GetText());
      colorStr >> bgColor;
      renderWindow->SetBackgroundColor(bgColor);
    }

    elem = _pluginElem->FirstChildElement("camera_pose");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      math::Pose3d pose;
      std::stringstream poseStr;
      poseStr << std::string(elem->GetText());
      poseStr >> pose;
      renderWindow->SetCameraPose(pose);
    }

    elem = _pluginElem->FirstChildElement("service");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      std::string service = elem->GetText();
      renderWindow->SetSceneService(service);
    }

    elem = _pluginElem->FirstChildElement("pose_topic");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      std::string topic = elem->GetText();
      renderWindow->SetPoseTopic(topic);
    }

    elem = _pluginElem->FirstChildElement("deletion_topic");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      std::string topic = elem->GetText();
      renderWindow->SetDeletionTopic(topic);
    }

    elem = _pluginElem->FirstChildElement("scene_topic");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      std::string topic = elem->GetText();
      renderWindow->SetSceneTopic(topic);
    }

    elem = _pluginElem->FirstChildElement("sky");
    if (nullptr != elem && nullptr != elem->GetText())
    {
      renderWindow->SetSkyEnabled(true);
      if (!elem->NoChildren())
        ignwarn << "Child elements of <sky> are not supported yet" << std::endl;
    }
  }
}

/////////////////////////////////////////////////
void RenderWindowItem::OnHovered(const ignition::math::Vector2i &_hoverPos)
{
  this->dataPtr->renderThread->ignRenderer.NewHoverEvent(_hoverPos);
}

/////////////////////////////////////////////////
void RenderWindowItem::mousePressEvent(QMouseEvent *_e)
{
  auto event = convert(*_e);
  event.SetPressPos(event.Pos());
  this->dataPtr->mouseEvent = event;

  this->dataPtr->renderThread->ignRenderer.NewMouseEvent(
      this->dataPtr->mouseEvent);
}

////////////////////////////////////////////////
void RenderWindowItem::mouseReleaseEvent(QMouseEvent *_e)
{
  this->dataPtr->mouseEvent = convert(*_e);

  this->dataPtr->renderThread->ignRenderer.NewMouseEvent(
      this->dataPtr->mouseEvent);
}

////////////////////////////////////////////////
void RenderWindowItem::mouseMoveEvent(QMouseEvent *_e)
{
  auto event = convert(*_e);
  event.SetPressPos(this->dataPtr->mouseEvent.PressPos());

  if (!event.Dragging())
    return;

  this->dataPtr->renderThread->ignRenderer.NewMouseEvent(event);
  this->dataPtr->mouseEvent = event;
}

////////////////////////////////////////////////
void RenderWindowItem::wheelEvent(QWheelEvent *_e)
{
  this->dataPtr->mouseEvent.SetType(common::MouseEvent::SCROLL);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
  this->dataPtr->mouseEvent.SetPos(_e->x(), _e->y());
#else
  this->dataPtr->mouseEvent.SetPos(_e->position().x(), _e->position().y());
#endif
  double scroll = (_e->angleDelta().y() > 0) ? -1.0 : 1.0;
  this->dataPtr->renderThread->ignRenderer.NewMouseEvent(
    this->dataPtr->mouseEvent);
  this->dataPtr->mouseEvent.SetScroll(scroll, scroll);
}

////////////////////////////////////////////////
void RenderWindowItem::HandleKeyPress(QKeyEvent *_e)
{
  this->dataPtr->renderThread->ignRenderer.HandleKeyPress(_e);
}

////////////////////////////////////////////////
void RenderWindowItem::HandleKeyRelease(QKeyEvent *_e)
{
  this->dataPtr->renderThread->ignRenderer.HandleKeyRelease(_e);
}

/////////////////////////////////////////////////
bool MinimalScene::eventFilter(QObject *_obj, QEvent *_event)
{
  if (_event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(_event);
    if (keyEvent)
    {
      auto renderWindow = this->PluginItem()->findChild<RenderWindowItem *>();
      renderWindow->HandleKeyPress(keyEvent);
    }
  }
  else if (_event->type() == QEvent::KeyRelease)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(_event);
    if (keyEvent)
    {
      auto renderWindow = this->PluginItem()->findChild<RenderWindowItem *>();
      renderWindow->HandleKeyRelease(keyEvent);
    }
  }

  // Standard event processing
  return QObject::eventFilter(_obj, _event);
}

/////////////////////////////////////////////////
void MinimalScene::OnHovered(int _mouseX, int _mouseY)
{
  auto renderWindow = this->PluginItem()->findChild<RenderWindowItem *>();
  renderWindow->OnHovered({_mouseX, _mouseY});
}

/////////////////////////////////////////////////
void MinimalScene::OnFocusWindow()
{
  auto renderWindow = this->PluginItem()->findChild<RenderWindowItem *>();
  renderWindow->forceActiveFocus();
}

// Register this plugin
IGNITION_ADD_PLUGIN(ignition::gui::plugins::MinimalScene,
                    ignition::gui::Plugin)
