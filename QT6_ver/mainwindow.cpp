#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QtCore/QProcess>
#include <QtCore/QEventLoop>
#include <QtWidgets/QTextEdit>
#include <QtGui/QTextCursor>
#include <QTextDocument>

#define SHOW_VERSION "Version:2.0.2"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort_aic(new QSerialPort),
    serialPort_apus(new QSerialPort),
    ymodemFileTransmit(new YmodemFileTransmit),
    ymodemFileReceive(new YmodemFileReceive)
{
    transmitButtonStatus = false;
    receiveButtonStatus  = false;

    ui->setupUi(this);
    this->setWindowTitle("Download Tool");
    
    //状态栏设置
    //不显示其内控件的边框
    // statusBar()->setStyleSheet("QStatusBar::item{border: 0px}");
    //永久部件
    statusBar()->addPermanentWidget(new QLabel(SHOW_VERSION));
    ui->Uart_Output->setReadOnly(true);
    ui->Uart_Output_apus->setReadOnly(true);

    // 查找串口号
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : ports) {
        ui->comPort->addItem(serialPortInfo.portName());
        ui->comPort_apus->addItem(serialPortInfo.portName());
    }
    recoverSettings();

    // 串口设置
    serialPort_aic->setPortName("COM1");
    serialPort_aic->setBaudRate(115200);
    serialPort_aic->setDataBits(QSerialPort::Data8);
    serialPort_aic->setStopBits(QSerialPort::OneStop);
    serialPort_aic->setParity(QSerialPort::NoParity);
    serialPort_aic->setFlowControl(QSerialPort::NoFlowControl);

    serialPort_apus->setPortName("COM1");
    serialPort_apus->setBaudRate(115200);
    serialPort_apus->setDataBits(QSerialPort::Data8);
    serialPort_apus->setStopBits(QSerialPort::OneStop);
    serialPort_apus->setParity(QSerialPort::NoParity);
    serialPort_apus->setFlowControl(QSerialPort::NoFlowControl);

    connect(ymodemFileTransmit, &YmodemFileTransmit::transmitProgress, this, &MainWindow::transmitProgress);
    connect(ymodemFileReceive, &YmodemFileReceive::receiveProgress, this, &MainWindow::receiveProgress);
    connect(ymodemFileTransmit, &YmodemFileTransmit::transmitStatus, this, &MainWindow::transmitStatus);
    connect(ymodemFileReceive, &YmodemFileReceive::receiveStatus, this, &MainWindow::receiveStatus);

    connect(ymodemFileTransmit, &YmodemFileTransmit::receive_data, this, &MainWindow::read_COM);

    connect(serialPort_aic, &QSerialPort::readyRead, this, &MainWindow::Log_Output);
    connect(serialPort_apus, &QSerialPort::readyRead, this, &MainWindow::Log_Output_apus);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete serialPort_aic;
    delete serialPort_apus;
    delete ymodemFileTransmit;
    delete ymodemFileReceive;
}

void MainWindow::on_comButton_clicked()
{
    static bool button_status = false;

    if (button_status == false) {
        serialPort_aic->setPortName(ui->comPort->currentText());
        serialPort_aic->setBaudRate(ui->comBaudRate->currentText().toInt());

        if (serialPort_aic->open(QSerialPort::ReadWrite) == true) {
            button_status = true;

            ui->comPort->setDisabled(true);
            ui->comBaudRate->setDisabled(true);
            ui->RefreshButton->setDisabled(true);
            ui->comButton->setText(u8"关闭串口");

            ui->transmitBrowse->setEnabled(true);
            ui->receiveBrowse->setEnabled(true);

            ui->Button_semi_auto_load->setEnabled(true);
            ui->Button_fully_auto_load->setEnabled(true);

            if (ui->transmitPath->text().isEmpty() != true) {
                ui->transmitButton->setEnabled(true);
            }

            if (ui->receivePath->text().isEmpty() != true) {
                ui->receiveButton->setEnabled(true);
            }
        } else {
            QMessageBox::warning(this, u8"串口打开失败", u8"请检查串口是否已被占用！", u8"关闭");
        }
    } else {
        button_status = false;

        serialPort_aic->close();

        ui->comPort->setEnabled(true);
        ui->comBaudRate->setEnabled(true);
        ui->RefreshButton->setEnabled(true);
        ui->comButton->setText(u8"打开串口");

        ui->transmitBrowse->setDisabled(true);
        ui->transmitButton->setDisabled(true);

        ui->receiveBrowse->setDisabled(true);
        ui->receiveButton->setDisabled(true);

        ui->Button_semi_auto_load->setDisabled(true);
        ui->Button_fully_auto_load->setDisabled(true);
    }
}

void MainWindow::on_transmitBrowse_clicked()
{
    ui->transmitPath->setText(QFileDialog::getOpenFileName(this, u8"打开文件", ".", u8"任意文件 (*.*)"));

    if (ui->transmitPath->text().isEmpty() != true) {
        ui->transmitButton->setEnabled(true);
    } else {
        ui->transmitButton->setDisabled(true);
    }
}

void MainWindow::on_receiveBrowse_clicked()
{
    ui->receivePath->setText(QFileDialog::getExistingDirectory(this, u8"选择目录", ".", QFileDialog::ShowDirsOnly));

    if (ui->receivePath->text().isEmpty() != true) {
        ui->receiveButton->setEnabled(true);
    } else {
        ui->receiveButton->setDisabled(true);
    }
}

void MainWindow::on_transmitButton_clicked()
{
    if (transmitButtonStatus == false) {
        serialPort_aic->close();

        ymodemFileTransmit->setFileName(ui->transmitPath->text());
        ymodemFileTransmit->setPortName(ui->comPort->currentText());
        ymodemFileTransmit->setPortBaudRate(ui->comBaudRate->currentText().toInt());

        if (ymodemFileTransmit->startTransmit() == true) {
            transmitButtonStatus = true;

            ui->comButton->setDisabled(true);

            ui->receiveBrowse->setDisabled(true);
            ui->receiveButton->setDisabled(true);

            ui->transmitBrowse->setDisabled(true);
            ui->transmitButton->setText(u8"取消");
            ui->transmitProgress->setValue(0);
        } else {
            QMessageBox::warning(this, u8"失败", u8"文件发送失败！", u8"关闭");
        }
    } else {
        ymodemFileTransmit->stopTransmit();
    }
}

void MainWindow::on_receiveButton_clicked()
{
    if (receiveButtonStatus == false) {
        serialPort_aic->close();

        ymodemFileReceive->setFilePath(ui->receivePath->text());
        ymodemFileReceive->setPortName(ui->comPort->currentText());
        ymodemFileReceive->setPortBaudRate(ui->comBaudRate->currentText().toInt());

        if (ymodemFileReceive->startReceive() == true) {
            receiveButtonStatus = true;

            ui->comButton->setDisabled(true);

            ui->transmitBrowse->setDisabled(true);
            ui->transmitButton->setDisabled(true);

            ui->receiveBrowse->setDisabled(true);
            ui->receiveButton->setText(u8"取消");
            ui->receiveProgress->setValue(0);
        } else {
            QMessageBox::warning(this, u8"失败", u8"文件接收失败！", u8"关闭");
        }
    } else {
        ymodemFileReceive->stopReceive();
    }
}

void MainWindow::transmitProgress(int progress)
{
    ui->transmitProgress->setValue(progress);
}

void MainWindow::receiveProgress(int progress)
{
    ui->receiveProgress->setValue(progress);
}

void MainWindow::transmitStatus(Ymodem::Status status)
{
    switch(status) {
        case YmodemFileTransmit::StatusEstablish:
        {
            break;
        }

        case YmodemFileTransmit::StatusTransmit:
        {
            break;
        }

        case YmodemFileTransmit::StatusFinish:
        {
            transmitButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->receiveBrowse->setEnabled(true);

            if (ui->receivePath->text().isEmpty() != true)
            {
                ui->receiveButton->setEnabled(true);
            }

            ui->transmitBrowse->setEnabled(true);
            ui->transmitButton->setText(u8"发送");

            QMessageBox::warning(this, u8"成功", u8"文件发送成功！", u8"关闭");

            break;
        }

        case YmodemFileTransmit::StatusAbort:
        {
            transmitButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->receiveBrowse->setEnabled(true);

            if (ui->receivePath->text().isEmpty() != true)
            {
                ui->receiveButton->setEnabled(true);
            }

            ui->transmitBrowse->setEnabled(true);
            ui->transmitButton->setText(u8"发送");

            // QMessageBox::warning(this, u8"失败", u8"文件发送失败！", u8"关闭");

            break;
        }

        case YmodemFileTransmit::StatusTimeout:
        {
            transmitButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->receiveBrowse->setEnabled(true);

            if (ui->receivePath->text().isEmpty() != true)
            {
                ui->receiveButton->setEnabled(true);
            }

            ui->transmitBrowse->setEnabled(true);
            ui->transmitButton->setText(u8"发送");

            QMessageBox::warning(this, u8"失败", u8"文件发送失败！", u8"关闭");

            break;
        }

        default:
        {
            transmitButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->receiveBrowse->setEnabled(true);

            if (ui->receivePath->text().isEmpty() != true)
            {
                ui->receiveButton->setEnabled(true);
            }

            ui->transmitBrowse->setEnabled(true);
            ui->transmitButton->setText(u8"发送");

            QMessageBox::warning(this, u8"失败", u8"文件发送失败！", u8"关闭");
        }
    }

    switch(status) {
        case YmodemFileTransmit::StatusFinish:
        case YmodemFileTransmit::StatusAbort:
        case YmodemFileTransmit::StatusTimeout:
        {
            serialPort_aic->open(QSerialPort::ReadWrite);
            emit download_status_cb(status);
            break;
        }
    }
}

void MainWindow::receiveStatus(YmodemFileReceive::Status status)
{
    switch(status) {
        case YmodemFileReceive::StatusEstablish:
        {
            break;
        }

        case YmodemFileReceive::StatusTransmit:
        {
            break;
        }

        case YmodemFileReceive::StatusFinish:
        {
            receiveButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->transmitBrowse->setEnabled(true);

            if (ui->transmitPath->text().isEmpty() != true)
            {
                ui->transmitButton->setEnabled(true);
            }

            ui->receiveBrowse->setEnabled(true);
            ui->receiveButton->setText(u8"接收");

            QMessageBox::warning(this, u8"成功", u8"文件接收成功！", u8"关闭");

            break;
        }

        case YmodemFileReceive::StatusAbort:
        {
            receiveButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->transmitBrowse->setEnabled(true);

            if (ui->transmitPath->text().isEmpty() != true) {
                ui->transmitButton->setEnabled(true);
            }

            ui->receiveBrowse->setEnabled(true);
            ui->receiveButton->setText(u8"接收");

            QMessageBox::warning(this, u8"失败", u8"文件接收失败！", u8"关闭");

            break;
        }

        case YmodemFileReceive::StatusTimeout:
        {
            receiveButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->transmitBrowse->setEnabled(true);

            if (ui->transmitPath->text().isEmpty() != true) {
                ui->transmitButton->setEnabled(true);
            }

            ui->receiveBrowse->setEnabled(true);
            ui->receiveButton->setText(u8"接收");

            QMessageBox::warning(this, u8"失败", u8"文件接收失败！", u8"关闭");

            break;
        }

        default:
        {
            receiveButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->transmitBrowse->setEnabled(true);

            if (ui->transmitPath->text().isEmpty() != true) {
                ui->transmitButton->setEnabled(true);
            }

            ui->receiveBrowse->setEnabled(true);
            ui->receiveButton->setText(u8"接收");

            QMessageBox::warning(this, u8"失败", u8"文件接收失败！", u8"关闭");
        }
    }
}

void MainWindow::delay_ms(int millisecondsToWait)
{
    QEventLoop loop;
    QTimer::singleShot(millisecondsToWait, &loop, &QEventLoop::quit);
    loop.exec();
}

void MainWindow::showLog(QByteArray log)
{
    if (!log.isEmpty()) {
        ui->LogOutput->append(log);
    } else {
        // qDebug() << "Log is empty!";
    }
}

void MainWindow::read_COM(uint8_t *buff, uint32_t len)
{
    Q_UNUSED(buff);
    Q_UNUSED(len);
    // ui->Uart_Output->append(QString(len));
}

void MainWindow::Log_Output()
{
    // 当有数据可读时，读取数据并打印出来
    QByteArray data = serialPort_aic->readAll();  // 读取所有可用数据
    
    // 只检查是否有文本被选中
    QTextCursor currentCursor = ui->Uart_Output->textCursor();
    bool hasSelection = currentCursor.hasSelection();

    // 移动到文本末尾
    currentCursor.movePosition(QTextCursor::End);
    ui->Uart_Output->setTextCursor(currentCursor);

    // 插入新文本
    ui->Uart_Output->insertPlainText(data);

    // 移动到文本末尾
    currentCursor.movePosition(QTextCursor::End);
    ui->Uart_Output->setTextCursor(currentCursor);

    emit log_data(data);
}

void MainWindow::on_Button_Enter_Boot_clicked()
{
    QByteArray sendBuf;

    sendBuf.clear();
    // sendBuf.append("x 8000000 400000\r\n");
    sendBuf.append("x 8000000 300000\r\n");
    serialPort_aic->write(sendBuf);

    showLog("Send boot command:" + sendBuf);
}

void MainWindow::on_Button_Set_Boot_clicked()
{
    QByteArray sendBuf;
    sendBuf.clear();
    sendBuf.append("\r\n");
    serialPort_aic->write(sendBuf);

    delay_ms(10);
    sendBuf.clear();
    sendBuf.append("F 1 3 1 2 1\r\nF 3\r\n");
    serialPort_aic->write(sendBuf);

    delay_ms(10);
    sendBuf.clear();
    sendBuf.append("F 3\r\n");
    serialPort_aic->write(sendBuf);
    showLog("Send boot command:" + sendBuf);
}

void MainWindow::on_Button_semi_auto_load_clicked()
{
    connect(this, &MainWindow::log_data, this, &MainWindow::download_check);
    connect(this, &MainWindow::download_status_cb, this, &MainWindow::download_status_check);

    OneClick_Download_Handler(DOWM_BOOT_START);
}

void MainWindow::download_check(QByteArray data)
{
    if (data.contains("C")) {
        OneClick_Download_Handler(MainWindow::DOWM_LOADING);
    } else if (data.contains("Mcu mode")) {
        
    }
}

void MainWindow::download_status_check(YmodemFileTransmit::Status status)
{
    switch(status) {
        case YmodemFileTransmit::StatusFinish:
        case YmodemFileTransmit::StatusAbort:
        case YmodemFileTransmit::StatusTimeout:
        {
            OneClick_Download_Handler(MainWindow::DOWM_BOOT_END);
            break;
        }
    }
}

void MainWindow::OneClick_Download_Handler(MainWindow::DOWM_STATUS status)
{
    showLog("DOWM_STATUS:" + status);
    switch(status) {
        case DOWM_INVALID:
            break;

        case DOWM_BOOT_START:
            on_Button_Enter_Boot_clicked();
            break;

        case DOWM_LOADING:
            on_transmitButton_clicked();
            disconnect(this, &MainWindow::log_data, this, &MainWindow::download_check);
            break;

        case DOWM_BOOT_END:
            delay_ms(50);
            on_Button_Set_Boot_clicked();
            delay_ms(100);
            on_Button_mcu_cun_clicked();

            disconnect(this, &MainWindow::download_status_cb, this, &MainWindow::download_status_check);
            break;

        default:
            break;
    }
}

void MainWindow::on_Button_Find_Previous_clicked()
{
    QString searchTerm = ui->LineEdit_Search->text();
    if (!searchTerm.isEmpty()) {
        QTextCursor cursor = ui->Uart_Output->textCursor();
        cursor = ui->Uart_Output->document()->find(searchTerm, cursor,
                                            QTextDocument::FindFlags(QTextDocument::FindBackward)); //查找上一个匹配项

        if (cursor.isNull()) {
            qDebug() << "no match found!";
        } else {
            ui->Uart_Output->setTextCursor(cursor);
        }
    }
}

void MainWindow::on_Button_Find_Next_clicked()
{
    QString searchTerm = ui->LineEdit_Search->text();
    if (!searchTerm.isEmpty()) {
        QTextCursor cursor = ui->Uart_Output->textCursor();
        cursor = ui->Uart_Output->document()->find(searchTerm, cursor); // 查找下一个匹配项

        if (cursor.isNull()) {
            qDebug() << "no match found!";
        } else {
            ui->Uart_Output->setTextCursor(cursor);
        }
    }
}

void MainWindow::on_Button_Clean_Uart_clicked()
{
    ui->Uart_Output->clear();
}

void MainWindow::on_ButtonLogClean_clicked()
{
    ui->LogOutput->clear();
}

void MainWindow::on_RefreshButton_clicked()
{
    ui->comPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : ports) {
        ui->comPort->addItem(serialPortInfo.portName());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    qDebug("on_Widget_destroyed");
    saveSettings();
}

void MainWindow::saveSettings()
{
    QSettings settings(SETTINGS_NAME, QSettings::IniFormat);
    settings.setValue("Window/Size", this->size());
    settings.setValue("Window/Position", this->pos());

    // 保存COM口相关信息
    settings.setValue(QString("comNumb"), ui->comPort->currentText());
    settings.setValue(QString("comBaudrate"), ui->comBaudRate->currentText());
    settings.setValue(QString("comNumbApus"), ui->comPort_apus->currentText());
    settings.setValue(QString("comBaudrateApus"), ui->comBaudRate_apus->currentText());
    settings.setValue(QString("comFlashBaudrateApus"), ui->comboBox_flash_apus->currentText());

    // 保存下载文件路径
    settings.setValue(QString("binPath"), ui->transmitPath->text());
    settings.setValue(QString("binPathApus"), ui->binPath_apus->text());
}

void MainWindow::recoverSettings()
{
    QSettings settings(SETTINGS_NAME, QSettings::IniFormat);
    QSize availableSize;
    if (QGuiApplication::primaryScreen()) {
        availableSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    } else {
        availableSize = QSize(800, 600); // 默认尺寸
    }
    QVariant windowSize(availableSize / 4 * 3);

    this->resize(settings.value("Window/Size", windowSize).toSize());
    
    // 恢复窗口位置，如果没有保存过则居中显示
    if (settings.contains("Window/Position")) {
        this->move(settings.value("Window/Position").toPoint());
    } else {
        // 没有保存过位置，居中显示
        this->setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                this->size(),
                QGuiApplication::primaryScreen()->availableGeometry()
            )
        );
    }
    
    // restoreState(settings.value("Window/windowState").toByteArray());

    // 设置COM口相关信息
    ui->comPort->setCurrentText(settings.value(QString("comNumb")).toString());
    ui->comBaudRate->setCurrentText(settings.value(QString("comBaudrate")).toString());
    ui->comPort_apus->setCurrentText(settings.value(QString("comNumbApus")).toString());
    ui->comBaudRate_apus->setCurrentText(settings.value(QString("comBaudrateApus")).toString());
    ui->comboBox_flash_apus->setCurrentText(settings.value(QString("comFlashBaudrateApus")).toString());

    // 设置下载文件路径
    ui->transmitPath->setText(settings.value(QString("binPath")).toString());
    ui->binPath_apus->setText(settings.value(QString("binPathApus")).toString());
}

void MainWindow::Log_Output_apus()
{
    // 当有数据可读时，读取数据并打印出来
    QByteArray data = serialPort_apus->readAll();  // 读取所有可用数据
    ui->Uart_Output_apus->insertPlainText(data);

    // 始终确保文本框的光标在最后一行
    QTextCursor cursor = ui->Uart_Output_apus->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->Uart_Output_apus->setTextCursor(cursor);
}

void MainWindow::on_RefreshButton_apus_clicked()
{
    ui->comPort_apus->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : ports) {
        ui->comPort_apus->addItem(serialPortInfo.portName());
    }
}

void MainWindow::on_comButton_apus_clicked()
{
    static bool button_status = false;

    if (button_status == false) {
        serialPort_apus->setPortName(ui->comPort_apus->currentText());
        serialPort_apus->setBaudRate(ui->comBaudRate_apus->currentText().toInt());

        if (serialPort_apus->open(QSerialPort::ReadWrite) == true) {
            button_status = true;

            ui->comPort_apus->setDisabled(true);
            ui->comBaudRate_apus->setDisabled(true);
            ui->RefreshButton_apus->setDisabled(true);
            ui->comButton_apus->setText(u8"关闭串口");

            ui->Button_flash_apus->setEnabled(true);
        } else {
            QMessageBox::warning(this, u8"串口打开失败", u8"请检查串口是否已被占用！", u8"关闭");
        }
    } else {
        button_status = false;

        serialPort_apus->close();

        ui->comPort_apus->setEnabled(true);
        ui->comBaudRate_apus->setEnabled(true);
        ui->RefreshButton_apus->setEnabled(true);
        ui->comButton_apus->setText(u8"打开串口");

        ui->Button_flash_apus->setDisabled(true);
    }
}


void MainWindow::on_Button_flash_apus_clicked()
{
    if (ui->comPort_apus->currentText().isEmpty() || 
        ui->comboBox_flash_apus->currentText().isEmpty() || 
        ui->binPath_apus->text().isEmpty()) {
        QMessageBox::warning(this, u8"配置错误", u8"请检查串口号,烧录波特率和文件路径", u8"关闭");
        return;
    }

    if (serialPort_apus->isOpen()) {
        serialPort_apus->close();
        showLog("close serialPort_apus");
    } else {
        showLog("close serialPort_apus error");
    }

    QString command = "bootx/flash.bat";
    QStringList args;
    args.append(ui->comPort_apus->currentText());           // 串口号
    args.append(ui->comboBox_flash_apus->currentText());    // 波特率
    args.append(ui->binPath_apus->text());                  // 文件路径

    apusProcess = new QProcess(this);
    apusProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(apusProcess, &QProcess::readyRead, this, &MainWindow::onReadProcessOutput);
    connect(apusProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onReadProcessOutput);
    connect(apusProcess, &QProcess::errorOccurred, this, &MainWindow::onFinishProcess);
    connect(apusProcess, &QProcess::finished, this, &MainWindow::onFinishProcess);

    apusProcess->start(command, args);//command是要执行的命令,args是参数
}

void MainWindow::on_pushButton_function_1_clicked()
{
    showLog("function_1_clicked");
    // on_Button_flash_apus_clicked();
}

void MainWindow::onReadProcessOutput()
{
    /* 接收数据 */
    QByteArray bytes = apusProcess->readAll();

    /* 显示 */
    if (!bytes.isEmpty()) {
        ui->Uart_Output_apus->insertPlainText(bytes);

        // 始终确保文本框的光标在最后一行
        QTextCursor cursor = ui->Uart_Output_apus->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->Uart_Output_apus->setTextCursor(cursor);
    }
    showLog(bytes);

    /* 信息输出 */
    qDebug() << bytes;
}

void MainWindow::onFinishProcess()
{
    /* 接收数据 */
    int flag = apusProcess->exitCode();

    // 判断有无错误
    if (flag != 0) {
        QString err = apusProcess->errorString();
        qDebug() << "error_process():" << err;
    } else {
        qDebug() << "Success:finishedProcess():" << flag;
    }

    if (serialPort_apus->open(QSerialPort::ReadWrite) != true) {
        QMessageBox::warning(this, u8"串口打开失败", u8"请检查串口是否已被占用！", u8"关闭");
    }
}

void MainWindow::on_Button_Find_Previous_apus_clicked()
{
    QString searchTerm = ui->LineEdit_Search_apus->text();
    if (!searchTerm.isEmpty()) {
        QTextCursor cursor = ui->Uart_Output_apus->textCursor();
        cursor = ui->Uart_Output_apus->document()->find(searchTerm, cursor,
                                                   QTextDocument::FindFlags(QTextDocument::FindBackward)); //查找上一个匹配项

        if (cursor.isNull()) {
            qDebug() << "no match found!";
        } else {
            ui->Uart_Output_apus->setTextCursor(cursor);
        }
    }
}

void MainWindow::on_Button_Find_Next_apus_clicked()
{
    QString searchTerm = ui->LineEdit_Search_apus->text();
    if (!searchTerm.isEmpty()) {
        QTextCursor cursor = ui->Uart_Output_apus->textCursor();
        cursor = ui->Uart_Output_apus->document()->find(searchTerm, cursor); // 查找下一个匹配项

        if (cursor.isNull()) {
            qDebug() << "no match found!";
        } else {
            ui->Uart_Output_apus->setTextCursor(cursor);
        }
    }
}

void MainWindow::on_Button_Clean_Uart_apus_clicked()
{
    ui->Uart_Output_apus->clear();
}

void MainWindow::on_transmitBrowse_apus_clicked()
{
    ui->binPath_apus->setText(QFileDialog::getOpenFileName(this, u8"打开文件", ".", u8"任意文件 (*.*)"));
}

void MainWindow::on_LineEdit_send_returnPressed()
{
    QString send_text = ui->LineEdit_send->text();

    QByteArray sendBuf;

    sendBuf = send_text.toLocal8Bit();
    sendBuf.append("\n");
    serialPort_aic->write(sendBuf);

    ui->LineEdit_send->clear();
    showLog("Send:" + sendBuf);
}


void MainWindow::on_Button_fully_auto_load_clicked()
{
    // reboot mcu
    serialPort_aic->write("reboot\n");

    // enter boot mode
    delay_ms(50);
    serialPort_aic->write("\n");

    // ready download
    connect(this, &MainWindow::log_data, this, &MainWindow::download_check);
    connect(this, &MainWindow::download_status_cb, this, &MainWindow::download_status_check);

    OneClick_Download_Handler(DOWM_BOOT_START);
}


void MainWindow::on_Button_mcu_cun_clicked()
{
    QByteArray sendBuf;

    sendBuf.clear();
    sendBuf.append("g 8000000\r\n");
    serialPort_aic->write(sendBuf);

    showLog("Send boot command:" + sendBuf);
}

void MainWindow::on_LineEdit_send_apus_returnPressed()
{
    QString send_text = ui->LineEdit_send_apus->text();

    QByteArray sendBuf;

    sendBuf = send_text.toLocal8Bit();
    sendBuf.append("\n");
    serialPort_apus->write(sendBuf);

    ui->LineEdit_send_apus->clear();
    showLog("Send:" + sendBuf);
}
