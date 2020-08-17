#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QFileDialog>
#include <QtNetwork/QTcpSocket>
#include <QRegExp>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    typedef enum
    {
        RED,
        BLUE,
        BLACK,
        GREEN
    }color_t;

    typedef enum
    {
        NOT_CONNECTED,
        BOOT,
        APPL
    }Mode;

private:
    void printLog(QString text,color_t color = BLACK);

private slots:
    void on_connectBut_clicked();
    void on_getFilePath_clicked();
    void slotConnected();
    void slotError(QAbstractSocket::SocketError err);
    void on_disconnectBut_clicked();
    void on_uploadBut_clicked();
    void on_bootBut_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket* tcpClient;
    Mode state = NOT_CONNECTED;
};
#endif // MAINWINDOW_H
