// 头文件代码
#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <QNetworkReply>
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

class DataSource : public QObject
{
    Q_OBJECT
public:
    enum SourceFrom { MoHe = 1, Office };

    explicit DataSource(QString fileName, SourceFrom = MoHe, QObject *parent = nullptr);

    void setSourceFrom(SourceFrom sf);

    void download();

    void close();

signals:
    void downloadFinished(SourceFrom sf, bool success, const QByteArray &data);

private:
    SourceFrom sourceFrom;

    QString serverlistFile;

    QNetworkAccessManager *manager;

    QNetworkReply *rep;

    static const QUrl OFFICE_URL;

    static const QUrl MOHE_URL;

    void downloadFromMohe();

    void parseMoHeData(QByteArray jsonData, QString &dataStr);

    void downloadFromOffice();

    void cleanupAndEmit(bool success, const QByteArray &data);
};

#endif // DATASOURCE_H
