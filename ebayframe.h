#ifndef EBAYFRAME_H
#define EBAYFRAME_H

#include <QWidget>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHash>

#include <QGridLayout>

#include <QFile>
#include <QLockFile>
#include <QTimer>

#include <QJsonObject>
#include <QJsonDocument>

#include <QDateTime>

#include <QMessageBox>
#include <QDebug>


#include "ebayordersframe.h"
#include "ebaymessagesframe.h"
#include "ebayinfoframe.h"
#include "ebaygoalsframe.h"
#include "ebaycache.h"

class EbayFrame : public QWidget
{
    Q_OBJECT

private:
    QNetworkAccessManager *manager;
    QHash<QString, QNetworkReply*> replyMap;
    QGridLayout layout;
    QJsonObject ebayConfigJson;
    QJsonObject ordersJson;
    QJsonObject* configJson;
    QJsonObject historyJson;
    bool isDarkMode = false;
    bool hasLock = false;
    QLockFile *httpCallLock;
    QTimer refreshTimer;

    EbayOrdersFrame* ordersFrame;
    EbayMessagesFrame* messagesFrame;
    EbayInfoFrame* infoFrame;
    EbayGoalsFrame* goalsFrame;
    EbayCache* cache;

    void loadJson();

    void refreshAccessToken();
    void getAwaitingShipments();

    void getMessages();

public:
    explicit EbayFrame(QJsonObject* configJson, QWidget *parent = nullptr);

    ~EbayFrame() {
        for (auto it = replyMap.begin(); it != replyMap.end(); it++) {
            it.value()->deleteLater();
        }
        if (hasLock) {
            httpCallLock->unlock();
            manager->deleteLater();
        }
        layout.deleteLater();
        delete httpCallLock;
    }

    void repopulate();

    void repopulateGoals();

    void darkMode();

    void rewriteJson();

    void checkManager();

signals:
    void refreshFinished(const QString &key);

    void replyFinished(const QString &key);

    void getOrdersFinished(const QString &key);

    void getMessagesFinished(const QString &key);

private slots:
    void handleResponse(const QString  &key);

    void handleRefresh(const QString &key);

    void handleGetOrders(const QString &key);

    void handleGetMessages(const QString &key);

    void timerTimeout();
};
#endif // EBAYFRAME_H
