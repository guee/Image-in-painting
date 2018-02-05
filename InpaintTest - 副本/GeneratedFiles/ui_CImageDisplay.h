/********************************************************************************
** Form generated from reading UI file 'CImageDisplay.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CIMAGEDISPLAY_H
#define UI_CIMAGEDISPLAY_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CImageDisplay
{
public:

    void setupUi(QWidget *CImageDisplay)
    {
        if (CImageDisplay->objectName().isEmpty())
            CImageDisplay->setObjectName(QStringLiteral("CImageDisplay"));
        CImageDisplay->resize(400, 300);

        retranslateUi(CImageDisplay);

        QMetaObject::connectSlotsByName(CImageDisplay);
    } // setupUi

    void retranslateUi(QWidget *CImageDisplay)
    {
        CImageDisplay->setWindowTitle(QApplication::translate("CImageDisplay", "CImageDisplay", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class CImageDisplay: public Ui_CImageDisplay {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CIMAGEDISPLAY_H
