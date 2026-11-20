QT += core gui quick quickcontrols2 testlib dbus network
CONFIG += c++20 testcase
TARGET = spruchband_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/banners.h ../src/desk.h ../src/figures.h ../src/gfx.h ../src/ipc.h \
           ../src/portal.h ../src/settings.h ../src/stage.h ../src/status.h ../src/theme.h ../src/translator.h
SOURCES += spruchband_tests.cpp ../src/backend.cpp ../src/banners.cpp ../src/desk.cpp ../src/figures.cpp \
           ../src/gfx.cpp ../src/ipc.cpp ../src/portal.cpp ../src/settings.cpp ../src/stage.cpp ../src/status.cpp \
           ../src/theme.cpp ../src/translator.cpp
RESOURCES += ../src/fonts.qrc
