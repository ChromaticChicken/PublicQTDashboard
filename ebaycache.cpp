#include "ebaycache.h"

EbayCache::EbayCache(QObject *parent)
    : QObject{parent}
{
    try {
        loadCache();
    } catch (std::runtime_error err) {
        qDebug() << err.what();
    }


    watcher.addPath("ebay.cache.json");

    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, this, &EbayCache::loadCache);

    timer.start(7 * 1000);

    QObject::connect(&timer, &QTimer::timeout, this, &EbayCache::checkCacheForExpired);
}

QByteArray EbayCache::get(QString key) {

    if (!cache.contains(key)) {
        return "";
    }

    QJsonObject item = cache.value(key).toObject();
    QDateTime expiringTime = QDateTime::fromString(item.value("expires_at").toString(), "ddd MMM d hh:mm:ss yyyy");
    if (expiringTime < QDateTime::currentDateTime()) {
        return "";
    }

    if (item.value("data").isString()) {
        QString jsonString = item.value("data").toString();
        return jsonString.toUtf8();
    }
    QJsonObject jsonObject = item.value("data").toObject();
    QJsonDocument jsonDoc(jsonObject);
    return jsonDoc.toJson();

}

void EbayCache::put(QJsonObject cacheItem, QString key) {
    QDateTime expiresTime = QDateTime::currentDateTime().addSecs(30);
    QJsonObject newData = {
        {"expires_at", expiresTime.toString()},
        {"data", cacheItem}
    };

    cache.insert(key, newData);

    saveCache();
}

void EbayCache::put(QByteArray cacheItem, QString key) {
    QDateTime expiresTime = QDateTime::currentDateTime().addSecs(30);
    QString data = QString::fromUtf8(cacheItem);
    QJsonObject newData = {
        {"expires_at", expiresTime.toString()},
        {"data", data}
    };

    cache.insert(key, newData);

    saveCache();
}

void EbayCache::loadCache() {
    try {
        QLockFile lockFile("ebay.cache.json.lock");
        lockFile.setStaleLockTime(3000);

        if (!lockFile.tryLock(1000)) {
            qCritical() << "Failed to accquire lock for ebay.cache.json";
            return;
        }

        // open the file
        QFile file("ebay.cache.json");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            lockFile.unlock();
            qCritical() << "Failed to open file: ebay.cache.json";
            return;
        }
        QJsonObject jsonObj;

        // put the data into a buffer (byte array)
        QByteArray jsonData = file.readAll();
        file.close();
        lockFile.unlock();

        // Try to parse the QByteArray as a jsonDoc
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &error);

        if (error.error != QJsonParseError::NoError) {
            qCritical() << "Failed to parse JSON:" << error.errorString();
            qCritical() << "Error offset:" << error.offset;
            qCritical() << "JSON data: " << jsonData;
            return;
        }

        if (jsonDoc.isNull()) {
            qCritical() << "Failed to parse JSON:" << error.errorString();
            return;
        }

        // Convert the jsonDocument to a jsonObject
        jsonObj = jsonDoc.object();
        cache =  jsonObj;
    } catch (std::exception err) {
        qCritical() << err.what();
    }
}

void EbayCache::saveCache() {
    // Create a lockfile
    QLockFile lockFile("ebay.cache.json.lock");

    // Try to lock said lockfile for 1000 milliseconds .1 second
    if (!lockFile.tryLock(1000)) {

        // If it can't access said lock file (somthing else has it locked already try again in 5000 milliseconds
        QTimer::singleShot(5000, this, [=](){
            this->saveCache();
        });
        return;
    }

    // Lock was sucssessfull so open the ebay.config.json file and rewrite it
    QFile file("ebay.cache.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument jsonDocument(cache);
        // qDebug() << "JSON document" << jsonDocument.toJson();
        file.write(jsonDocument.toJson());
        file.close();
    } else { // If unable to open the file for writing show a messagebox with that information
        QMessageBox::critical(nullptr, "Error", "Failed to open file for writing:\n" + file.errorString());
    }

    // Unlock the lockfile so something else can access the file at a later point
    lockFile.unlock();
}

void EbayCache::checkCacheForExpired() {

    try {
        QStringList keysToRemove;

        for (auto it = cache.constBegin(); it != cache.constEnd(); it++) {
            QString key = it.key();
            QJsonObject object = cache.value(key).toObject();
            QString expiringTimeString = object.value("expires_at").toString();

            QDateTime expiringTimeObj = QDateTime::fromString(expiringTimeString, "ddd MMM d hh:mm:ss yyyy");
            if (expiringTimeObj <= QDateTime::currentDateTime()) {
                qDebug() << "removing: " << key;
                keysToRemove.append(key);
            }
        }

        for (const QString &key : keysToRemove) {
            cache.remove(key);
        }

        if (!keysToRemove.isEmpty()) {
            saveCache();
        }
        timer.start(7 * 1000);
    } catch (std::exception err) {
        qCritical() << err.what();
    }
}

