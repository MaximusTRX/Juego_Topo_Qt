#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <qpaintbox.h>
#include <QDateTime>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressBar>
#include <QtWidgets>
#include <QDebug>

#define MAXTIMEOUTSIDE 2000

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /*!
     * \brief onQTimer1
     *
     * Recibe la señal timeout de QTimer
     */
    void onQTimer1();

    void onQSerialPort1Rx();

    void on_pushButton_3_clicked();

    void sendData();

    void decodeData();

    void on_encodeData_clicked();

    void on_checkBox_3_toggled(bool checked);

    void ledsPrint();

private:
    Ui::MainWindow *ui;

    QSerialPort *QSerialPort1;

    QTimer *QTimer1;
    QPaintBox *QPaintBox1;

    QString strRx, strRxProcess;

    uint8_t bufRX[48], index, nbytes, cks, header, timeoutRx;
    uint8_t payLoad[4], ID, length;

    int counter;

    typedef union {
        int32_t i32;
        int8_t i8[4];
        uint32_t ui32;
        uint16_t ui16[2];
        uint8_t ui8[4];
    }_udat;
    _udat myWord;

    typedef enum{
        ACK=0x0D,
        ALIVE=0xF0,
        FIRMWARE=0xF1,
        CHANGE_BOTONES=0xFA,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        GET_BOTONES=0xFD,
        OTHERS
    }_eID;

    uint16_t ledSelect=0;
    uint16_t botonSelect=0;
};
#endif // MAINWINDOW_H
