#ifndef EBAYGOALSFRAME_H
#define EBAYGOALSFRAME_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QVBoxLayout>
#include <QColor>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


#include <QTimer>
#include <QMessageBox>

#include <QFile>
#include <QLockFile>

class EbayGoalsFrame : public QWidget
{
    Q_OBJECT
public:
    explicit EbayGoalsFrame(const QColor& color, QJsonObject* configJson, QWidget* parent = nullptr);

    ~EbayGoalsFrame(){
        emit aboutToClose();
        channel->deregisterObject(this);
        channel->deleteLater();
        webEngine.close();
        webEngine.deleteLater();
    }

    void repopulate();

    void darkMode();

    void setJson(QJsonObject* configJson);

private:
    QString name;
    QColor color;
    QWebEngineView webEngine;
    QVBoxLayout layout;
    QWebChannel *channel;
    QJsonObject* configJson;
    bool isDarkMode = false;

    void rewriteJson();
    QString getCSS();

    QString getBody();

    QString getGoalsScript();

public slots:

    void inputSubmitted(const QString &input, const QString goalNumber);

signals:
    void aboutToClose();
};

#endif // EBAYGOALSFRAME_H
