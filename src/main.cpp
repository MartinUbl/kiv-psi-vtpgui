/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#include "general.h"

#include <QApplication>
#include "gui/mainwindow.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
