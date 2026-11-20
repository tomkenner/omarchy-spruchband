QT += core gui qml quick quickcontrols2 dbus network

CONFIG += c++20 release
TARGET = spruchband
TEMPLATE = app
DEFINES += APP_VERSION=\\\"1.0.1\\\"
LIBS += -lLayerShellQtInterface

HEADERS += \
    src/backend.h \
    src/banners.h \
    src/daemon.h \
    src/desk.h \
    src/figures.h \
    src/gfx.h \
    src/ipc.h \
    src/overlay.h \
    src/portal.h \
    src/preview.h \
    src/settings.h \
    src/stage.h \
    src/status.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/banners.cpp \
    src/daemon.cpp \
    src/desk.cpp \
    src/figures.cpp \
    src/gfx.cpp \
    src/ipc.cpp \
    src/overlay.cpp \
    src/portal.cpp \
    src/preview.cpp \
    src/settings.cpp \
    src/stage.cpp \
    src/status.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc src/fonts.qrc
