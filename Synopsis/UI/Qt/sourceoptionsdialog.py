# Form implementation generated from reading ui file 'sourceoptionsdialog.ui'
#
# Created: Tue Nov 12 00:34:51 2002
#      by: The PyQt User Interface Compiler (pyuic)
#
# WARNING! All changes made in this file will be lost!


from qt import *


class SourceOptionsDialog(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if name == None:
            self.setName("SourceOptionsDialog")

        self.resize(600,558)
        self.setCaption(self.trUtf8("Source Action Options"))

        SourceOptionsDialogLayout = QVBoxLayout(self,11,6,"SourceOptionsDialogLayout")

        Layout5 = QHBoxLayout(None,0,6,"Layout5")

        self.TextLabel1 = QLabel(self,"TextLabel1")
        self.TextLabel1.setText(self.trUtf8("Name:"))
        Layout5.addWidget(self.TextLabel1)

        self.NameEdit = QLineEdit(self,"NameEdit")
        self.NameEdit.setText(self.trUtf8("Action Name"))
        self.NameEdit.setMaxLength(40)
        Layout5.addWidget(self.NameEdit)
        SourceOptionsDialogLayout.addLayout(Layout5)

        self.GroupBox1 = QGroupBox(self,"GroupBox1")
        self.GroupBox1.setTitle(self.trUtf8("File Selection Rules"))
        self.GroupBox1.setColumnLayout(0,Qt.Vertical)
        self.GroupBox1.layout().setSpacing(6)
        self.GroupBox1.layout().setMargin(11)
        GroupBox1Layout = QVBoxLayout(self.GroupBox1.layout())
        GroupBox1Layout.setAlignment(Qt.AlignTop)

        self.TextLabel2 = QLabel(self.GroupBox1,"TextLabel2")
        self.TextLabel2.setText(self.trUtf8("Source files are selected by applying these rules in order:"))
        GroupBox1Layout.addWidget(self.TextLabel2)

        self.RuleList = QListView(self.GroupBox1,"RuleList")
        self.RuleList.addColumn(self.trUtf8("Type"))
        self.RuleList.header().setClickEnabled(0,self.RuleList.header().count() - 1)
        self.RuleList.addColumn(self.trUtf8("Path"))
        self.RuleList.header().setClickEnabled(0,self.RuleList.header().count() - 1)
        self.RuleList.addColumn(self.trUtf8("Filename"))
        self.RuleList.header().setClickEnabled(0,self.RuleList.header().count() - 1)
        self.RuleList.addColumn(self.trUtf8("Options"))
        self.RuleList.header().setClickEnabled(0,self.RuleList.header().count() - 1)
        self.RuleList.setAllColumnsShowFocus(1)
        self.RuleList.setRootIsDecorated(1)
        GroupBox1Layout.addWidget(self.RuleList)

        Layout7 = QHBoxLayout(None,0,6,"Layout7")

        self.InsertButton = QPushButton(self.GroupBox1,"InsertButton")
        self.InsertButton.setText(self.trUtf8("Add New Rule ... "))
        Layout7.addWidget(self.InsertButton)
        spacer = QSpacerItem(91,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout7.addItem(spacer)

        self.DeleteButton = QPushButton(self.GroupBox1,"DeleteButton")
        self.DeleteButton.setEnabled(0)
        self.DeleteButton.setText(self.trUtf8("Delete Rule"))
        Layout7.addWidget(self.DeleteButton)

        self.EditButton = QPushButton(self.GroupBox1,"EditButton")
        self.EditButton.setEnabled(0)
        self.EditButton.setText(self.trUtf8("Edit Rule"))
        Layout7.addWidget(self.EditButton)

        self.UpButton = QPushButton(self.GroupBox1,"UpButton")
        self.UpButton.setEnabled(0)
        self.UpButton.setText(self.trUtf8("Up"))
        Layout7.addWidget(self.UpButton)

        self.DownButton = QPushButton(self.GroupBox1,"DownButton")
        self.DownButton.setEnabled(0)
        self.DownButton.setText(self.trUtf8("Down"))
        Layout7.addWidget(self.DownButton)
        GroupBox1Layout.addLayout(Layout7)
        SourceOptionsDialogLayout.addWidget(self.GroupBox1)

        Layout8 = QHBoxLayout(None,0,6,"Layout8")
        spacer_2 = QSpacerItem(21,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout8.addItem(spacer_2)

        self.TextLabel3 = QLabel(self,"TextLabel3")
        self.TextLabel3.setText(self.trUtf8("You can test your rules by clicking here:"))
        Layout8.addWidget(self.TextLabel3)

        self.TestButton = QPushButton(self,"TestButton")
        self.TestButton.setText(self.trUtf8("Test Rules"))
        Layout8.addWidget(self.TestButton)
        SourceOptionsDialogLayout.addLayout(Layout8)

        self.Frame3 = QFrame(self,"Frame3")
        self.Frame3.setFrameShape(QFrame.HLine)
        self.Frame3.setFrameShadow(QFrame.Sunken)
        SourceOptionsDialogLayout.addWidget(self.Frame3)

        Layout5_2 = QHBoxLayout(None,0,6,"Layout5_2")

        self.CancelButton = QPushButton(self,"CancelButton")
        self.CancelButton.setText(self.trUtf8("Cancel"))
        Layout5_2.addWidget(self.CancelButton)
        spacer_3 = QSpacerItem(400,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout5_2.addItem(spacer_3)

        self.OkButton = QPushButton(self,"OkButton")
        self.OkButton.setText(self.trUtf8("Ok"))
        Layout5_2.addWidget(self.OkButton)
        SourceOptionsDialogLayout.addLayout(Layout5_2)

        self.connect(self.OkButton,SIGNAL("clicked()"),self.OkButton_clicked)
        self.connect(self.CancelButton,SIGNAL("clicked()"),self.CancelButton_clicked)

        self.init()

    def init(self):

        print "Disabling sorting for RuleList"
        self.RuleList.setSorting(-1)
        

    def OkButton_clicked(self):

        self.accept()
        

    def CancelButton_clicked(self):

        self.reject()
        
