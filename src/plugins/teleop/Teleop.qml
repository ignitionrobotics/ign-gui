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

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import "qrc:/qml"

Rectangle {
  color:"transparent"
  Layout.minimumWidth: 300
  Layout.minimumHeight: 900
  anchors.fill: parent
  focus: true

  // Topic input
  Label {
    id: topicLabel
    text: "Topic:"
    anchors.top: parent.top
    anchors.topMargin: 10
    anchors.left: parent.left
    anchors.leftMargin: 5
  }
  TextField {
    id: topicField
    anchors.top: topicLabel.bottom
    anchors.topMargin: 5
    anchors.left: parent.left
    anchors.leftMargin: 5
    Layout.fillWidth: true
    text:"/cmd_vel"
    placeholderText: qsTr("Topic to publish...")
    onEditingFinished: {
      Teleop.OnTopicSelection(text)
    }
  }

  // Velocity input
  Label {
    id: velocityLabel
    text: "Velocity:"
    anchors.top: topicField.bottom
    anchors.topMargin: 10
    anchors.left: parent.left
    anchors.leftMargin: 5
  }
  // Linear velocity input
  Label {
    id: linearVelLabel
    text: "Linear"
    color: "dimgrey"
    anchors.top: velocityLabel.bottom
    anchors.topMargin: 15
    anchors.left: parent.left
    anchors.leftMargin: 5
  }
  IgnSpinBox {
    id: linearVelField
    anchors.top: velocityLabel.bottom
    anchors.topMargin: 5
    anchors.left: linearVelLabel.right
    anchors.leftMargin: 5
    Layout.fillWidth: true
    value: 0.0
    maximumValue: 10.0
    minimumValue: 0.0
    decimals: 2
    stepSize: 0.10
    onEditingFinished:{
      Teleop.OnLinearVelSelection(value)
    } 
  }
  
  // Angular velocity input
  Label {
    id: angularVelLabel
    text: "Angular"
    color: "dimgrey"
    anchors.top: velocityLabel.bottom
    anchors.topMargin: 15
    anchors.left: linearVelField.right
    anchors.leftMargin: 10
  }
  IgnSpinBox {
    id: angularVelField
    anchors.top: velocityLabel.bottom
    anchors.topMargin: 5
    anchors.left: angularVelLabel.right
    anchors.leftMargin: 5
    Layout.fillWidth: true
    value: 0.0
    maximumValue: 2.0
    minimumValue: 0.0
    decimals: 2
    stepSize: 0.10
    onEditingFinished:{
      Teleop.OnAngularVelSelection(value)
    } 
  }

  // Button grid
  GridLayout {
    id: buttonsGrid
    anchors.top: angularVelField.bottom
    anchors.topMargin: 15
    anchors.left: parent.left
    anchors.leftMargin: 40
    Layout.fillWidth: true
    columns: 4
    Button {
      id: forwardButton
      text: "\u25B2"
      checkable: true
      Layout.row: 0
      Layout.column: 1
      onClicked: {
        Teleop.linearDir = forwardButton.checked ? 1 : 0
        if(backwardButton.checked)
          backwardButton.checked = false
        slidersSwitch.checked = false
        Teleop.OnTeleopTwist()
      }
      ToolTip.visible: hovered
      ToolTip.text: "Forward"
      Material.background: Material.primary
      contentItem: Label {
        renderType: Text.NativeRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.family: "Helvetica"
        font.pointSize: 10
        color: "black"
        text: forwardButton.text
      }
    }
    Button {
      id: leftButton
      text: "\u25C0"
      checkable: true
      Layout.row: 1
      Layout.column: 0
      onClicked: {
        Teleop.angularDir = leftButton.checked ? 1 : 0
        if(rightButton.checked)
          rightButton.checked = false
        slidersSwitch.checked = false
        Teleop.OnTeleopTwist()
      }
      Material.background: Material.primary
      contentItem: Label {
        renderType: Text.NativeRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.family: "Helvetica"
        font.pointSize: 10
        color: "black"
        text: leftButton.text
      }
    }
    Button {
      id: rightButton
      text: "\u25B6"
      checkable: true
      Layout.row: 1
      Layout.column: 2
      onClicked: {
        Teleop.angularDir = rightButton.checked ? -1 : 0
        if(leftButton.checked)
          leftButton.checked = false
        slidersSwitch.checked = false
        Teleop.OnTeleopTwist()
      }
      Material.background: Material.primary
      contentItem: Label {
        renderType: Text.NativeRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.family: "Helvetica"
        font.pointSize: 10
        color: "black"
        text: rightButton.text
      }
    }
    Button {
      id: backwardButton
      text: "\u25BC"
      checkable: true
      Layout.row: 2
      Layout.column: 1
      onClicked: {
        Teleop.linearDir = backwardButton.checked ? -1 : 0
        if(forwardButton.checked)
          forwardButton.checked = false
        slidersSwitch.checked = false
        Teleop.OnTeleopTwist()
      }
      Material.background: Material.primary
      contentItem: Label {
        renderType: Text.NativeRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.family: "Helvetica"
        font.pointSize: 10
        color: "black"
        text: backwardButton.text
      }
    }
    Button {
      id: stopButton
      text: "Stop"
      checkable: false
      Layout.row: 1
      Layout.column: 1
      onClicked: {
        Teleop.linearDir = 0
        Teleop.angularDir = 0
        forwardButton.checked = false
        leftButton.checked = false
        rightButton.checked = false
        backwardButton.checked = false
        linearVelSlider.value = 0
        angularVelSlider.value = 0
        slidersSwitch.checked = false
        Teleop.OnTeleopTwist()
      }
      Material.background: Material.primary
      contentItem: Label {
        renderType: Text.NativeRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.family: "Helvetica"
        font.pointSize: 10
        color: "black"
        text: stopButton.text
      }
    }
  }

  //Keyboard's switch
  Switch {
    id: keySwitch
    anchors.top: buttonsGrid.bottom
    anchors.topMargin: 10
    anchors.left: parent.left
    anchors.leftMargin: 5
    onClicked: {
      forwardButton.checked = false
      leftButton.checked = false
      rightButton.checked = false
      backwardButton.checked = false
      Teleop.OnKeySwitch(checked);
    }
    ToolTip.visible: hovered
    ToolTip.text: checked ? qsTr("Disable keyboard") : qsTr("Enable keyboard")
  }
  Label {
    id: keyboardSwitchLabel
    text: "Input from keyboard (WASD)"
    anchors.horizontalCenter : keySwitch.horizontalCenter
    anchors.verticalCenter : keySwitch.verticalCenter
    anchors.left: keySwitch.right
    anchors.leftMargin: 5
  }

  // Slider's switch
  Switch {
    id: slidersSwitch
    anchors.top: keySwitch.bottom
    anchors.topMargin: 10
    anchors.left: keySwitch.left
    onClicked: {
      Teleop.OnSlidersSwitch(checked);
      if(checked){
        forwardButton.checked = false
        leftButton.checked = false
        rightButton.checked = false
        backwardButton.checked = false
        linearVelField.value = linearVelSlider.value.toFixed(2)
        angularVelField.value = angularVelSlider.value.toFixed(2)
        Teleop.OnLinearVelSelection(linearVelSlider.value)
        Teleop.OnAngularVelSelection(angularVelSlider.value)
        Teleop.OnTeleopTwist()
      }
    }
    ToolTip.visible: hovered
    ToolTip.text: checked ? qsTr("Disable sliders") : qsTr("Enable sliders")
  }
  Label {
    id: slidersSwitchLabel
    text: "Input from sliders"
    anchors.horizontalCenter : slidersSwitch.horizontalCenter
    anchors.verticalCenter : slidersSwitch.verticalCenter
    anchors.left: slidersSwitch.right
    anchors.leftMargin: 5
  }

  TextField {
    id: linearVelMaxTextField
    anchors.top: slidersSwitch.bottom
    anchors.topMargin: 10
    anchors.horizontalCenter : angularVelSlider.horizontalCenter
    width: 40
    text:"1.0"
  }

  // Vertical slider
  Slider {
    id: linearVelSlider
    height: 150
    width: 50
    orientation: Qt.Vertical
    anchors.top: linearVelMaxTextField.bottom
    anchors.horizontalCenter : angularVelSlider.horizontalCenter
    handle: Rectangle {
      y: linearVelSlider.topPadding + linearVelSlider.visualPosition * (linearVelSlider.availableHeight - height)
      x: linearVelSlider.leftPadding + linearVelSlider.availableWidth / 2 - width / 2
      implicitWidth: 25
      implicitHeight: 10
      color: linearVelSlider.pressed ? "#f0f0f0" : "#f6f6f6"
      border.color: "black"
    }
    enabled: slidersSwitch.checked

    from: linearVelMinTextField.text
    to: linearVelMaxTextField.text
    stepSize: 0.01

    onMoved: {
      linearVelField.value = linearVelSlider.value.toFixed(2)
      Teleop.OnLinearVelSelection(linearVelSlider.value)
      Teleop.OnTeleopTwist()
    }
  }

  TextField {
    id: linearVelMinTextField
    anchors.top: linearVelSlider.bottom
    anchors.horizontalCenter : linearVelSlider.horizontalCenter
    width: 40
    text:"-1.0"
  }

  Label {
    id: currentLinearVelSliderLabel
    anchors.verticalCenter : linearVelSlider.verticalCenter
    anchors.left : linearVelSlider.right
    text: linearVelSlider.value.toFixed(2) + " m/s"
  }

  TextField {
    id: angularVelMinTextField
    anchors.verticalCenter : angularVelSlider.verticalCenter
    anchors.left : slidersSwitch.left
    width: 40
    text:"-1.0"
  }

  // Horizontal slider
  Slider {
    id: angularVelSlider
    height: 50
    width: 175
    anchors.top: linearVelSlider.bottom
    anchors.topMargin: 50
    anchors.left : angularVelMinTextField.right
    anchors.leftMargin: 10
    handle: Rectangle {
      x: angularVelSlider.leftPadding + angularVelSlider.visualPosition * (angularVelSlider.availableWidth - width)
      y: angularVelSlider.topPadding + angularVelSlider.availableHeight / 2 - height / 2
      implicitWidth: 10
      implicitHeight: 25
      color: angularVelSlider.pressed ? "#f0f0f0" : "#f6f6f6"
      border.color: "black"
    }
    enabled: slidersSwitch.checked

    from: angularVelMinTextField.text
    to: angularVelMaxTextField.text
    stepSize: 0.01

    onMoved: {
      angularVelField.value = angularVelSlider.value.toFixed(2)
      Teleop.OnAngularVelSelection(angularVelSlider.value)
      Teleop.OnTeleopTwist()
    }
  }

  TextField {
    id: angularVelMaxTextField
    anchors.verticalCenter : angularVelSlider.verticalCenter
    anchors.left : angularVelSlider.right
    anchors.leftMargin: 10
    width: 40
    text:"1.0"
  }

  Label {
    id: currentAngularVelSliderLabel
    anchors.horizontalCenter : angularVelSlider.horizontalCenter
    anchors.top : angularVelSlider.bottom
    text: angularVelSlider.value.toFixed(2) + " rad/s"
  }
}
