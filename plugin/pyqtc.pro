TARGET = pyqtc
TEMPLATE = lib

DEFINES += PYQTC_LIBRARY

# pyqtc files

SOURCES += \
    plugin.cpp \
    pythonfilter.cpp

HEADERS +=\
    plugin.h \
    pythonfilter.h

OTHER_FILES = pyqtc.pluginspec


# Qt Creator linking

## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/home/david/src/qt-creator

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=/home/david/src/qt-creator

PROVIDER = davidsansome

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/locator/locator.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia
