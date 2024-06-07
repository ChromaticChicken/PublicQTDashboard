#include "ebayinfoframe.h"

EbayInfoFrame::EbayInfoFrame(const QColor& color, QWidget* parent)
    : QWidget{parent}
{
    this->configJson = nullptr;
    this->ordersJson = nullptr;
    this->color = color;

    layout.addWidget(&webEngine);
    layout.setContentsMargins(0,0,0,0); // This removes the margin between the edge of the frame and the html content
    setLayout(&layout);

    repopulate();
}

void EbayInfoFrame::repopulate() {
    QString html = "<!DOCTYPE html><html><head>"
                   "<meta charset='UTF-8'>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "<title>eBay Messages</title>";

    html += getCSS();

    html += "<body>";

    QPair<qint64, qint64> numOrders = calculateSoldItems();

    html += "<h3>Num orders past 7 days</h3>";
    html += "<h4>" + QString::number(numOrders.second) + "</h4>";
    html += "<h3>Num orders this month</h3>";
    html += "<h4>" + QString::number(numOrders.first) + "</h4>";

    html += "</body></html>";

    webEngine.setHtml(html);

    return;
}

void EbayInfoFrame::setOrdersJson(QJsonObject* ordersJson) {
    this->ordersJson = ordersJson;
}

QPair<qint64, qint64> EbayInfoFrame::calculateSoldItems() {
    if (ordersJson == nullptr) {
        return QPair<qint64, qint64>(0,0);
    }

    if (!ordersJson->value("orders").isArray()) {
        return QPair<qint64, qint64>(0,0);
    }
    QJsonArray ordersArray = ordersJson->value("orders").toArray();
    qint64 weeksOrders = 0;
    qint64 monthsOrders = 0;
    QDate today = QDate::currentDate();
    QDate firstOfMonth = QDate(today.year(), today.month(), 1);
    for (auto&& order : ordersArray) {
        QJsonObject orderObj = order.toObject();
        QString creationDateString = orderObj.value("creationDate").toString();
        QDateTime creationDateTime = QDateTime::fromString(creationDateString, Qt::ISODate);
        QDate creationDate = creationDateTime.date();


        if (creationDate >= today.addDays(-7)) {
            weeksOrders++;
        }
        if (creationDate >= firstOfMonth ) {
            monthsOrders++;
        }
    }
    return QPair<qint64, qint64>(monthsOrders, weeksOrders);
}

QString EbayInfoFrame::getCSS() {
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

void EbayInfoFrame::darkMode() {
    isDarkMode = !isDarkMode;
    repopulate();
}
