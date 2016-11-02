#!/usr/bin/python

import os
import sys
import re
from PyQt4.QtCore import *
from PyQt4.QtGui import *
#import xml.etree.ElementTree as ET
import lxml.etree as ET

class MainWindow(QMainWindow):

#######################################################################################################################
# constructor: create main window
    def __init__(self):

        # initialise current file name
        self.fileName = ""

        # create grid layout    
        super(MainWindow, self).__init__()
        self.grid = QGridLayout()

        # create main tree widget
        self.listWidget = QTreeWidget()
        header = QTreeWidgetItem(["Name","Target Type","Target Device","Target Register", "First Index", "Length", "Channel", "Value"])
        self.listWidget.setHeaderItem(header)
        self.listWidget.currentItemChanged.connect(self.selectListItem)
        self.grid.addWidget(self.listWidget,1,1,1,3)
        
        # enable drag&drop, but not on the top-level
        self.listWidget.setDragDropMode(QAbstractItemView.InternalMove)
        self.listWidget.invisibleRootItem().setFlags(Qt.ItemIsEnabled)

        # set column sizes of tree
        self.listWidget.setColumnWidth(0, 350)
        self.listWidget.setColumnWidth(1, 180)
        self.listWidget.setColumnWidth(2, 110)
        self.listWidget.setColumnWidth(3, 250)
        self.listWidget.setColumnWidth(4, 80)
        self.listWidget.setColumnWidth(5, 70)
        self.listWidget.setColumnWidth(6, 70)
        self.listWidget.setColumnWidth(7, 70)
        
        # buttons for adding and deleting items
        self.buttonAddReg = QPushButton()
        self.buttonAddReg.setText('Add Register')
        self.grid.addWidget(self.buttonAddReg,2,1,1,1)
        self.buttonAddReg.clicked.connect(self.addRegister)
        self.buttonAddMod = QPushButton()
        self.buttonAddMod.setText('Add Module')
        self.grid.addWidget(self.buttonAddMod,2,2,1,1)
        self.buttonAddMod.clicked.connect(self.addModule)
        self.buttonDel = QPushButton()
        self.buttonDel.setText('Delete Item')
        self.grid.addWidget(self.buttonDel,2,3,1,1)
        self.buttonDel.clicked.connect(self.deleteItem)

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
        self.resize(1200, 800)

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
# add new register to the tree
    def addRegister(self):
            
        # create item
        item = QTreeWidgetItem(self.listWidget.currentItem(), ["newRegister"])
            
        # make the target type column a combo box
        self.listWidget.setItemWidget(item, 1, self.createTargetTypeComboBox("redirectedRegister") )
            
        # make item editable
        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsDragEnabled )
        
        # select the new entry in list
        self.listWidget.setCurrentItem(item)

#######################################################################################################################
# add new module to the tree
    def addModule(self):
            
        # create item
        item = QTreeWidgetItem(self.listWidget.currentItem(), ["newModule"])
            
        # make item editable
        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled )
        
        # select the new entry in list
        self.listWidget.setCurrentItem(item)

#######################################################################################################################
# delete item from the tree
    def deleteItem(self):
    
        # if module, ask first
        if self.listWidget.itemWidget(self.listWidget.currentItem(), 1) is None:
          msg = "Are you sure you want to delete the selected module with all registers and sub-modules inside?"
          reply = QMessageBox.question(self, 'Delete Item', msg, QMessageBox.Yes, QMessageBox.No)
          if reply != QMessageBox.Yes:
            return

        self.listWidget.currentItem().parent().removeChild(self.listWidget.currentItem())

#######################################################################################################################
# create a combo box to select the target type
    def createTargetTypeComboBox(self, selectedItem):
        box = QComboBox()
        box.setEditable(False)
        box.addItem("redirected register", "redirectedRegister")
        box.addItem("channel of 2D register","redirectedChannel")
        box.addItem("constant","constant")
        box.addItem("variable","variable")
        box.setCurrentIndex( box.findData(selectedItem) )
        return box


#######################################################################################################################
# select a different list item
    def selectListItem(self):
        if self.listWidget.itemWidget(self.listWidget.currentItem(), 1) is None:
          self.buttonAddReg.setEnabled(True)
          self.buttonAddMod.setEnabled(True)
        else:
          self.buttonAddReg.setEnabled(False)
          self.buttonAddMod.setEnabled(False)
          
        if self.listWidget.currentItem() == self.listWidget.topLevelItem(0):
          self.buttonDel.setEnabled(False)
        else:
          self.buttonDel.setEnabled(True)

#######################################################################################################################
# recusively populate the list widget with entries from the XML tree
    def updateList(self, theData):
        # clear the list first and create the top-level element
        self.listWidget.clear()
        toplevelItem = QTreeWidgetItem(self.listWidget, ["logicalNameMap"] )
        toplevelItem.setExpanded(True)
        # start with the root element, interate through the entire tree
        self.updateListInternal(toplevelItem, theData)

#######################################################################################################################
# internally called by updateList()
    def updateListInternal(self, parentItem, entry):
        for child in entry:

            # create entry for module
            if(child.tag == "module"):
              data = [ child.get("name"), "", "", "", "", "", "", "" ]
              item = QTreeWidgetItem(parentItem, data)
              item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled )
              item.setExpanded(True)

               # recures into module
              self.updateListInternal(item, child)

            # create entry for register
            else:

              # collect item data        
              data = [ child.get("name"), child.tag, "", "", "", "", "", "" ]
              if child.find("targetDevice") is not None:
                data[2] = child.find("targetDevice").text
              if child.find("targetRegister") is not None:
                data[3] = child.find("targetRegister").text
              if child.find("targetStartIndex") is not None:
                data[4] = child.find("targetStartIndex").text
              if child.find("numberOfElements") is not None:
                data[5] = child.find("numberOfElements").text
              if child.find("targetChannel") is not None:
                data[6] = child.find("targetChannel").text
              if child.find("value") is not None:
                data[7] = child.find("value").text
            
              # create item
              item = QTreeWidgetItem(parentItem, data)
            
              # make the target type column a combo box
              self.listWidget.setItemWidget(item, 1, self.createTargetTypeComboBox(child.tag) )
            
              # make item editable
              item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsDragEnabled )

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
        theData = ET.parse(fileName).getroot()
        
        # populate list (recusively through all modules)
        self.updateList(theData)
            
        # store file name and change window title
        self.fileName = os.path.abspath(fileName)
        self.setWindowTitle(os.path.basename(self.fileName)+" ("+os.path.dirname(self.fileName)+") - mtca4u Logical Name Mapping editor")
        self._fileSaveAgainAction.setEnabled(True)

#######################################################################################################################
# show open lmap file dialogue
    def createXmlData(self):
             
        # create root element
        theData = ET.Element("logicalNameMap")
        
        # recursively iterate through the tree
        self.createXmlDataInternal(theData, self.listWidget.topLevelItem(0))
        
        return theData

#######################################################################################################################
# internally called by createXmlData
    def createXmlDataInternal(self, xmlElement, treeElement):
             
        for index in range(0,treeElement.childCount()):
          child = treeElement.child(index)
          
          # registers
          if(child.childCount() == 0):
            wg = self.listWidget.itemWidget(child, 1)
            targetType = str(wg.itemData(wg.currentIndex()).toString())
            if targetType == "redirectedRegister":
              xmlChild = ET.SubElement(xmlElement, targetType, name=str(child.text(0)))
              ET.SubElement(xmlChild, "targetDevice").text = str(child.text(2))
              ET.SubElement(xmlChild, "targetRegister").text = str(child.text(3))
              if str(child.text(4)) != "":
                ET.SubElement(xmlChild, "targetStartIndex").text = str(child.text(4))
              if str(child.text(5)) != "":
                ET.SubElement(xmlChild, "numberOfElements").text = str(child.text(5))
            elif targetType == "redirectedChannel":
              xmlChild = ET.SubElement(xmlElement, targetType, name=str(child.text(0)))
              ET.SubElement(xmlChild, "targetDevice").text = str(child.text(2))
              ET.SubElement(xmlChild, "targetRegister").text = str(child.text(3))
              ET.SubElement(xmlChild, "targetChannel").text = str(child.text(6))
            elif targetType == "constant" or targetType == "variable":
              xmlChild = ET.SubElement(xmlElement, targetType, name=str(child.text(0)))
              ET.SubElement(xmlChild, "value").text = str(child.text(7))
              ET.SubElement(xmlChild, "type").text = "integer"
            else:
              QMessageBox.information(self, 'Error', 'Unknown target type: "'+targetType+'".')

          # modules
          else:
              xmlChild = ET.SubElement(xmlElement, "module", name=str(child.text(0)))
              self.createXmlDataInternal(xmlChild, child)
        
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
        
        # iterate through tree data
        theData = self.createXmlData()
        
        # format XML properly (with indentation, but without adding spaces and new lines to the text nodes)
        tree = ET.ElementTree()
        tree._setroot(theData)
        uglyXml = ET.tostring(tree, pretty_print = True)
        text_re = re.compile('>\n\s+([^<>\s].*?)\n\s+</', re.DOTALL)    
        prettyXml = text_re.sub('>\g<1></', uglyXml)
        
        # safe to file
        with open(fileName, "w") as text_file:
          text_file.write(prettyXml)

        # show message of success
        QMessageBox.information(self, 'Map saved', 'Logical name mapping has been saved to file "'+fileName+'".')
        
        # store file name and change window title
        self.fileName = os.path.abspath(fileName)
        self.setWindowTitle(os.path.basename(self.fileName)+" ("+os.path.dirname(self.fileName)+") - mtca4u Logical Name Mapping editor")
        self._fileSaveAgainAction.setEnabled(True)

#########################################################################################
# save lmap file with same name
    def saveFileAgain(self):
        self.saveFile(self.fileName)

#######################################################################################################################
# close the editor
    def exit(self):
        msg = "Are you sure you want to close the editor?"
        reply = QMessageBox.question(self, 'mtca4u Logical Name Mapping editor', msg, QMessageBox.Yes, QMessageBox.No)
        if reply != QMessageBox.Yes:
          return
        self.close()

#######################################################################################################################
# close the editor
    def closeEvent(self, event):
        msg = "Are you sure you want to close the editor?"
        reply = QMessageBox.question(self, 'mtca4u Logical Name Mapping editor', msg, QMessageBox.Yes, QMessageBox.No)
        if reply != QMessageBox.Yes:
          event.ignore()
        else:
          event.accept()

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
