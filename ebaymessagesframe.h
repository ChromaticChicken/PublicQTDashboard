#ifndef EBAYMESSAGESFRAME_H
#define EBAYMESSAGESFRAME_H

#include <QWidget>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QMap>

#include <QJsonObject>
#include <QJsonDocument>

class EbayMessagesFrame : public QWidget
{
    Q_OBJECT
public:
    explicit EbayMessagesFrame(const QColor& color, QWidget* parent = nullptr);

    void repopulate();

    void setConfig(QByteArray &config);

    void setConfig(QJsonObject &config);

    void darkMode();

private:
    QWebEngineView webEngine;
    QByteArray config;
    QColor color;
    QVBoxLayout layout;
    bool isDarkMode = false;

    void processXml(QXmlStreamReader &xmlReader, QMap<QString, QString> &myMap);

    QString getCSS();
signals:
};

#endif // EBAYMESSAGESFRAME_H
