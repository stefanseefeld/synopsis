/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void SourceInsertWizard::init()
{
# default path is 0, 1, 2, 6
self.setAppropriate(QWizard.page(self, 3), 0)
self.setAppropriate(QWizard.page(self, 4), 0)
self.setAppropriate(QWizard.page(self, 5), 0)
self.rule_type = 'simple'
self.relative_option = 0 # not relative
self.setFinishEnabled(QWizard.page(self,7), 1)
}

void SourceInsertWizard::SelectFilesButton_clicked()
{
}


void SourceInsertWizard::IncludeRadio_clicked()
{
self.rule_type = 'simple'
self.setAppropriate(QWizard.page(self, 1), 1)
self.setAppropriate(QWizard.page(self, 2), 1)
self.setAppropriate(QWizard.page(self, 3), 0)
self.setAppropriate(QWizard.page(self, 4), 0)
self.setAppropriate(QWizard.page(self, 5), 0)
self.setAppropriate(QWizard.page(self, 6), 1)
}


void SourceInsertWizard::ExcludeRadio_clicked()
{
self.rule_type = 'exclude'
self.setAppropriate(QWizard.page(self, 1), 0)
self.setAppropriate(QWizard.page(self, 2), 0)
self.setAppropriate(QWizard.page(self, 3), 0)
self.setAppropriate(QWizard.page(self, 4), 0)
self.setAppropriate(QWizard.page(self, 5), 1)
self.setAppropriate(QWizard.page(self, 6), 0)
}


void SourceInsertWizard::SimpleRadio_clicked()
{
self.rule_type = 'simple'
self.setAppropriate(QWizard.page(self, 2), 1)
self.setAppropriate(QWizard.page(self, 3), 0)
self.setAppropriate(QWizard.page(self, 4), 0)
}


void SourceInsertWizard::DirRadio_clicked()
{
self.rule_type = 'glob'
self.setAppropriate(QWizard.page(self, 2), 0)
self.setAppropriate(QWizard.page(self, 3), 1)
self.setAppropriate(QWizard.page(self, 4), 1)
}


void SourceInsertWizard::BaseRadio_clicked()
{
self.rule_type = 'recursive glob'
self.setAppropriate(QWizard.page(self, 2), 0)
self.setAppropriate(QWizard.page(self, 3), 1)
self.setAppropriate(QWizard.page(self, 4), 1)
}


void SourceInsertWizard::AddFilesButton_clicked()
{
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
}


void SourceInsertWizard::RemoveFileButton_clicked()
{
item = self.FileList.firstChild()
while item:
	next = item.nextSibling()
	if self.FileList.isSelected(item):
		self.FileList.takeItem(item)
	item = next
}


void SourceInsertWizard::FileList_selectionChanged()
{
item = self.FileList.firstChild()
while item:
	if self.FileList.isSelected(item):
		self.RemoveFileButton.setEnabled(1)
		return
	item = item.nextSibling()
self.RemoveFileButton.setEnabled(0)
}


void SourceInsertWizard::RelativeNoneRadio_clicked()
{
self.relative_option = 0
self.CurrentDirectoryLabel.setEnabled(0)
self.RelativeThisEdit.setEnabled(0)
self.RelativeThisButton.setEnabled(0)
}


void SourceInsertWizard::RelativeCurrentRadio_clicked()
{
self.relative_option = 1
self.CurrentDirectoryLabel.setEnabled(1)
self.RelativeThisEdit.setEnabled(0)
self.RelativeThisButton.setEnabled(0)
}


void SourceInsertWizard::RelativeThisRadio_clicked()
{
self.relative_option = 2
self.CurrentDirectoryLabel.setEnabled(0)
self.RelativeThisEdit.setEnabled(1)
self.RelativeThisButton.setEnabled(1)
}


void SourceInsertWizard::RelativeThisButton_clicked()
{
dir = QFileDialog.getExistingDirectory(None, self, "getdir", "Select relative directory", 1)
if dir:
      self.RelativeThisEdit.setText(str(dir))
}


void SourceInsertWizard::AddDirectoryButton_clicked()
{
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
}

void SourceInsertWizard::RemoveDirectoryButton_clicked()
{
item = self.DirList.firstChild()
while item:
	next = item.nextSibling()
	if self.DirList.isSelected(item):
		self.DirList.takeItem(item)
	item = next
}


void SourceInsertWizard::DirList_selectionChanged()
{
item = self.DirList.firstChild()
while item:
	if self.DirList.isSelected(item):
		self.RemoveDirectoryButton.setEnabled(1)
		return
	item = item.nextSibling()
self.RemoveDirectoryButton.setEnabled(0)
}
