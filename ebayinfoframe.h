#ifndef EBAYINFOFRAME_H
#define EBAYINFOFRAME_H

#include <QWidget>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QPair>

#include <QJsonArray>
#include <QJsonObject>

class EbayInfoFrame : public QWidget
{
    Q_OBJECT
public:
    explicit EbayInfoFrame(const QColor& color, QWidget* parent = nullptr);

    void repopulate();

    void setOrdersJson(QJsonObject* ordersJson);

    void darkMode();

private:
    QJsonObject* ordersJson;
    QJsonObject* configJson;
    QWebEngineView webEngine;
    QVBoxLayout layout;
    QColor color;
    bool isDarkMode = false;

    QPair<qint64, qint64> calculateSoldItems();

    QString getCSS();

signals:
};

#endif // EBAYINFOFRAME_H
