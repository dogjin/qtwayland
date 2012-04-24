/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mockclient.h"

#include <QElapsedTimer>
#include <QSocketNotifier>

#include <private/qguiapplication_p.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>


MockClient::MockClient()
    : display(wl_display_connect(0))
    , compositor(0)
    , output(0)
{
    if (!display)
        qFatal("MockClient(): wl_display_connect() failed");

    wl_display_add_global_listener(display, handleGlobal, this);

    fd = wl_display_get_fd(display, 0, 0);

    QSocketNotifier *readNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(readNotifier, SIGNAL(activated(int)), this, SLOT(readEvents()));

    QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::eventDispatcher;
    connect(dispatcher, SIGNAL(awake()), this, SLOT(flushDisplay()));

    QElapsedTimer timeout;
    timeout.start();
    do {
        QCoreApplication::processEvents();
    } while (!(compositor && output) && timeout.elapsed() < 1000);

    if (!compositor || !output)
        qFatal("MockClient(): failed to receive globals from display");
}

const wl_output_listener MockClient::outputListener = {
    MockClient::outputGeometryEvent,
    MockClient::outputModeEvent
};

MockClient::~MockClient()
{
    wl_display_disconnect(display);
}

void MockClient::handleGlobal(wl_display *, uint32_t id, const char *interface, uint32_t, void *data)
{
    resolve(data)->handleGlobal(id, QByteArray(interface));
}

void MockClient::outputGeometryEvent(void *data, wl_output *,
                                     int32_t x, int32_t y,
                                     int32_t width, int32_t height,
                                     int, const char *, const char *)
{
    resolve(data)->geometry = QRect(x, y, width, height);
}

void MockClient::outputModeEvent(void *, wl_output *, uint32_t,
                                 int, int, int)
{
}

void MockClient::readEvents()
{
    wl_display_iterate(display, WL_DISPLAY_READABLE);
}

void MockClient::flushDisplay()
{
    wl_display_flush(display);
}

void MockClient::handleGlobal(uint32_t id, const QByteArray &interface)
{
    if (interface == "wl_compositor") {
        compositor = static_cast<wl_compositor *>(wl_display_bind(display, id, &wl_compositor_interface));
    } else if (interface == "wl_output") {
        output = static_cast<wl_output *>(wl_display_bind(display, id, &wl_output_interface));
        wl_output_add_listener(output, &outputListener, this);
    } else if (interface == "wl_shm") {
        shm = static_cast<wl_shm *>(wl_display_bind(display, id, &wl_shm_interface));
    }
}

wl_surface *MockClient::createSurface()
{
    flushDisplay();
    return wl_compositor_create_surface(compositor);
}

ShmBuffer::ShmBuffer(const QSize &size, wl_shm *shm)
    : handle(0)
{
    int stride = size.width() * 4;
    int alloc = stride * size.height();

    char filename[] = "/tmp/wayland-shm-XXXXXX";

    int fd = mkstemp(filename);
    if (fd < 0) {
        qWarning("open %s failed: %s", filename, strerror(errno));
        return;
    }

    if (ftruncate(fd, alloc) < 0) {
        qWarning("ftruncate failed: %s", strerror(errno));
        close(fd);
        return;
    }

    void *data = mmap(0, alloc, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unlink(filename);

    if (data == MAP_FAILED) {
        qWarning("mmap failed: %s", strerror(errno));
        close(fd);
        return;
    }

    image = QImage(static_cast<uchar *>(data), size.width(), size.height(), stride, QImage::Format_ARGB32_Premultiplied);
    shm_pool = wl_shm_create_pool(shm,fd,alloc);
    handle = wl_shm_pool_create_buffer(shm_pool,0, size.width(), size.height(),
                                   stride, WL_SHM_FORMAT_ARGB8888);
    close(fd);
}

ShmBuffer::~ShmBuffer()
{
    munmap(image.bits(), image.byteCount());
    wl_buffer_destroy(handle);
    wl_shm_pool_destroy(shm_pool);
}
