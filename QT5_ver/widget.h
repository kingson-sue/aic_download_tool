﻿#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QSerialPortInfo>
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include "QDesktopWidget"
#include <QProcess>

#include "YmodemFileTransmit.h"
#include "YmodemFileReceive.h"

#define SETTINGS_NAME   "aic_download_set.ini"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    typedef enum {
        DOWM_INVALID,
        DOWM_BOOT_START,
        DOWM_LOADING,
        DOWM_BOOT_END
    } DOWM_STATUS;

signals:
    void log_data(QByteArray data);
    void download_status_cb(YmodemFileTransmit::Status status);

private slots:
    void transmitProgress(int progress);
    void receiveProgress(int progress);
    void transmitStatus(YmodemFileTransmit::Status status);
    void receiveStatus(YmodemFileReceive::Status status);

    void showLog(QByteArray log);
    void on_Button_Enter_Boot_clicked();
    void on_Button_Set_Boot_clicked();
    void Log_Output();
    void Log_Output_apus();
    void read_COM(uint8_t *buff, uint32_t len);
    void download_check(QByteArray data);
    void download_status_check(YmodemFileTransmit::Status status);

    // tab:aic
    void on_comButton_clicked();
    void on_transmitBrowse_clicked();
    void on_receiveBrowse_clicked();
    void on_transmitButton_clicked();
    void on_receiveButton_clicked();
    void on_Button_OneClick_Download_clicked();
    void on_Button_Find_Previous_clicked();
    void on_Button_Find_Next_clicked();
    void on_Button_Clean_Uart_clicked();
    void on_ButtonLogClean_clicked();
    void on_RefreshButton_clicked();

    // tab:apus
    void on_RefreshButton_apus_clicked();
    void on_comButton_apus_clicked();
    void on_Button_flash_apus_clicked();
    void on_pushButton_function_1_clicked();
    void onReadProcessOutput();
    void onFinishProcess();
    void on_Button_Find_Previous_apus_clicked();
    void on_Button_Find_Next_apus_clicked();
    void on_Button_Clean_Uart_apus_clicked();
    void on_transmitBrowse_apus_clicked();

private:
    void delay_ms(int millisecondsToWait);
    void OneClick_Download_Handler(Widget::DOWM_STATUS status);
    void saveSettings();
    void recoverSettings();

    Ui::Widget *ui;
    QSerialPort *serialPort_aic;
    QSerialPort *serialPort_apus;
    YmodemFileTransmit *ymodemFileTransmit;
    YmodemFileReceive *ymodemFileReceive;
    QProcess *apusProcess;

    bool transmitButtonStatus;
    bool receiveButtonStatus;

protected:
    virtual void closeEvent(QCloseEvent *);

};

#endif // WIDGET_H
