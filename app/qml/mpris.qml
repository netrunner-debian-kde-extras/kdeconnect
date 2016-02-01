/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ColumnLayout
{
    id: root
    property QtObject mprisInterface

    Component.onCompleted: {
        mprisInterface.requestPlayerList();
    }

    Item { Layout.fillHeight: true }
    ComboBox {
        Layout.fillWidth: true
        model: root.mprisInterface.playerList
        onCurrentTextChanged: root.mprisInterface.player = currentText
    }
    Label {
        Layout.fillWidth: true
        text: root.mprisInterface.nowPlaying
    }
    RowLayout {
        Layout.fillWidth: true
        Button {
            Layout.fillWidth: true
            iconName: "media-skip-backward"
            onClicked: root.mprisInterface.sendAction("Previous")
        }
        Button {
            Layout.fillWidth: true
            iconName: root.mprisInterface.isPlaying ? "media-playback-pause" : "media-playback-start"
            onClicked: root.mprisInterface.sendAction("PlayPause");
        }
        Button {
            Layout.fillWidth: true
            iconName: "media-skip-forward"
            onClicked: root.mprisInterface.sendAction("Next")
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { text: i18n("Volume:") }
        Slider {
            value: root.mprisInterface.volume
            maximumValue: 100
            Layout.fillWidth: true
        }
    }
    Item { Layout.fillHeight: true }
}
