#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    counter = 0;

    QTimer1 = new QTimer(this);
    connect(QTimer1, &QTimer::timeout, this, &MainWindow::onQTimer1);

    QPaintBox1 = new QPaintBox(0, 0, ui->widget);

    ui->statusbar->showMessage("Powered by Maxi");

    QSerialPort1 = new QSerialPort(this);
    QSerialPort1->setPortName("COM3");
    QSerialPort1->setBaudRate(115200);
    QSerialPort1->setDataBits(QSerialPort::Data8);
    QSerialPort1->setParity(QSerialPort::NoParity);
    QSerialPort1->setFlowControl(QSerialPort::NoFlowControl);
    connect(QSerialPort1, &QSerialPort::readyRead, this, &MainWindow::onQSerialPort1Rx);

    header = 0; //Esperando la 'U';

    ui->comboBox->addItem("ALIVE", 0xF0);
    ui->comboBox->addItem("Get Leds", 0xFB);

    ui->encodeData->setEnabled(false);
    ui->plainTextEdit->setVisible(false);

    ui->toolBar->addAction("Debugging");
//    ui->addWidget(QToolButton);
    setWindowTitle("Hola");

    QTimer1->start(50);
}

MainWindow::~MainWindow()
{
    delete QTimer1;
    delete QPaintBox1;
    delete ui;
}

void MainWindow::onQTimer1(){
    if(header){
        timeoutRx--;
        if(!timeoutRx)
            header = 0;
    }
}

void MainWindow::on_pushButton_3_clicked(){
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();
        ui->pushButton_3->setText("OPEN");
        ui->encodeData->setEnabled(false);
    }
    else{
        if(QSerialPort1->open(QSerialPort::ReadWrite)){
            ui->encodeData->setEnabled(true);
            ui->pushButton_3->setText("CLOSE");
            //Pregunto version del firmware
            ID = GET_LEDS;
            length = 0;
            sendData();
        }else{
            QMessageBox::information(this, "PORT", "NO se pudo abrir el PUERTO");
        }
    }
}

void MainWindow::onQSerialPort1Rx(){
    int count;
    uint8_t *buf;
    QString strHex;
    count = QSerialPort1->bytesAvailable();
    if(count <= 0)
        return;


    buf = new uint8_t[count];
    QSerialPort1->read((char *)buf, count);

    strHex = "<-0x";
    for (int i=0; i<count; i++) {
        strHex = strHex + QString("%1").arg(buf[i], 2, 16, QChar('0')).toUpper();
    }
    ui->plainTextEdit->appendPlainText(strHex);

    for (int i=0; i<count; i++) {
        switch (header) {
        case 0://Esperando la 'U'
            if(buf[i] == 'U'){
                header = 1;
                timeoutRx = 3;
            }
            break;
        case 1://'N'
            if(buf[i] == 'N')
                header = 2;
            else{
                header = 0;
                i--;
            }
            break;
        case 2://'E'
            if(buf[i] == 'E')
                header = 3;
            else{
                header = 0;
                i--;
            }
            break;
        case 3://'R'
            if(buf[i] == 'R')
                header = 4;
            else{
                header = 0;
                i--;
            }
            break;
        case 4://Cantidad de Bytes
            nbytes = buf[i];
            header = 5;
            break;
        case 5://El TOKEN ':'
            if(buf[i] == ':'){
                header = 6;
                cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ nbytes ^ ':';
                index = 0;
            }
            else{
                header = 0;
                i--;
            }
            break;
        case 6://ID + PAYLOAD + CKS
            bufRX[index++] = buf[i];
            if(nbytes != 1)
                cks ^= buf[i];
            nbytes--;
            if(!nbytes){
                header = 0;
                if(buf[i] == cks)
                    decodeData();
            }
            break;
        default:
            header = 0;
        }
    }

    delete [] buf;
}

void MainWindow::decodeData(){
    strRxProcess = "Powered by Maxi----Version: 0x";

    switch (bufRX[0]) {
    case 0x11:
        ui->plainTextEdit->appendPlainText(QString("%1").arg(bufRX[1], 2, 16, QChar('0')).toUpper());
        break;

    case FIRMWARE:
        strRxProcess += QString("%1").arg(bufRX[1], 2, 16, QChar('0')).toUpper();
        ui->statusbar->showMessage(strRxProcess);
        setWindowTitle(strRxProcess);
        break;

    case GET_LEDS:
        myWord.ui16[0] = bufRX[1];
        myWord.ui16[1] = bufRX[2];
        ledSelect = myWord.ui32;
        strRxProcess = QString("%1").arg(myWord.ui32, 2, 16, QChar('0')).toUpper();
        ui->plainTextEdit->appendPlainText(strRxProcess);
        leds_botons_Print();
        break;

    case CHANGE_BOTONES:
        botones.numButton = bufRX[1];
        botones.flanco = bufRX[2];

        myWord.ui8[0] = bufRX[3];
        myWord.ui8[0] = bufRX[4];
        myWord.ui8[0] = bufRX[5];
        myWord.ui8[0] = bufRX[6];
        botones.timerRead = myWord.ui32;

        ui->lcdNumber->display(QString("%1").arg(botones.timerRead, 2, 10, QChar('0')));
        strRxProcess = QString("%1").arg(myWord.ui32, 2, 16, QChar('0')).toUpper();
        ui->plainTextEdit->appendPlainText(strRxProcess);
        leds_botons_Print();
    }
}

void MainWindow::on_encodeData_clicked()
{
    uint8_t cmd;
    bool readyToSend = false;

    cmd = ui->comboBox->currentData().toInt();

    ui->plainTextEdit->appendPlainText(QString("%1").arg(cmd, 2, 16, QChar('0')).toUpper());

    switch (cmd) {
    case ALIVE:
        ID = ALIVE;
        length = 0;
        readyToSend = true;
        break;

    case GET_LEDS:
        ID = GET_LEDS;
        length = 0;
        readyToSend = true;
        break;
    }

    if (readyToSend)
    {
        sendData();
    }
}

void MainWindow::sendData()
{
    uint8_t tx[12];
    QString str;

    if(QSerialPort1->isOpen()){
        tx[0] = 'U';    //HEADER
        tx[1] = 'N';
        tx[2] = 'E';
        tx[3] = 'R';
        tx[4] = 2 + length;  //NBYTES - Cantidad de bytes (2 + nPayload). El Alive no tiene payload
        tx[5] = ':';    //TOKEN
        tx[6] = ID;   //ID de Alive
        int i;
        if(length != 0){
            for (i=0; i<(length); i++) {
                tx[7+i] = payLoad[i];  //PAYLOAD
            }
        }
        tx[7+length] = 0;

        for(int i=0; i<7+length; i++)
            tx[7+length] ^= tx[i]; //XOR de todos los bytes transmitidos

        QSerialPort1->write((char *)tx, (8+length));
        strRx = "0x";
    }
    else
        QMessageBox::information(this, "PORT", "Abrir el PUERTO");

    str = "->0x";
    for (int i=0; i<8+length; i++) {
        str = str + QString("%1").arg(tx[i], 2, 16, QChar('0')).toUpper();
    }
    ui->plainTextEdit->appendPlainText(str);
}

void MainWindow::on_checkBox_3_toggled(bool checked)
{
    if (checked)
        ui->plainTextEdit->setVisible(true);
    else
        ui->plainTextEdit->setVisible(false);
}

void MainWindow::leds_botons_Print()
{
    QPen pen;
    QBrush brush;
    QPainter paint(QPaintBox1->getCanvas());
    QFont font = paint.font();

    int w = QPaintBox1->width();
    int h = QPaintBox1->height();
    int radio = 20;

    pen.setColor(0x213950);
    pen.setWidth(3);
    paint.setPen(pen);
    brush.setColor(0x213950);
    brush.setStyle(Qt::SolidPattern);
    paint.setBrush(brush);
    paint.drawRect(0,0,w,h);

    int ledHigh = h*1/4-radio;
    font.setPixelSize(20);
    paint.setFont(font);
    pen.setWidth(3);
    pen.setColor(Qt::white);
    paint.setPen(pen);
    paint.drawText(w*4/5-radio-10,ledHigh-radio-10, "LED_4");
    paint.drawText(w*3/5-radio-10,ledHigh-radio-10, "LED_3");
    paint.drawText(w*2/5-radio-10,ledHigh-radio-10, "LED_2");
    paint.drawText(w*1/5-radio-10,ledHigh-radio-10, "LED_1");

    QColor contornoLed = Qt::black;
    QColor ledOff = Qt::gray;
    QColor ledOn = Qt::blue;

    if (ledSelect & 0x01){
        pen.setWidth(2);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOn);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*4/5-radio, ledHigh, 2*radio, 2*radio);
    }else {
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOff);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*4/5-radio, ledHigh, 2*radio, 2*radio);
    }

    if (ledSelect & 0x02){
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOn);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*3/5-radio, ledHigh, 2*radio, 2*radio);
    }else {
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOff);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*3/5-radio, ledHigh, 2*radio, 2*radio);
    }

    if (ledSelect & 0x04){
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOn);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*2/5-radio, ledHigh, 2*radio, 2*radio);
    }else{
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOff);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*2/5-radio, ledHigh, 2*radio, 2*radio);
    }

    if (ledSelect & 0x08)
    {
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOn);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w/5-radio, ledHigh, 2*radio, 2*radio);
    }else {
        pen.setWidth(3);
        pen.setColor(contornoLed);
        paint.setPen(pen);
        brush.setColor(ledOff);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w/5-radio, ledHigh, 2*radio, 2*radio);
    }

    int botonHigh = h*3/4-radio;

    font.setPixelSize(20);
    paint.setFont(font);
    pen.setWidth(3);
    pen.setColor(Qt::white);
    paint.setPen(pen);
    paint.drawText(w*4/5-radio-15,botonHigh-radio-10, "Boton_4");
    paint.drawText(w*3/5-radio-15,botonHigh-radio-10, "Boton_3");
    paint.drawText(w*2/5-radio-15,botonHigh-radio-10, "Boton_2");
    paint.drawText(w*1/5-radio-15,botonHigh-radio-10, "Boton_1");
    //    paint.drawRoundedRect(w*4/5-radio,h/2-radio,20,20,40);

    QColor contorno = Qt::black;
    QColor fondoNotPress = Qt::gray;
    QColor fondoPress = Qt::darkGray;
    QColor circuloNotPress = Qt::gray;
    QColor circuloPress = Qt::darkRed;

    if (botones.numButton & 0x01){
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*4/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*4/5-15,botonHigh+15,30,30);
    }else{
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*4/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*4/5-15,botonHigh+15,30,30);
    }

    if (botones.numButton & 0x02){
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*3/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*3/5-15,botonHigh+15,30,30);
    }else{
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*3/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*3/5-15,botonHigh+15,30,30);
    }

    if (botones.numButton & 0x04){
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*2/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*2/5-15,botonHigh+15,30,30);
    }else{
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*2/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*2/5-15,botonHigh+15,30,30);
    }

    if (botones.numButton & 0x08){
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*1/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*1/5-15,botonHigh+15,30,30);
    }else{
        //Set & Draw cuadrado
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(fondoNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawRect(w*1/5-30,botonHigh,60,60);
        //Set & Draw circulo
        pen.setWidth(3);
        pen.setColor(contorno);
        paint.setPen(pen);
        brush.setColor(circuloNotPress);
        brush.setStyle(Qt::SolidPattern);
        paint.setBrush(brush);
        paint.drawEllipse(w*1/5-15,botonHigh+15,30,30);
    }

    QPaintBox1->update();
}

