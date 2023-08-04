// 类文件代码
#include "datasource.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

const QUrl DataSource::OFFICE_URL = QUrl(
    "http://jx3comm.xoyocdn.com/jx3hd/zhcn_hd/serverlist/serverlist.ini");
const QUrl DataSource::MOHE_URL = QUrl("https://spider2.jx3box.com/api/spider/server/server_state");

DataSource::DataSource(QString fileName, SourceFrom sf, QObject *parent)
    : QObject(parent)
    , sourceFrom(sf)
    , serverlistFile(fileName)
    , manager(new QNetworkAccessManager(this))
    , rep(nullptr)
{
    // 初始化
}

void DataSource::setSourceFrom(SourceFrom sf)
{
    sourceFrom = sf;
}

void DataSource::download()
{
    close(); // 先关闭上一次的请求

    switch (sourceFrom) {
    case SourceFrom::Office:
        downloadFromOffice();
        break;
    default:
        downloadFromMohe();
        break;
    }
}

void DataSource::close()
{
    if (rep) {
        if (rep->isRunning()) {
            rep->abort();
        }
        rep->deleteLater();
        rep = nullptr;
    }
}

void DataSource::downloadFromMohe()
{
    rep = manager->get(QNetworkRequest(MOHE_URL));
    connect(rep, &QNetworkReply::finished, this, [this]() {
        QString dataStr;
        if (rep->error() == QNetworkReply::NoError) {
            parseMoHeData(rep->readAll(), dataStr);
            cleanupAndEmit(true, dataStr.trimmed().toUtf8());
        } else {
            cleanupAndEmit(false, QByteArray());
        }
    });
}

// 解析魔盒网站的Json数据
void DataSource::parseMoHeData(QByteArray jsonData, QString &dataStr)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "invalid mohe json data";
        return;
    }

    QJsonArray serverArray = jsonDoc.array();

    for (const QJsonValue &serverValue : serverArray) {
        if (serverValue.isObject()) {
            // 取json结构的zone_name，server_name，ip_address，ip_port，connect_state，heat
            QJsonObject serverObj = serverValue.toObject();
            QString zoneName = serverObj.value("zone_name").toString();
            QString serverName = serverObj.value("server_name").toString();
            QString ipAddress = serverObj.value("ip_address").toString();
            QString ipPort = serverObj.value("ip_port").toString();
            QString connState = serverObj.value("connect_state").toString();
            QString sheat;
            qint8 heat = serverObj.value("heat").toString().toInt();
            switch (heat) {
            case 8:
                sheat = "火爆";
                break;
            case 7:
                sheat = "繁忙";
                break;
            case 6:
                sheat = "良好";
                break;
            default:
                sheat = "维护";
                break;
            };

            dataStr += zoneName + "\t" + serverName + "\t" + ipAddress + "\t" + ipPort + "\t"
                       + connState + "\t" + sheat + "\n";
        }
    }
}

void DataSource::downloadFromOffice()
{
    rep = manager->get(QNetworkRequest(OFFICE_URL));
    connect(rep, &QNetworkReply::finished, this, [this]() {
        if (rep->error() == QNetworkReply::NoError) {
            // 读取数据将其转为本地编码
            QByteArray data = rep->readAll();
            QStringList strList = QString::fromLocal8Bit(data).split('\n');

            // 定义一个set辅助去重，去重后的数据存放到dataStr
            QSet<QString> uniqqueSet;
            QString dataStr;
            foreach (const QString &str, strList) {
                QStringList line = str.split('\t');
                // 跳过有比赛开头的服务器
                if (line.at(0).startsWith("比赛")) {
                    continue;
                }

                QString s = line.at(11) + "\t" + line.at(10) + "\t" + line.at(3) + "\t"
                            + line.at(4);

                if (uniqqueSet.contains(s)) {
                    continue;
                }
                uniqqueSet.insert(s);
                dataStr += s + "\n";
            }

            cleanupAndEmit(true, dataStr.trimmed().toUtf8());
        } else {
            cleanupAndEmit(false, QByteArray());
        }
    });
}

void DataSource::cleanupAndEmit(bool success, const QByteArray &data)
{
    rep->deleteLater();
    rep = nullptr;
    emit downloadFinished(sourceFrom, success, data);
}
