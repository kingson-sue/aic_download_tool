#include "widget.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include "ui_widget.h"

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
    QTextStream(stdout) << currentDateTime << " " << message << endl;

    // 输出到文件
    QFile file("download_tool.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << currentDateTime << " " << message << endl;
        file.close();
    }
}

int main(int argc, char *argv[])
{
    // 设置自定义消息处理函数
    qInstallMessageHandler(customMessageHandler);

    QApplication a(argc, argv);
    Widget w;
    w.show();

    return a.exec();
}
