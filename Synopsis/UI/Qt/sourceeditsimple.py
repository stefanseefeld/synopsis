# Form implementation generated from reading ui file 'sourceeditsimple.ui'
#
# Created: Tue Nov 12 01:20:05 2002
#      by: The PyQt User Interface Compiler (pyuic)
#
# WARNING! All changes made in this file will be lost!


from qt import *


class SourceEditSimple(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if name == None:
            self.setName("SourceEditSimple")

        self.resize(600,480)
        self.setCaption(self.trUtf8("Edit Simple Rule"))

        SourceEditSimpleLayout = QVBoxLayout(self,11,6,"SourceEditSimpleLayout")

        self.TextLabel7 = QLabel(self,"TextLabel7")
        self.TextLabel7.setText(self.trUtf8("Select the files you want to include in this rule:"))
        SourceEditSimpleLayout.addWidget(self.TextLabel7)

        self.GroupBox3 = QGroupBox(self,"GroupBox3")
        self.GroupBox3.setTitle(self.trUtf8("Files"))
        self.GroupBox3.setColumnLayout(0,Qt.Vertical)
        self.GroupBox3.layout().setSpacing(6)
        self.GroupBox3.layout().setMargin(11)
        GroupBox3Layout = QVBoxLayout(self.GroupBox3.layout())
        GroupBox3Layout.setAlignment(Qt.AlignTop)

        self.FileList = QListView(self.GroupBox3,"FileList")
        self.FileList.addColumn(self.trUtf8("Filename"))
        self.FileList.setSelectionMode(QListView.Extended)
        GroupBox3Layout.addWidget(self.FileList)

        Layout12 = QHBoxLayout(None,0,6,"Layout12")

        self.AddFilesButton = QPushButton(self.GroupBox3,"AddFilesButton")
        self.AddFilesButton.setText(self.trUtf8("Add files"))
        Layout12.addWidget(self.AddFilesButton)
        spacer = QSpacerItem(140,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout12.addItem(spacer)

        self.MakeRelativeButton = QPushButton(self.GroupBox3,"MakeRelativeButton")
        self.MakeRelativeButton.setEnabled(0)
        self.MakeRelativeButton.setText(self.trUtf8("Make relative ... "))
        QToolTip.add(self.MakeRelativeButton,self.trUtf8("Makes the selected files relative to a given directory, if they have absolute pathnames"))
        Layout12.addWidget(self.MakeRelativeButton)

        self.RemoveFileButton = QPushButton(self.GroupBox3,"RemoveFileButton")
        self.RemoveFileButton.setEnabled(0)
        self.RemoveFileButton.setText(self.trUtf8("Remove from list"))
        Layout12.addWidget(self.RemoveFileButton)
        GroupBox3Layout.addLayout(Layout12)
        SourceEditSimpleLayout.addWidget(self.GroupBox3)

        self.Frame3_2 = QFrame(self,"Frame3_2")
        self.Frame3_2.setFrameShape(QFrame.HLine)
        self.Frame3_2.setFrameShadow(QFrame.Sunken)
        SourceEditSimpleLayout.addWidget(self.Frame3_2)

        Layout5 = QHBoxLayout(None,0,6,"Layout5")

        self.CancelButton = QPushButton(self,"CancelButton")
        self.CancelButton.setText(self.trUtf8("Cancel"))
        Layout5.addWidget(self.CancelButton)
        spacer_2 = QSpacerItem(400,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout5.addItem(spacer_2)

        self.OkButton = QPushButton(self,"OkButton")
        self.OkButton.setText(self.trUtf8("Ok"))
        Layout5.addWidget(self.OkButton)
        SourceEditSimpleLayout.addLayout(Layout5)

        self.connect(self.AddFilesButton,SIGNAL("clicked()"),self.AddFilesButton_clicked)
        self.connect(self.MakeRelativeButton,SIGNAL("clicked()"),self.MakeRelativeButton_clicked)
        self.connect(self.RemoveFileButton,SIGNAL("clicked()"),self.RemoveFileButton_clicked)
        self.connect(self.FileList,SIGNAL("selectionChanged()"),self.FileList_selectionChanged)
        self.connect(self.OkButton,SIGNAL("clicked()"),self.OkButton_clicked)
        self.connect(self.CancelButton,SIGNAL("clicked()"),self.CancelButton_clicked)

    def AddFilesButton_clicked(self):

        names = QFileDialog.getOpenFileNames(
        	"Files (*.h *.hh *.hpp *.c *.cc *.cpp *.py *.idl)",
        	None,
        	self,
        	"select files dialog",
        	"Select the files to include in the rule:")
        if names:
        	# Find existing names
        	existing = []
        	item = self.FileList.firstChild()
        	while item:
        		existing.append(str(item.text(0)))
        		item = item.nextSibling()
        	print existing
        	for name in names:
        		if str(name) not in existing:
        			QListViewItem(self.FileList, name)
        

    def RemoveFileButton_clicked(self):

        item = self.FileList.firstChild()
        while item:
        	next = item.nextSibling()
        	if self.FileList.isSelected(item):
        		self.FileList.takeItem(item)
        	item = next
        

    def FileList_selectionChanged(self):

        item = self.FileList.firstChild()
        while item:
        	if self.FileList.isSelected(item):
        		self.RemoveFileButton.setEnabled(1)
        		self.MakeRelativeButton.setEnabled(1)
        		return
        	item = item.nextSibling()
        self.RemoveFileButton.setEnabled(0)
        self.MakeRelativeButton.setEnabled(0)
        

    def MakeRelativeButton_clicked(self):

        import string, os
        base = QFileDialog.getExistingDirectory(None, self, "getdir", "Select relative directory", 1)
        if base:
        	base = str(base)
        	if base and base[-1] != '/': base = base + '/'
        	files = []
         	item = self.FileList.firstChild()
        	while item:
        		if self.FileList.isSelected(item):
        		    file = str(item.text(0))
        		    file = self.make_relative(base, file)
        		    item.setText(0, file)
        		item = item.nextSibling()
        

    def OkButton_clicked(self):

        self.accept()
        

    def CancelButton_clicked(self):

        self.reject()
        
