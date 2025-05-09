#include "mainwindow.h"

#include <QApplication>

#include <QtWidgets/QApplication>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QIcon>


// 自定义消息处理函数
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 获取当前时间
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 格式化消息
    QString message;
    switch (type) {
    case QtDebugMsg:
        message = QString("Debug: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtWarningMsg:
        message = QString("Warning: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtCriticalMsg:
        message = QString("Critical: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtFatalMsg:
        message = QString("Fatal: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    default:
        message = QString("Info: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    }

    // 输出到控制台
    QTextStream(stdout) << currentDateTime << " " << message << Qt::endl;

    // 输出到文件
    QFile file("download_tool.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << currentDateTime << " " << message << Qt::endl;
        file.close();
    }
}

int main(int argc, char *argv[])
{
    // 设置自定义消息处理函数
    qInstallMessageHandler(customMessageHandler);

    QApplication a(argc, argv);
    
    // 设置应用程序图标
    QIcon appIcon(":/logo/resource/SerialPortYmodem.ico");
    a.setWindowIcon(appIcon);
    
    MainWindow w;
    w.show();
    return a.exec();
}
