# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

# This is a reminder that you are using a generated .pro file.
# Remove it when you are finished editing this file.
message("You are running qmake on a generated .pro file. This may not work!")


HEADERS += ../Inpaint/Inpaint.h \
    ../Inpaint/MaskDrawer.h \
    ./InpaintGui.h \
    ./CImageDisplay.h \
    ./CImageDir.h
SOURCES += ../Inpaint/GRect.cpp \
    ../Inpaint/GSize.cpp \
    ../Inpaint/Inpaint.cpp \
    ../Inpaint/Inpaint2.cpp \
    ../Inpaint/MaskDrawer.cpp \
    ./CImageDir.cpp \
    ./CImageDisplay.cpp \
    ./InpaintGui.cpp \
    ./main.cpp
FORMS += ./CImageDir.ui \
    ./CImageDisplay.ui \
    ./InpaintGui.ui
RESOURCES += InpaintGui.qrc
