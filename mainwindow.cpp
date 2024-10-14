#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QSerialPort>
#include <QTimer>
#include <cstdlib>

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

unsigned char calculateCRC(const QByteArray& data) {
    unsigned char crc = 0x00;  // Начальное значение CRC
    unsigned char polynomial = 0x07;  // Полином для CRC-8    x^8 + x^2 + x + 1 = 0x07

    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<unsigned char>(data[i]);  // XOR текущий байт с CRC

        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;  // Сдвиг влево и XOR с полиномом
            } else {
                crc <<= 1;  // Просто сдвиг влево
            }
        }
    }

    return crc;
}

// Функция для искажения одного случайного бита
void introduceRandomError(QByteArray& data) {
    int probability = rand() % 100;  // Генерация случайного числа от 0 до 99

    if (probability < 70) {  // 70% вероятность искажения
        int randomByteIndex = rand() % data.size();  // Случайный байт
        int randomBitIndex = rand() % 8;  // Случайный бит в этом байте

        data[randomByteIndex] ^= (1 << randomBitIndex);  // Инвертируем выбранный бит
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

QByteArray applyByteStuffing(const QByteArray& data) {
    QByteArray stuffedData;
    char ESC = 0x1B; // Символ для экранирования
    char FLAG = '$'; // Флаг начала пакета

    for (char byte : data) {
        if (byte == ESC || byte == FLAG) {
            stuffedData.append(ESC);  // Добавляем ESC перед управляющими символами
        }
        stuffedData.append(byte);  // Добавляем сам байт
    }

    return stuffedData;
}

QByteArray removeByteStuffing(const QByteArray& stuffedData, QList<int>& modifiedIndices) {
    QByteArray unstuffedData;
    char ESC = 0x1B;  // Предположим, что символ экранирования - 0x1B
    bool isEscaped = false;

    for (int i = 0; i < stuffedData.size(); ++i) {
        char byte = stuffedData[i];

        if (isEscaped) {
            modifiedIndices.append(i - 1);  // Сохраняем индекс экранирующего байта
            unstuffedData.append(byte);  // Добавляем экранированный байт
            isEscaped = false;  // Сбрасываем флаг экранирования
        } else if (byte == ESC) {
            isEscaped = true;  // Устанавливаем флаг экранирования
        } else {
            unstuffedData.append(byte);  // Добавляем обычный байт
        }
    }

    return unstuffedData;
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
        QByteArray stuffedData = serialPort1->readAll();

        if (!stuffedData.isEmpty()) {
            ui->textEditOutput1->clear();

            QList<int> modifiedIndices;
            QByteArray unstuffedData = removeByteStuffing(stuffedData, modifiedIndices);

            QString frameInfo = "Принятый кадр (до дебайт-стаффинга):\n";
            for (int i = 0; i < stuffedData.size(); ++i) {
                if (modifiedIndices.contains(i)) {
                    frameInfo += QString("<b>%1</b> ").arg(QString::number(static_cast<unsigned char>(stuffedData[i]), 16).rightJustified(2, '0'));
                } else {
                    frameInfo += QString("%1 ").arg(QString::number(static_cast<unsigned char>(stuffedData[i]), 16).rightJustified(2, '0'));
                }
            }

            ui->textEditOutput1->append(frameInfo);

            QString unstuffedFrameInfo = QString("Кадр после дебайт-стаффинга: %1").arg(QString(unstuffedData));
            ui->textEditOutput1->append(unstuffedFrameInfo);
        }
    }
}

void MainWindow::readDataFromPort2() {
    if (serialPort2->isOpen()) {
        QByteArray stuffedData = serialPort2->readAll();

        if (!stuffedData.isEmpty()) {
            ui->textEditOutput2->clear();

            // Искажаем данные с вероятностью 70%
            introduceRandomError(stuffedData);

            QList<int> modifiedIndices;
            QByteArray unstuffedData = removeByteStuffing(stuffedData, modifiedIndices);

            QString frameInfo = "Принятый кадр (до дебайт-стаффинга):\n";
            for (int i = 0; i < stuffedData.size(); ++i) {
                if (modifiedIndices.contains(i)) {
                    frameInfo += QString("<b>%1</b> ").arg(QString::number(static_cast<unsigned char>(stuffedData[i]), 16).rightJustified(2, '0'));
                } else {
                    frameInfo += QString("%1 ").arg(QString::number(static_cast<unsigned char>(stuffedData[i]), 16).rightJustified(2, '0'));
                }
            }

            ui->textEditOutput2->append(frameInfo);

            QString unstuffedFrameInfo = QString("Кадр после дебайт-стаффинга: %1").arg(QString(unstuffedData));
            ui->textEditOutput2->append(unstuffedFrameInfo);

            // Получаем последнее значение как FCS
            unsigned char receivedFCS = stuffedData.right(1)[0];
            unstuffedData.chop(1);  // Убираем FCS из данных для корректной проверки

            // Рассчитываем FCS для поля Data
            unsigned char calculatedFCS = calculateCRC(unstuffedData);

            QString fcsInfo = QString("Полученное FCS: %1\nРассчитанное FCS: %2")
                                  .arg(QString::number(receivedFCS, 16))
                                  .arg(QString::number(calculatedFCS, 16));

            ui->textEditOutput2->append(fcsInfo);

            // Проверяем FCS
            if (receivedFCS == calculatedFCS) {
                ui->textEditOutput2->append("FCS совпадает, данные корректны.");
            } else {
                ui->textEditOutput2->append("Ошибка FCS, данные повреждены.");
            }
        }
    }
}



void MainWindow::sendDataFromPort1() {
    ui->textEdit2->clear();
    ui->textEditOutput2->clear();

    configurePort1();
    configurePort2();

    QString text = ui->textEditInput1->toPlainText();
    QByteArray data = text.toUtf8();

    // Применяем байт-стаффинг к данным
    QByteArray stuffedData = applyByteStuffing(data);

    // Рассчитываем FCS для поля Data
    unsigned char fcsValue = calculateCRC(stuffedData);
    QByteArray fcs(1, fcsValue);  // Добавляем FCS (1 байт для CRC-8)

    // Пакет данных (включаем поля: флаг, адреса, данные, FCS)
    char groupName = 19;
    QByteArray flag = QByteArray::fromStdString("$" + std::string(1, 'a' + groupName));
    QByteArray destinationAddress(2, 0);  // Адрес назначения
    QByteArray sourceAddress = QByteArray::number(1);  // Адрес источника
    QByteArray otherFields(2, 0);  // Прочие поля

    // Собираем пакет с данными
    QByteArray packet;
    packet.append(flag);
    packet.append(destinationAddress);
    packet.append(sourceAddress);
    packet.append(otherFields);
    packet.append(stuffedData);  // Добавляем данные после байт-стаффинга
    packet.append(fcs);  // Добавляем FCS

    int byteCount = packet.size();

    if (serialPort1->isOpen()) {
        serialPort1->write(packet);
        serialPort1->waitForBytesWritten(10);
    }

    // Визуализация пакета для отправки
    QString packetInfo = QString("Пакет для отправки:\n");
    for (int i = 0; i < packet.size(); ++i) {
        packetInfo += QString("%1 ").arg(QString::number(static_cast<unsigned char>(packet[i]), 16).rightJustified(2, '0'));
    }

    ui->textEdit2->append(packetInfo);
}




void MainWindow::sendDataFromPort2() {
    ui->textEdit->clear();
    ui->textEditOutput1->clear();

    configurePort1();
    configurePort2();

    QString text = ui->textEditInput2->toPlainText();
    QByteArray data = text.toUtf8();

    QByteArray stuffedData = applyByteStuffing(data);

    char groupName = 19;
    QByteArray flag = QByteArray::fromStdString("$" + std::string(1, 'a' + groupName));
    QByteArray destinationAddress(2, 0);
    QByteArray sourceAddress = QByteArray::number(1);
    QByteArray otherFields(2, 0);
    QByteArray fcs(2, 0);

    QByteArray packet;
    packet.append(flag);
    packet.append(destinationAddress);
    packet.append(sourceAddress);
    packet.append(otherFields);
    packet.append(stuffedData);
    packet.append(fcs);

    int byteCount = packet.size();

    if (serialPort2->isOpen()) {
        serialPort2->write(packet);
        serialPort2->waitForBytesWritten(10);
    }

    QString byteInfoBefore = QString("Данные до байт-стаффинга: \"%1\"\n"
                                     "Байты: %2")
                                 .arg(QString(data))
                                 .arg(QString(data.toHex(' ')));

    QString byteInfoAfter = QString("Данные после байт-стаффинга: \"%1\"\n"
                                    "Байты: %2")
                                .arg(QString(stuffedData))
                                .arg(QString(stuffedData.toHex(' ')));

    QString packetInfo = QString("Пакет для отправки:\n"
                                 "Флаг: \"%1\"\n"
                                 "Адрес назначения: %2\n"
                                 "Адрес источника: %3\n"
                                 "Прочие поля: %4\n"
                                 "Данные: \"%5\"\n"
                                 "FCS: %6\n"
                                 "Общий размер пакета: %7 байт")
                             .arg(QString(flag))
                             .arg(QString(destinationAddress.toHex(' ')))
                             .arg(QString(sourceAddress.toHex(' ')))
                             .arg(QString(otherFields.toHex(' ')))
                             .arg(QString(stuffedData.toHex(' ')))
                             .arg(QString(fcs.toHex(' ')))
                             .arg(byteCount);

    ui->textEdit->append(byteInfoBefore);
    ui->textEdit->append(byteInfoAfter);
    ui->textEdit->append(packetInfo);
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

