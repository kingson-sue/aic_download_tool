#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "version.h"

#include <QtCore/QProcess>
#include <QtCore/QEventLoop>
#include <QtWidgets/QTextEdit>
#include <QtGui/QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>

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
    
    // 初始化文件路径管理
    // 将第一个路径项的信号连接
    connect(ui->pathCheckBox1, &QCheckBox::toggled, this, &MainWindow::onPathCheckBoxToggled);
    connect(ui->pathBrowseButton1, &QPushButton::clicked, this, &MainWindow::onPathBrowseButtonClicked);
    
    // 添加到路径项列表中
    PathItem firstItem;
    firstItem.checkBox = ui->pathCheckBox1;
    firstItem.lineEdit = ui->pathLineEdit1;
    firstItem.browseButton = ui->pathBrowseButton1;
    pathItems.append(firstItem);
    
    // 添加按钮信号连接
    connect(ui->addPathButton, &QPushButton::clicked, this, &MainWindow::onAddPathButtonClicked);
    
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

            ui->addPathButton->setEnabled(true);
            ui->receiveBrowse->setEnabled(true);

            // 启用所有路径项的浏览按钮
            for (const PathItem &item : pathItems) {
                item.browseButton->setEnabled(true);
            }

            ui->Button_semi_auto_load->setEnabled(true);
            ui->Button_fully_auto_load->setEnabled(true);

            if (!getSelectedFilePath().isEmpty()) {
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

        // 禁用所有路径项的浏览按钮
        for (const PathItem &item : pathItems) {
            item.browseButton->setDisabled(true);
        }
        ui->addPathButton->setDisabled(true);
        ui->transmitButton->setDisabled(true);

        ui->receiveBrowse->setDisabled(true);
        ui->receiveButton->setDisabled(true);

        ui->Button_semi_auto_load->setDisabled(true);
        ui->Button_fully_auto_load->setDisabled(true);
    }
}

void MainWindow::onPathBrowseButtonClicked()
{
    QObject *sender = QObject::sender();
    
    // 找到发送信号的按钮对应的路径项
    for (int i = 0; i < pathItems.size(); ++i) {
        if (pathItems[i].browseButton == sender) {
            QString filePath = QFileDialog::getOpenFileName(this, u8"打开文件", ".", u8"任意文件 (*.*)");
            
            if (!filePath.isEmpty()) {
                pathItems[i].lineEdit->setText(filePath);
                pathItems[i].checkBox->setChecked(true);
                
                // 更新发送按钮状态
                updateTransmitPath();
            }
            break;
        }
    }
}

void MainWindow::onPathCheckBoxToggled(bool checked)
{
    QObject *sender = QObject::sender();
    
    // 如果选中了一个复选框，取消其他复选框选中状态
    if (checked) {
        for (const PathItem &item : pathItems) {
            if (item.checkBox != sender && item.checkBox->isChecked()) {
                item.checkBox->setChecked(false);
            }
        }
        
        // 更新发送按钮状态
        updateTransmitPath();
    } else {
        // 确保至少有一个选中项
        bool anyChecked = false;
        for (const PathItem &item : pathItems) {
            if (item.checkBox->isChecked()) {
                anyChecked = true;
                break;
            }
        }
        
        if (!anyChecked && !pathItems.isEmpty()) {
            // 如果没有选中项，选中第一个
            QCheckBox *checkBox = qobject_cast<QCheckBox*>(sender);
            checkBox->setChecked(true);
        }
    }
}

void MainWindow::onAddPathButtonClicked()
{
    addPathItem();
}

void MainWindow::addPathItem(const QString &path)
{
    // 创建新的行布局
    QHBoxLayout *rowLayout = new QHBoxLayout();
    
    // 创建路径项组件
    QCheckBox *checkBox = new QCheckBox();
    QLineEdit *lineEdit = new QLineEdit();
    QPushButton *browseButton = new QPushButton("...");
    
    lineEdit->setReadOnly(true);
    browseButton->setEnabled(serialPort_aic->isOpen());
    browseButton->setMinimumWidth(30);
    browseButton->setMaximumWidth(30);
    
    if (!path.isEmpty()) {
        lineEdit->setText(path);
    }
    
    // 添加到布局
    rowLayout->addWidget(checkBox);
    rowLayout->addWidget(lineEdit);
    rowLayout->addWidget(browseButton);
    
    // 添加到主布局
    ui->transmitPathLayout->addLayout(rowLayout);
    
    // 连接信号
    connect(checkBox, &QCheckBox::toggled, this, &MainWindow::onPathCheckBoxToggled);
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onPathBrowseButtonClicked);
    
    // 添加到路径项列表
    PathItem item;
    item.checkBox = checkBox;
    item.lineEdit = lineEdit;
    item.browseButton = browseButton;
    pathItems.append(item);
    
    // 默认不选中
    checkBox->setChecked(false);
}

QString MainWindow::getSelectedFilePath() const
{
    for (const PathItem &item : pathItems) {
        if (item.checkBox->isChecked()) {
            return item.lineEdit->text();
        }
    }
    return QString();
}

void MainWindow::updateTransmitPath()
{
    QString selectedPath = getSelectedFilePath();
    ui->transmitPath->setText(selectedPath);
    
    if (!selectedPath.isEmpty() && serialPort_aic->isOpen()) {
        ui->transmitButton->setEnabled(true);
    } else {
        ui->transmitButton->setDisabled(true);
    }
}

void MainWindow::selectPathItem(int index)
{
    if (index >= 0 && index < pathItems.size()) {
        for (int i = 0; i < pathItems.size(); ++i) {
            pathItems[i].checkBox->setChecked(i == index);
        }
        updateTransmitPath();
    }
}

void MainWindow::clearAllPathItems()
{
    // 保留第一个路径项，删除其余路径项
    for (int i = pathItems.size() - 1; i > 0; --i) {
        PathItem item = pathItems.takeAt(i);
        
        // 删除组件和布局
        QLayout *layout = item.checkBox->parentWidget()->layout();
        if (layout) {
            layout->removeWidget(item.checkBox);
            layout->removeWidget(item.lineEdit);
            layout->removeWidget(item.browseButton);
            
            delete item.checkBox;
            delete item.lineEdit;
            delete item.browseButton;
            
            // 从主布局中删除行布局
            ui->transmitPathLayout->removeItem(layout);
            delete layout;
        }
    }
    
    // 清空第一个路径项的内容
    if (!pathItems.isEmpty()) {
        pathItems[0].lineEdit->clear();
        pathItems[0].checkBox->setChecked(true);
    }
}

void MainWindow::on_transmitBrowse_clicked()
{
    // 这个函数可能不再需要，但为了兼容性保留
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

        // 获取当前选中的文件路径
        QString selectedPath = getSelectedFilePath();
        if (selectedPath.isEmpty()) {
            QMessageBox::warning(this, u8"错误", u8"请选择一个文件路径！", u8"关闭");
            return;
        }
        
        ymodemFileTransmit->setFileName(selectedPath);
        ymodemFileTransmit->setPortName(ui->comPort->currentText());
        ymodemFileTransmit->setPortBaudRate(ui->comBaudRate->currentText().toInt());

        if (ymodemFileTransmit->startTransmit() == true) {
            transmitButtonStatus = true;

            ui->comButton->setDisabled(true);

            ui->receiveBrowse->setDisabled(true);
            ui->receiveButton->setDisabled(true);

            // 禁用所有路径项操作
            for (const PathItem &item : pathItems) {
                item.checkBox->setDisabled(true);
                item.browseButton->setDisabled(true);
            }
            ui->addPathButton->setDisabled(true);
            ui->transmitButton->setText(u8"取消");
            ui->transmitProgress->setValue(0);
        } else {
            QMessageBox::warning(this, u8"失败", u8"文件发送失败！", u8"关闭");
        }
    } else {
        ymodemFileTransmit->stopTransmit();
    }
}

void MainWindow::transmitStatus(Ymodem::Status status)
{
    switch(status) {
        case YmodemFileTransmit::StatusEstablish:
        case YmodemFileTransmit::StatusTransmit:
            break;

        case YmodemFileTransmit::StatusFinish:
        case YmodemFileTransmit::StatusAbort:
        case YmodemFileTransmit::StatusTimeout:
        default:
        {
            transmitButtonStatus = false;

            ui->comButton->setEnabled(true);

            ui->receiveBrowse->setEnabled(true);

            if (ui->receivePath->text().isEmpty() != true)
            {
                ui->receiveButton->setEnabled(true);
            }

            // 启用所有路径项操作
            for (const PathItem &item : pathItems) {
                item.checkBox->setEnabled(true);
                item.browseButton->setEnabled(true);
            }
            ui->addPathButton->setEnabled(true);
            ui->transmitButton->setText(u8"发送");

            if (status == YmodemFileTransmit::StatusFinish) {
                QMessageBox::information(this, u8"成功", u8"文件发送成功！", u8"关闭");
            } else if (status != YmodemFileTransmit::StatusAbort) {
                QMessageBox::warning(this, u8"失败", u8"文件发送失败！", u8"关闭");
            }
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

void MainWindow::on_receiveButton_clicked()
{
    if (receiveButtonStatus == false) {
        serialPort_aic->close();

        ymodemFileReceive->setFilePath(ui->receivePath->text());
        ymodemFileReceive->setPortName(ui->comPort->currentText());
        ymodemFileReceive->setPortBaudRate(ui->comBaudRate->currentText().toInt());

        if (ymodemFileReceive->startReceive() == true) {
            receiveButtonStatus = true;

            ui->comButton->setEnabled(true);

            // 禁用所有路径项操作
            for (const PathItem &item : pathItems) {
                item.checkBox->setDisabled(true);
                item.browseButton->setDisabled(true);
            }
            ui->addPathButton->setDisabled(true);
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

void MainWindow::receiveStatus(YmodemFileReceive::Status status)
{
    switch(status) {
        case YmodemFileReceive::StatusEstablish:
        case YmodemFileReceive::StatusTransmit:
            break;

        case YmodemFileReceive::StatusFinish:
        case YmodemFileReceive::StatusAbort:
        case YmodemFileReceive::StatusTimeout:
        default:
        {
            receiveButtonStatus = false;

            ui->comButton->setEnabled(true);

            // 启用所有路径项操作
            for (const PathItem &item : pathItems) {
                item.checkBox->setEnabled(true);
                item.browseButton->setEnabled(true);
            }
            ui->addPathButton->setEnabled(true);

            if (ui->transmitPath->text().isEmpty() != true) {
                ui->transmitButton->setEnabled(true);
            }

            ui->receiveBrowse->setEnabled(true);
            ui->receiveButton->setText(u8"接收");

            if (status == YmodemFileReceive::StatusFinish) {
                QMessageBox::information(this, u8"成功", u8"文件接收成功！", u8"关闭");
            } else {
                QMessageBox::warning(this, u8"失败", u8"文件接收失败！", u8"关闭");
            }
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
    settings.setValue(QString("binPath"), getSelectedFilePath());
    
    // 保存所有文件路径
    QStringList pathList;
    for (const PathItem &item : pathItems) {
        QString path = item.lineEdit->text();
        if (!path.isEmpty()) {
            pathList << path;
        }
    }
    settings.setValue(QString("binPathList"), pathList);
    
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

    // 清空现有路径项
    clearAllPathItems();
    
    // 恢复文件路径列表
    QStringList pathList = settings.value(QString("binPathList")).toStringList();
    
    // 设置第一个路径项
    if (!pathList.isEmpty() && !pathItems.isEmpty()) {
        pathItems[0].lineEdit->setText(pathList.first());
        pathList.removeFirst();
    }
    
    // 添加剩余路径项
    for (const QString &path : pathList) {
        if (!path.isEmpty()) {
            addPathItem(path);
        }
    }
    
    // 恢复选中状态
    QString selectedPath = settings.value(QString("binPath")).toString();
    bool found = false;
    
    for (int i = 0; i < pathItems.size(); ++i) {
        if (pathItems[i].lineEdit->text() == selectedPath) {
            pathItems[i].checkBox->setChecked(true);
            found = true;
            break;
        }
    }
    
    if (!found && !pathItems.isEmpty()) {
        pathItems[0].checkBox->setChecked(true);
    }
    
    updateTransmitPath();
    
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

void MainWindow::on_Button_cancel_flash_apus_clicked()
{
    // 先断开所有信号连接，防止回调触发
    if (apusProcess) {
        disconnect(apusProcess, &QProcess::readyRead, this, &MainWindow::onReadProcessOutput);
        disconnect(apusProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onReadProcessOutput);
        disconnect(apusProcess, &QProcess::errorOccurred, this, &MainWindow::onFinishProcess);
        disconnect(apusProcess, &QProcess::finished, this, &MainWindow::onFinishProcess);
    }

    if (apusProcess && apusProcess->state() != QProcess::NotRunning) {
        showLog("正在终止APUS烧录进程...");
        
        // 在Windows上终止可能的子进程
        #ifdef Q_OS_WIN
        QProcess killProcess;
        QString pid = QString::number(apusProcess->processId());
        showLog("终止进程ID: " + pid.toUtf8() + "及其子进程");
        killProcess.start("taskkill", QStringList() << "/F" << "/T" << "/PID" << pid);
        killProcess.waitForFinished(3000);
        #endif
        
        delete apusProcess;
        apusProcess = nullptr;
        
        showLog("APUS进程已终止");
    } else {
        showLog("没有正在运行的APUS烧录进程");
        
        if (apusProcess) {
            delete apusProcess;
            apusProcess = nullptr;
        }
    }
    
    // 无论进程状态如何，都确保串口关闭
    if (serialPort_apus->isOpen()) {
        serialPort_apus->clear();  // 清除缓冲区
        serialPort_apus->close();
        showLog("串口已关闭");
    }
    
    // 等待一段时间后重新打开串口
    delay_ms(100);
    
    // 尝试重新打开串口
    serialPort_apus->setPortName(ui->comPort_apus->currentText());
    serialPort_apus->setBaudRate(ui->comBaudRate_apus->currentText().toInt());
    
    // 设置短超时，避免长时间阻塞
    serialPort_apus->setReadBufferSize(1024);
    
    if (!serialPort_apus->open(QSerialPort::ReadWrite)) {
        showLog("重新打开串口失败: " + serialPort_apus->errorString().toUtf8());
        QMessageBox::warning(this, u8"警告", u8"串口仍被占用，请手动关闭相关程序后重试", u8"确定");
    } else {
        showLog("串口已重新打开");
        // QMessageBox::information(this, u8"操作成功", u8"烧录进程已终止，串口已重新打开", u8"确定");

        
    }
}
