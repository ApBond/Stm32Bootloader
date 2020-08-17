#include "mainwindow.h"
#include "ui_mainwindow.h"

#define PACKET_LENGTH 256

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //Настройка параметров соединения
    tcpClient = new QTcpSocket(this);
    connect(tcpClient, SIGNAL(connected()), SLOT(slotConnected()));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotError(QAbstractSocket::SocketError)));
    ui->state->setText("Статус устройства: not connected");
}

MainWindow::~MainWindow()
{
    delete tcpClient;
    delete ui;
}

void MainWindow::on_connectBut_clicked()
{
    QString ipAdress = ui->ipAddr->text();
    QRegExp ipRegExp("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$");
    if(!ipRegExp.exactMatch(ipAdress))//Проверка корректности ip адреса
    {
        printLog("Некорректный ip устройства",RED);
    }
    else
    {
        if(tcpClient->state()==0)
        {
            printLog("Подключение...");
            tcpClient->connectToHost(ipAdress,80);//Подключение к устройству
        }
    }
}

void MainWindow::on_getFilePath_clicked()
{
    QString filePath = QFileDialog :: getOpenFileName(0,"Выберите файл прошивки","*.bin");
    ui->filePath->setText(filePath);
}

void MainWindow::slotConnected()
{
    char buff[4];
    printLog("Устройство подключено.",BLUE);
    tcpClient->write("getstate");//Команда проверки состояни устройство
    tcpClient->waitForBytesWritten();
    tcpClient->waitForReadyRead(2000);
    tcpClient->read(buff,sizeof(buff));
    if(!memcmp(buff,"boot",4))//Режим загрузчика
    {
        printLog("Boot mode",BLUE);
        ui->uploadBut->setEnabled(true);
        ui->state->setText("Статус устройства: boot");
        state = BOOT;
        ui->pushButton_2->setEnabled(true);

    }
    else if(!memcmp(buff,"appl",4))//Режим выполнения основной программы
    {
        printLog("Application mode",BLUE);
        state = APPL;
        ui->state->setText("Статус устройства: application");
    }
    else
    {
        printLog("Ошибка синхронизации",RED);
        on_disconnectBut_clicked();
        return;
    }
    ui->bootBut->setEnabled(true);
    ui->connectBut->setEnabled(false);
    ui->disconnectBut->setEnabled(true);
}

void MainWindow::slotError(QAbstractSocket::SocketError err)
{
    //Настройка GUI
    tcpClient->disconnectFromHost();
    ui->connectBut->setEnabled(true);
    ui->disconnectBut->setEnabled(false);
    ui->uploadBut->setEnabled(false);
    ui->bootBut->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    if(err==1)
    {
        printLog("Соединение разорвано");

    }
    else
    {
        printLog("Ошибка подключения",RED);
    }
    ui->state->setText("Статус устройства: not connected");
}

void MainWindow::on_disconnectBut_clicked()
{
    tcpClient->close();
    ui->connectBut->setEnabled(true);
    ui->disconnectBut->setEnabled(false);
    ui->uploadBut->setEnabled(false);
    ui->bootBut->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    printLog("Соединение разорвано");
    ui->state->setText("Статус устройства: not connected");
}

void MainWindow::on_uploadBut_clicked()
{
    QFile uploadFile(ui->filePath->text());
    if(!uploadFile.exists())//Проверка наличия файла прошивки
    {
        printLog("Файл не наден",RED);
    }
    else if(!uploadFile.open(QIODevice::ReadOnly))//Открытие файла прошивки
    {
        printLog("Ошибка открытия файла",RED);
    }
    else
    {
        char data[7]="start";//Команда начала загрузки
        uint16_t key = rand();//Генерация ключа 16ти битного ключа
        data[5]=(char)(key>>8);//Преобразование ключа в 8ми битный формат
        data[6]=(char)(key);
        qint64 fileSize=uploadFile.size();//Определение размера прошивки
        tcpClient->write(data,7);
        tcpClient->write((char *)&fileSize);//Отправка пакета
        tcpClient->flush();
        tcpClient->waitForReadyRead(2000);//Ожидание ответа от устройства
        tcpClient->read(data,7);
        if(data[0]==(char)(key>>8) && data[1]==(char)(key))//Проверка совпадения ключа
        {
            printLog("Запись...");
            char buff[PACKET_LENGTH];
            while(!uploadFile.atEnd())
            {
                int blockSize = uploadFile.read(buff,PACKET_LENGTH);//Считывание блока из файла
                tcpClient->write(buff,blockSize);//Отправка блока прошивки
                tcpClient->flush();

            }
            tcpClient->waitForReadyRead(2000);
            tcpClient->read(data,5);
            QString rez = data;
            if(!memcmp("ready",data,5))//Проверка успешного завершения загрузки
            {
                printLog("Загрузка завершена",BLUE);
            }
            else
            {
                printLog("Ошибка загрузки",RED);
                on_disconnectBut_clicked();
            }
        }
        else
        {
            printLog("Ошибка синхронизации",RED);
            on_disconnectBut_clicked();
        }
    }
}

void MainWindow::on_bootBut_clicked()
{
    char buff[6];
    if(state==APPL)
    {
        tcpClient->write("boot");//Команда перехода в режим загрузчика
    }
    else if(state==BOOT)
    {
        tcpClient->write("appl");//Команда перехода к основной программе
    }
    tcpClient->waitForBytesWritten();
    tcpClient->waitForReadyRead(2000);
    tcpClient->read(buff,sizeof(buff));
    if(!memcmp(buff,"toboot",6))
    {
        printLog("Boot mode включён. Устройство было перезагружено",BLUE);
    }
    else if(!memcmp(buff,"toappl",6))
    {
        printLog("Applicaion mode включён. Устройство было перезагружено",BLUE);
    }
    else if(!memcmp(buff,"noappl",6))
    {
        printLog("Прошивка отсуствует, смена режима невозможна.",RED);
    }
    else
    {
        printLog("Ошибка синхронизации",RED);
        on_disconnectBut_clicked();
    }
}

void MainWindow::printLog(QString text,color_t color)
{
    switch (color)
    {
        case BLACK:
            ui->logWindow->setTextColor(QColor(0,0,0));
            break;
        case RED:
            ui->logWindow->setTextColor(QColor(255,0,0));
            break;
        case BLUE:
            ui->logWindow->setTextColor(QColor(0,0,255));
            break;
        case GREEN:
            ui->logWindow->setTextColor(QColor(0,255,0));
            break;
    }
    QTime time;
    time = time.currentTime();
    text = time.toString("hh:mm:ss") + " : " +text;
    ui->logWindow->append(text);
}

void MainWindow::on_pushButton_2_clicked()
{
    QString saveAppPath = QFileDialog::getSaveFileName(this,"Укажите путь к файлу","*.bin");
    QFile saveAppFile(saveAppPath);
    if(saveAppFile.open(QIODevice::WriteOnly))
    {
        char buff[512];
        quint32 appLen;
        quint32 len;
        quint32 validLen=0;
        tcpClient->write("load");//Команда скачивания прошивки
        tcpClient->flush();
        tcpClient->waitForReadyRead(5000);
        tcpClient->read(buff,13);
        if(!memcmp(buff,"loadstart",9))//Команда начала скачивания
        {
            printLog("Загрузка...");
            memcpy(&appLen,buff+9,4);
            while(validLen<appLen)
            {
                if(tcpClient->waitForReadyRead(5000))//Ожидание пакета
                {
                    len = tcpClient->read(buff,sizeof(buff));
                    validLen+=len;
                    saveAppFile.write(buff,len);//Запись пакета в файл
                }
                else
                {
                    printLog("Ошибка загрузки",RED);
                    saveAppFile.close();
                    return;
                }
            }
            printLog("Прошивка скачана успешно",BLUE);
            saveAppFile.close();
        }
        else if(!memcmp(buff,"notapp",6))//Уведомление об отсуствии прошивки в устройстве
        {
            printLog("Программа отсутствует",RED);
        }
    }
    else
    {
        printLog("Ошибка записи в файл.",RED);
    }
}
