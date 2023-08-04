#ifndef WIDGET_H
#define WIDGET_H

#include <QTimer>
#include <QWidget>
#include "datasource.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:

    void on_pbnMonitor_clicked();

    void on_cbxServerLine_currentIndexChanged(int index);

    // 定义槽函数 slotDownloadFinished，用于接收 downloadFinished 信号
    void slotDownloadFinished(DataSource::SourceFrom sf, bool success, const QByteArray &data);

private:
    Ui::Widget *ui;
    static const QString SERVERLIST_FILE;

    QTimer timer;

    DataSource *ds;

    QStringList serverList;

    void loadData();

    void removeDuplicates(QStringList serverList);

    void loadComBoxItem();

    void startMonitoring();

    void download();

    void loadMoheData(QByteArray data);
};
#endif // WIDGET_H
