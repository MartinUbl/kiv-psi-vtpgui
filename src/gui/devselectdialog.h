#pragma once

#include <QWidget>
#include <QDialog>
#include "ui_devselectdialog.h"

class devselectdialog : public QDialog
{
        Q_OBJECT

    public:
        devselectdialog(QWidget *parent = Q_NULLPTR);
        ~devselectdialog();

        int selectedInterface;

    private:
        Ui::devselectdialog ui;

    private slots:
        void OKButton_clicked();
        void CancelButton_clicked();
};
