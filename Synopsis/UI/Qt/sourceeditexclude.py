# Form implementation generated from reading ui file 'sourceeditexclude.ui'
#
# Created: Tue Nov 12 00:34:10 2002
#      by: The PyQt User Interface Compiler (pyuic)
#
# WARNING! All changes made in this file will be lost!


from qt import *


class SourceEditExclude(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if name == None:
            self.setName("SourceEditExclude")

        self.resize(600,141)
        self.setCaption(self.trUtf8("Edit Exclude Rule"))

        SourceEditExcludeLayout = QVBoxLayout(self,11,6,"SourceEditExcludeLayout")

        self.TextLabel12 = QLabel(self,"TextLabel12")
        self.TextLabel12.setText(self.trUtf8("Glob expression:\n"
"(matching files will be excluded from the list of files)"))
        self.TextLabel12.setAlignment(QLabel.WordBreak | QLabel.AlignVCenter)
        SourceEditExcludeLayout.addWidget(self.TextLabel12)

        self.GlobEdit = QLineEdit(self,"GlobEdit")
        self.GlobEdit.setText(self.trUtf8("*"))
        SourceEditExcludeLayout.addWidget(self.GlobEdit)

        self.Frame3_2 = QFrame(self,"Frame3_2")
        self.Frame3_2.setFrameShape(QFrame.HLine)
        self.Frame3_2.setFrameShadow(QFrame.Sunken)
        SourceEditExcludeLayout.addWidget(self.Frame3_2)

        Layout5 = QHBoxLayout(None,0,6,"Layout5")

        self.CancelButton = QPushButton(self,"CancelButton")
        self.CancelButton.setText(self.trUtf8("Cancel"))
        Layout5.addWidget(self.CancelButton)
        spacer = QSpacerItem(400,0,QSizePolicy.Expanding,QSizePolicy.Minimum)
        Layout5.addItem(spacer)

        self.OkButton = QPushButton(self,"OkButton")
        self.OkButton.setText(self.trUtf8("Ok"))
        Layout5.addWidget(self.OkButton)
        SourceEditExcludeLayout.addLayout(Layout5)

        self.connect(self.OkButton,SIGNAL("clicked()"),self.OkButton_clicked)
        self.connect(self.CancelButton,SIGNAL("clicked()"),self.CancelButton_clicked)

    def OkButton_clicked(self):

        self.accept()
        

    def CancelButton_clicked(self):

        self.reject()
        
