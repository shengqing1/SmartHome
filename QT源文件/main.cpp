#include "mainwindow.h"
#include <QApplication>
extern QByteArray token;
int main(int argc, char* argv[]) {

    // 适配高分辨率屏幕
#if (QT_VERSION >= QT_VERSION_CHECK(5,6,0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return a.exec();
}
