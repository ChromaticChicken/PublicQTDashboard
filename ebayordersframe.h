#ifndef EBAYORDERSFRAME_H
#define EBAYORDERSFRAME_H

#include <QWidget>

#include <QWebEngineView>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QJsonArray>


class EbayOrdersFrame : public QWidget
{
    Q_OBJECT
public:
    explicit EbayOrdersFrame(const QColor& color, QWidget* parent = nullptr);

    void setOrdersJson(QJsonObject* ordersJson);
    void darkMode();

    void repopulate();
private:

    QWebEngineView webEngine;
    QColor color;
    QJsonObject* ordersJson;
    QVBoxLayout layout;
    bool isDarkMode = false;

    QString getCSS();

signals:

};

#endif // EBAYORDERSPAGE_H
