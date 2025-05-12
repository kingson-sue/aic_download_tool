#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtWidgets/QWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtGui/QScreen>
#include <QtCore/QProcess>
#include <QCloseEvent>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

#include "YmodemFileTransmit.h"
#include "YmodemFileReceive.h"

#define SETTINGS_NAME   "aic_download_set.ini"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    void transmitStatus(Ymodem::Status status);
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
    void on_Button_semi_auto_load_clicked();
    void on_Button_Find_Previous_clicked();
    void on_Button_Find_Next_clicked();
    void on_Button_Clean_Uart_clicked();
    void on_ButtonLogClean_clicked();
    void on_RefreshButton_clicked();
    
    // 新增文件路径管理功能
    void onAddPathButtonClicked();
    void onPathCheckBoxToggled(bool checked);
    void onPathBrowseButtonClicked();

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

    void on_LineEdit_send_returnPressed();

    void on_Button_fully_auto_load_clicked();

    void on_Button_mcu_cun_clicked();

    void on_LineEdit_send_apus_returnPressed();

    void on_Button_cancel_flash_apus_clicked();

private:
    void delay_ms(int millisecondsToWait);
    void OneClick_Download_Handler(MainWindow::DOWM_STATUS status);
    void saveSettings();
    void recoverSettings();
    
    // 文件路径管理相关函数
    void addPathItem(const QString &path = QString());
    void selectPathItem(int index);
    QString getSelectedFilePath() const;
    void updateTransmitPath();
    void clearAllPathItems();
    
    // 文件路径项组件列表
    struct PathItem {
        QCheckBox *checkBox;
        QLineEdit *lineEdit;
        QPushButton *browseButton;
    };
    QList<PathItem> pathItems;

    Ui::MainWindow *ui;
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
#endif // MAINWINDOW_H
