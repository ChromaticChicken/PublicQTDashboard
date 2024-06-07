#include "fullframe.h"

FullFrame::FullFrame(QJsonObject* configJson, QWidget *parent)
    : QWidget{parent}, layout{new QGridLayout(this)}
{
    // Set the margins on the outside of the grid of areas to be 0 (I do this so the layout can then define margins)
    setContentsMargins(0, 0, 0, 0);

    // Make a QGridLayout and set the FullFrames layout to be that layout
    setLayout(layout);
    // Make the margins on the outside be 10 px all around except the top be only 3 px
    layout->setContentsMargins(10, 3, 10, 10);
    // Make the spacing between items to be 10 px
    layout->setSpacing(10);

    // Color for the frames (gray slightly blue ish)
    QColor areaFrameColor(203, 203, 213);

    // Iterate over the QJsonArray within the QJsonObject
    int row = 0;
    int col = 0;
    QStringList keys = configJson->keys();
    for (const QString &key : keys) { // Iterate over the keys in the jsonObject aka the area names

        // Create a new areaFrame and put it in the dictionary
        areaFramesMap[key] = new AreaFrame(areaFrameColor, key, configJson, this);

        // add the areaFrame to the layout
        layout->addWidget(areaFramesMap[key], row, col);
        if (col == 2){ // If the column is 2 aka this is the 3rd box from left to right
            // Set the column back to 0 and increase the row count by 1
            col = 0;
            row++;
        }
        else {
            // Increase the column count
            col++;
        }
    }

}

void FullFrame::repopulateAll(){
    // Iterate over each of the areas and repopulate that area
    for (auto it = areaFramesMap.begin(); it != areaFramesMap.end(); it++) {
        it.value()->repopulate();
    }
}

void FullFrame::darkMode(){
    // Iterate over each of the areas and turn dark mode on/off
    for (auto it = areaFramesMap.begin(); it != areaFramesMap.end(); it++) {
        it.value()->darkMode();
    }
}
