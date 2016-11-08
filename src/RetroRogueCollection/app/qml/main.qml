import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.1
import QtGraphicalEffects 1.0

ApplicationWindow{
    id: terminalWindow

    property alias windowScale: appSettings.windowScale

    title: "Retro " + terminalContainer.title
    color: "black"
    minimumWidth: 320
    minimumHeight: 200

    function setDimensions(){
        if (visibility === Window.Windowed){
            contentItem.implicitWidth = terminalContainer.naturalWidth*terminalWindow.windowScale
            contentItem.implicitHeight = terminalContainer.naturalHeight*terminalWindow.windowScale
            width = __width;
            height = __height;
        }
    }

    onVisibilityChanged: setDimensions()
    onWindowScaleChanged: setDimensions()

    // Save window properties automatically
    onXChanged: if (visibility == Window.Windowed && x > 0) appSettings.x = x
    onYChanged: if (visibility == Window.Windowed && y > 0) appSettings.y = y
    onWidthChanged: appSettings.width = width
    onHeightChanged: appSettings.height = height

    // Load saved window geometry and show the window
    Component.onCompleted: {
        setDimensions();
        appSettings.handleFontChanged();

        x = appSettings.x
        y = appSettings.y

        //width = appSettings.width
        //height = appSettings.height

        visible = true
    }

    visible: false

    property bool fullscreen: appSettings.fullscreen
    onFullscreenChanged:{
        mainMenu.visible = !fullscreen;
        // Switching from maximized to fullscreen isn't working properly,
        // so we set to windowed mode first.
        visibility = Window.Windowed;
        if (fullscreen) {
            visibility = Window.FullScreen;
        }
    }

    Action {
        id: fullscreenAction
        text: qsTr("Fullscreen\t")
        shortcut: "Alt+Return"
        onTriggered: appSettings.fullscreen = !appSettings.fullscreen;
        checkable: true
        checked: appSettings.fullscreen
    }
    Action {
        id: aspectAction
        text: qsTr("Maintain Aspect Ratio\t")
        shortcut: "Alt+A"
        onTriggered: terminalContainer.maintainAspect = !terminalContainer.maintainAspect
        checkable: true
        checked: terminalContainer.maintainAspect

    }
    Action {
        id: changeGraphicsAction
        text: qsTr("Change Graphics\t")
        shortcut: "`"
        onTriggered: appSettings.rogue.nextGraphicsMode()
    }
    Action {
        id: saveAction
        text: qsTr("Save Game\t")
        shortcut: "Ctrl+S"
        onTriggered: appSettings.rogue.saveGame()
    }
    Action {
        id: quitAction
        text: qsTr("Quit\t")
        shortcut: "Alt+F4"
        onTriggered: Qt.quit();
    }
    Action{
        id: showsettingsAction
        text: qsTr("Settings\t")
        onTriggered: {
            settingswindow.show();
            settingswindow.requestActivate();
            settingswindow.raise();
        }
    }
    Action{
        id: resetZoom
        text: qsTr("Reset Zoom")
        shortcut: "Ctrl+0"
        onTriggered: {
            terminalWindow.visibility = Window.Windowed;
            terminalWindow.windowScale = 2 ;
        }
    }
    Action{
        id: zoomIn
        text: qsTr("Zoom In")
        shortcut: "Ctrl++"
        onTriggered: {
            terminalWindow.visibility = Window.Windowed;
            terminalWindow.windowScale += 1;
        }
    }
    Action{
        id: zoomOut
        text: qsTr("Zoom Out")
        shortcut: "Ctrl+-"
        onTriggered: {
            terminalWindow.visibility = Window.Windowed;
            if (terminalWindow.windowScale > 1)
                terminalWindow.windowScale -= 1 ;
        }
    }
    //Action{
    //    id: showAboutAction
    //    text: qsTr("About")
    //    onTriggered: {
    //        aboutDialog.show();
    //       aboutDialog.requestActivate();
    //        aboutDialog.raise();
    //    }
    //}

    menuBar: CRTMainMenuBar{
        id: mainMenu
    }
    ApplicationSettings{
        id: appSettings
    }
    TerminalContainer{
        id: terminalContainer

        anchors.centerIn: parent

        function getScale(){
            var scaleX = parent.width/terminalContainer.naturalWidth;
            var scaleY = parent.height/terminalContainer.naturalHeight;
            return Math.min(scaleX, scaleY);
        }

        function getScaleX() {
            var scaleX = parent.width/terminalContainer.naturalWidth;
            return maintainAspect ? getScale() : scaleX;
        }

        function getScaleY() {
            var scaleY = parent.height/terminalContainer.naturalHeight;
            return maintainAspect ? getScale() : scaleY;
        }

        width: getScaleX() * terminalContainer.naturalWidth * appSettings.windowScaling
        height: getScaleY() * terminalContainer.naturalHeight * appSettings.windowScaling

        property alias maintainAspect: appSettings.maintainAspect

        onNaturalWidthChanged: terminalWindow.setDimensions()
        onNaturalHeightChanged: terminalWindow.setDimensions()

        transform: Scale {
            xScale: 1 / appSettings.windowScaling
            yScale: 1 / appSettings.windowScaling
            origin.x: terminalContainer.width/2
            origin.y: terminalContainer.height/2
        }

        Keys.onPressed: {
            if (event.modifiers & Qt.AltModifier)
            {
                if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9){
                    terminalWindow.visibility = Window.Windowed;
                    terminalWindow.windowScale = event.key - Qt.Key_0;
                }
            }
        }
    }
    SettingsWindow{
        id: settingswindow
        visible: false
    }
    //AboutDialog{
    //    id: aboutDialog
    //    visible: false
    //}
}