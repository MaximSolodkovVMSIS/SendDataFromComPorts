#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QSerialPort>
#include <QTimer>

QSerialPort *serialPort1;
QSerialPort *serialPort2;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serialPort1 = new QSerialPort(this);
    serialPort2 = new QSerialPort(this);


    updateComPortList(ui->comboBoxComPort1, ui->comboBoxComPort2);
    populateParityOptions(ui ->comboBoxParity1);
    populateParityOptions(ui->comboBoxParity2);

    connect(ui->pushButtonSend1, &QPushButton::clicked, this, &MainWindow::sendDataFromPort1);
    connect(ui->pushButtonSend2, &QPushButton::clicked, this, &MainWindow::sendDataFromPort2);

    connect(serialPort1, &QSerialPort::readyRead, this, &MainWindow::readDataFromPort1);
    connect(serialPort2, &QSerialPort::readyRead, this, &MainWindow::readDataFromPort2);

    connect(ui->comboBoxComPort1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::configurePort1);
    connect(ui->comboBoxComPort2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::configurePort2);

}

void MainWindow::updateComPortList(QComboBox *comboBoxComPort1, QComboBox *comboBoxComPort2) {
    comboBoxComPort1->clear();
    comboBoxComPort2->clear();

    const auto ports = QSerialPortInfo::availablePorts();

    if (ports.size() == 2) {
        comboBoxComPort1->addItem(ports[0].portName());
        comboBoxComPort2->addItem(ports[1].portName());
        comboBoxComPort1->setEnabled(false);
        comboBoxComPort2->setEnabled(false);
    } else {
        for (const QSerialPortInfo &port : ports) {
            QString portName = port.portName();
            comboBoxComPort1->addItem(portName);
            comboBoxComPort2->addItem(portName);
        }
        comboBoxComPort1->setEnabled(true);
        comboBoxComPort2->setEnabled(true);
    }
}

void MainWindow::populateParityOptions(QComboBox *comboBoxParity) {
    comboBoxParity->clear();

    comboBoxParity->addItem("No Parity", QSerialPort::NoParity);
    comboBoxParity->addItem("Even Parity", QSerialPort::EvenParity);
    comboBoxParity->addItem("Odd Parity", QSerialPort::OddParity);
    comboBoxParity->addItem("Mark Parity", QSerialPort::MarkParity);
    comboBoxParity->addItem("Space Parity", QSerialPort::SpaceParity);
}

void MainWindow::configurePort1() {
    QString portName = ui->comboBoxComPort1->currentText();
    serialPort1->setPortName(portName);
    serialPort1->setBaudRate(QSerialPort::Baud9600);
    serialPort1->setParity(static_cast<QSerialPort::Parity>(ui->comboBoxParity1->currentData().toInt()));
    if (serialPort1->isOpen()) {
        serialPort1->close();
    }
    serialPort1->open(QIODevice::ReadWrite);
}

void MainWindow::configurePort2() {
    QString portName = ui->comboBoxComPort2->currentText();
    serialPort2->setPortName(portName);
    serialPort2->setBaudRate(QSerialPort::Baud9600);
    serialPort2->setParity(static_cast<QSerialPort::Parity>(ui->comboBoxParity2->currentData().toInt()));
    if (serialPort2->isOpen()) {
        serialPort2->close();
    }
    serialPort2->open(QIODevice::ReadWrite);

}

void MainWindow::readDataFromPort1() {
    if (serialPort1->isOpen()) {
        QByteArray data = serialPort1->readAll();
        if (!data.isEmpty()) {
            ui->textEditOutput1->clear();
            ui->textEditOutput1->insertPlainText(QString::fromUtf8(data));
        }
    }
}

void MainWindow::readDataFromPort2() {
    if (serialPort2->isOpen()) {
        QByteArray data = serialPort2->readAll();
        if (!data.isEmpty()) {
            ui->textEditOutput2->clear();
            ui->textEditOutput2->insertPlainText(QString::fromUtf8(data));
        }
    }
}

void MainWindow::sendDataFromPort1() {
    ui->textEdit->clear();
    ui->textEditOutput2->clear();

    configurePort1();
    configurePort2();

    QString text = ui->textEditInput1->toPlainText();
    QByteArray data = text.toUtf8();
    int byteCount = data.size();

    // Отправка данных
    if (serialPort1->isOpen()) {
        serialPort1->write(data);
        serialPort1->waitForBytesWritten(10);
    }

    QString info = QString("From port %1 to port %2 send:\nString: \"%3\"(bytes: %4) Speed: 9600 Parity: %5\n")
                       .arg(ui->comboBoxComPort1->currentText())
                       .arg(ui->comboBoxComPort2->currentText())
                       .arg(text)
                       .arg(byteCount)
                       .arg(parityToString(serialPort1->parity()));

    ui->textEdit->append(info);
}

void MainWindow::sendDataFromPort2() {
    ui->textEdit->clear();
    ui->textEditOutput1->clear();

    configurePort1();
    configurePort2();

    QString text = ui->textEditInput2->toPlainText();
    QByteArray data = text.toUtf8();
    int byteCount = data.size();

    if (serialPort2->isOpen()) {
        serialPort2->write(data);
        serialPort2->waitForBytesWritten(10);
    }

    QString info = QString("From port %1 to port %2 send:\nString: \"%3\"(bytes: %4) Speed: 9600 Parity: %5\n")
                       .arg(ui->comboBoxComPort2->currentText())
                       .arg(ui->comboBoxComPort1->currentText())
                       .arg(text)
                       .arg(byteCount)
                       .arg(parityToString(serialPort2->parity()));

    ui->textEdit->append(info);
}


QString MainWindow::parityToString(QSerialPort::Parity parity) {
    switch (parity) {
    case QSerialPort::NoParity:
        return "No Parity";
    case QSerialPort::EvenParity:
        return "Even Parity";
    case QSerialPort::OddParity:
        return "Odd Parity";
    case QSerialPort::MarkParity:
        return "Mark Parity";
    case QSerialPort::SpaceParity:
        return "Space Parity";
    default:
        return "Unknown Parity";
    }
}


MainWindow::~MainWindow()
{
    delete ui;
    if(serialPort1->isOpen()) {
        serialPort1->close();
    }
    if(serialPort2->isOpen()) {
        serialPort2->close();
    }
}

