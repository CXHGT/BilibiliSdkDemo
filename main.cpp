#include "mainwindow.h"
#include "BilibiliGetCodeDialog.h"
#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    BilibiliGetCodeDialog bdialog;
    bdialog.show();
    if (bdialog.exec() == QDialog::Accepted)
    {
        std::cout << "code :" << bdialog.getCodeText() << " , isSave : " << bdialog.isSaveCode() << std::endl;
        MainWindow w{bdialog.getCodeText(), nullptr};
        w.show();
        return a.exec();
    }
    return 0;
}
