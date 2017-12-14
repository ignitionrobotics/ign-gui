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

#include <string>
#include <iostream>

#include "ignition/gui/CollapsibleWidget.hh"
#include "ignition/gui/Helpers.hh"

namespace ignition
{
  namespace gui
  {
    class CollapsibleWidgetPrivate
    {
      /// \brief Whether the widget is collapsed or expanded.
      public: bool expanded = false;

      /// \brief Widget which holds the collapsible content.
      public: QWidget *content;
    };
  }
}

using namespace ignition;
using namespace gui;

/////////////////////////////////////////////////
CollapsibleWidget::CollapsibleWidget(const std::string &_key)
    : dataPtr(new CollapsibleWidgetPrivate)
{
  // Button label
  auto buttonLabel = new QLabel(tr(humanReadable(_key).c_str()));
  buttonLabel->setToolTip(tr(_key.c_str()));
  buttonLabel->setObjectName("collapsibleButtonLabel");

  // Button icon initialized to ▲
  auto buttonIcon = new QLabel(QString::fromUtf8("\u25b2"));
  buttonIcon->setObjectName("buttonIcon");

  // Button layout
  auto buttonLayout = new QHBoxLayout();
  buttonLayout->addWidget(buttonLabel);
  buttonLayout->addWidget(buttonIcon);
  buttonLayout->setAlignment(buttonIcon, Qt::AlignRight);

  // Button
  auto button = new QPushButton();
  button->setLayout(buttonLayout);
  button->setCheckable(true);
  button->setObjectName("collapsibleButton");

  this->connect(button, SIGNAL(toggled(bool)), this, SLOT(Toggle(bool)));

  // Content
  this->dataPtr->content = new QWidget();
  this->dataPtr->content->setObjectName("collapsibleContent");
  this->dataPtr->content->setVisible(false);
  this->dataPtr->content->setLayout(new QVBoxLayout());
  this->dataPtr->content->layout()->setContentsMargins(0, 0, 0, 0);
  this->dataPtr->content->layout()->setSpacing(0);

  // Layout
  auto mainLayout = new QVBoxLayout;
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(button);
  mainLayout->addWidget(this->dataPtr->content);
  this->setLayout(mainLayout);
  this->setEnabled(false);
}

/////////////////////////////////////////////////
CollapsibleWidget::~CollapsibleWidget()
{
}

/////////////////////////////////////////////////
void CollapsibleWidget::Toggle(const bool _checked)
{
  // Toggle the content
  this->dataPtr->content->setVisible(_checked);

  auto icon = this->findChild<QLabel *>("buttonIcon");
  // Change to ▼
  if (_checked)
  {
    icon->setText(QString::fromUtf8("\u25bc"));

    // Get the index property. If this property is set, then we assume the
    // collapsible widget is part of a list of collapsible widgets. The first
    // widget in a list of collapsible widgets should not alter the top
    // margin.
    QVariant indexProperty = this->property("index");
    this->layout()->setContentsMargins(0,
        this->ContentCount() > 0 && indexProperty.isValid() &&
        indexProperty.toInt() == 0 ? 0 : 16,
        0,
        this->ContentCount() > 0 ? 16 : 0);
  }
  // Change to ▲
  else
  {
    icon->setText(QString::fromUtf8("\u25b2"));
    this->layout()->setContentsMargins(0, 0, 0, 0);
  }

  this->dataPtr->expanded = _checked;
}

/////////////////////////////////////////////////
bool CollapsibleWidget::SetValue(const QVariant _value)
{
  // Set value of first property
  auto prop = this->findChild<PropertyWidget *>();

  if (!prop)
    return false;

  return prop->SetValue(_value);
}

/////////////////////////////////////////////////
QVariant CollapsibleWidget::Value() const
{
  // Get value of first property
  auto prop = this->findChild<PropertyWidget *>();

  if (!prop)
    return QVariant();

  return prop->Value();
}

/////////////////////////////////////////////////
bool CollapsibleWidget::IsExpanded() const
{
  return this->dataPtr->expanded;
}

/////////////////////////////////////////////////
void CollapsibleWidget::SetReadOnly(const bool _readOnly,
                                    const bool /*_explicit*/)
{
  // Only apply to properties, but no other widgets, such as the button
  auto props = this->findChildren<PropertyWidget *>();
  for (auto prop : props)
    prop->SetReadOnly(_readOnly, false);
}

/////////////////////////////////////////////////
bool CollapsibleWidget::ReadOnly() const
{
  // Not read-only if at least one child isn't
  auto props = this->findChildren<PropertyWidget *>();
  for (auto prop : props)
  {
    if (!prop->ReadOnly())
      return false;
  }

  return true;
}

/////////////////////////////////////////////////
void CollapsibleWidget::AppendContent(QWidget *_widget)
{
  this->dataPtr->content->layout()->addWidget(_widget);
  this->setEnabled(true);
}

/////////////////////////////////////////////////
unsigned int CollapsibleWidget::ContentCount() const
{
  return this->dataPtr->content->layout()->count();
}

