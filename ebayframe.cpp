#include "ebayframe.h"

EbayFrame::EbayFrame(QJsonObject* configJson, QWidget *parent)
    : QWidget{parent}
{
    this->configJson = configJson;

    manager = nullptr;

    httpCallLock = new QLockFile("http.call.lock");

    // Color for the frames (gray slightly blue ish)
    QColor frameColor(203, 203, 213);

    this->ordersFrame = new EbayOrdersFrame(frameColor, this);
    this->messagesFrame = new EbayMessagesFrame(frameColor, this);
    this->infoFrame = new EbayInfoFrame(frameColor, this);
    this->goalsFrame = new EbayGoalsFrame(frameColor, configJson, this);
    this->cache = new EbayCache(this);

    // Set the margins on the outside of the grid of areas to be 0 (I do this so the layout can then define margins)
    setContentsMargins(0, 0, 0, 0);

    // Make a QGridLayout and set the FullFrames layout to be that layout
    setLayout(&layout);
    // Make the margins on the outside be 10 px all around except the top be only 3 px
    layout.setContentsMargins(10, 3, 10, 10);
    // Make the spacing between items to be 10 px
    layout.setSpacing(10);    

    layout.addWidget(ordersFrame, 0, 0, 2, 1);
    layout.addWidget(messagesFrame, 0, 1, 2, 1);
    layout.addWidget(infoFrame, 0, 2, 2, 1);
    layout.addWidget(goalsFrame, 2, 0, 1, 3);

    // Set stretch factors for rows and columns
    layout.setRowStretch(0, 1); // Make row 0 expandable
    layout.setRowStretch(1, 1); // Make row 1 expandable
    layout.setRowStretch(2, 1); // Make row 2 expandable
    layout.setColumnStretch(0, 1); // Make column 0 expandable
    layout.setColumnStretch(1, 1); // Make column 1 expandable
    layout.setColumnStretch(2, 1); // Make column 2 expandable

    QObject::connect(&refreshTimer, &QTimer::timeout, this, &EbayFrame::timerTimeout);

    refreshTimer.start(60 * 1000); // create a timer to run every minute


    try {
        loadJson();
    } catch (std::runtime_error err) {
        qDebug() << err.what();
    }

    QObject::connect(this, &EbayFrame::refreshFinished, this, &EbayFrame::handleRefresh);
    QObject::connect(this, &EbayFrame::replyFinished, this, &EbayFrame::handleResponse);
    QObject::connect(this, &EbayFrame::getOrdersFinished, this, &EbayFrame::handleGetOrders);
    QObject::connect(this, &EbayFrame::getMessagesFinished, this, &EbayFrame::handleGetMessages);


    repopulate();

    refreshAccessToken();


    // if (hasLock || httpCallLock->tryLock(1000)) {
    //     hasLock = true;
    //     refreshAccessToken();
    // } else {
    //     QTimer::singleShot(5000, this, [=](){
    //         this->refreshAccessToken();
    //     });
    // }
}

void EbayFrame::repopulate() {
    ordersFrame->repopulate();
    messagesFrame->repopulate();
    infoFrame->repopulate();
    goalsFrame->repopulate();
}

void EbayFrame::repopulateGoals() {
    goalsFrame->repopulate();
}

void EbayFrame::darkMode() {
    isDarkMode = !isDarkMode;

    ordersFrame->darkMode();
    messagesFrame->darkMode();
    infoFrame->darkMode();
    goalsFrame->darkMode();

    repopulate();
}


void EbayFrame::loadJson() {
    // open the file
    QFile file("ebay.config.json");
    QJsonObject jsonObj;
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open JSON file");
    }

    // put the data into a buffer (byte array)
    QByteArray jsonData = file.readAll();
    file.close();

    // Try to parse the QByteArray as a jsonDoc
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &error);

    if (jsonDoc.isNull()) {
        QString err = "Failed to parse JSON:" + error.errorString();
        throw std::runtime_error(err.toStdString());
    }

    // Convert the jsonDocument to a jsonObject
    if (jsonDoc.isObject()) {
        jsonObj = jsonDoc.object();
        ebayConfigJson =  jsonObj;
    }
}

void EbayFrame::getAwaitingShipments() {
    qDebug() << "geting awating shipments";

    QDateTime dateTime = QDateTime::currentDateTime();
    qint64 month = dateTime.date().month() -1;
    qint64 year = dateTime.date().year();
    if (month == 0 ) { // January is 1 but we try to reduce it by one so make it 12
        month = 12;
        year -= 1;
    }
    dateTime.setDate(QDate(year, month, 1));
    dateTime.setTime(QTime::fromMSecsSinceStartOfDay(0));

    QString url = "https://api.ebay.com/sell/fulfillment/v1/order?filter=creationdate:%5B"+dateTime.toString("yyyy-MM-ddTHH:mm:ss.zzzZ") + "..%5D&limit=200&fieldGroups=TAX_BREAKDOWN";

    QByteArray byteArray = cache->get(url);
    if (byteArray != "") {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(byteArray);
        if (jsonDoc.isObject()) {
            QJsonObject cachedItem = jsonDoc.object();
            if (!cachedItem.isEmpty()) {
                ordersFrame->setOrdersJson(&cachedItem);
                ordersFrame->repopulate();

                infoFrame->setOrdersJson(&cachedItem);
                infoFrame->repopulate();
                return;
            }
        }
    }

    if (hasLock || httpCallLock->tryLock(1000)) {
        hasLock = true;

        QNetworkRequest request(QUrl::fromUserInput(url));

        request.setRawHeader("Accept", "application/json");
        request.setRawHeader("Authorization", "Bearer " + ebayConfigJson["eBay"].toObject()["access_token"].toString().toUtf8());

        if (manager == nullptr) {
            manager = new QNetworkAccessManager(this);
        }

        replyMap[url] = manager->get(request);

        QObject::connect(replyMap[url], &QNetworkReply::finished, this, [this, url]() {
            emit getOrdersFinished(url);
        });

        qDebug() << "sent GET orders request";

    } else {
        QTimer::singleShot(5000, this, [=](){
            this->getAwaitingShipments();
        });
    }

}

void EbayFrame::refreshAccessToken() {
    qDebug() << "refreshing access token";

    QString expiringTime = ebayConfigJson.value("eBay").toObject().value("expires_at").toString();
    QDateTime expiringObj = QDateTime::fromString(expiringTime, "ddd MMM d hh:mm:ss yyyy");
    QDateTime now = QDateTime::currentDateTime();
    if (now <= expiringObj) {
        getAwaitingShipments();
        getMessages();
        return;
    }

    if (hasLock || httpCallLock->tryLock(1000)) {
        hasLock = true;

        QString url = "https://api.ebay.com/identity/v1/oauth2/token";
        QNetworkRequest request(QUrl::fromUserInput(url));

        QString credentials = ebayConfigJson.value("eBay").toObject().value("client_ID").toString() + ":";
        credentials += ebayConfigJson.value("eBay").toObject().value("client_secret").toString();

        QByteArray base64Credentials = credentials.toUtf8().toBase64();

        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        request.setRawHeader("Authorization", "Basic " + base64Credentials);

        QByteArray data = "grant_type=refresh_token";
        data.append('&');
        data.append("refresh_token=" + ebayConfigJson.value("eBay").toObject().value("refresh_token").toString().toUtf8());

        if (manager == nullptr) {
            manager = new QNetworkAccessManager(this);
        }

        replyMap[url] = manager->post(request, data);

        QObject::connect(replyMap[url], &QNetworkReply::finished, this, [this, url]() {
            emit refreshFinished(url);
        });

        qDebug() << "Sent POST refresh request";

    } else {
        QTimer::singleShot(5000, this, [=](){
            this->loadJson();
            this->refreshAccessToken();
        });
    }
}


void EbayFrame::getMessages() {
    qDebug() << "getting messages";

    QByteArray xml_data = R"(
        <?xml version="1.0" encoding="utf-8"?>
        <GetMyMessagesRequest xmlns="urn:ebay:apis:eBLBaseComponents">
            <ErrorLanguage>en_US</ErrorLanguage>
            <WarningLevel>High</WarningLevel>
            <DetailLevel>ReturnHeaders</DetailLevel>
        </GetMyMessagesRequest>)";


    QString url = "https://api.ebay.com/ws/api.dll";

    QByteArray cachedItem = cache->get(url);

    if (cachedItem != "") {
        messagesFrame->setConfig(cachedItem);
        messagesFrame->repopulate();
        return;
    }

    if (hasLock || httpCallLock->tryLock(1000)) {
        hasLock = true;

        QNetworkRequest request( QUrl::fromUserInput(url) );

        request.setRawHeader("X-EBAY-API-SITEID", "0");
        request.setRawHeader("X-EBAY-API-COMPATIBILITY-LEVEL", "967");
        request.setRawHeader("X-EBAY-API-CALL-NAME", "GetMyMessages");
        request.setRawHeader("X-EBAY-API-IAF-TOKEN", ebayConfigJson["eBay"].toObject()["access_token"].toString().toUtf8());

        if (manager == nullptr) {
            manager = new QNetworkAccessManager(this);
        }

        replyMap[url] = manager->post(request, xml_data);

        QObject::connect(replyMap[url], &QNetworkReply::finished, this, [this, url]() {
            emit getMessagesFinished(url);
        });

        qDebug() << "Sent POST get Messages request";

    } else {
        QTimer::singleShot(5000, this, [=](){
            this->getMessages();
        });
    }

}

void EbayFrame::rewriteJson() {
    // Create a lockfile
    QLockFile lockFile("ebay.config.json.lock");

    // Try to lock said lockfile for 1000 milliseconds .1 second
    if (!lockFile.tryLock(1000)) {

        // If it can't access said lock file (somthing else has it locked already try again in 5000 milliseconds
        QTimer::singleShot(5000, this, [=](){
            this->rewriteJson();
        });
        return;
    }

    // Lock was sucssessfull so open the ebay.config.json file and rewrite it
    QFile file("ebay.config.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument jsonDocument(ebayConfigJson);
        file.write(jsonDocument.toJson());
        file.close();
    } else { // If unable to open the file for writing show a messagebox with that information
        QMessageBox::critical(nullptr, "Error", "Failed to open file for writing:\n" + file.errorString());
    }

    // Unlock the lockfile so something else can access the file at a later point
    lockFile.unlock();
}

void EbayFrame::handleResponse(const QString &key) {
    try {
        qDebug() << key;


        if (replyMap[key]->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << replyMap[key]->errorString();
        } else {
            QByteArray responseData = replyMap[key]->readAll();
            qDebug() << "Response:" << responseData;
        }

        replyMap[key]->deleteLater();
        replyMap.remove(key);
        checkManager();
    } catch (std::exception err) {
        qCritical() << err.what();
    }

}

void EbayFrame::handleGetOrders(const QString &key) {
    try {
        qDebug() << key;

        if (replyMap[key]->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << replyMap[key]->errorString();
        } else {
            QByteArray responseData = replyMap[key]->readAll();

            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                ordersJson = jsonDoc.object();

            } else {
                qDebug() << "Failed to convert to jsonObject";
            }
        }

        replyMap[key]->deleteLater();
        replyMap.remove(key);

        ordersFrame->setOrdersJson(&ordersJson);
        ordersFrame->repopulate();

        infoFrame->setOrdersJson(&ordersJson);
        infoFrame->repopulate();

        cache->put(ordersJson, key);
        checkManager();
    } catch (std::exception err) {
        qCritical() << err.what();
    }
}

void EbayFrame::handleRefresh(const QString &key) {
    try {
        qDebug() << key;

        if (replyMap[key]->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << replyMap[key]->errorString();
        } else {
            QByteArray responseData = replyMap[key]->readAll();

            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                QJsonObject jsonObject = jsonDoc.object();

                QJsonObject configCopy = ebayConfigJson["eBay"].toObject();
                configCopy["access_token"] = jsonObject["access_token"].toString();

                QDateTime expiringTime = QDateTime::currentDateTime();
                qint64 expires_in = jsonObject.value("expires_in").toInteger();
                expiringTime = expiringTime.addSecs(expires_in - 100);
                configCopy["expires_at"] = expiringTime.toString();

                ebayConfigJson["eBay"] = configCopy;

                rewriteJson();
                getAwaitingShipments();
                getMessages();
            } else {
                qDebug() << "Failed to convert to jsonObject";
            }

        }

        replyMap[key]->deleteLater();
        replyMap.remove(key);
        checkManager();
    } catch (std::exception err) {
        qCritical() << err.what();
    }
}

void EbayFrame::handleGetMessages(const QString &key) {
    try {
        qDebug() << key;

        if (replyMap[key]->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << replyMap[key]->errorString();
        } else {
            QByteArray responseData = replyMap[key]->readAll();

            messagesFrame->setConfig(responseData);
            messagesFrame->repopulate();

            cache->put(responseData, key);

        }

        replyMap[key]->deleteLater();
        replyMap.remove(key);
        checkManager();
    } catch (std::exception err) {
        qCritical() << err.what();
    }
}

void EbayFrame::timerTimeout() {
    try {
        qDebug() << "timer finished";
        // By calling refreshAccessToken all other http calling functions will also be called
        refreshAccessToken();

        refreshTimer.start(60 * 1000);
    } catch (std::exception err) {
        qCritical() << err.what();
    }
}

void EbayFrame::checkManager() {
    if (!hasLock) {
        return;
    }
    if (manager->children().isEmpty()) {
        manager->deleteLater();
        hasLock = false;
        httpCallLock->unlock();
    }
}
