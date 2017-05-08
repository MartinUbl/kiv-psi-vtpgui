/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#pragma once

#include <QWidget>
#include <QDialog>
#include "ui_devselectdialog.h"

/*
 * Class representing device selection dialog
 */
class devselectdialog : public QDialog
{
        Q_OBJECT

    public:
        devselectdialog(QWidget *parent = Q_NULLPTR);
        ~devselectdialog();

        // selected interface from interface list
        std::string selectedInterface;

    private:
        Ui::devselectdialog ui;

        std::vector<std::string> m_interfaceNames;

    private slots:
        void OKButton_clicked();
        void CancelButton_clicked();
};
