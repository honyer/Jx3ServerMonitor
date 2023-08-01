#ifndef WIDGET_H
#define WIDGET_H

#include <QNetworkAccessManager>
#include <QTimer>
#include <QWidget>

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
    void on_pbnUpdate_clicked();

    void on_pbnMonitor_clicked();

    void on_cbxServerLine_currentIndexChanged(int index);

private:
    Ui::Widget *ui;
    QTimer timer;
    QNetworkAccessManager *manager;

    static const QString JX3_SERVER_URL;
    static const QString SERVER_FILE;

    QSet<QString> uniqueSet;
    QSet<QString> uniqueServerLine;
    QSet<QString> uniqueServerName;

    void loadFile();

    void downLoad();

    void removeDuplicates(QStringList serverList);

    void loadComBoxItem();

    void startMonitoring();
};
#endif // WIDGET_H
