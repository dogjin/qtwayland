/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtWayland.Compositor 1.0

WaylandCompositor {
    id: compositor

    property var primarySurfacesArea: null

    Component {
        id: screenComponent
        Screen { }
    }

    Component {
        id: chromeComponent
        Chrome {
        }
    }

    Component {
        id: surfaceComponent
        WaylandSurface {
            property QtObject shellSurface: null
        }
    }

    extensions: [
        DefaultShell {
            id: defaultShell

            Component {
                id: shellSurfaceComponent
                DefaultShellSurface {
                    property Item chrome
                    property var previousMousePosition
                    property var originalInputEventsEnabled
                    Connections {
                        target: chrome ? chrome : null
                        onMouseMove: {
                            var deltaX = windowPosition.x - previousMousePosition.x
                            var deltaY = windowPosition.y - previousMousePosition.y
                            chrome.x = chrome.x + deltaX
                            chrome.y = chrome.y + deltaY
                            previousMousePosition = windowPosition;
                        }
                        onMouseRelease: {
                            chrome.inputEventsEnabled = originalInputEventsEnabled;
                        }
                    }

                    onStartMove: {
                        previousMousePosition = chrome.mousePressPosition
                        originalInputEventsEnabled = chrome.inputEventsEnabled
                        chrome.inputEventsEnabled = false;
                    }

                }
            }

            onCreateShellSurface: {
                var item = chromeComponent.createObject(defaultOutput.surfaceArea, { "surface": surface } );
                var shellSurface = shellSurfaceComponent.createObject( null, { "chrome": item });
                shellSurface.chrome = item;
                shellSurface.initialize(defaultShell, surface, client, id);
                surface.shellSurface = shellSurface;
            }

            Component.onCompleted: {
                initialize();
            }
        }
    ]

    onCreateSurface: {
        var surface = surfaceComponent.createObject(0, { } );
        surface.initialize(compositor, client, id, version);

    }

    Component.onCompleted: {
        screenComponent.createObject(0, { "outputSpace" : defaultOutputSpace } );
    }
}
