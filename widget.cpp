#include "widget.h"
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include "./ui_widget.h"

const QString Widget::JX3_SERVER_URL
    = "http://jx3comm.xoyocdn.com/jx3hd/zhcn_hd/serverlist/serverlist.ini";

const QString Widget::SERVER_FILE = "server.ini";

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("剑网三开服监控");

    ui->pteContent->setReadOnly(true);
    ui->pbnMonitor->setCheckable(true);
    ui->pbnMonitor->setText("🟢开始监控");

    timer.setParent(this);

    connect(&timer, &QTimer::timeout, this, &Widget::startMonitoring);

    manager = new QNetworkAccessManager(this);
    loadFile();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::loadFile()
{
    QFile file(SERVER_FILE);
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open file for writing.";
            file.close();
            return;
        }
        qint64 size = file.size();
        if (size > 0) {
            QByteArray data = file.readAll();
            QStringList strList = QString::fromUtf8(data).split('\n');
            ui->pteContent->appendPlainText("读取文件server.ini成功！");
            removeDuplicates(strList);

        } else {
            ui->pteContent->appendPlainText("读取文件大小为空，请更新服务器信息！");
        }
        file.close();

    } else {
        ui->pteContent->appendPlainText("文件 server.ini 不存在，自动更新信息...");
        downLoad();
    }
}

void Widget::downLoad()
{
    const QUrl url(JX3_SERVER_URL);
    QNetworkRequest req(url);
    QNetworkReply *rep = manager->get(req);
    connect(rep, &QNetworkReply::finished, this, [this, rep]() {
        if (rep->error() == QNetworkReply::NoError) {
            QByteArray data = rep->readAll();
            QStringList strList = QString::fromLocal8Bit(data).split('\n');
            QStringList serverList;
            foreach (const QString &str, strList) {
                QStringList line = str.split('\t');
                if (line.at(0).startsWith("比赛")) {
                    continue;
                }

                QString s = line.at(11) + "\t" + line.at(10) + "\t" + line.at(3) + "\t"
                            + line.at(4);
                serverList.append(s);
            }

            // 去重列表
            removeDuplicates(serverList);
            QFile file(SERVER_FILE);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qWarning() << "Failed to open file for writing.";
                return 1;
            }

            QTextStream out(&file);
            foreach (const QString server, uniqueSet) {
                out << server << Qt::endl;
            }
            file.close();
        }

        rep->deleteLater();
    });

    ui->pteContent->appendPlainText("保存文件到server.ini");
}

void Widget::removeDuplicates(QStringList serverList)
{
    uniqueSet.clear();
    uniqueServerLine.clear();
    uniqueServerName.clear();
    foreach (const QString &server, serverList) {
        if (server.trimmed().isEmpty()) {
            continue;
        }
        uniqueSet.insert(server);
        uniqueServerLine.insert(server.split('\t').at(0));
        uniqueServerName.insert(server.split('\t').at(1));
    }

    loadComBoxItem();
}

void Widget::loadComBoxItem()
{
    QStringList serverLine;
    foreach (const QString &line, uniqueServerLine) {
        serverLine.append(line);
    }
    serverLine.sort();
    ui->cbxServerLine->clear();
    ui->cbxServerLine->addItems(serverLine);
}

void Widget::startMonitoring()
{
    QStringList server;
    foreach (const QString &s, uniqueSet) {
        QStringList line = s.split('\t');
        if (ui->cbxServerLine->currentText() == line.at(0)
            && ui->cbxServerName->currentText() == line.at(1)) {
            server = s.split('\t');
            break;
        }
    }

    ui->pteContent->appendPlainText("正在监控" + server.at(1));

    QString serverIP = server.at(2);
    qint16 serverPort = server.at(3).toInt();

    // 创建 QTcpSocket
    QTcpSocket *tp = new QTcpSocket(this);

    // 连接成功的槽函数
    QObject::connect(tp, &QTcpSocket::connected, [this, tp]() {
        ui->pteContent->appendPlainText("服务器开启！");
        tp->close();
        tp->deleteLater(); // 在连接关闭后删除 QTcpSocket 对象
    });

    // 连接失败的槽函数
    QObject::connect(tp, &QTcpSocket::errorOccurred, [this, tp](QAbstractSocket::SocketError error) {
        Q_UNUSED(error);
        ui->pteContent->appendPlainText("服务器正在维护...");
        tp->close();
        tp->deleteLater(); // 在连接关闭后删除 QTcpSocket 对象
    });

    // 开始连接
    tp->connectToHost(serverIP, serverPort);
}

void Widget::on_pbnUpdate_clicked()
{
    QString str = "downloaing from: <a href=\"" + JX3_SERVER_URL + "\">" + JX3_SERVER_URL + "</a>";
    ui->pteContent->appendHtml(str);
    downLoad();
}

void Widget::on_pbnMonitor_clicked()
{
    if (ui->pbnMonitor->isChecked()) {
        ui->pbnMonitor->setText("🔴停止监控");
        ui->pteContent->clear(); // 清空内容
        startMonitoring();       // 开始监控
        timer.start(3000);       // 每隔3秒触发一次
    } else {
        ui->pbnMonitor->setText("🟢开始监控");
        timer.stop(); // 停止定时器
    }
}

void Widget::on_cbxServerLine_currentIndexChanged(int index)
{
    ui->cbxServerName->clear();

    foreach (const QString &si, uniqueSet) {
        QStringList server = si.split('\t');
        if (ui->cbxServerLine->currentText() == server.at(0)) {
            ui->cbxServerName->addItem(server.at(1));
        }
    }
}
