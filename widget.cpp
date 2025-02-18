#include "widget.h"
#include "ui_widget.h"
#include <QProcess>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    serialPort_aic(new QSerialPort),
    serialPort_apus(new QSerialPort),
    ymodemFileTransmit(new YmodemFileTransmit),
    ymodemFileReceive(new YmodemFileReceive)
{
    transmitButtonStatus = false;
    receiveButtonStatus  = false;

    ui->setupUi(this);
    
    QSerialPortInfo serialPortInfo;

    // 查找串口号
    foreach(serialPortInfo, QSerialPortInfo::availablePorts()) {
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

    connect(ymodemFileTransmit, SIGNAL(transmitProgress(int)), this, SLOT(transmitProgress(int)));
    connect(ymodemFileReceive, SIGNAL(receiveProgress(int)), this, SLOT(receiveProgress(int)));
    connect(ymodemFileTransmit, SIGNAL(transmitStatus(YmodemFileTransmit::Status)), this, SLOT(transmitStatus(YmodemFileTransmit::Status)));
    connect(ymodemFileReceive, SIGNAL(receiveStatus(YmodemFileReceive::Status)), this, SLOT(receiveStatus(YmodemFileReceive::Status)));

    connect(ymodemFileTransmit, &YmodemFileTransmit::receive_data, this, &Widget::read_COM);

    connect(serialPort_aic, &QSerialPort::readyRead, this, &Widget::Log_Output);
    connect(serialPort_apus, &QSerialPort::readyRead, this, &Widget::Log_Output_apus);
}

Widget::~Widget()
{
    delete ui;
    delete serialPort_aic;
    delete serialPort_apus;
    delete ymodemFileTransmit;
    delete ymodemFileReceive;
}

void Widget::on_comButton_clicked()
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

            ui->Button_OneClick_Download->setEnabled(true);

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

        ui->Button_OneClick_Download->setDisabled(true);
    }
}

void Widget::on_transmitBrowse_clicked()
{
    ui->transmitPath->setText(QFileDialog::getOpenFileName(this, u8"打开文件", ".", u8"任意文件 (*.*)"));

    if (ui->transmitPath->text().isEmpty() != true) {
        ui->transmitButton->setEnabled(true);
    } else {
        ui->transmitButton->setDisabled(true);
    }
}

void Widget::on_receiveBrowse_clicked()
{
    ui->receivePath->setText(QFileDialog::getExistingDirectory(this, u8"选择目录", ".", QFileDialog::ShowDirsOnly));

    if (ui->receivePath->text().isEmpty() != true) {
        ui->receiveButton->setEnabled(true);
    } else {
        ui->receiveButton->setDisabled(true);
    }
}

void Widget::on_transmitButton_clicked()
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

void Widget::on_receiveButton_clicked()
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

void Widget::transmitProgress(int progress)
{
    ui->transmitProgress->setValue(progress);
}

void Widget::receiveProgress(int progress)
{
    ui->receiveProgress->setValue(progress);
}

void Widget::transmitStatus(Ymodem::Status status)
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

void Widget::receiveStatus(YmodemFileReceive::Status status)
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

void Widget::delay_ms(int millisecondsToWait)
{
    QEventLoop loop;
    QTimer::singleShot(millisecondsToWait, &loop, SLOT(quit()));
    loop.exec();
}

void Widget::showLog(QByteArray log)
{
    if (!log.isEmpty()) {
        ui->LogOutput->append(log);
    } else {
        // qDebug() << "Log is empty!";
    }
}

void Widget::read_COM(uint8_t *buff, uint32_t len)
{
    Q_UNUSED(buff);
    Q_UNUSED(len);
    // ui->Uart_Output->append(QString(len));
}

void Widget::Log_Output()
{
    // 当有数据可读时，读取数据并打印出来
    QByteArray data = serialPort_aic->readAll();  // 读取所有可用数据
    ui->Uart_Output->insertPlainText(data);

    // 始终确保文本框的光标在最后一行
    QTextCursor cursor = ui->Uart_Output->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->Uart_Output->setTextCursor(cursor);
    emit log_data(data);
}

void Widget::on_Button_Enter_Boot_clicked()
{
    QByteArray sendBuf;

    sendBuf.clear();
    sendBuf.append("x 8000000 600000\r\n");
    serialPort_aic->write(sendBuf);

    showLog("Send boot command:" + sendBuf);
}

void Widget::on_Button_Set_Boot_clicked()
{
    QByteArray sendBuf;

    sendBuf.clear();
    sendBuf.append("F 1 3 1 2 1\r\nF 3\r\n");
    serialPort_aic->write(sendBuf);

    showLog("Send boot command:" + sendBuf);
}

void Widget::on_Button_OneClick_Download_clicked()
{
    connect(this, &Widget::log_data, this, &Widget::download_check);
    connect(this, &Widget::download_status_cb, this, &Widget::download_status_check);

    OneClick_Download_Handler(DOWM_BOOT_START);
}

void Widget::download_check(QByteArray data)
{
    if (data.contains("C")) {
        OneClick_Download_Handler(Widget::DOWM_LOADING);
    }
}

void Widget::download_status_check(YmodemFileTransmit::Status status)
{
    switch(status) {
        case YmodemFileTransmit::StatusFinish:
        case YmodemFileTransmit::StatusAbort:
        case YmodemFileTransmit::StatusTimeout:
        {
            OneClick_Download_Handler(Widget::DOWM_BOOT_END);
            break;
        }
    }
}

void Widget::OneClick_Download_Handler(Widget::DOWM_STATUS status)
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
            disconnect(this, &Widget::log_data, this, &Widget::download_check);
            break;

        case DOWM_BOOT_END:
            delay_ms(50);
            on_Button_Set_Boot_clicked();
            delay_ms(100);
            on_Button_Set_Boot_clicked();

            disconnect(this, &Widget::download_status_cb, this, &Widget::download_status_check);
            break;

        default:
            break;
    }
}

void Widget::on_Button_Find_Previous_clicked()
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

void Widget::on_Button_Find_Next_clicked()
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

void Widget::on_Button_Clean_Uart_clicked()
{
    ui->Uart_Output->clear();
}

void Widget::on_ButtonLogClean_clicked()
{
    ui->LogOutput->clear();
}

void Widget::on_RefreshButton_clicked()
{
    QSerialPortInfo serialPortInfo;

    ui->comPort->clear();
    foreach(serialPortInfo, QSerialPortInfo::availablePorts()) {
        ui->comPort->addItem(serialPortInfo.portName());
    }
}

void Widget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    qDebug("on_Widget_destroyed");
    saveSettings();
}

void Widget::saveSettings()
{
    QSettings settings(SETTINGS_NAME, QSettings::IniFormat);
    settings.setValue("Window/Size", this->size());

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

void Widget::recoverSettings()
{
    QSettings settings(SETTINGS_NAME, QSettings::IniFormat);
    const QSize availableSize = QApplication::desktop()->availableGeometry(this).size();
    QVariant windowSize(availableSize / 4 * 3);

    this->resize(settings.value("Window/Size", windowSize).toSize());
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

void Widget::Log_Output_apus()
{
    // 当有数据可读时，读取数据并打印出来
    QByteArray data = serialPort_apus->readAll();  // 读取所有可用数据
    ui->Uart_Output_apus->insertPlainText(data);

    // 始终确保文本框的光标在最后一行
    QTextCursor cursor = ui->Uart_Output_apus->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->Uart_Output_apus->setTextCursor(cursor);
}

void Widget::on_RefreshButton_apus_clicked()
{
    QSerialPortInfo serialPortInfo;

    ui->comPort_apus->clear();
    foreach(serialPortInfo, QSerialPortInfo::availablePorts()) {
        ui->comPort_apus->addItem(serialPortInfo.portName());
    }
}

void Widget::on_comButton_apus_clicked()
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


void Widget::on_Button_flash_apus_clicked()
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

    connect(apusProcess,SIGNAL(readyRead()),this,SLOT(onReadProcessOutput()));                          //读命令行数据
    connect(apusProcess,SIGNAL(readyReadStandardOutput()),this,SLOT(onReadProcessOutput()));            //读命令行标准数据（兼容）
    connect(apusProcess,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(onFinishProcess()));    //命令行错误处理
    connect(apusProcess,SIGNAL(finished(int)),this,SLOT(onFinishProcess()));                            //命令行结束处理

    apusProcess->start(command, args);//command是要执行的命令,args是参数
}

void Widget::on_pushButton_function_1_clicked()
{
    showLog("function_1_clicked");
    // on_Button_flash_apus_clicked();
}

void Widget::onReadProcessOutput()
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

void Widget::onFinishProcess()
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

void Widget::on_Button_Find_Previous_apus_clicked()
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

void Widget::on_Button_Find_Next_apus_clicked()
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

void Widget::on_Button_Clean_Uart_apus_clicked()
{
    ui->Uart_Output_apus->clear();
}

void Widget::on_transmitBrowse_apus_clicked()
{
    ui->binPath_apus->setText(QFileDialog::getOpenFileName(this, u8"打开文件", ".", u8"任意文件 (*.*)"));
}
