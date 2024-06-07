#include "areaframe.h"

AreaFrame::AreaFrame(const QColor& color, const QString& name, QJsonObject* configJson, QWidget* parent)
    : QFrame{parent}, webEngine{this}, layout{this}
{

    this->name = name;

    this->channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("qtBridge"), this);
    webEngine.page()->setWebChannel(channel);


    this->configJson = configJson;

    layout.addWidget(&webEngine);
    layout.setContentsMargins(0,0,0,0); // This removes the margin between the edge of the frame and the html content
    setLayout(&layout);

    repopulate();
}


void AreaFrame::darkMode(){
    // If dark mode is on turn it off; if it is off turn it on, then repopulate
    isDarkMode = !isDarkMode;
    repopulate();
}

void AreaFrame::repopulate() {

    // Set the headers/meta data for the page
    QString html = "<!DOCTYPE html><html><head>"
                   "<meta charset='UTF-8'>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "<title>Area Goals</title>"
                   "<script src='qrc:///qtwebchannel/qwebchannel.js'></script>"; // This script is needed to connect the Javascript to the C++

    // Get the Stying for the page
    html += getCSS();

    // Close the head and open the body (all visible items will be in the body of the page)
    html += "</head>";

    html += getBody();

    // Honestly I am not sure if this is really needed I have it as theoretically it might be needed for when the page closes to make sure everything closes
    html += "<script>"
            "const channel = new QWebChannel(qt.webChannelTransport, function(channel) {"
                // "window.qtBridge = channel.objects.qtBridge;"
            "});";

    // If dark mode is on define it to be on
    if (isDarkMode) {
        html += "const isDarkMode = true;";
    } else {
        html += "const isDarkMode = false;";
    }

    // Get the functions that happen for each goal
    html += getGoalsScript();

    // Add handle Button function this is called whenever the plus or minus button is pressed
    html += "function handleButton(goalNumber, increment) {"
                "var progress = document.getElementById(goalNumber).value;"
                "var progressAsInt = parseFloat(progress);"
                "channel.objects.qtBridge.inputSubmitted(progressAsInt +increment, goalNumber);"
            "}";

    // Close the script body and html
    html +=
        "</script></body></html>";

    // Set the page on the html page to be this new page
    webEngine.setHtml(html);

    // This doesn't show up when the .exe is used so this can honestly stay and the user would never notice it.
    qDebug() << this->name << "\n" << html << "\n";
}

/********************************************************************************************************
 *                                             Public Slots                                             *
 ********************************************************************************************************/

void AreaFrame::inputSubmitted(const QString &input, const QString goalNumber) {
    // Get this area's goals
    QJsonArray goalsArray = (*configJson)[this->name].toArray();
    QString targetValue;

    int i = 0;
    for (auto&& goal : goalsArray){ // Iterate over the goals

        // If i is equal to the goalNumber as an int
        if (i == goalNumber.toInt()) {
            // Convert that copy of the goal to be an object
            QJsonObject goalObj = goal.toObject();
            // Set the current value of the goal to be the submitted value
            goalObj["current_value"] = input;

            // Save the target value (this is only used for the funny box)
            targetValue = goalObj["target_value"].toString();

            // Set the goal to be the copy with changed current value
            goal = goalObj;
            break;
        }
        // Increase i
        i++;
    }
    // Set the this area's goals to be the edited array
    (*configJson)[this->name] = goalsArray;


    // Create a message box saying that the information was submitted that closes automatically in 1 second
    QMessageBox msgBox;
    msgBox.setWindowTitle("Success");
    msgBox.setText("Information Submited");
    msgBox.setStandardButtons(QMessageBox::NoButton);
    QTimer::singleShot(1000, &msgBox, [&msgBox]() {
        msgBox.accept();
    });
    msgBox.exec();

    // If the input (current value) is 69 and target value is 420
    if (input == "69" && targetValue == "420") {
        QFont font;
        font.setPointSize(69);

        // Create another message box that closes after 2 seconds of an emoji
        QMessageBox funnyBox;
        funnyBox.setWindowTitle("Nice");
        funnyBox.setText("\U0001F44C");
        funnyBox.setFont(font);
        funnyBox.setStandardButtons(QMessageBox::NoButton);
        QTimer::singleShot(2000, &funnyBox, [&funnyBox]() {
            funnyBox.accept();
        });

        funnyBox.exec();
    }

    // Rewrite the json file with the new data
    rewriteJson();

}

/********************************************************************************************************
 *                                            Private Methods                                           *
 ********************************************************************************************************/

void AreaFrame::rewriteJson(){
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
        QJsonDocument jsonDocument(*configJson);
        file.write(jsonDocument.toJson());
        file.close();
    } else { // If unable to open the file for writing show a messagebox with that information
        QMessageBox::critical(nullptr, "Error", "Failed to open file for writing:\n" + file.errorString());
    }

    // Unlock the lockfile so something else can access the file at a later point
    lockFile.unlock();
}

QString AreaFrame::getCSS() {

    QString styling = "<style> ";
    // April Fools joke (turn everything into U of U)
    QDate aprilFirst(QDate::currentDate().year(), 4, 1);
    if (QDate::currentDate() == aprilFirst) {
        if (!isDarkMode){
            this->color = QColor(52, 52, 42);
            styling +=  "body { background-color: " + color.name() + "; "
                            "font-size: 5vmin; "
                            "font-family: Arial, sans-serif;"
                            "margin-left: 18px;"
                            "color: white"
                        "}"
                        "hr { height: 2px; border-width: 0; background-color: #BE0000; width: 90%; }";

        } else {
            this->color = QColor(190, 0, 0);
            styling +=  "body { background-color: " + color.name() + "; "
                            "font-size: 5vmin; "
                            "font-family: Arial, sans-serif;"
                            "margin-left: 18px;"
                            "color: white"
                        "}"
                        "hr { height: 2px; border-width: 0; background-color: #34342A; width: 90%; }";
        }
    }
    else if (!isDarkMode){
        this->color = QColor(203, 203, 213);
        styling +=  "body { background-color: " + color.name() + "; "
                        "font-size: 5vmin; "
                        "font-family: Arial, sans-serif;"
                        "margin-left: 18px;"
                        "color: black"
                    "}"
                    "hr { height: 2px; border-width: 0; background-color: #002E5D; width: 90%; }";
    } else {
        this->color = QColor(0, 46, 93);
        styling +=  "body { background-color: " + color.name() + "; "
                        "font-size: 5vmin; "
                        "font-family: Arial, sans-serif;"
                        "margin-left: 18px;"
                        "color: white"
                    "}"
                    "hr { height: 2px; border-width: 0; background-color: #CBCBD5; width: 90%; }";
    }

    styling += "h1 { text-align: center; margin: 0.5em; font-size: 2em;}"
               "h3 { text-align: center; font-size: 1.15em;}"
               "input[type='text'] { margin-right: 5px; font-size: 5vmin; width: 9em;}"
               "button { margin-right: 2.5px; font-weight: bold;  font-size: 5vmin;}"
               "</style>";


    return styling;
}

QString AreaFrame::getBody() {

    QString html = "";

    // Add the area name as the title as well as a line underneath the title
    html += "<body><h1>" + name + "</h1>" + "<hr>";

    // Add each goal
    int i = 0;
    QJsonArray areaArray = configJson->value(name).toArray();
    foreach (const QJsonValue &value, areaArray) {
        if (value.isObject()) {
            QJsonObject goal = value.toObject();

            QString goalName = goal.value("name").toString();
            QString targetValue = goal.value("target_value").toString();
            QString currentValue = goal.value("current_value").toString();
            QString endDate = goal.value("end_date").toString();
            QString goalNum = QString::number(i);

            html += "<h3 id='title_" + goalNum + "'>" + goalName + "</h3>"
                    "<div>"
                        "Target: <span id='targetValue_" + goalNum +"'>" + targetValue + "</span> <br>"
                        "<span id='progressLabel_" + goalNum + "'> Current Progress: " + "<input type='text'  value='" + currentValue + "' id='" + goalNum + "'>"
                            "<button type='button' class='button_" + goalNum +"' onclick='handleButton(" + goalNum + ", 1)'>+</button>"
                            "<button type='button' class='button_" + goalNum +"' onclick='handleButton(" + goalNum + ", -1)'>–</button> </span><br>"
                        "<span id='finishLabel_" + goalNum + "'> Finish By: <span id='endDate_" + goalNum +"'>" + endDate + "</span> </span> <br>"
                    "</div>";
            i++;
        }
    }

    return html;
}

QString AreaFrame::getGoalsScript() {

    QString html = "";

    QJsonArray areaArray = configJson->value(name).toArray();

    for (int j=0; j<areaArray.size(); j++) {
        QString goalNum = QString::number(j);
        QString name = "inputField" + goalNum;
        html += "var " + name + " = document.getElementById('" + goalNum + "');"
                + name + ".addEventListener('keydown', function(event) {"
                         "if (event.keyCode === 13) {"
                         "var inputValue = " + name + ".value;"
                         "channel.objects.qtBridge.inputSubmitted(inputValue, " + goalNum + ");"
                            "}"
                            "});";

        html += "var progressLabel = document.getElementById('progressLabel_" + goalNum + "');"
                "var finishLabel = document.getElementById('finishLabel_" + goalNum + "');"
                "var inputValue = document.getElementById('" + goalNum + "').value;"
                "var targetValue = parseFloat(document.getElementById('targetValue_" + goalNum + "').innerText);"
                "var endDateString = document.getElementById('endDate_" + goalNum + "').innerText;"
                "var endDate = new Date(endDateString);"
                ""
                "var currentValue = parseFloat(inputValue);"
                ""
                "var today = new Date();"
                "today.setHours(0, 0, 0, 0);"
                "endDate.setHours(0, 0, 0, 0);"
                ""
                "if (currentValue >= targetValue) {"
                    "progressLabel.style.color = 'green';"
                "} else {"
                    "if (today.getMonth() === 3 && today.getDate() === 1) {"
                        "progressLabel.style.color = 'white';"
                    "} else {"
                        "if (isDarkMode == false) {"
                            "progressLabel.style.color = 'black';"
                        "} else {"
                            "progressLabel.style.color = 'white';"
                        "}"
                    "}"
                "}"
                "if (endDate < today) {"
                    "finishLabel.style.color = 'red';"
                "} else if (endDate.getTime() === today.getTime()) {"
                    "finishLabel.style.color = '#8B4000';"
                "} else {"
                    "if (today.getMonth() === 3 && today.getDate() === 1) {"
                        "progressLabel.style.color = 'white';"
                    "} else {"
                        "if (isDarkMode == false) {"
                            "progressLabel.style.color = 'black';"
                        "} else {"
                            "progressLabel.style.color = 'white';"
                        "}"
                    "}"
                "}"
                ""
                "if (today.getMonth() === 3 && today.getDate() === 1) {"
                    "document.body.style.color = 'white';"
                "} else {"
                    "if (isDarkMode == false) {"
                        "document.body.style.color = 'black';"
                    "} else {"
                        "document.body.style.color = 'white';"
                    "}"
                "}";


        // check if the current progress is a number. if it is not remove the two buttons (the plus and minus button because if they were pressed the information would be lost)
        html += "var buttons_" + goalNum + "= document.getElementsByClassName('button_" + goalNum + "');"
                + name + ".addEventListener('input', function() {"
                    "if (isNaN(" + name + ".value) || " + name + ".value === '') {"
                        "for (var button of buttons_" + goalNum + ") {"
                            "button.style.display = 'none';"
                        "}"
                    "} else {"
                        "for (var button of buttons_" + goalNum + ") {"
                            "button.style.display = 'inline-block';"
                        "}"
                    "}"
                "});";

        // At the start of the page load, check if the buttons should be hidden
        html += "document.addEventListener('DOMContentLoaded', function() {"
                    "if (isNaN(" + name + ".value) || " + name + ".value === '') {"
                        "for (var button of buttons_" + goalNum + ") {"
                            "button.style.display = 'none';"
                        "}"
                    "}"
                "});";

    }

    return html;
}

