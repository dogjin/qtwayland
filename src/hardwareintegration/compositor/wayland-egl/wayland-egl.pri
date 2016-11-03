INCLUDEPATH += $$PWD

!contains(QT_CONFIG, no-pkg-config) {
    CONFIG += link_pkgconfig
    PKGCONFIG += wayland-server wayland-egl
} else {
    LIBS += -lwayland-egl -lwayland-server
}

CONFIG += egl
QT += egl_support-private

SOURCES += \
    $$PWD/waylandeglclientbufferintegration.cpp

HEADERS += \
    $$PWD/waylandeglclientbufferintegration.h
