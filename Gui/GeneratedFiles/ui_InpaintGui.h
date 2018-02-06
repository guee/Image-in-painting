/********************************************************************************
** Form generated from reading UI file 'InpaintGui.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INPAINTGUI_H
#define UI_INPAINTGUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include "cimagedir.h"
#include "cimagedisplay.h"

QT_BEGIN_NAMESPACE

class Ui_InpaintTestClass
{
public:
    QAction *actInpaint;
    QAction *actSelRect;
    QAction *actSelPencil;
    QAction *actEnlarge;
    QAction *actShrink;
    QAction *actOriginal;
    QAction *actMove;
    QAction *actOpen;
    QAction *actSave;
    QAction *actSelPath;
    QAction *actUndo;
    QAction *actRedo;
    QAction *actInpaint2;
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    CImageDir *widgetDir;
    CImageDisplay *widgetImage;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *InpaintTestClass)
    {
        if (InpaintTestClass->objectName().isEmpty())
            InpaintTestClass->setObjectName(QStringLiteral("InpaintTestClass"));
        InpaintTestClass->resize(1233, 706);
        actInpaint = new QAction(InpaintTestClass);
        actInpaint->setObjectName(QStringLiteral("actInpaint"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/InpaintTest/Resources/iconInpaint.png"), QSize(), QIcon::Normal, QIcon::Off);
        actInpaint->setIcon(icon);
        actSelRect = new QAction(InpaintTestClass);
        actSelRect->setObjectName(QStringLiteral("actSelRect"));
        actSelRect->setCheckable(true);
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/InpaintTest/Resources/iconRect.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon1.addFile(QStringLiteral(":/InpaintTest/Resources/iconRectOn.png"), QSize(), QIcon::Normal, QIcon::On);
        actSelRect->setIcon(icon1);
        actSelPencil = new QAction(InpaintTestClass);
        actSelPencil->setObjectName(QStringLiteral("actSelPencil"));
        actSelPencil->setCheckable(true);
        actSelPencil->setChecked(false);
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/InpaintTest/Resources/iconPencil.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon2.addFile(QStringLiteral(":/InpaintTest/Resources/iconPencilOn.png"), QSize(), QIcon::Normal, QIcon::On);
        actSelPencil->setIcon(icon2);
        actEnlarge = new QAction(InpaintTestClass);
        actEnlarge->setObjectName(QStringLiteral("actEnlarge"));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/InpaintTest/Resources/iconEnlarge.png"), QSize(), QIcon::Normal, QIcon::Off);
        actEnlarge->setIcon(icon3);
        actShrink = new QAction(InpaintTestClass);
        actShrink->setObjectName(QStringLiteral("actShrink"));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/InpaintTest/Resources/iconShrink.png"), QSize(), QIcon::Normal, QIcon::Off);
        actShrink->setIcon(icon4);
        actOriginal = new QAction(InpaintTestClass);
        actOriginal->setObjectName(QStringLiteral("actOriginal"));
        QIcon icon5;
        icon5.addFile(QStringLiteral(":/InpaintTest/Resources/iconZoom.png"), QSize(), QIcon::Normal, QIcon::Off);
        actOriginal->setIcon(icon5);
        actMove = new QAction(InpaintTestClass);
        actMove->setObjectName(QStringLiteral("actMove"));
        actMove->setCheckable(true);
        QIcon icon6;
        icon6.addFile(QStringLiteral(":/InpaintTest/Resources/iconMove.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon6.addFile(QStringLiteral(":/InpaintTest/Resources/iconMoveOn.png"), QSize(), QIcon::Normal, QIcon::On);
        actMove->setIcon(icon6);
        actOpen = new QAction(InpaintTestClass);
        actOpen->setObjectName(QStringLiteral("actOpen"));
        QIcon icon7;
        icon7.addFile(QStringLiteral(":/InpaintTest/Resources/iconOpen.png"), QSize(), QIcon::Normal, QIcon::Off);
        actOpen->setIcon(icon7);
        actSave = new QAction(InpaintTestClass);
        actSave->setObjectName(QStringLiteral("actSave"));
        QIcon icon8;
        icon8.addFile(QStringLiteral(":/InpaintTest/Resources/iconSave.png"), QSize(), QIcon::Normal, QIcon::Off);
        actSave->setIcon(icon8);
        actSelPath = new QAction(InpaintTestClass);
        actSelPath->setObjectName(QStringLiteral("actSelPath"));
        actSelPath->setCheckable(true);
        QIcon icon9;
        icon9.addFile(QStringLiteral(":/InpaintTest/Resources/iconPath.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon9.addFile(QStringLiteral(":/InpaintTest/Resources/iconPathOn.png"), QSize(), QIcon::Normal, QIcon::On);
        actSelPath->setIcon(icon9);
        actUndo = new QAction(InpaintTestClass);
        actUndo->setObjectName(QStringLiteral("actUndo"));
        QIcon icon10;
        icon10.addFile(QStringLiteral(":/InpaintTest/Resources/iconUndo.png"), QSize(), QIcon::Normal, QIcon::Off);
        actUndo->setIcon(icon10);
        actRedo = new QAction(InpaintTestClass);
        actRedo->setObjectName(QStringLiteral("actRedo"));
        QIcon icon11;
        icon11.addFile(QStringLiteral(":/InpaintTest/Resources/iconRedo.png"), QSize(), QIcon::Normal, QIcon::Off);
        actRedo->setIcon(icon11);
        actInpaint2 = new QAction(InpaintTestClass);
        actInpaint2->setObjectName(QStringLiteral("actInpaint2"));
        QIcon icon12;
        icon12.addFile(QStringLiteral(":/InpaintTest/Resources/iconInpaint2.png"), QSize(), QIcon::Normal, QIcon::Off);
        actInpaint2->setIcon(icon12);
        centralWidget = new QWidget(InpaintTestClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        widgetDir = new CImageDir(centralWidget);
        widgetDir->setObjectName(QStringLiteral("widgetDir"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widgetDir->sizePolicy().hasHeightForWidth());
        widgetDir->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(widgetDir);

        widgetImage = new CImageDisplay(centralWidget);
        widgetImage->setObjectName(QStringLiteral("widgetImage"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(3);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(widgetImage->sizePolicy().hasHeightForWidth());
        widgetImage->setSizePolicy(sizePolicy1);

        horizontalLayout->addWidget(widgetImage);

        InpaintTestClass->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(InpaintTestClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        mainToolBar->setMovable(false);
        mainToolBar->setIconSize(QSize(32, 32));
        mainToolBar->setFloatable(false);
        InpaintTestClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(InpaintTestClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        InpaintTestClass->setStatusBar(statusBar);

        mainToolBar->addAction(actOpen);
        mainToolBar->addAction(actSave);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actMove);
        mainToolBar->addAction(actSelRect);
        mainToolBar->addAction(actSelPath);
        mainToolBar->addAction(actSelPencil);
        mainToolBar->addAction(actUndo);
        mainToolBar->addAction(actRedo);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actInpaint);
        mainToolBar->addAction(actInpaint2);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actEnlarge);
        mainToolBar->addAction(actShrink);
        mainToolBar->addAction(actOriginal);

        retranslateUi(InpaintTestClass);

        QMetaObject::connectSlotsByName(InpaintTestClass);
    } // setupUi

    void retranslateUi(QMainWindow *InpaintTestClass)
    {
        InpaintTestClass->setWindowTitle(QApplication::translate("InpaintTestClass", "InpaintTest", Q_NULLPTR));
        actInpaint->setText(QApplication::translate("InpaintTestClass", "\344\277\256\350\241\245", Q_NULLPTR));
        actSelRect->setText(QApplication::translate("InpaintTestClass", "\347\237\251\345\275\242", Q_NULLPTR));
        actSelPencil->setText(QApplication::translate("InpaintTestClass", "\347\224\273\347\254\224", Q_NULLPTR));
        actEnlarge->setText(QApplication::translate("InpaintTestClass", "\346\224\276\345\244\247", Q_NULLPTR));
        actShrink->setText(QApplication::translate("InpaintTestClass", "\347\274\251\345\260\217", Q_NULLPTR));
        actOriginal->setText(QApplication::translate("InpaintTestClass", "\351\200\202\345\220\210", Q_NULLPTR));
        actMove->setText(QApplication::translate("InpaintTestClass", "\347\247\273\345\212\250", Q_NULLPTR));
        actOpen->setText(QApplication::translate("InpaintTestClass", "\346\211\223\345\274\200", Q_NULLPTR));
        actSave->setText(QApplication::translate("InpaintTestClass", "\344\277\235\345\255\230", Q_NULLPTR));
        actSelPath->setText(QApplication::translate("InpaintTestClass", "\350\267\257\345\276\204", Q_NULLPTR));
        actUndo->setText(QApplication::translate("InpaintTestClass", "\346\222\244\346\266\210", Q_NULLPTR));
        actRedo->setText(QApplication::translate("InpaintTestClass", "\351\207\215\345\201\232", Q_NULLPTR));
        actInpaint2->setText(QApplication::translate("InpaintTestClass", "\344\277\256\350\241\245", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class InpaintTestClass: public Ui_InpaintTestClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INPAINTGUI_H
