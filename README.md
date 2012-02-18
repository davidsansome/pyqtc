Pyqtc is a plugin for Qt Creator that adds Python code-completion, navigation
and documentation.

How to install
==============

Dependencies
------------

You need:

  * Protobuf 2.4.0 or greater.
    Some distributions don't ship version 2.4.0 yet.  If yours doesn't you must
    download it from http://code.google.com/p/protobuf/downloads/list and
    compile from source.
  * Qt Creator 2.4 or greater.
  * The Qt Creator source code.  You don't have to compile Qt Creator yourself,
    but you do need to have the source available.  You can download it from
    http://qt.nokia.com/downloads/downloads#qt-creator
  * Python and Pylint

Compiling pyqtc
---------------

You need to tell cmake the locations of the Qt Creator source and binaries.
These might be in the same place if you've built Qt Creator yourself, otherwise
they will be different.  Pyqtc will be installed into the binary directory, the
source directory will be used to find header files.

  mkdir bin
  cd bin
  cmake .. -DQTC_SOURCE=/path/to/qtcreator/source \
           -DQTC_BINARY=/path/to/qtcreator/installation
  make
  make install

Features
========

Code completion
---------------

![Code completion](https://raw.github.com/davidsansome/pyqtc/master/images/completion.png)

Pyqtc statically analyses your Python code as you type and offers code
completion suggestions automatically after a period (.), or after pressing
Ctrl+Space.

Function calltips
-----------------

![Function calltips](https://raw.github.com/davidsansome/pyqtc/master/images/completion.png)

After an opening bracket, Pyqtc will show a tooltip containing information about
the arguments in the function you're about to call.

Hover tooltips
--------------

![Tooltips](https://raw.github.com/davidsansome/pyqtc/master/images/completion.png)

Pyqtc will show docstrings for functions, modules and classes if you hover over
them using the mouse.

Navigation
----------

Pressing the F2 key will jump to the definition of the Python symbol under the
cursor.
Pressing Ctrl+K lets you jump straight to any Python class and function
definitions in the current project, or browse the Python symbols in the current
file.

Documentation
-------------

The documentation for Python 2.7.2 is available for browsing using Qt Creator's
Help tab.

![Documentation](https://raw.github.com/davidsansome/pyqtc/master/images/help.png)
