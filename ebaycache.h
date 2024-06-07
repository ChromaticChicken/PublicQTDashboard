#ifndef EBAYCACHE_H
#define EBAYCACHE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>

#include <QLockFile>
#include <QFile>

#include <QMessageBox>

#include <QDateTime>
#include <QTimer>
#include <QFileSystemWatcher>

class EbayCache : public QObject
{
    Q_OBJECT
public:
    explicit EbayCache(QObject *parent = nullptr);


    QByteArray get(QString key);
    void put(QJsonObject cacheItem, QString key);
    void put(QByteArray cacheItem, QString key);

private:
    QJsonObject cache;
    QFileSystemWatcher watcher;
    QTimer timer;

    void saveCache();

private slots:
    void loadCache();
    void checkCacheForExpired();

signals:
};

#endif // EBAYCACHE_H
