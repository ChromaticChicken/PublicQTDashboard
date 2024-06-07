#ifndef FULLFRAME_H
#define FULLFRAME_H

#include <QWidget>
#include <QGridLayout>
#include <QFrame>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <QDebug>

#include "areaframe.h"

class FullFrame : public QWidget
{
    Q_OBJECT

private:
    QMap<QString, AreaFrame*> areaFramesMap;
    QGridLayout* layout;
    QJsonObject* configJson;
    bool isDarkMode = false;


public:
    FullFrame(QJsonObject* configJson, QWidget *parent = nullptr);
    ~FullFrame(){
        for (auto it = areaFramesMap.begin(); it != areaFramesMap.end(); ++it) {
            it.value()->deleteLater();
        }
    }

    void repopulateAll();

    void darkMode();

};

#endif // FULLFRAME_H
