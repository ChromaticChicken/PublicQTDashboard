# GOALS DASHBOARD

## How to create your own build

1. Install QT6 at [https://www.qt.io](https://www.qt.io)
    * You will need to install the QWebEngineWidgets as well
    * I suggest getting QT Creator as well to make your life easier
    
2. Download MVSC
3. Use QT Creator to build your project
4. You will need to add two config files
    1. config.json this will contain your goals
    2. ebay.config.json this will contain your ebay API configurations
    * Examples of what should be in those files are given
5. You will need to install Python (possibly python 3.12.3 exactly along with selenium, autohttp, and filelock)
6. Use QT's tool windeployqt to add all of the necessary DLL files to the directory (after creating the .exe)


Hopefully nothing will have to be changed for it to work. 
GOOD LUCK :)
