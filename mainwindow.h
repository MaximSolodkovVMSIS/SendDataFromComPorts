#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QtSerialPort/QSerialPortInfo>
#include <QSerialPort>
#include <QTextEdit>

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

private:
    void updateComPortList(QComboBox *comboBoxComPort1, QComboBox *comboBoxComPort2);
    void populateParityOptions(QComboBox *comboBoxParity);
    void configurePort1();
    void configurePort2();
    void sendDataFromPort1();
    void sendDataFromPort2();
    void readDataFromPort1();
    void readDataFromPort2();
    QString parityToString(QSerialPort::Parity parity);
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
