#include <QApplication>
#include "ETH.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    ETH w;
    w.show();
    return a.exec();
}
