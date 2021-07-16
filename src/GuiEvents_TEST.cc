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

#include <gtest/gtest.h>

#include "test_config.h"  // NOLINT(build/include)
#include "ignition/gui/GuiEvents.hh"

using namespace ignition;
using namespace gui;

/////////////////////////////////////////////////
TEST(GuiEventsTest, Render)
{
  events::Render event;

  EXPECT_LT(QEvent::User, event.type());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, SnapIntervals)
{
  events::SnapIntervals event({1, 2, 3}, {4, 5, 6}, {7, 8, 9});

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(math::Vector3d(1, 2, 3), event.Position());
  EXPECT_EQ(math::Vector3d(4, 5, 6), event.Rotation());
  EXPECT_EQ(math::Vector3d(7, 8, 9), event.Scale());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, SpawnFromDescription)
{
  events::SpawnFromDescription event("banana");

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ("banana", event.Description());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, SpawnFromPath)
{
  events::SpawnFromPath event("banana");

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ("banana", event.FilePath());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, HoverToScene)
{
  events::HoverToScene event({1, 2, 3});

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(math::Vector3d(1, 2, 3), event.Point());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, LeftClickToScene)
{
  events::LeftClickToScene event({1, 2, 3});

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(math::Vector3d(1, 2, 3), event.Point());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, LeftClickOnScene)
{
  ignition::common::MouseEvent mouse;
  mouse.SetAlt(true);
  mouse.SetShift(true);
  events::LeftClickOnScene event(mouse);

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_FALSE(event.Mouse().Control());
  EXPECT_TRUE(event.Mouse().Alt());
  EXPECT_TRUE(event.Mouse().Shift());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, RightClickToScene)
{
  events::RightClickToScene event({1, 2, 3});

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(math::Vector3d(1, 2, 3), event.Point());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, RightClickOnScene)
{
  ignition::common::MouseEvent mouse;
  mouse.SetControl(true);
  mouse.SetAlt(true);
  events::RightClickOnScene event(mouse);

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_TRUE(event.Mouse().Control());
  EXPECT_TRUE(event.Mouse().Alt());
  EXPECT_FALSE(event.Mouse().Shift());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, KeyPressOnScene)
{
  ignition::common::KeyEvent key;
  key.SetKey(49);
  key.SetControl(true);
  key.SetAlt(false);
  key.SetShift(false);
  events::KeyPressOnScene event(key);

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(49, event.Key().Key());
  EXPECT_TRUE(event.Key().Control());
  EXPECT_FALSE(event.Key().Shift());
  EXPECT_FALSE(event.Key().Alt());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, KeyReleaseOnScene)
{
  ignition::common::KeyEvent key;
  key.SetKey(49);
  key.SetControl(true);
  key.SetAlt(true);
  key.SetShift(true);
  events::KeyReleaseOnScene event(key);

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(49, event.Key().Key());
  EXPECT_TRUE(event.Key().Control());
  EXPECT_TRUE(event.Key().Shift());
  EXPECT_TRUE(event.Key().Alt());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, DropdownMenuEnabled)
{
  events::DropdownMenuEnabled event(true);

  EXPECT_LT(QEvent::User, event.type());
  EXPECT_EQ(true, event.MenuEnabled());

  events::DropdownMenuEnabled event2(false);

  EXPECT_LT(QEvent::User, event2.type());
  EXPECT_EQ(false, event2.MenuEnabled());
}

/////////////////////////////////////////////////
TEST(GuiEventsTest, BlockOrbit)
{
  events::BlockOrbit event(true);
  EXPECT_LT(QEvent::User, event.type());
  EXPECT_TRUE(event.Block());

  events::BlockOrbit event2(false);
  EXPECT_LT(QEvent::User, event2.type());
  EXPECT_FALSE(event2.Block());
}
