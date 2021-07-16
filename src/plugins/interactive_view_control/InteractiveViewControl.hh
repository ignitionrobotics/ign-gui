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

#ifndef IGNITION_GUI_PLUGINS_INTERACTIVEVIEWCONTROL_HH_
#define IGNITION_GUI_PLUGINS_INTERACTIVEVIEWCONTROL_HH_

#include <memory>

#include "ignition/gui/Plugin.hh"

namespace ignition
{
namespace gui
{
namespace plugins
{
  class InteractiveViewControlPrivate;

  /// \brief This Plugin allows to control the scene3D camera
  /// with the mouse
  class InteractiveViewControl : public Plugin
  {
    Q_OBJECT

    /// \brief Constructor
    public: InteractiveViewControl();

    /// \brief Destructor
    public: virtual ~InteractiveViewControl();

    // Documentation inherited
    public: virtual void LoadConfig(const tinyxml2::XMLElement *_pluginElem)
        override;

    // Documentation inherited
    private: bool eventFilter(QObject *_obj, QEvent *_event) override;

    /// \internal
    /// \brief Pointer to private data.
    private: std::unique_ptr<InteractiveViewControlPrivate> dataPtr;
  };
}
}
}

#endif
