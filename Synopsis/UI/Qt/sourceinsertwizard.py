# Form implementation generated from reading ui file 'sourceinsertwizard.ui'
#
# Created: Tue Nov 12 00:23:21 2002
#      by: The PyQt User Interface Compiler (pyuic)
#
# WARNING! All changes made in this file will be lost!


from qt import *


class SourceInsertWizard(QWizard):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QWizard.__init__(self,parent,name,modal,fl)

        if name == None:
            self.setName("SourceInsertWizard")

        self.resize(600,480)
        self.setCaption(self.trUtf8("Source Action - Insert Rule Wizard"))
        f = QFont(self.font())
        f.setPointSize(20)
        self.setTitleFont(f)


        self.page = QWidget(self,"page")
        pageLayout = QVBoxLayout(self.page,11,6,"pageLayout")

        self.TextLabel9 = QLabel(self.page,"TextLabel9")
        self.TextLabel9.setText(self.trUtf8("Select the direction for this rule.\n"
"\n"
"By making a rule 'Exclude', you can ignore certain files that you don't want documented. For example, you might want a glob expression to ignore the CVS directories."))
        self.TextLabel9.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        pageLayout.addWidget(self.TextLabel9)

        self.ButtonGroup2 = QButtonGroup(self.page,"ButtonGroup2")
        self.ButtonGroup2.setTitle(self.trUtf8("Rule direction"))
        self.ButtonGroup2.setColumnLayout(0,Qt.Vertical)
        self.ButtonGroup2.layout().setSpacing(6)
        self.ButtonGroup2.layout().setMargin(11)
        ButtonGroup2Layout = QVBoxLayout(self.ButtonGroup2.layout())
        ButtonGroup2Layout.setAlignment(Qt.AlignTop)

        self.IncludeRadio = QRadioButton(self.ButtonGroup2,"IncludeRadio")
        self.IncludeRadio.setText(self.trUtf8("Include these files"))
        self.IncludeRadio.setChecked(1)
        ButtonGroup2Layout.addWidget(self.IncludeRadio)

        self.ExcludeRadio = QRadioButton(self.ButtonGroup2,"ExcludeRadio")
        self.ExcludeRadio.setText(self.trUtf8("Exclude these files"))
        ButtonGroup2Layout.addWidget(self.ExcludeRadio)
        pageLayout.addWidget(self.ButtonGroup2)
        spacer = QSpacerItem(0,158,QSizePolicy.Minimum,QSizePolicy.Expanding)
        pageLayout.addItem(spacer)
        self.addPage(self.page,self.trUtf8("Select Rule Direction"))

        self.select_type = QWidget(self,"select_type")
        select_typeLayout = QVBoxLayout(self.select_type,11,6,"select_typeLayout")

        self.TextLabel3 = QLabel(self.select_type,"TextLabel3")
        self.TextLabel3.setText(self.trUtf8("Select the type of source file rule you want to insert:"))
        select_typeLayout.addWidget(self.TextLabel3)

        self.ButtonGroup1 = QButtonGroup(self.select_type,"ButtonGroup1")
        self.ButtonGroup1.setTitle(self.trUtf8("Rule Type"))
        self.ButtonGroup1.setColumnLayout(0,Qt.Vertical)
        self.ButtonGroup1.layout().setSpacing(6)
        self.ButtonGroup1.layout().setMargin(11)
        ButtonGroup1Layout = QVBoxLayout(self.ButtonGroup1.layout())
        ButtonGroup1Layout.setAlignment(Qt.AlignTop)

        self.SimpleRadio = QRadioButton(self.ButtonGroup1,"SimpleRadio")
        self.SimpleRadio.setText(self.trUtf8("Simple"))
        self.SimpleRadio.setChecked(1)
        ButtonGroup1Layout.addWidget(self.SimpleRadio)

        self.TextLabel4 = QLabel(self.ButtonGroup1,"TextLabel4")
        self.TextLabel4.setText(self.trUtf8("A rule that selects a simple list of filenames"))
        self.TextLabel4.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        self.TextLabel4.setIndent(22)
        ButtonGroup1Layout.addWidget(self.TextLabel4)

        self.DirRadio = QRadioButton(self.ButtonGroup1,"DirRadio")
        self.DirRadio.setText(self.trUtf8("Directory and Glob expression"))
        ButtonGroup1Layout.addWidget(self.DirRadio)

        self.TextLabel5 = QLabel(self.ButtonGroup1,"TextLabel5")
        self.TextLabel5.setText(self.trUtf8("A rule that specifies one or more directories and a glob expression to use for selecting files. The glob expression (eg: *.hpp) can also select multiple types (eg: \"*.hpp\", \"*.cpp\")"))
        self.TextLabel5.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        self.TextLabel5.setIndent(22)
        ButtonGroup1Layout.addWidget(self.TextLabel5)

        self.BaseRadio = QRadioButton(self.ButtonGroup1,"BaseRadio")
        self.BaseRadio.setText(self.trUtf8("Recursive Directory and Glob expression"))
        ButtonGroup1Layout.addWidget(self.BaseRadio)

        self.TextLabel5_2 = QLabel(self.ButtonGroup1,"TextLabel5_2")
        self.TextLabel5_2.setText(self.trUtf8("A rule that specifies one or more directories and a glob expression to use for selecting files, but will also recurse into sub-directories to find matching files"))
        self.TextLabel5_2.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        self.TextLabel5_2.setIndent(22)
        ButtonGroup1Layout.addWidget(self.TextLabel5_2)
        select_typeLayout.addWidget(self.ButtonGroup1)
        spacer_2 = QSpacerItem(0,33,QSizePolicy.Minimum,QSizePolicy.Expanding)
        select_typeLayout.addItem(spacer_2)

        self.TextLabel6 = QLabel(self.select_type,"TextLabel6")
        self.TextLabel6.setText(self.trUtf8("Note that in all cases, the directories can be made relative to the current directory, rather than specifying absolute paths."))
        self.TextLabel6.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        select_typeLayout.addWidget(self.TextLabel6)
        self.addPage(self.select_type,self.trUtf8("Select Rule Type"))

        self.simple = QWidget(self,"simple")
        simpleLayout = QVBoxLayout(self.simple,11,6,"simpleLayout")

        self.TextLabel7 = QLabel(self.simple,"TextLabel7")
        self.TextLabel7.setText(self.trUtf8("Select the files you want to include in this rule:"))
        simpleLayout.addWidget(self.TextLabel7)

        self.GroupBox3 = QGroupBox(self.simple,"GroupBox3")
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

        Layout4 = QHBoxLayout(None,0,6,"Layout4")

        self.AddFilesButton = QPushButton(self.GroupBox3,"AddFilesButton")
        self.AddFilesButton.setText(self.trUtf8("Add files"))
        Layout4.addWidget(self.AddFilesButton)
        spacer_3 = QSpacerItem(131,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout4.addItem(spacer_3)

        self.RemoveFileButton = QPushButton(self.GroupBox3,"RemoveFileButton")
        self.RemoveFileButton.setEnabled(0)
        self.RemoveFileButton.setText(self.trUtf8("Remove from list"))
        Layout4.addWidget(self.RemoveFileButton)
        GroupBox3Layout.addLayout(Layout4)
        simpleLayout.addWidget(self.GroupBox3)
        self.addPage(self.simple,self.trUtf8("Select Files"))

        self.page_2 = QWidget(self,"page_2")
        pageLayout_2 = QVBoxLayout(self.page_2,11,6,"pageLayout_2")

        self.TextLabel7_2 = QLabel(self.page_2,"TextLabel7_2")
        self.TextLabel7_2.setText(self.trUtf8("Select the directories you want to include in this rule:"))
        pageLayout_2.addWidget(self.TextLabel7_2)

        self.GroupBox3_2 = QGroupBox(self.page_2,"GroupBox3_2")
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

        Layout7 = QHBoxLayout(None,0,6,"Layout7")

        self.AddDirectoryButton = QPushButton(self.GroupBox3_2,"AddDirectoryButton")
        self.AddDirectoryButton.setText(self.trUtf8("Add directory"))
        Layout7.addWidget(self.AddDirectoryButton)
        spacer_4 = QSpacerItem(121,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout7.addItem(spacer_4)

        self.RemoveDirectoryButton = QPushButton(self.GroupBox3_2,"RemoveDirectoryButton")
        self.RemoveDirectoryButton.setEnabled(0)
        self.RemoveDirectoryButton.setText(self.trUtf8("Remove directory"))
        Layout7.addWidget(self.RemoveDirectoryButton)
        GroupBox3_2Layout.addLayout(Layout7)
        pageLayout_2.addWidget(self.GroupBox3_2)
        self.addPage(self.page_2,self.trUtf8("Select Directories"))

        self.page_3 = QWidget(self,"page_3")
        pageLayout_3 = QVBoxLayout(self.page_3,11,6,"pageLayout_3")

        self.TextLabel11 = QLabel(self.page_3,"TextLabel11")
        self.TextLabel11.setText(self.trUtf8("Enter the glob expression for selecting files. You can enter multiple expressions by using a comma. Extra whitespace is removed. To use a real space or comma in the filename, quote it with a \"\\\" character, eg:\n"
"\n"
"*.cc - select all files ending in .cc\n"
"*.c, *.h - select all files ending in .c or .h\n"
"temp?.hpp - the ? matches one character, eg: temp1.hpp and temp2.hpp but not tempAll.hpp\n"
"*\\ * - Any file with a space in it\n"
"* - All files"))
        self.TextLabel11.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        pageLayout_3.addWidget(self.TextLabel11)

        self.TextLabel12 = QLabel(self.page_3,"TextLabel12")
        self.TextLabel12.setText(self.trUtf8("Glob Expression:"))
        pageLayout_3.addWidget(self.TextLabel12)

        self.FileGlobEdit = QLineEdit(self.page_3,"FileGlobEdit")
        self.FileGlobEdit.setText(self.trUtf8("*"))
        pageLayout_3.addWidget(self.FileGlobEdit)
        spacer_5 = QSpacerItem(0,50,QSizePolicy.Minimum,QSizePolicy.Expanding)
        pageLayout_3.addItem(spacer_5)
        self.addPage(self.page_3,self.trUtf8("Select File Glob Expression"))

        self.page_4 = QWidget(self,"page_4")
        pageLayout_4 = QVBoxLayout(self.page_4,11,6,"pageLayout_4")

        self.TextLabel13 = QLabel(self.page_4,"TextLabel13")
        self.TextLabel13.setText(self.trUtf8("Each filename matched by previous rules will be compared to this Glob expression. If it matches then the file will be removed from the list of files for this Source Action.\n"
"\n"
"Note that the expression is compared to the whole filename, but a / is always prefixed to simplify the matching of directory names.\n"
"\n"
"Eg: */CVS/*  matches all CVS directories\n"
"Eg: */.*   matches all hidden files (those that start with a . are hidden by convention in Unix).\n"
"Eg: /Subdir/OldFile.hpp  matches one file in the given project sub-directory\n"
"Eg: */stdafx.h   matches the file stdafx.h in any directory"))
        self.TextLabel13.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        pageLayout_4.addWidget(self.TextLabel13)

        self.TextLabel14 = QLabel(self.page_4,"TextLabel14")
        self.TextLabel14.setText(self.trUtf8("Enter Glob Expression or select from list:"))
        pageLayout_4.addWidget(self.TextLabel14)

        self.ExcludeCombo = QComboBox(0,self.page_4,"ExcludeCombo")
        self.ExcludeCombo.insertItem(self.trUtf8("<type here or click arrow for a list>"))
        self.ExcludeCombo.insertItem(self.trUtf8("*/CVS/*"))
        self.ExcludeCombo.insertItem(self.trUtf8("*/.*"))
        self.ExcludeCombo.insertItem(self.trUtf8("*/stdafx.*"))
        self.ExcludeCombo.setEditable(1)
        self.ExcludeCombo.setInsertionPolicy(QComboBox.AtTop)
        pageLayout_4.addWidget(self.ExcludeCombo)
        spacer_6 = QSpacerItem(0,41,QSizePolicy.Minimum,QSizePolicy.Expanding)
        pageLayout_4.addItem(spacer_6)
        self.addPage(self.page_4,self.trUtf8("Select Exclude Expression"))

        self.page_5 = QWidget(self,"page_5")
        pageLayout_5 = QVBoxLayout(self.page_5,11,6,"pageLayout_5")

        self.TextLabel1 = QLabel(self.page_5,"TextLabel1")
        self.TextLabel1.setText(self.trUtf8("Using absolute pathnames is generally not a good idea, since it limits the ability to distribute the config file with your software.\n"
"\n"
"You can make any directories you have specified relative to either the current directory, or a specified directory."))
        self.TextLabel1.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        pageLayout_5.addWidget(self.TextLabel1)

        self.ButtonGroup3 = QButtonGroup(self.page_5,"ButtonGroup3")
        self.ButtonGroup3.setTitle(self.trUtf8("Select relative option"))
        self.ButtonGroup3.setColumnLayout(0,Qt.Vertical)
        self.ButtonGroup3.layout().setSpacing(6)
        self.ButtonGroup3.layout().setMargin(11)
        ButtonGroup3Layout = QVBoxLayout(self.ButtonGroup3.layout())
        ButtonGroup3Layout.setAlignment(Qt.AlignTop)

        self.RelativeNoneRadio = QRadioButton(self.ButtonGroup3,"RelativeNoneRadio")
        self.RelativeNoneRadio.setText(self.trUtf8("Don't change my directories"))
        self.RelativeNoneRadio.setChecked(1)
        ButtonGroup3Layout.addWidget(self.RelativeNoneRadio)

        self.RelativeCurrentRadio = QRadioButton(self.ButtonGroup3,"RelativeCurrentRadio")
        self.RelativeCurrentRadio.setText(self.trUtf8("Make directories relative to current directory"))
        ButtonGroup3Layout.addWidget(self.RelativeCurrentRadio)

        self.CurrentDirectoryLabel = QLabel(self.ButtonGroup3,"CurrentDirectoryLabel")
        self.CurrentDirectoryLabel.setEnabled(0)
        self.CurrentDirectoryLabel.setText(self.trUtf8("Current Directory: foo"))
        self.CurrentDirectoryLabel.setIndent(20)
        ButtonGroup3Layout.addWidget(self.CurrentDirectoryLabel)

        self.RelativeThisRadio = QRadioButton(self.ButtonGroup3,"RelativeThisRadio")
        self.RelativeThisRadio.setText(self.trUtf8("Make directories relative to this directory:"))
        ButtonGroup3Layout.addWidget(self.RelativeThisRadio)

        Layout6 = QHBoxLayout(None,0,6,"Layout6")

        self.RelativeThisEdit = QLineEdit(self.ButtonGroup3,"RelativeThisEdit")
        self.RelativeThisEdit.setEnabled(0)
        self.RelativeThisEdit.setSizePolicy(QSizePolicy(7,0,0,1,self.RelativeThisEdit.sizePolicy().hasHeightForWidth()))
        self.RelativeThisEdit.setText(self.trUtf8("Click the ... button to select a directory"))
        Layout6.addWidget(self.RelativeThisEdit)

        self.RelativeThisButton = QPushButton(self.ButtonGroup3,"RelativeThisButton")
        self.RelativeThisButton.setEnabled(0)
        self.RelativeThisButton.setMaximumSize(QSize(32,32767))
        self.RelativeThisButton.setText(self.trUtf8("..."))
        Layout6.addWidget(self.RelativeThisButton)
        ButtonGroup3Layout.addLayout(Layout6)
        pageLayout_5.addWidget(self.ButtonGroup3)
        self.addPage(self.page_5,self.trUtf8("Make directories relative"))

        self.page_6 = QWidget(self,"page_6")
        pageLayout_6 = QVBoxLayout(self.page_6,11,6,"pageLayout_6")

        self.TextLabel8 = QLabel(self.page_6,"TextLabel8")
        self.TextLabel8.setText(self.trUtf8("That's it!\n"
"\n"
"Your new Source Rule is now complete.\n"
"\n"
"Click Finish to accept this new rule, or go Back to make changes."))
        pageLayout_6.addWidget(self.TextLabel8)

        self.RuleInfo = QTextEdit(self.page_6,"RuleInfo")
        self.RuleInfo.setText(self.trUtf8("Text info for rule goes here ..."))
        self.RuleInfo.setReadOnly(1)
        pageLayout_6.addWidget(self.RuleInfo)
        self.addPage(self.page_6,self.trUtf8("Finished"))

        self.connect(self.IncludeRadio,SIGNAL("clicked()"),self.IncludeRadio_clicked)
        self.connect(self.ExcludeRadio,SIGNAL("clicked()"),self.ExcludeRadio_clicked)
        self.connect(self.SimpleRadio,SIGNAL("clicked()"),self.SimpleRadio_clicked)
        self.connect(self.DirRadio,SIGNAL("clicked()"),self.DirRadio_clicked)
        self.connect(self.BaseRadio,SIGNAL("clicked()"),self.BaseRadio_clicked)
        self.connect(self.AddFilesButton,SIGNAL("clicked()"),self.AddFilesButton_clicked)
        self.connect(self.RemoveFileButton,SIGNAL("clicked()"),self.RemoveFileButton_clicked)
        self.connect(self.FileList,SIGNAL("selectionChanged()"),self.FileList_selectionChanged)
        self.connect(self.RelativeNoneRadio,SIGNAL("clicked()"),self.RelativeNoneRadio_clicked)
        self.connect(self.RelativeCurrentRadio,SIGNAL("clicked()"),self.RelativeCurrentRadio_clicked)
        self.connect(self.RelativeThisRadio,SIGNAL("clicked()"),self.RelativeThisRadio_clicked)
        self.connect(self.RelativeThisButton,SIGNAL("clicked()"),self.RelativeThisButton_clicked)
        self.connect(self.AddDirectoryButton,SIGNAL("clicked()"),self.AddDirectoryButton_clicked)
        self.connect(self.RemoveDirectoryButton,SIGNAL("clicked()"),self.RemoveDirectoryButton_clicked)
        self.connect(self.DirList,SIGNAL("selectionChanged()"),self.DirList_selectionChanged)

        self.init()

    def init(self):

        # default path is 0, 1, 2, 6
        self.setAppropriate(QWizard.page(self, 3), 0)
        self.setAppropriate(QWizard.page(self, 4), 0)
        self.setAppropriate(QWizard.page(self, 5), 0)
        self.rule_type = 'simple'
        self.relative_option = 0 # not relative
        self.setFinishEnabled(QWizard.page(self,7), 1)
        

    def SelectFilesButton_clicked(self):
        print "SourceInsertWizard.SelectFilesButton_clicked(): Not implemented yet"

    def IncludeRadio_clicked(self):

        self.rule_type = 'simple'
        self.setAppropriate(QWizard.page(self, 1), 1)
        self.setAppropriate(QWizard.page(self, 2), 1)
        self.setAppropriate(QWizard.page(self, 3), 0)
        self.setAppropriate(QWizard.page(self, 4), 0)
        self.setAppropriate(QWizard.page(self, 5), 0)
        self.setAppropriate(QWizard.page(self, 6), 1)
        

    def ExcludeRadio_clicked(self):

        self.rule_type = 'exclude'
        self.setAppropriate(QWizard.page(self, 1), 0)
        self.setAppropriate(QWizard.page(self, 2), 0)
        self.setAppropriate(QWizard.page(self, 3), 0)
        self.setAppropriate(QWizard.page(self, 4), 0)
        self.setAppropriate(QWizard.page(self, 5), 1)
        self.setAppropriate(QWizard.page(self, 6), 0)
        

    def SimpleRadio_clicked(self):

        self.rule_type = 'simple'
        self.setAppropriate(QWizard.page(self, 2), 1)
        self.setAppropriate(QWizard.page(self, 3), 0)
        self.setAppropriate(QWizard.page(self, 4), 0)
        

    def DirRadio_clicked(self):

        self.rule_type = 'glob'
        self.setAppropriate(QWizard.page(self, 2), 0)
        self.setAppropriate(QWizard.page(self, 3), 1)
        self.setAppropriate(QWizard.page(self, 4), 1)
        

    def BaseRadio_clicked(self):

        self.rule_type = 'recursive glob'
        self.setAppropriate(QWizard.page(self, 2), 0)
        self.setAppropriate(QWizard.page(self, 3), 1)
        self.setAppropriate(QWizard.page(self, 4), 1)
        

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
        		return
        	item = item.nextSibling()
        self.RemoveFileButton.setEnabled(0)
        

    def RelativeNoneRadio_clicked(self):

        self.relative_option = 0
        self.CurrentDirectoryLabel.setEnabled(0)
        self.RelativeThisEdit.setEnabled(0)
        self.RelativeThisButton.setEnabled(0)
        

    def RelativeCurrentRadio_clicked(self):

        self.relative_option = 1
        self.CurrentDirectoryLabel.setEnabled(1)
        self.RelativeThisEdit.setEnabled(0)
        self.RelativeThisButton.setEnabled(0)
        

    def RelativeThisRadio_clicked(self):

        self.relative_option = 2
        self.CurrentDirectoryLabel.setEnabled(0)
        self.RelativeThisEdit.setEnabled(1)
        self.RelativeThisButton.setEnabled(1)
        

    def RelativeThisButton_clicked(self):

        dir = QFileDialog.getExistingDirectory(None, self, "getdir", "Select relative directory", 1)
        if dir:
              self.RelativeThisEdit.setText(str(dir))
        

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
        		return
        	item = item.nextSibling()
        self.RemoveDirectoryButton.setEnabled(0)
        
