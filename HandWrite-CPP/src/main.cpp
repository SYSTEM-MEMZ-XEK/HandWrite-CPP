#include <QApplication>
#include "mainwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("HandWrite");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("HandWrite");
    
    // 创建并显示主窗口
    HandWrite::MainWindow window;
    window.setWindowTitle("HandWrite Generator");
    window.show();
    
    return app.exec();
}
