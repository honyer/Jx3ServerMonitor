#include "widget.h"
#include <QFile>
#include "./ui_widget.h"

const QString Widget::SERVERLIST_FILE = "server.ini";

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    // 设置窗口标题
    this->setWindowTitle("剑网三开服监控");
    // 按钮设定
    ui->pteContent->setReadOnly(true);
    ui->pbnMonitor->setCheckable(true);
    ui->pbnMonitor->setText("🟢开始监控");

    // 数据源初始化
    ds = new DataSource(SERVERLIST_FILE, DataSource::MoHe, this);
    connect(ds, &DataSource::downloadFinished, this, &Widget::slotDownloadFinished);

    timer.setParent(this);

    connect(&timer, &QTimer::timeout, this, &Widget::startMonitoring);

    loadData();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::loadData()
{
    ds->setSourceFrom(DataSource::Office);
    ds->download();
}

void Widget::startMonitoring()
{
    QStringList server;
    foreach (const QString &s, serverList) {
        QStringList line = s.split('\t');
        if (ui->cbxServerLine->currentText() == line.at(0)
            && ui->cbxServerName->currentText() == line.at(1)) {
            server = s.split('\t');
            break;
        }
    }
    ui->pteContent->appendPlainText("正在监控" + server.at(1));

    if (ui->rbtOffice->isChecked()) {
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
        QObject::connect(tp,
                         &QTcpSocket::errorOccurred,
                         [this, tp](QAbstractSocket::SocketError error) {
                             Q_UNUSED(error);
                             ui->pteContent->appendPlainText("服务器正在维护...");
                             tp->close();
                             tp->deleteLater(); // 在连接关闭后删除 QTcpSocket 对象
                         });

        // 开始连接
        tp->connectToHost(serverIP, serverPort);

    } else if (ui->rbtMohe->isChecked()) {
        ds->setSourceFrom(DataSource::MoHe);
        ds->download();
    }
}

void Widget::loadMoheData(QByteArray data)
{
    QString dataStr = QString::fromUtf8(data);
    QStringList dataList = dataStr.split('\n');
    for (const QString &str : dataList) {
        QStringList list = str.split('\t');
        if (list.at(0) == ui->cbxServerLine->currentText()
            && list.at(1) == ui->cbxServerName->currentText()) {
            //            qDebug() << str;
            ui->pteContent->appendPlainText(str);
            break;
        }
    }
}

void Widget::on_pbnMonitor_clicked()
{
    if (ui->pbnMonitor->isChecked()) {
        ui->pbnMonitor->setText("🔴停止监控");
        ui->pteContent->clear(); // 清空内容

        startMonitoring(); // 开始监控

        timer.start(3000); // 每隔3秒触发一次
    } else {
        ui->pbnMonitor->setText("🟢开始监控");
        timer.stop(); // 停止定时器
    }
}

void Widget::on_cbxServerLine_currentIndexChanged(int index)
{
    ui->cbxServerName->clear();

    foreach (const QString &si, serverList) {
        QStringList server = si.split('\t');
        if (ui->cbxServerLine->currentText() == server.at(0)) {
            ui->cbxServerName->addItem(server.at(1));
        }
    }
}

void Widget::slotDownloadFinished(DataSource::SourceFrom sf, bool success, const QByteArray &data)
{
    if (!success) {
        ui->pteContent->appendPlainText("下载失败，请检查网络连接");
        return;
    }

    QString str = QString::fromUtf8(data);
    switch (sf) {
    case DataSource::Office:
        serverList = str.split('\n');
        loadComBoxItem();
        break;
    default:
        loadMoheData(data);
        break;
    }
}

void Widget::loadComBoxItem()
{
    QStringList serverLine;
    QSet<QString> unqServerLine;
    foreach (const QString &line, serverList) {
        QString s = line.split('\t').at(0);
        if (unqServerLine.contains(s)) {
            continue;
        }
        unqServerLine.insert(s);
        serverLine.append(s);
    }
    serverLine.sort();
    ui->cbxServerLine->clear();
    ui->cbxServerLine->addItems(serverLine);
}
