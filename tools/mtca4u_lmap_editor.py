#!/usr/bin/python

import os
import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import xml.etree.ElementTree as ET

class MainWindow(QMainWindow):

#######################################################################################################################
# constructor: create main window
    def __init__(self):
    
        super(MainWindow, self).__init__()
        self.grid = QGridLayout()
        
        # create empty XML tree
        self.theData = ET.Element("logicalNameMap")

        # create main list
        self.listWidget = QListWidget()
        self.listWidget.currentItemChanged.connect(self.selectListItem)
        self.grid.addWidget(self.listWidget,1,1,1,5)
        
        #
        # editing fields
        #
        
        # name
        self.grid.addWidget(QLabel('Name:'),2,1)
        self.edName = QLineEdit()
        self.grid.addWidget(self.edName,2,2,1,4)
        self.edName.textChanged.connect(self.changeName)
        
        # target type
        self.grid.addWidget(QLabel('Target type:'),3,1)
        self.edType = QComboBox()
        self.edType.setEditable(False)
        self.edType.addItem("- select -", "")
        self.edType.addItem("full register", "register")
        self.edType.addItem("range of 1D register", "range")
        self.edType.addItem("channel of 2D register","channel")
        self.edType.addItem("integer constant","int_constant")
        self.grid.addWidget(self.edType,3,2)
        self.edType.currentIndexChanged.connect(self.changeTargetType)
        
        # target device
        self.grid.addWidget(QLabel('Target device alias:'),4,1)
        self.edDevice = QLineEdit()
        self.edDevice.setEnabled(False)
        self.grid.addWidget(self.edDevice,4,2,1,4)
        
        # target register
        self.grid.addWidget(QLabel('Target register name:'),5,1)
        self.edRegister = QLineEdit()
        self.edRegister.setEnabled(False)
        self.grid.addWidget(self.edRegister,5,2,1,4)
        
        # first index
        self.grid.addWidget(QLabel('First index:'),6,1)
        self.edFirstIndex = QSpinBox()
        self.edFirstIndex.setEnabled(False)
        self.grid.addWidget(self.edFirstIndex,6,2,1,1)
        
        # length (number of indices)
        self.grid.addWidget(QLabel('Length (nb. of indices):'),7,1)
        self.edLength = QSpinBox()
        self.edLength.setEnabled(False)
        self.grid.addWidget(self.edLength,7,2,1,1)
        
        # channel
        self.grid.addWidget(QLabel('Channel:'),8,1)
        self.edChannel = QSpinBox()
        self.edChannel.setEnabled(False)
        self.grid.addWidget(self.edChannel,8,2,1,1)
        
        # value
        self.grid.addWidget(QLabel('Value:'),9,1)
        self.edValue = QSpinBox()
        self.edValue.setEnabled(False)
        self.grid.addWidget(self.edValue,9,2,1,1)
        
        # buttons
        self.edAdd = QPushButton('Save as new entry')
        self.edAdd.setEnabled(False)
        self.grid.addWidget(self.edAdd,10,1)
        self.edAdd.clicked.connect(self.saveNewEntry)

        self.edUpdate = QPushButton('Update entry')
        self.edUpdate.setEnabled(False)
        self.grid.addWidget(self.edUpdate,10,2)
        self.edUpdate.clicked.connect(self.updateEntry)

        self.edDelete = QPushButton('Delete entry')
        self.edDelete.setEnabled(False)
        self.grid.addWidget(self.edDelete,10,3)
        self.edDelete.clicked.connect(self.deleteEntry)

        # add the menu
        self.makeMenu()

        # add status bar
        self.statusBar = QStatusBar()
        self.setStatusBar(self.statusBar)
        self.statusBar.showMessage('Ready.')

        # add grid layout
        self.widget = QWidget();
        self.widget.setLayout(self.grid);
        self.setCentralWidget(self.widget);

        # set window title and size
        self.setWindowTitle("mtca4u Logical Name Mapping editor")
        self.resize(800, 800)

#######################################################################################################################
# create the menu
    def makeMenu(self):

        menuBar = self.menuBar()
        fileMenu = menuBar.addMenu("&File")

        self._fileOpenAction = QAction("&Open...", None)
        self.connect(self._fileOpenAction, SIGNAL('triggered()'), self.openFileDialog)
        fileMenu.addAction(self._fileOpenAction)

        self._fileSaveAction = QAction("Save &As...", None)
        self.connect(self._fileSaveAction, SIGNAL('triggered()'), self.saveFileDialog)
        fileMenu.addAction(self._fileSaveAction)

        self._fileSaveAgainAction = QAction("&Save", None)
        self._fileSaveAgainAction.setEnabled(False)
        self.connect(self._fileSaveAgainAction, SIGNAL('triggered()'), self.saveFileAgain)
        fileMenu.addAction(self._fileSaveAgainAction)

        self._exitAction = QAction("&Quit", None)
        self.connect(self._exitAction, SIGNAL('triggered()'), self.exit)
        fileMenu.addAction(self._exitAction)

#######################################################################################################################
# save new entry to the list
    def saveNewEntry(self):
        
        # create new entry element (with name attribute)
        entry = ET.SubElement(self.theData,"entry")
        entry.set("name", str(self.edName.text()) )
        
        # fill element with data
        self.fillElement(entry)
        
        # add list entry
        item = QListWidgetItem(self.edName.text())
        self.listWidget.addItem(item)
        
        # select the new entry in list
        self.listWidget.setCurrentItem(item)

#######################################################################################################################
# update existing entry in the list
    def updateEntry(self):
        
        # get name of selected item
        name = str(self.listWidget.currentItem().text())
        
        # find element
        entry = self.theData.find("entry[@name='"+name+"']")
        
        # update name
        entry.set("name", str(self.edName.text()) )
        self.listWidget.currentItem().setText(self.edName.text())
        
        # strip content
        for elem in entry: 
            entry.remove(elem)
        
        # fill with new data
        self.fillElement(entry)

#######################################################################################################################
# delete existing entry from the list
    def deleteEntry(self):
        
        # get name of selected item
        name = str(self.listWidget.currentItem().text())
        
        # find element
        entry = self.theData.find("entry[@name='"+name+"']")
        
        # remove from root
        self.theData.remove(entry)
        
        # remove from list
        self.listWidget.takeItem(self.listWidget.currentRow())

#######################################################################################################################
# fill XML element with data from editor
    def fillElement(self, entry):
        
        # add sub-element for the type
        elem = ET.SubElement(entry, "type")
        type = str( self.edType.itemData( self.edType.currentIndex() ).toString() )
        elem.text = type
        
        # add type-dependent sub-elements
        if type == "register":
            elem = ET.SubElement(entry, "device")
            elem.text = str( self.edDevice.text() )
            elem = ET.SubElement(entry, "register")
            elem.text = str( self.edRegister.text() )
        elif type == "range":
            elem = ET.SubElement(entry, "device")
            elem.text = str( self.edDevice.text() )
            elem = ET.SubElement(entry, "register")
            elem.text = str( self.edRegister.text() )
            elem = ET.SubElement(entry, "index")
            elem.text = str( self.edFirstIndex.value() )
            elem = ET.SubElement(entry, "length")
            elem.text = str( self.edLength.value() )
        elif type == "channel":
            elem = ET.SubElement(entry, "device")
            elem.text = str( self.edDevice.text() )
            elem = ET.SubElement(entry, "register")
            elem.text = str( self.edRegister.text() )
            elem = ET.SubElement(entry, "channel")
            elem.text = str( self.edChannel.value() )
        elif type == "int_constant":
            elem = ET.SubElement(entry, "value")
            elem.text = str( self.edValue.value() )

#######################################################################################################################
# select a different list item
    def selectListItem(self):
        name = str(self.listWidget.currentItem().text())
        
        # find xml element "entry" with the given name
        elem = self.theData.find("entry[@name='"+name+"']")
        
        # set editor fields
        self.edName.setText(name)
        type = elem.find("type").text
        self.edType.setCurrentIndex( self.edType.findData(type) )
        if type == "register":
            self.edDevice.setText(elem.find("device").text)
            self.edRegister.setText(elem.find("register").text)
        elif type == "range":
            self.edDevice.setText(elem.find("device").text)
            self.edRegister.setText(elem.find("register").text)
            self.edFirstIndex.setValue(int(elem.find("index").text))
            self.edLength.setValue(int(elem.find("length").text))
        elif type == "channel":
            self.edDevice.setText(elem.find("device").text)
            self.edRegister.setText(elem.find("register").text)
            self.edChannel.setValue(int(elem.find("channel").text))
        elif type == "int_constant":
            self.edValue.setValue(int(elem.find("value").text))

        # enable buttons
        self.edAdd.setEnabled(False)
        self.edUpdate.setEnabled(True)
        self.edDelete.setEnabled(True)

#######################################################################################################################
# update existing entry in the list
    def changeName(self):
        name = str(self.edName.text())
        if name == "":
            self.edAdd.setEnabled(False)
            self.edUpdate.setEnabled(False)
            self.edDelete.setEnabled(False)
        elif self.theData.find("entry[@name='"+name+"']") is not None:
            self.edAdd.setEnabled(False)
            self.edUpdate.setEnabled(True)
            self.edDelete.setEnabled(True)
        elif self.listWidget.currentItem():
            self.edAdd.setEnabled(True)
            self.edUpdate.setEnabled(True)
            self.edDelete.setEnabled(True)
        else:
            self.edAdd.setEnabled(True)
            self.edUpdate.setEnabled(False)
            self.edDelete.setEnabled(False)


#######################################################################################################################
# change target type: update disabled and enabled fields
    def changeTargetType(self):
        type = self.edType.itemData( self.edType.currentIndex() )
        if type == "register":
            self.edDevice.setEnabled(True)
            self.edRegister.setEnabled(True)
            self.edFirstIndex.setEnabled(False)
            self.edLength.setEnabled(False)
            self.edChannel.setEnabled(False)
            self.edValue.setEnabled(False)
        elif type == "range":
            self.edDevice.setEnabled(True)
            self.edRegister.setEnabled(True)
            self.edFirstIndex.setEnabled(True)
            self.edLength.setEnabled(True)
            self.edChannel.setEnabled(False)
            self.edValue.setEnabled(False)
        elif type == "channel":
            self.edDevice.setEnabled(True)
            self.edRegister.setEnabled(True)
            self.edFirstIndex.setEnabled(False)
            self.edLength.setEnabled(False)
            self.edChannel.setEnabled(True)
            self.edValue.setEnabled(False)
        elif type == "int_constant":
            self.edDevice.setEnabled(False)
            self.edRegister.setEnabled(False)
            self.edFirstIndex.setEnabled(False)
            self.edLength.setEnabled(False)
            self.edChannel.setEnabled(False)
            self.edValue.setEnabled(True)
        else:
            self.edDevice.setEnabled(False)
            self.edRegister.setEnabled(False)
            self.edFirstIndex.setEnabled(False)
            self.edLength.setEnabled(False)
            self.edChannel.setEnabled(False)
            self.edValue.setEnabled(False)
             

#######################################################################################################################
# show open lmap file dialogue
    def openFileDialog(self):
        
        # create open device dialog
        dlg = QFileDialog(self)
        dlg.setAcceptMode(QFileDialog.AcceptOpen)
        dlg.setWindowTitle('Open XML logical map file')
        dlg.setViewMode( QFileDialog.Detail )
        dlg.setNameFilters( [self.tr('XML Logical Map Files (*.xlmap)'), self.tr('All Files (*)')] )
        dlg.setDefaultSuffix('xlmap')
        if self.fileName != "":
            dlg.setDirectory( os.path.dirname(self.fileName) )
        
        # show dialog, open only if user did not cancel
        if dlg.exec_() :
            # file name must be converted into standard python string
            name = str(dlg.selectedFiles()[0])
            # open the file
            self.openFile(name)

#######################################################################################################################
# open lmap file
    def openFile(self, fileName):
        # parse xml file
        tree = ET.parse(fileName)
        self.theData = tree.getroot()
        # populate list
        self.listWidget.clear()
        for entry in self.theData.findall("entry"):
            self.listWidget.addItem(QListWidgetItem(entry.get("name")))
            
        # store file name and change window title
        self.fileName = os.path.abspath(fileName)
        self.setWindowTitle(os.path.basename(self.fileName)+" ("+os.path.dirname(self.fileName)+") - mtca4u Logical Name Mapping editor")
        self._fileSaveAgainAction.setEnabled(True)
        
#######################################################################################################################
# show save lmap file dialogue
    def saveFileDialog(self):
        # create file-save dialog
        dlg = QFileDialog(self)
        dlg.setAcceptMode(QFileDialog.AcceptSave)
        dlg.setWindowTitle('Save XML logical map file')
        dlg.setViewMode( QFileDialog.Detail )
        dlg.setNameFilters( [self.tr('XML Logical Map Files (*.xlmap)'), self.tr('All Files (*)')] )
        dlg.setDefaultSuffix('xlmap')
        if self.fileName != "":
            dlg.setDirectory( os.path.dirname(self.fileName) )
        
        # show dialog, save only if user did not cancel
        if dlg.exec_() :
            # file name must be converted into standard python string
            name = str(dlg.selectedFiles()[0])
            # save the file
            self.saveFile(name)

#########################################################################################
# save lmap file
    def saveFile(self, fileName):
        # create element tree
        tree = ET.ElementTree()
        tree._setroot(self.theData)
        tree.write(fileName)
        # show message of success
        QMessageBox.information(self, 'Map saved', 'Logical name mapping has been saved to file "'+name+'".')
        
        # store file name and change window title
        self.fileName = os.path.abspath(fileName)
        self.setWindowTitle(os.path.basename(self.fileName)+" ("+os.path.dirname(self.fileName)+") - mtca4u Logical Name Mapping editor")
        self._fileSaveAgainAction.setEnabled(True)

#########################################################################################
# save lmap file with same name
    def saveFileAgain(self):
        self.saveFile(self.fileName)

#######################################################################################################################
# open lmap file
    def exit(self):
        self.close()

#######################################################################################################################
# main function: initialise app
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    if len(sys.argv) > 1 and sys.argv[1] != "":
        try:
            window.openFile(sys.argv[1])
        except IOError:
            QMessageBox.warning(window, 'Cannot open file', 'Cannot open file: '+sys.argv[1]+'.')
    window.show()
    sys.exit(app.exec_())
