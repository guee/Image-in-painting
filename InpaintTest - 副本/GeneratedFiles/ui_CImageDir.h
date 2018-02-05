/********************************************************************************
** Form generated from reading UI file 'CImageDir.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CIMAGEDIR_H
#define UI_CIMAGEDIR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CImageDir
{
public:
    QVBoxLayout *verticalLayout;
    QListView *listView;

    void setupUi(QWidget *CImageDir)
    {
        if (CImageDir->objectName().isEmpty())
            CImageDir->setObjectName(QStringLiteral("CImageDir"));
        CImageDir->resize(333, 207);
        verticalLayout = new QVBoxLayout(CImageDir);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        listView = new QListView(CImageDir);
        listView->setObjectName(QStringLiteral("listView"));
        listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        listView->setMovement(QListView::Static);
        listView->setViewMode(QListView::IconMode);

        verticalLayout->addWidget(listView);


        retranslateUi(CImageDir);

        QMetaObject::connectSlotsByName(CImageDir);
    } // setupUi

    void retranslateUi(QWidget *CImageDir)
    {
        CImageDir->setWindowTitle(QApplication::translate("CImageDir", "CImageDir", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class CImageDir: public Ui_CImageDir {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CIMAGEDIR_H
