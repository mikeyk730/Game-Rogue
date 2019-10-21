import QtQuick 2.2
import QtQuick.Controls 1.1

MenuBar {
    id: defaultMenuBar
    property bool visible: true
    Menu {
        title: qsTr("File")
        visible: defaultMenuBar.visible
        MenuItem {action: saveAction}
        MenuItem {action: quitAction}
    }
    Menu{
        title: qsTr("View")
        visible: defaultMenuBar.visible
        MenuItem {action: fullscreenAction; visible: fullscreenAction.enabled}
        MenuSeparator{}
        MenuItem {action: zoomIn}
        MenuItem {action: zoomOut}
        MenuItem {action: resetZoom}
        MenuSeparator{}
        MenuItem {action: aspectAction}
        MenuSeparator{}
        MenuItem {action: soundAction}
        MenuItem {action: changeGraphicsAction}
    }
    Menu{
        id: profilesMenu
        title: qsTr("Profiles")
        visible: defaultMenuBar.visible
        Instantiator{
            model: appSettings.profilesList
            delegate: MenuItem {
                text: model.text+'\t'
                onTriggered: {
                    appSettings.loadProfileString(obj_string);
                    appSettings.handleFontChanged();
                }
            }
            onObjectAdded: profilesMenu.insertItem(index, object)
            onObjectRemoved: profilesMenu.removeItem(object)
        }
        MenuSeparator{}
        MenuItem {action: showsettingsAction}
    }
    Menu{
        title: qsTr("Help")
        visible: defaultMenuBar.visible
        MenuItem {action: showAboutAction}
    }
}
