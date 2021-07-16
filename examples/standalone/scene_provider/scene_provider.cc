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

#include <chrono>
#include <iostream>
#include <string>
#include <ignition/math/Rand.hh>
#include <ignition/msgs/pose_v.pb.h>
#include <ignition/msgs/scene.pb.h>
#include <ignition/msgs/scene.pb.h>
#include <ignition/msgs/world_stats.pb.h>
#include <ignition/transport/Node.hh>

using namespace std::chrono_literals;

//////////////////////////////////////////////////
bool sceneService(ignition::msgs::Scene &_rep)
{
  std::cout << "Returning scene" << std::endl;

  // Light
  auto lightMsg = _rep.add_light();

  auto diffuseMsg = lightMsg->mutable_diffuse();
  diffuseMsg->set_r(1.0);
  diffuseMsg->set_g(1.0);
  diffuseMsg->set_b(1.0);

  auto directionMsg = lightMsg->mutable_direction();
  directionMsg->set_x(0.0);
  directionMsg->set_y(0.0);
  directionMsg->set_z(1.0);

  // Box
  auto modelMsg = _rep.add_model();
  modelMsg->set_id(1);
  modelMsg->set_is_static(true);
  modelMsg->set_name("box_model");

  auto linkMsg = modelMsg->add_link();
  linkMsg->set_id(2);
  linkMsg->set_name("box_link");

  auto visMsg = linkMsg->add_visual();
  visMsg->set_id(3);
  visMsg->set_name("box_vis");

  auto geomMsg = visMsg->mutable_geometry();
  auto boxMsg = geomMsg->mutable_box();
  auto boxSize = boxMsg->mutable_size();
  boxSize->set_x(1.0);
  boxSize->set_y(2.0);
  boxSize->set_z(3.0);

  return true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ignition::transport::Node node;

  // Scene service
  node.Advertise("/example/scene", sceneService);

  // Periodic pose updated
  auto statsPub =
    node.Advertise<ignition::msgs::WorldStatistics>("/example/stats");

  // Periodic pose updated
  auto posePub = node.Advertise<ignition::msgs::Pose_V>("/example/pose");

  ignition::msgs::Pose_V poseVMsg;
  auto poseMsg = poseVMsg.add_pose();
  poseMsg->set_id(1);
  poseMsg->set_name("box_model");
  auto positionMsg = poseMsg->mutable_position();

  const double change{0.1};

  double x{0.0};
  double y{0.0};
  double z{0.0};

  std::chrono::steady_clock::duration timePoint =
    std::chrono::steady_clock::duration::zero();
  ignition::msgs::WorldStatistics msgWorldStatistics;

  while (true)
  {
    std::this_thread::sleep_for(100ms);

    x += ignition::math::Rand::DblUniform(-change, change);
    y += ignition::math::Rand::DblUniform(-change, change);
    z += ignition::math::Rand::DblUniform(-change, change);

    positionMsg->set_x(x);
    positionMsg->set_y(y);
    positionMsg->set_z(z);
    posePub.Publish(poseVMsg);

    timePoint += 100ms;
    msgWorldStatistics.set_real_time_factor(1);

    auto s = std::chrono::duration_cast<std::chrono::seconds>(timePoint);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(timePoint-s);

    msgWorldStatistics.mutable_sim_time()->set_sec(s.count());
    msgWorldStatistics.mutable_sim_time()->set_nsec(ns.count());
    statsPub.Publish(msgWorldStatistics);
  }

  ignition::transport::waitForShutdown();
}
