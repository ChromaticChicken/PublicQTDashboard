#ifndef GOALSDASHBOARD_H
#define GOALSDASHBOARD_H

#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QProcess>

#include <QVBoxLayout>
#include <QStackedWidget>

#include <QLabel>
#include <QDateEdit>

#include <QFile>
#include <QLockFile>
#include <QDir>
#include <QFileSystemWatcher>

#include <QTimer>
#include <QKeyEvent>
#include <QList>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudio>
#include <QRandomGenerator>
#include <map>

#pragma push_macro("slots")
#undef slots
#include <Python.h>
#pragma pop_macro("slots")

#include "fullframe.h"
#include "ebayframe.h"

class GoalsDashboard : public QMainWindow
{
    Q_OBJECT

public:
    GoalsDashboard(QWidget *parent = nullptr);
    ~GoalsDashboard() {
        if (isEbayMode){
            ebayFrame->deleteLater();
        } else {
            fullFrame->deleteLater();
        }

        audio->deleteLater();
        Py_Finalize();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QJsonObject configJson;
    FullFrame *fullFrame = nullptr;
    EbayFrame *ebayFrame = nullptr;
    QTimer *myDailyTimer;
    QFileSystemWatcher watcher;
    QList<int> m_keySequence;
    QList<int> desiredSequence = {Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right, Qt::Key_Left, Qt::Key_Right, Qt::Key_B, Qt::Key_A};
    QFile sourceFile;
    QAudioSink* audio = nullptr;
    std::map<int, int> sampleRateDictionary;
    std::map<int, int> channelCountDictionary;
    // QStackedLayout layout;
    QStackedWidget centralWidget;
    QMenu *editGoalMenu = nullptr;
    QMenu *removeGoalMenu = nullptr;
    bool isDarkMode = false;
    bool isEbayMode = false;

 /*
 * THIS IS AN EASTER EGG IT"S NOT ACTUALLY IMPORTANT
 * To add more songs find a .wav file that you want to add. Copy that wav file into the konamiCode folder, add it's characterists to the dictionary and increase the range of the QRandomGenerator by 1
 * To find a songs sample rate, channel count and sample format I have been using https://www.maztr.com/audiofileanalyzer
 */
    void checkSequence();

    void rewriteJson();

    void makeBackup();

    void updateHistory();

    void updateRepeating();

    void loadJson();

    // THIS IS ALSO AN EASTER EGG
    void invertAll();

    void startDailyTimer();

    void editGoalsGoalSelected(QString areaName, QString goalName);

    void editGoalSubmit(QString areaName, QString goalName, QDate startDate, QDate endDate, QString target, bool isNewGoal, bool repeating, QString daysUntilRepeat);

    void removeGoalSelected(QString areaName, QString goalName);

    void populateMenus();

    void displayPythonError();

private slots:

    void fileChanged();

    void dailyTimerFinished();

    void jsonToCsv();

    void darkMode();

    void ebayMode();

    void refreshRefreshToken();
};
#endif // GOALSDASHBOARD_H
