# Form implementation generated from reading ui file 'sourceeditglob.ui'
#
# Created: Tue Nov 12 01:22:01 2002
#      by: The PyQt User Interface Compiler (pyuic)
#
# WARNING! All changes made in this file will be lost!


from qt import *


class SourceEditGlob(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if name == None:
            self.setName("SourceEditGlob")

        self.resize(600,480)
        self.setCaption(self.trUtf8("Edit Glob Rule"))

        SourceEditGlobLayout = QVBoxLayout(self,11,6,"SourceEditGlobLayout")

        self.TextLabel7_2 = QLabel(self,"TextLabel7_2")
        self.TextLabel7_2.setText(self.trUtf8("Select the directories you want to include in this rule:"))
        SourceEditGlobLayout.addWidget(self.TextLabel7_2)

        self.GroupBox3_2 = QGroupBox(self,"GroupBox3_2")
        self.GroupBox3_2.setTitle(self.trUtf8("Directories"))
        self.GroupBox3_2.setColumnLayout(0,Qt.Vertical)
        self.GroupBox3_2.layout().setSpacing(6)
        self.GroupBox3_2.layout().setMargin(11)
        GroupBox3_2Layout = QVBoxLayout(self.GroupBox3_2.layout())
        GroupBox3_2Layout.setAlignment(Qt.AlignTop)

        self.DirList = QListView(self.GroupBox3_2,"DirList")
        self.DirList.addColumn(self.trUtf8("Path"))
        self.DirList.setSelectionMode(QListView.Extended)
        GroupBox3_2Layout.addWidget(self.DirList)

        Layout16 = QHBoxLayout(None,0,6,"Layout16")

        self.AddDirectoryButton = QPushButton(self.GroupBox3_2,"AddDirectoryButton")
        self.AddDirectoryButton.setText(self.trUtf8("Add directory"))
        Layout16.addWidget(self.AddDirectoryButton)
        spacer = QSpacerItem(130,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout16.addItem(spacer)

        self.MakeRelativeButton = QPushButton(self.GroupBox3_2,"MakeRelativeButton")
        self.MakeRelativeButton.setEnabled(0)
        self.MakeRelativeButton.setText(self.trUtf8("Make relative ... "))
        QToolTip.add(self.MakeRelativeButton,self.trUtf8("Makes the selected directories relative to a given directory, if they are absolute"))
        Layout16.addWidget(self.MakeRelativeButton)

        self.RemoveDirectoryButton = QPushButton(self.GroupBox3_2,"RemoveDirectoryButton")
        self.RemoveDirectoryButton.setEnabled(0)
        self.RemoveDirectoryButton.setText(self.trUtf8("Remove directory"))
        Layout16.addWidget(self.RemoveDirectoryButton)
        GroupBox3_2Layout.addLayout(Layout16)
        SourceEditGlobLayout.addWidget(self.GroupBox3_2)

        self.TextLabel12 = QLabel(self,"TextLabel12")
        self.TextLabel12.setText(self.trUtf8("Glob Expression:"))
        SourceEditGlobLayout.addWidget(self.TextLabel12)

        self.FileGlobEdit = QLineEdit(self,"FileGlobEdit")
        self.FileGlobEdit.setText(self.trUtf8("*"))
        SourceEditGlobLayout.addWidget(self.FileGlobEdit)

        self.RecurseCheckBox = QCheckBox(self,"RecurseCheckBox")
        self.RecurseCheckBox.setText(self.trUtf8("Recursively search subdirectories for matching files"))
        SourceEditGlobLayout.addWidget(self.RecurseCheckBox)

        self.Frame3_2 = QFrame(self,"Frame3_2")
        self.Frame3_2.setFrameShape(QFrame.HLine)
        self.Frame3_2.setFrameShadow(QFrame.Sunken)
        SourceEditGlobLayout.addWidget(self.Frame3_2)

        Layout5 = QHBoxLayout(None,0,6,"Layout5")

        self.CancelButton = QPushButton(self,"CancelButton")
        self.CancelButton.setText(self.trUtf8("Cancel"))
        Layout5.addWidget(self.CancelButton)
        spacer_2 = QSpacerItem(400,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout5.addItem(spacer_2)

        self.OkButton = QPushButton(self,"OkButton")
        self.OkButton.setText(self.trUtf8("Ok"))
        Layout5.addWidget(self.OkButton)
        SourceEditGlobLayout.addLayout(Layout5)

        self.connect(self.OkButton,SIGNAL("clicked()"),self.OkButton_clicked)
        self.connect(self.CancelButton,SIGNAL("clicked()"),self.CancelButton_clicked)
        self.connect(self.AddDirectoryButton,SIGNAL("clicked()"),self.AddDirectoryButton_clicked)
        self.connect(self.MakeRelativeButton,SIGNAL("clicked()"),self.MakeRelativeButton_clicked)
        self.connect(self.RemoveDirectoryButton,SIGNAL("clicked()"),self.RemoveDirectoryButton_clicked)
        self.connect(self.DirList,SIGNAL("selectionChanged()"),self.DirList_selectionChanged)

    def AddDirectoryButton_clicked(self):

        path = QFileDialog.getExistingDirectory(
        	None,
        	self,
        	"select dir dialog",
        	"Select the directory to include in the rule:", 1)
        if path:
        	# Find existing names
        	existing = []
        	item = self.DirList.firstChild()
        	while item:
        		existing.append(str(item.text(0)))
        		item = item.nextSibling()
        	if str(path) not in existing:
        		QListViewItem(self.DirList, path)
        

    def RemoveDirectoryButton_clicked(self):

        item = self.DirList.firstChild()
        while item:
        	next = item.nextSibling()
        	if self.DirList.isSelected(item):
        		self.DirList.takeItem(item)
        	item = next
        

    def DirList_selectionChanged(self):

        item = self.DirList.firstChild()
        while item:
        	if self.DirList.isSelected(item):
        		self.RemoveDirectoryButton.setEnabled(1)
        		self.MakeRelativeButton.setEnabled(1)
        		return
        	item = item.nextSibling()
        self.RemoveDirectoryButton.setEnabled(0)
        self.MakeRelativeButton.setEnabled(0)
        

    def MakeRelativeButton_clicked(self):

        base = QFileDialog.getExistingDirectory(None, self, "getdir", "Select relative directory", 1)
        if base:
        	base = str(base)
        	if base and base[-1] != '/': base = base + '/'
        	print "Base is:",base
        	files = []
         	item = self.DirList.firstChild()
        	while item:
        		if self.DirList.isSelected(item):
        		    file = str(item.text(0))
        		    file = self.make_relative(base, file)
        		    item.setText(0, file)
        		item = item.nextSibling()
        

    def OkButton_clicked(self):

        self.accept()
        

    def CancelButton_clicked(self):

        self.reject()
        
