#include "ebayordersframe.h"

EbayOrdersFrame::EbayOrdersFrame(const QColor& color, QWidget* parent)
    : QWidget{parent}, webEngine{this}, layout{this}
{
    this->ordersJson = nullptr;
    this->color = color;

    layout.addWidget(&webEngine);
    layout.setContentsMargins(0,0,0,0); // This removes the margin between the edge of the frame and the html content
    setLayout(&layout);

    repopulate();
}

void EbayOrdersFrame::repopulate() {
    if (ordersJson == nullptr) {
        return;
    }

    QString html = "<!DOCTYPE html><html><head>"
                   "<meta charset='UTF-8'>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "<title>Area Goals</title>";

    html += getCSS();

    html += "<body>";

    if (!ordersJson->isEmpty()) {
        qint64 numOrders = 0;
        if (!ordersJson->value("orders").isArray()) {
            html += "<p>Failed to open json</p></body></html>";
            webEngine.setHtml(html);
        }
        QJsonArray ordersArray = ordersJson->value("orders").toArray();
        for (auto&& order : ordersArray) {
            if (!order.isObject()) {
                continue;
            }
            QJsonObject orderObj = order.toObject();
            if (!orderObj["lineItems"].isArray()) {
                continue;
            }

            QJsonObject cancelStatusObj = orderObj.value("cancelStatus").toObject();
            if (cancelStatusObj.value("cancelState").toString() == "CANCELED") {
                continue;
            }

            QJsonArray lineItems = orderObj["lineItems"].toArray();
            for (auto&& lineItem : lineItems) {
                QJsonObject lineItemObj = lineItem.toObject();
                QString fulfillmentStatus = lineItemObj.value("lineItemFulfillmentStatus").toString();

                if (fulfillmentStatus != "NOT_STARTED") {
                    continue;
                }



                numOrders++;

                QString itemTitle = lineItemObj.value("title").toString();
                QString shipByDate = "N/A";
                if (lineItemObj.value("lineItemFulfillmentInstructions").isObject()) {
                    QJsonObject fulfillmentInstructions = lineItemObj.value("lineItemFulfillmentInstructions").toObject();
                    shipByDate = fulfillmentInstructions.value("shipByDate").toString();
                    if (shipByDate != "") {
                        QDateTime dateTime = QDateTime::fromString(shipByDate, Qt::ISODate);
                        shipByDate = dateTime.toString();
                    } else {
                        shipByDate = "N/A";
                    }
                }
                html += "<h4>Item: " + itemTitle + "</h4>";
                html += "<h5> Ship by: " + shipByDate + "</h5>";

            }
        }
        html += "<h3>Total Orders: " + QString::number(numOrders) + "<h3>";
    }

    html += "</body></html>";

    webEngine.setHtml(html);
}

void EbayOrdersFrame::setOrdersJson(QJsonObject* ordersJson) {
    this->ordersJson = ordersJson;
}

void EbayOrdersFrame::darkMode() {
    isDarkMode = !isDarkMode;
    repopulate();
}

QString EbayOrdersFrame::getCSS() {
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