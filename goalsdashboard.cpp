#include "goalsdashboard.h"
#include "fullframe.h"

/********************************************************************************************************
 *                                            Public Methods                                            *
 ********************************************************************************************************/

GoalsDashboard::GoalsDashboard(QWidget *parent)
    : QMainWindow(parent), centralWidget(this)
{
    // load config json information
    loadJson();

    // Set styling
    QDate aprilFirst = QDate(QDate::currentDate().year(), 4, 1);
    if (QDate::currentDate() == aprilFirst) { // If today is april first
        // Set the styling to the menuBar and QMenus within the menu bar
        setStyleSheet("QMenuBar { background-color: #BE0000; color: white; padding: 2px 5px; font: 15px;}"
                      "QMenuBar::item {background-color: #34342A; }"
                      "QMenuBar::item:selected { background-color: #444; color: white; }"
                      "QMenu { font-size: 15px; background-color: #34342A; color: white; }"
                      "QMenu::item:selected {background-color: #444; color: white; }");
        // Change the background color of the MainWindow to be U of U Red
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(190, 0, 0)); //#BE0000
        setAutoFillBackground(true);
        setPalette(pal);

    } else {
        // Set some styling to the menuBar and QMenus within the menu bar
        setStyleSheet("QMenuBar { background-color: #002E5D; color: black; padding: 2px 5px; font: 15px;}"
                      "QMenuBar::item {background-color: #CBCBD5;}"
                      "QMenuBar::item:selected { background-color: #333; color: white; }"
                      "QMenu { font-size: 15px; background-color: #CBCBD5;}"
                      "QMenu::item:selected {background-color: #333; color: white; }");
        // Change the background color of the MainWindow to be Navy Blue (BYU's primary color)
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(0, 46, 93));
        setAutoFillBackground(true);
        setPalette(pal);
    }

    // Add a watcher to the config.json file and connect it so that when it changes it reloads the file and populates areas
    watcher.addPath("config.json");
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, this, [&](const QString& path) {
        fileChanged();
    });

    // Create a QMenuBar and add it to the GoalsDashboard
    QMenuBar *menuBar = this->menuBar();

    // Create Menus for the edit goals and remove goals and populate them
    editGoalMenu = menuBar->addMenu("Edit/Create Goal");
    removeGoalMenu = menuBar->addMenu("Remove Goal");
    populateMenus();

    // Connect when an action in the edit menu is triggered that the function is called
    connect(editGoalMenu, &QMenu::triggered, this, [=](QAction *action) {
        QMenu *areaMenu = qobject_cast<QMenu*>(action->parent());
        editGoalsGoalSelected(areaMenu->title(), action->text());
    });

    // Connect when an action in the remove menu is triggered that the function is called
    connect(removeGoalMenu, &QMenu::triggered, this, [=](QAction *action) {
        QMenu *areaMenu = qobject_cast<QMenu*>(action->parent());
        removeGoalSelected(areaMenu->title(), action->text());
    });

    // Create various actions
    QAction *jsonToCsvAction = menuBar->addAction("Create History as CSV");
    QAction *darkModeAction = menuBar->addAction("Dark Mode");
    QAction *ebayAction = menuBar->addAction("eBay Mode");
    QAction *refreshAction = menuBar->addAction("refresh refresh token");

    // Connect actions to slots (a type of function)
    QObject::connect(jsonToCsvAction, &QAction::triggered, this, &GoalsDashboard::jsonToCsv);
    QObject::connect(darkModeAction, &QAction::triggered, this, &GoalsDashboard::darkMode);
    QObject::connect(ebayAction, &QAction::triggered, this, &GoalsDashboard::ebayMode);
    QObject::connect(refreshAction, &QAction::triggered, this, &GoalsDashboard::refreshRefreshToken);

    // Set the Window Icon to be BYU Y
    QIcon icon("BYU.png");
    this->setWindowIcon(icon);

    // Create a daily timer and connect it so that the backup/history will be updated daily
    myDailyTimer = new QTimer(this);
    connect(myDailyTimer, &QTimer::timeout, this, &GoalsDashboard::dailyTimerFinished);
    dailyTimerFinished();
    startDailyTimer();

    // Create the fullFrame and set it to be what is the center of view
    this->fullFrame = new FullFrame(&configJson, this);

    setCentralWidget(&centralWidget);

    // setLayout(&layout);
    centralWidget.addWidget(fullFrame);

    ebayFrame = new EbayFrame(&configJson, this);
    centralWidget.addWidget(ebayFrame);

    // add sample rates to dictionary for the various wav files (see checkSequence)
    sampleRateDictionary[1] = 44100;
    sampleRateDictionary[2] = 48000;
    sampleRateDictionary[3] = 44100;
    sampleRateDictionary[4] = 44100;

    // Add channel counts to dictionary for the various wav files (see checkSequence)
    channelCountDictionary[1] = 1;
    channelCountDictionary[2] = 2;
    channelCountDictionary[3] = 2;
    channelCountDictionary[4] = 2;

    //initialize the Python interpreter
    Py_Initialize();

    if (!Py_IsInitialized()) {
        qCritical() << "Python initialization failed";
    }
}

/********************************************************************************************************
 *                                           Protected Methods                                          *
 ********************************************************************************************************/

void GoalsDashboard::keyPressEvent(QKeyEvent *event) {
    // add the keystroke to the end of the list
    m_keySequence.append(event->key());

    // If the list is more than 10 keystrokes remove from the front of the list (oldest keystroke)
    while (m_keySequence.size() > 10) {
        m_keySequence.pop_front();
    }

    // Check if the sequence of keypresses is the correct sequence
    checkSequence();
}

/********************************************************************************************************
 *                                            Private Methods                                           *
 ********************************************************************************************************/


/*
 * THIS IS AN EASTER EGG IT"S NOT ACTUALLY IMPORTANT
 * To add more songs find a .wav file that you want to add. Copy that wav file into the konamiCode folder, add it's characterists to the dictionary and increase the range of the QRandomGenerator by 1
 * To find a songs sample rate, channel count and sample format I have been using https://www.maztr.com/audiofileanalyzer
 */
void GoalsDashboard::checkSequence() {
    // If the sequence of keypresses is not the desired sequence don't do anything
    if (m_keySequence != desiredSequence) {
        return;
    }

    // If audio is currently playing turn it off and exit the function
    if (audio != nullptr && audio->state() == QAudio::ActiveState) {
        audio->stop();
        return;
    }

    // If audio is no longer playing, but it was at some point, close the sourceFile.
    if (audio != nullptr) {
        sourceFile.close();
    }

    // Generate a random in from [1,5) inclusive 1 excluse 5 aka 1-4
    int randFile = QRandomGenerator::global()->bounded(1,5);

    // Set the fileName to be the random int and open it for reading
    QString filePath = "konamiCode/" + QString::number(randFile) + ".wav";
    sourceFile.setFileName(filePath);
    sourceFile.open(QIODevice::ReadOnly);

    // Define how the audio will be played using the dictionaries given
    QAudioFormat format;
    format.setSampleRate(sampleRateDictionary[randFile]);
    format.setChannelCount(channelCountDictionary[randFile]);
    format.setSampleFormat(QAudioFormat::Int16);

    // Check to see if the device can actually play the audio file/format at all
    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (!info.isFormatSupported(format)) {
        qWarning() << "audio format not supported by backend, cannot play audio.";
        return;
    }

    // Play the audio
    audio = new QAudioSink(format, this);
    audio->start(&sourceFile);
}

void GoalsDashboard::rewriteJson() {
    // Create a lockfile
    QLockFile lockFile("config.json.lock");

    // Try to lock said lockfile for 1000 milliseconds .1 second
    if (!lockFile.tryLock(1000)) {

        // If it can't access said lock file (somthing else has it locked already try again in 5000 milliseconds
        QTimer::singleShot(5000, this, [=](){
            this->rewriteJson();
        });
        return;
    }

    // Lock was sucssessfull so open the config.json file and rewrite it
    QFile file("config.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument jsonDocument(configJson);
        file.write(jsonDocument.toJson());
        file.close();
    } else { // If unable to open the file for writing show a messagebox with that information
        QMessageBox::critical(nullptr, "Error", "Failed to open file for writing:\n" + file.errorString());
    }

    // Unlock the lockfile so something else can access the file at a later point
    lockFile.unlock();
}

void GoalsDashboard::makeBackup() {
    // Create today's date as a string
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    // Define the file path to the backup
    QString backupDirPath = "backups";
    QString backupFilePath = backupDirPath + "/file_backup_" + date + ".json";

    // Check if the backup file already exists
    if (QFile::exists(backupFilePath)) {
        return;
    }

    // Ensure the backup directory exists
    QDir dir;
    if (!dir.exists(backupDirPath)) {
        dir.mkpath(backupDirPath);
    }

    // Copy the file to create the backup
    if (!QFile::copy("config.json", backupFilePath)) {
        QMessageBox::critical(nullptr, "Error", "Failed to create backup\n");
    }
}

void GoalsDashboard::updateHistory() {
    // Load history.json file
    QFile historyFile("history.json");
    historyFile.open(QFile::ReadOnly);
    QJsonDocument historyDoc = QJsonDocument::fromJson(historyFile.readAll());
    historyFile.close();

    // Parse JSON data
    QJsonObject historyObject = historyDoc.object();

    // Check if the last_changed date is today
    QDate today = QDate::currentDate();
    if (historyObject.value("last_changed").toString() == today.toString(Qt::ISODate)) {
        // If the last_changed date is today, return without doing anything
        return;
    }

    // Append current goals to history
    for (const QString& category : configJson.keys()) { // Iterate over the categories
        // Get the goals in the current config file and the history file for that category
        QJsonArray currentCategoryArray = configJson.value(category).toArray();
        QJsonArray historyCategoryArray = historyObject.value(category).toArray();

        for (const QJsonValue& currentValue : currentCategoryArray) { // Iterate over the goals in the current config file
            QJsonObject currentGoal = currentValue.toObject();
            bool goalExistsInHistory = false;

            // Iterate over the goals in the history config file
            for (int i = 0; i < historyCategoryArray.size(); ++i) {
                QJsonObject historyGoal = historyCategoryArray[i].toObject();

                // Check if the name of the currentGoal and the History goal are the same && the starting dates are the same for both to ensure that they are the same goals
                if (historyGoal.value("name") == currentGoal.value("name") && historyGoal.value("start_date") == currentGoal.value("start_date")) {
                    // Append the current_value to the daily_values array
                    QJsonArray dailyValues = historyGoal.value("daily_values").toArray();
                    dailyValues.append(currentGoal.value("current_value"));
                    historyGoal.insert("daily_values", dailyValues);

                    // Check if today is the day after the goal's end date. If it is add the progress_at_end_date value
                    if(QDate::currentDate() == QDate::fromString(historyGoal.value("end_date").toString(), "ddd MMM d yyyy").addDays(1)) {
                        historyGoal.insert("progress_at_end_date", currentGoal.value("current_value"));
                    }


                    // Replace the old goal in the history array with the updated goal
                    historyCategoryArray.replace(i, historyGoal);
                    goalExistsInHistory = true;
                    break;
                }

            }

            // If the current goal could not be found in the history
            if (!goalExistsInHistory) {
                // Create a new goal with the current_value in the daily_values array
                QJsonObject newGoal;
                newGoal.insert("name", currentGoal.value("name"));
                newGoal.insert("start_date", currentGoal.value("start_date"));
                newGoal.insert("end_date", currentGoal.value("end_date"));
                newGoal.insert("target_value", currentGoal.value("target_value"));
                QJsonArray dailyValues;
                dailyValues.append(currentGoal.value("current_value"));
                newGoal.insert("daily_values", dailyValues);

                // In the off chance that today is the day after the goal's end date, add the progress_at_end_date value
                if(QDate::currentDate() == QDate::fromString(currentGoal.value("end_date").toString(), "ddd MMM d yyyy").addDays(1)) {
                    newGoal.insert("progress_at_end_date", currentGoal.value("current_value").toString());
                }
                // Add the goal to the history array
                historyCategoryArray.append(newGoal);
            }
        }

        // If the category is not found, insert a new category if it is found replace it
        historyObject.insert(category, historyCategoryArray);
    }

    // Update the last_changed date to today
    historyObject.insert("last_changed", today.toString(Qt::ISODate));



    // Write the updated JSON data back to the history.json file
    // Create a lockfile
    QLockFile lockFile("history.json.lock");
    // Try to lock said lockfile for 1000 milliseconds .1 second
    if (!lockFile.tryLock(1000)) {
        // If it can't access said lock file (somthing else has it locked already try again in 5000 milliseconds
        QTimer::singleShot(10000, this, [=](){
            this->updateHistory();
        });
        return;
    }

    // Lock was sucssessfull so open the config.json file and rewrite it
    if (historyFile.open(QIODevice::WriteOnly)) {

        historyFile.write(QJsonDocument(historyObject).toJson());
        historyFile.close();
    } else {
        QMessageBox::critical(nullptr, "Error", "Failed to open file for writing:\n" + historyFile.errorString());
    }

    // Unlock the lockFile
    lockFile.unlock();
}

void GoalsDashboard::startDailyTimer(){

    // Get the current time
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // Get the time until midnight tomorrow
    QDateTime tomorrowDateTime = currentDateTime.addDays(1);
    tomorrowDateTime.setTime(QTime(0, 0, 0));
    qint64 timeDifference = currentDateTime.msecsTo(tomorrowDateTime);

    // Get a random number of milliseconds up to an hour that this will be delayed by 0-3,599,999 (that way if there are multiple instances of the program they don't all go off at the exact same time)
    qint64 randomDelay = QRandomGenerator::global()->bounded(60 * 60 * 1000);
    timeDifference += randomDelay;

    // If somehow we already passed that go another day (both the delay was tiny and the time until the next day was tiny)
    if (timeDifference < 0) {
        tomorrowDateTime = tomorrowDateTime.addDays(1);
        timeDifference = currentDateTime.msecsTo(tomorrowDateTime);

        // Get the random delay again
        qint64 randomDelay = QRandomGenerator::global()->bounded(60 * 60 * 1000);
        timeDifference += randomDelay;
    }

    // Start the timer
    myDailyTimer->start(timeDifference);
}

void GoalsDashboard::updateRepeating() {
    // Go over each category
    bool anyUpdated = false;
    for (QString& category : configJson.keys()) {
        QJsonArray currentCategoryArray = configJson.value(category).toArray();

        // Go over each goal within the category
        for (auto&& currentValue : currentCategoryArray) {
            QJsonObject currentGoal = currentValue.toObject();

            if (currentGoal.contains("days_until_repeat")){
                if (QDate::fromString(currentGoal.value("end_date").toString()) < QDate::currentDate()){
                    int daysUntilRepeat = currentGoal.value("days_until_repeat").toString().toInt();
                    currentGoal["start_date"] = QDate::currentDate().toString();
                    currentGoal["current_value"] = "0";
                    currentGoal["end_date"] = QDate::currentDate().addDays(daysUntilRepeat -1 ).toString(); //subtract one from daysUntilRepeat so that it counts the day that it currently is as one of the days
                    anyUpdated = true;

                    currentValue = currentGoal;
                }
            }
        }


        int n = currentCategoryArray.size();
        for (int i = 0; i < n -1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                QJsonObject obj1 = currentCategoryArray[j].toObject();
                QJsonObject obj2 = currentCategoryArray[j + 1].toObject();
                QDate date1 = QDate::fromString(obj1.value("end_date").toString(), "ddd MMM d yyyy");
                QDate date2 = QDate::fromString(obj2.value("end_date").toString(), "ddd MMM d yyyy");
                if (date1 > date2) {
                    currentCategoryArray[j] = obj2;
                    currentCategoryArray[j + 1] = obj1;
                }
            }
        }


        configJson[category] = currentCategoryArray;
    }

    if (anyUpdated){
        rewriteJson();
    }
}

void GoalsDashboard::invertAll() {

    // Check if today is april first
    QDate aprilFirst(QDate::currentDate().year(), 4, 1);
    if (QDate::currentDate() == aprilFirst) {
        // Set the window icon to be the U of U logo
        QIcon icon("konamiCode/Utah.png");
        this->setWindowIcon(icon);

        // Change the background color to be U of U red
        QPalette palette = this->palette();
        palette.setColor(QPalette::Window, QColor(190, 0, 0));
        this->setPalette(palette);

        if (this->ebayFrame != nullptr) {
            ebayFrame->repopulate();
        }

        // If the fullFrame is already defined repopulate the areaFrames so they will also change
        if (this->fullFrame != nullptr) {
            this->fullFrame->repopulateAll();
        }

        // stop the function
        return;
    }
    // Today is not april first

    // Change the window icon back to BYU logo
    QIcon icon("BYU.png");
    this->setWindowIcon(icon);

    // Change the background color to be BYU Blue
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(0, 46, 93));
    this->setPalette(palette);

    if (this->ebayFrame != nullptr) {
        ebayFrame->repopulate();
    }

    // If the fullFrame is already defined repopulate the areaFrames so they will also change
    if (this->fullFrame != nullptr) {
        this->fullFrame->repopulateAll();
    }

}

void GoalsDashboard::loadJson() {

    qDebug() << "loading config.json";

    QLockFile lockFile("config.json.lock");
    lockFile.setStaleLockTime(10000);
    if (!lockFile.tryLock(1000)) {
        qCritical() << "Failed to acquire lock for config.json";
        return;
    }

    // open the file
    QFile file("config.json");
    QJsonObject jsonObj;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lockFile.unlock();
        qCritical() << "Failed to open file: config.json";
        return;
    }

    // put the data into a buffer (byte array)
    QByteArray jsonData = file.readAll();
    file.close();
    lockFile.unlock();

    // Try to parse the QByteArray as a jsonDoc
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &error);
    if (jsonDoc.isNull()) {
        qCritical() << "Failed to parse JSON:" << error.errorString();
        return;
    }

    // Convert the jsonDocument to a jsonObject
    jsonObj = jsonDoc.object();
    configJson =  jsonObj;

    if (editGoalMenu != nullptr) {
        populateMenus();
    }
}

void GoalsDashboard::editGoalsGoalSelected(QString areaName, QString goalName) {
    // Create a new QDialog
    std::unique_ptr<QDialog> openDialog(new QDialog);
    openDialog->setWindowTitle("Edit/Create Goals");

    // Create a submit button and the layout
    QPushButton *submitButton;
    QVBoxLayout *layout = new QVBoxLayout(openDialog.get());

    // Create a line edit to let the user put in the goal name if needed
    QLineEdit *newGoalNameEdit;

    // If the user want's to make a new goal
    if (goalName == "Add new goal") {
        // Create a label and add it to the layout
        QLabel *newGoalName = new QLabel("New Goal's Name:", openDialog.get());
        layout->addWidget(newGoalName);

        // Define the QLineEdit and add it to the layout
        newGoalNameEdit = new QLineEdit(openDialog.get());
        newGoalNameEdit->setPlaceholderText("Enter name here...");
        layout->addWidget(newGoalNameEdit);
    }
    else {
        // Define the goalsArray to find the goal that will be edited
        QJsonArray goalsArray = configJson.value(areaName).toArray();

        // Iterate over the goals in the goalsArray
        for (const auto& goal : goalsArray) {
            QJsonObject goalObj = goal.toObject();

            // Check if the goal is the desired goal
            if (goalObj.value("name") == goalName) {
                // Define a label and add it to the label
                QLabel *currentTarget = new QLabel("Current Target: " + goalObj.value("target_value").toString(), openDialog.get());
                layout->addWidget(currentTarget);
            }
        }
    }
    // Create a label and add it to the layout
    QLabel *newTarget = new QLabel("Target:", openDialog.get());
    layout->addWidget(newTarget);

    // Create a line edit to set the target value and add it to the layout
    QLineEdit *editTarget = new QLineEdit(openDialog.get());
    editTarget->setPlaceholderText("Enter target value here...");
    layout->addWidget(editTarget);

    // Create a checkbox and hide it
    QCheckBox *repeatCheckBox = new QCheckBox("Repeating Goal", openDialog.get());
    repeatCheckBox->hide();

    // If we are creating a new goal show the checkbox
    if (goalName == "Add new goal") {
        layout->addWidget(repeatCheckBox);
        repeatCheckBox->show();
    }

    // Create a label and show it
    QLabel *endDate = new QLabel("End Date:", openDialog.get());
    layout->addWidget(endDate);

    // Create a date edit so the user can select the end date set the default end date to be one week away
    QDateEdit *endDateEdit = new QDateEdit(openDialog.get());
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setDate(QDate::currentDate().addDays(7));
    endDateEdit->show();
    layout->addWidget(endDateEdit);

    // Create a label and combobox and hide them
    QLabel *repeatLabel = new QLabel("How many days until the goal should repeat?", openDialog.get());
    QComboBox *repeatComboBox = new QComboBox(openDialog.get());
    repeatLabel->hide();
    repeatComboBox->hide();

    // Populate the repeatComboBox with all intigers up to 14
    for (int i = 1; i <= 14; i++) {
        repeatComboBox->addItem(QString::number(i));
    }

    // Check if we are creating a new goal
    if (goalName == "Add new goal") {
        // Connect the checkBox so we can show various things if it is checked or not
        QObject::connect(repeatCheckBox, &QCheckBox::stateChanged, openDialog.get(), [&]() {
            // If the checkBox is now checked
            if (repeatCheckBox->isChecked()) {
                // Remove these widgets from the layout and hide some
                layout->removeWidget(submitButton);
                layout->removeWidget(endDateEdit);
                layout->removeWidget(endDate);
                endDateEdit->hide();
                endDate->hide();

                // Add these widgets to the layout and show them (order matters)
                layout->addWidget(repeatLabel);
                repeatLabel->show();
                layout->addWidget(repeatComboBox);
                repeatComboBox->show();
                layout->addWidget(submitButton);
            } else {
                // Remove these widgets from the layout and hide some of them
                layout->removeWidget(submitButton);
                layout->removeWidget(repeatLabel);
                layout->removeWidget(repeatComboBox);
                repeatLabel->hide();
                repeatComboBox->hide();

                // Add these widgets to the layout and show them (order matters)
                layout->addWidget(endDate);
                endDate->show();
                layout->addWidget(endDateEdit);
                endDateEdit->show();
                layout->addWidget(submitButton);
            }
        });
    }

    // Add the submitbutton to the layout
    submitButton = new QPushButton("Submit", openDialog.get());
    layout->addWidget(submitButton);

    // Connect the submit button so this will be called when it is pressed
    connect(submitButton, &QPushButton::clicked, openDialog.get(), [=, &openDialog]() {
        // Create a string and a boolean to make sure that the function call goes well
        QString newGoalName;
        bool isNewGoal;

        // Check if the goal is a new goal or not and set the values
        if (goalName == "Add new goal") {
            newGoalName = newGoalNameEdit->text();
            isNewGoal = true;
        }
        else {
            newGoalName = goalName;
            isNewGoal = false;
        }

        // Submit the goal
        editGoalSubmit(areaName, newGoalName, QDate::currentDate(), endDateEdit->date(), editTarget->text(), isNewGoal, repeatCheckBox->isChecked(), repeatComboBox->currentText());
        openDialog->close();
    });

    // Set the layout and open the DialogBox
    openDialog->setLayout(layout);
    openDialog->exec();
}

void GoalsDashboard::editGoalSubmit(QString areaName, QString goalName, QDate startDate, QDate endDate, QString target, bool isNewGoal, bool repeating, QString daysUntilRepeat) {
    // Check if it is a new goal
    if (isNewGoal) {
        // Create a new QJsonObject to populate
        QJsonObject newGoal;

        // Add the name, current_value, end_date, start_date, and target value to the goal
        newGoal["name"] = goalName;
        newGoal["current_value"] = "0";
        newGoal["end_date"] = endDate.toString();
        newGoal["start_date"] = startDate.toString();
        newGoal["target_value"] = target;

        // Check if the goal is repeating
        if (repeating) {
            // set the end_date and add the days_until_repeat values
            QString repeatDate = QDate::currentDate().addDays(daysUntilRepeat.toInt()).toString();
            newGoal["end_date"] = repeatDate;
            newGoal["days_until_repeat"] = daysUntilRepeat;
        }

        // Define the areaArray to add the goal to
        QJsonArray areaArray = configJson[areaName].toArray();

        // Iterate over the array
        bool inserted = false;
        QDate newGoalDate = QDate::fromString(endDate.toString(), "ddd MMM d yyyy");
        for (int i = 0; i < areaArray.size(); ++i) {
            QDate otherGoalDate = QDate::fromString(areaArray[i].toObject().value("end_date").toString(), "ddd MMM d yyyy");
            // Check if the end_date for the new goal is before the end date of the checked goal
            if (newGoalDate < otherGoalDate) {
                // Put the goal in before the other goal
                areaArray.insert(i, newGoal);
                inserted = true;
                break;
            }
        }

        // If the goal was never inserted put the goal at the end of the array
        if (!inserted) {
            areaArray.append(newGoal);
        }

        // Make the array in the json data be the new areaArray
        configJson[areaName] = areaArray;
    }
    else { // If the goal is not a new goal
        // Go over each of the goals in the areaArray
        QJsonArray goalsArray = configJson[areaName].toArray();
        for (auto&& goal : goalsArray){
            // Create a copy of the goal
            QJsonObject goalObj = goal.toObject();
            // If the goal is the desired goal (aka names match)
            if (goalObj.value("name").toString() == goalName) {
                // Set these values
                goalObj["current_value"] = "0";
                goalObj["end_date"] = endDate.toString();
                goalObj["start_date"] = startDate.toString();
                goalObj["target_value"] = target;

                // Set the goal within the goalsArray to be the new goalObj
                goal = goalObj;
                break;
            }
        }

        // BUBBLE SORT sorting goals by end_date
        int n = goalsArray.size();
        for (int i = 0; i < n -1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                QJsonObject obj1 = goalsArray[j].toObject();
                QJsonObject obj2 = goalsArray[j + 1].toObject();
                QDate date1 = QDate::fromString(obj1.value("end_date").toString(), "ddd MMM d yyyy");
                QDate date2 = QDate::fromString(obj2.value("end_date").toString(), "ddd MMM d yyyy");
                if (date1 > date2) {
                    goalsArray[j] = obj2;
                    goalsArray[j + 1] = obj1;
                }
            }
        }

        configJson[areaName] = goalsArray;
    }

    // Rewrite the config.json file
    rewriteJson();

    // Previously I also would repopulate the areas, but because it is done whenever the file is changed
}

void GoalsDashboard::removeGoalSelected(QString areaName, QString goalName) {

    // Make a copy of the areaArray (goalsArray)
    QJsonArray goalsArray = configJson[areaName].toArray();

    // Iterate over each of the goals in that array
    for (int i = 0; i < goalsArray.size(); i++){
        QJsonObject goalObj = goalsArray[i].toObject();

        // If the goal is the goal we want to remove, remove it
        if (goalObj.value("name").toString() == goalName) {
            goalsArray.removeAt(i);
            break;
        }
    }
    // Replace the area Array in the configJson object with the copy
    configJson[areaName] = goalsArray;

    // Rewrite the config.json file
    rewriteJson();

    // Previously I also would repopulate the areas, but because it is done whenever the file is changed
}

void GoalsDashboard::populateMenus() {
    // Remove the old items from the menus
    editGoalMenu->clear();
    removeGoalMenu->clear();

    // Define the styling for the submenus
    QDate aprilFirst = QDate(QDate::currentDate().year(), 4, 1);
    QString styling;
    if (QDate::currentDate() == aprilFirst) { // If today is April first
        if (!isDarkMode) { // If dark mode is not on
            styling = "QMenu { font-size: 15px; background-color: #34342A; color: white; }"
                      "QMenu::item:selected {background-color: #444; color: white; }";
        } else { // If dark mode is on
            styling = "QMenu { font-size: 15px; background-color: #BE0000; color: white; }"
                      "QMenu::item:selected {background-color: #444; color: white; }";
        }
    }
    else if (!isDarkMode) { // If today is not April first and dark mode is not on
        styling = "QMenu { font-size: 15px; background-color: #CBCBD5;}"
                  "QMenu::item:selected {background-color: #333; color: white; }";
    } else { // If dark mode is on
        styling = "QMenu { font-size: 15px; background-color: #002E5D; color: white; }"
                  "QMenu::item:selected {background-color: #333;}";
    }

    // Iterate over the categories
    for (const QString& category : configJson.keys()) {
        // Create a copy of the areaArray (currentCategoryArray)
        QJsonArray currentCategoryArray = configJson.value(category).toArray();

        // Create a submenu of the category
        QMenu *currentCategoryEditMenu = new QMenu(category);
        editGoalMenu->addMenu(currentCategoryEditMenu);
        currentCategoryEditMenu->setStyleSheet(styling);

        // Create a submenu of the category
        QMenu *currentCategoryRemoveMenu = new QMenu(category);
        removeGoalMenu->addMenu(currentCategoryRemoveMenu);
        currentCategoryRemoveMenu->setStyleSheet(styling);

        // Iterate over the goals in the currentCategoryArray
        for (const QJsonValue& currentValue : currentCategoryArray) {
            // Create a copy of the goal
            QJsonObject goalObj = currentValue.toObject();
            QString goalName = goalObj.value("name").toString();
            // Create Actions in each menu of that goalName
            currentCategoryEditMenu->addAction(goalName);
            currentCategoryRemoveMenu->addAction(goalName);
        }
        // Add an action in ONLY the EDIT goal menu of Add new goal
        currentCategoryEditMenu->addAction("Add new goal");
    }
}

/********************************************************************************************************
 *                                             Private Slots                                            *
 ********************************************************************************************************/


void GoalsDashboard::fileChanged() {
    // Get the new Json Data
    try {
        loadJson();
    } catch (std::exception e) {
        qCritical() << "error while loading goals file" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception caught while loading goals config";
    }

    // Repopulate the fullFrame->AreaFrames with the new information
    fullFrame->repopulateAll();
    qDebug() << "repopulating ebayFrame";
    ebayFrame->repopulateGoals();

}

void GoalsDashboard::dailyTimerFinished(){
    // The the daily backup of the config file
    makeBackup();
    // Update the history.json file
    updateHistory();
    // Check any repeating goal and update any that need to be updated
    updateRepeating();

    // If today is april first invert all
    QDate aprilFirst(QDate::currentDate().year(), 4, 1);
    if (QDate::currentDate() == aprilFirst) {
        invertAll();
    } // If today is april 2nd invert it back
    else if(QDate::currentDate() == aprilFirst.addDays(1)){
        invertAll();
    }

    // restart the daily timer
    startDailyTimer();
}


void GoalsDashboard::jsonToCsv() {
    // Load JSON data
    QFile jsonFile("history.json");
    jsonFile.open(QFile::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();

    // Prepare data for CSV
    QJsonObject jsonObject = doc.object();
    QStringList categories = jsonObject.keys();
    categories.removeOne("last_changed");

    // Prepare CSV data
    const QString& csvFilePath = "../history.csv";
    QString csvData;
    QTextStream stream(&csvData);

    // Write the header
    stream << "category,name,start_date,end_date,target_value,progress_at_end_date,daily_values\n";

    // Write data
    for (const QString& category : categories) {
        QJsonArray jsonArray = jsonObject.value(category).toArray();
        for (const QJsonValue& goalVal : jsonArray) {
            QJsonObject goalObj = goalVal.toObject();
            QJsonArray dailyValues = goalObj.value("daily_values").toArray();
            QString dailyValuesStr;
            for (const QJsonValue& dailyValue : dailyValues) {
                dailyValuesStr += dailyValue.toString() + ",";
            }

            stream << category << ","
                   << goalObj.value("name").toString() << ","
                   << goalObj.value("start_date").toString() << ","
                   << goalObj.value("end_date").toString() << ","
                   << goalObj.value("target_value").toString() << ","
                   << (goalObj.contains("progress_at_end_date") ? goalObj.value("progress_at_end_date").toString() : "N/A") << ","
                   << dailyValuesStr << "\n";
        }
    }

    // Get a filelock for the csv file
    QLockFile lockFile("../history.csv.lock");
    if (!lockFile.tryLock(1000)) {
        QTimer::singleShot(10000, this, [=](){
            jsonToCsv();
        });
        return;
    }

    // Write CSV data to file
    QFile csvFile(csvFilePath);
    if (csvFile.open(QIODevice::WriteOnly | QFile::Truncate)) {
        csvFile.write(csvData.toUtf8());
        csvFile.close();
        lockFile.unlock();
        QMessageBox::information(nullptr, "Info", "CSV finished writing");
    } else {
        QMessageBox::critical(nullptr, "Error", "Failed to open file for writing:\n" + csvFile.errorString());
    }

    // Unlock the filelock
    lockFile.unlock();
}

void GoalsDashboard::darkMode() {
    // If dark mode is on turn it off, if it is off turn it on
    isDarkMode = !isDarkMode;

    QDate aprilFirst(QDate::currentDate().year(), 4, 1);
    if (QDate::currentDate() == aprilFirst) {
        if (!isDarkMode) {
            // Set the styling of the Qmenu to be white text in a dark gray box
            setStyleSheet("QMenuBar { background-color: #BE0000; color: white; padding: 2px 5px; font: 15px;}"
                          "QMenuBar::item {background-color: #34342A; }"
                          "QMenuBar::item:selected { background-color: #444; color: white; }"
                          "QMenu { font-size: 15px; background-color: #34342A; color: white; }"
                          "QMenu::item:selected {background-color: #444; color: white; }");

            // Change the background color to be U of U red
            QPalette pal = palette();
            pal.setColor(QPalette::Window, QColor(190, 0, 0)); //#002E5D
            setPalette(pal);
        } else {
            // Set the styling of the Qmenu to be white text in a U of U red box
            setStyleSheet("QMenuBar { background-color: #34342A; color: white; padding: 2px 5px; font: 15px;}"
                          "QMenuBar::item {background-color: #BE0000; }"
                          "QMenuBar::item:selected { background-color: #444; color: white; }"
                          "QMenu { font-size: 15px; background-color: #BE0000; color: white; }"
                          "QMenu::item:selected {background-color: #444; color: white; }");

            // Change the background color to be Dark Gray
            QPalette pal = palette();
            pal.setColor(QPalette::Window, QColor(52, 52, 42)); //#002E5D
            setPalette(pal);
        }
    } // It is not April first
    else if (!isDarkMode) { // If dark mode is now off
        // Set the styling of the Qmenu to be black text in a gray box
        setStyleSheet("QMenuBar { background-color: #002E5D; color: black; padding: 2px 5px; font: 15px;}"
                      "QMenuBar::item {background-color: #CBCBD5;}"
                      "QMenuBar::item:selected { background-color: #333; color: white; }"
                      "QMenu { font-size: 15px; background-color: #CBCBD5;}"
                      "QMenu::item:selected {background-color: #333; color: white; }");

        // Change the background color to be BYU Blue
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(0, 46, 93)); //#002E5D
        setPalette(pal);
    } else { // If dark mode is now on
        // Set the styling of the Qmenu to be white text in a blue box
        setStyleSheet("QMenuBar { background-color: #CBCBD5; color: white; padding: 2px 5px; font: 15px;}"
                      "QMenuBar::item {background-color: #002E5D;}"
                      "QMenuBar::item:selected { background-color: #333;}"
                      "QMenu { font-size: 15px; background-color: #002E5D; color: white}"
                      "QMenu::item:selected {background-color: #333;}");

        // Change the background color to be gray
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(203, 203, 213)); //#CBCBD5
        setPalette(pal);
    }

    // Turn all of the areaFrames to darkmode via the fullFrame
    fullFrame->darkMode();
    ebayFrame->darkMode();

    // Recolor the menus by repopulating them
    populateMenus();
}


void GoalsDashboard::ebayMode() {
    isEbayMode = !isEbayMode;

    if (isEbayMode) {
        centralWidget.setCurrentIndex(1);
    }
    else {
        centralWidget.setCurrentIndex(0);
    }
}

void GoalsDashboard::refreshRefreshToken() {

    // Import refreshRefreshToken.py
    PyObject* pModule = PyImport_ImportModule("refreshRefreshToken");

    if (pModule == nullptr) {
        displayPythonError();
        return;
    }

    // Find the main function in refreshRefreshToken.py
    PyObject* pFunc = PyObject_GetAttrString(pModule, "main");
    Py_DECREF(pModule);

    if (pFunc == nullptr || !PyCallable_Check(pFunc)) {
        displayPythonError();
        return;
    }

    // Call the main function and save the result
    PyObject* result = PyObject_CallObject(pFunc, nullptr);
    Py_DECREF(pFunc);

    if (result == nullptr) {
        displayPythonError();
        return;
    }

    // It completed successfully do the simple cleanup
    qDebug() << "Refresh token script executed successfully";
    Py_DECREF(result);
}

void GoalsDashboard::displayPythonError() {

    // Get the type, value, and traceback of the error that occured
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

    // Convert the type and value PyObjects to strings (that way if getting the traceback fails we still have this)
    PyObject* str_exc_type = PyObject_Repr(ptype);
    PyObject* str_exc_value = PyObject_Repr(pvalue);
    PyObject* pyStrType = PyUnicode_AsEncodedString(str_exc_type, "utf-8", "Error ~");
    PyObject* pyStrValue = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "Error ~");
    QByteArray strExcType = PyBytes_AS_STRING(pyStrType);
    QByteArray strExcValue = PyBytes_AS_STRING(pyStrValue);

    // Import traceback module
    PyObject* tracebackModule = PyImport_ImportModule("traceback");

    if (tracebackModule == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Failed to execute refresh token script\nType: " + QString::fromUtf8(strExcType) + "\nValue: " + QString::fromUtf8(strExcValue));
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        Py_XDECREF(str_exc_type);
        Py_XDECREF(str_exc_value);
        Py_XDECREF(pyStrType);
        Py_XDECREF(pyStrValue);
        return;
    }

    // Find the format_exception function in the traceback module
    PyObject* tracebackFormat = PyObject_GetAttrString(tracebackModule, "format_exception");
    Py_DECREF(tracebackModule);


    if (tracebackFormat == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Failed to execute refresh token script\nType: " + QString::fromUtf8(strExcType) + "\nValue: " + QString::fromUtf8(strExcValue));
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        Py_XDECREF(str_exc_type);
        Py_XDECREF(str_exc_value);
        Py_XDECREF(pyStrType);
        Py_XDECREF(pyStrValue);
        return;
    }
    // Format the traceback along with the type and value of the error
    PyObject* tracebackArgs = Py_BuildValue("(OOO)", ptype, pvalue, ptraceback);
    PyObject* tracebackList = PyObject_CallObject(tracebackFormat, tracebackArgs);
    PyObject* tracebackStr = PyUnicode_Join(PyUnicode_FromString(""), tracebackList);

    PyObject* pyStrTraceback = PyUnicode_AsEncodedString(tracebackStr, "utf-8", "Error ~");
    QByteArray strExcTraceback = PyBytes_AS_STRING(pyStrTraceback);

    QMessageBox::critical(nullptr, "Error", "Failed to execute refresh token script:\n " + QString::fromUtf8(strExcTraceback));

    Py_DECREF(tracebackStr);
    Py_DECREF(pyStrTraceback);
    Py_DECREF(tracebackList);
    Py_DECREF(tracebackArgs);
    Py_DECREF(tracebackFormat);

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    Py_XDECREF(str_exc_type);
    Py_XDECREF(str_exc_value);
    Py_XDECREF(pyStrType);
    Py_XDECREF(pyStrValue);
    Py_XDECREF(pyStrTraceback);
    return;
}
