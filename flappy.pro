TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

win32:INCLUDEPATH +=  C:/Software/SDL/include

win32:LIBS += -LC:/Software/SDL/lib -lmingw32 -mwindows -mconsole -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf
unix:LIBS += -L/usr/lib -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf

QMAKE_CXXFLAGS += -std=c++11
