#include "producer_consumer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    producer_consumer w;
    w.show();
    
    return a.exec();
}
