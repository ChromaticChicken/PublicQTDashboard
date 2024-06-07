#ifndef AREAFRAME_H
#define AREAFRAME_H

#include <QWidget>
#include <QFrame>
#include <QColor>
#include <QString>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QFile>
#include <QLockFile>

#include <QWebEngineView>
#include <QWebChannel>

#include <QFont>
#include <QTimer>

#include <QtCore/QUrl>
#include <QMessageBox>

#include <QRect>

#include <QDebug>


class AreaFrame : public QFrame
{
    Q_OBJECT

private:
    QString name;
    QColor color;
    QWebEngineView webEngine;
    QVBoxLayout layout;
    QWebChannel *channel;
    QJsonObject* configJson;
    bool isDarkMode = false;

public:
    explicit AreaFrame(const QColor& color, const QString& name, QJsonObject* configJson, QWidget* parent = nullptr);

    ~AreaFrame(){
        emit aboutToClose();
        channel->deregisterObject(this);
        channel->deleteLater();
        webEngine.close();
        webEngine.deleteLater();
    }

    /*
        How to best debug this section is to copy the html printed on the console and use an online html viewer to check it.
        I have been using https://html.onlineviewer.net/
        It can automatically format it so it is easy to read as well as give simple warnings if there is something wrong
    */
    void repopulate();

    void darkMode();


public slots:

    void inputSubmitted(const QString &input, const QString goalNumber);

private:
    void rewriteJson();
    QString getCSS();

    QString getBody();

    QString getGoalsScript();

signals:
    void aboutToClose();

};

#endif // AREAFRAME_H
