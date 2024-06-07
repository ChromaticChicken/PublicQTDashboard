#include "ebaymessagesframe.h"

EbayMessagesFrame::EbayMessagesFrame(const QColor& color, QWidget* parent)
    : QWidget{parent}
{
    this->config = "";
    this->color = color;

    layout.addWidget(&webEngine);
    layout.setContentsMargins(0,0,0,0); // This removes the margin between the edge of the frame and the html content
    setLayout(&layout);

    repopulate();
}

void EbayMessagesFrame::repopulate() {
    if (config == "") {
        return;
    }

    QString html = "<!DOCTYPE html><html><head>"
                   "<meta charset='UTF-8'>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "<title>eBay Messages</title>";

    html += "<body>";

    html += getCSS();

    QXmlStreamReader xmlReader(config);
    QMap<QString, QString> myMap;
    processXml(xmlReader, myMap);

    QMap<QString, QString>::const_iterator it;
    for (it = myMap.constBegin(); it != myMap.constEnd(); it++) {
        if (it.value() == "false") {
            if (it.key().contains("un mensaje acerca")) {
                continue;
            }
            QString subject = it.key();
            subject.replace(" sent a message about", ":");
            qint64 index = subject.indexOf("#");
            if (index != -1) {
                subject = subject.left(index);
            }

            html += "<h4>" + subject + "</h4>";
        }
    }

    html += "</body></html>";

    webEngine.setHtml(html);
}


QString EbayMessagesFrame::getCSS() {
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
                            "color: white;"
                            "text-align: center;"
                        "}"
                            "hr { height: 2px; border-width: 0; background-color: #BE0000; width: 90%; }";

        } else {
            this->color = QColor(190, 0, 0);
            styling +=  "body { background-color: " + color.name() + "; "
                            "font-size: 5vmin; "
                            "font-family: Arial, sans-serif;"
                            "margin-left: 18px;"
                            "color: white;"
                            "text-align: center;"
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
                        "color: black;"
                        "text-align: center;"
                    "}"
                    "hr { height: 2px; border-width: 0; background-color: #002E5D; width: 90%; }";
    } else {
        this->color = QColor(0, 46, 93);
        styling +=  "body { background-color: " + color.name() + "; "
                        "font-size: 5vmin; "
                        "font-family: Arial, sans-serif;"
                        "margin-left: 18px;"
                        "color: white;"
                        "text-align: center;"
                    "}"
                    "hr { height: 2px; border-width: 0; background-color: #CBCBD5; width: 90%; }";
    }


    styling += "h1 { text-align: center; margin: 0.5em; font-size: 2em;}"
               "h3 { text-align: center; font-size: 1.15em;}"
               "input[type='text'] { margin-right: 5px; font-size: 5vmin; width: 9em;}"
               "button { margin-right: 2.5px; font-weight: bold;  font-size: 5vmin;}"
                ".goalsContainer { width: 100%; }"
                "</style>";


    return styling;
}

void EbayMessagesFrame::processXml(QXmlStreamReader &xmlReader, QMap<QString, QString> &myMap) {

    // We are now at the important part, the actual messages
    QString elementSubject;
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {

        QXmlStreamReader::TokenType token = xmlReader.readNext();
        if (token == QXmlStreamReader::StartElement) {

            QStringView elementName = xmlReader.name();
            if (elementName == QString("Subject")) {
                elementSubject = xmlReader.readElementText();
            }
            if (elementName == QString("Read")) {
                myMap.insert(elementSubject, xmlReader.readElementText());
            }
        }
    }

    if (xmlReader.hasError()) {
        qDebug() << "Error: " << xmlReader.errorString();
    }
    return;
}

void EbayMessagesFrame::setConfig(QByteArray &config) {
    this->config = config;
}

void EbayMessagesFrame::setConfig(QJsonObject &config) {
    QJsonDocument jsonDocument(config);
    this->config = jsonDocument.toJson();
}

void EbayMessagesFrame::darkMode() {
    isDarkMode = !isDarkMode;
    repopulate();
}
