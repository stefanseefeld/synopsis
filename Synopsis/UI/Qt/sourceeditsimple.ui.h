/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void SourceEditSimple::AddFilesButton_clicked()
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


void SourceEditSimple::RemoveFileButton_clicked()
{
item = self.FileList.firstChild()
while item:
	next = item.nextSibling()
	if self.FileList.isSelected(item):
		self.FileList.takeItem(item)
	item = next
}


void SourceEditSimple::FileList_selectionChanged()
{
item = self.FileList.firstChild()
while item:
	if self.FileList.isSelected(item):
		self.RemoveFileButton.setEnabled(1)
		self.MakeRelativeButton.setEnabled(1)
		return
	item = item.nextSibling()
self.RemoveFileButton.setEnabled(0)
self.MakeRelativeButton.setEnabled(0)
}

void SourceEditSimple::MakeRelativeButton_clicked()
{
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
}


void SourceEditSimple::OkButton_clicked()
{
self.accept()
}


void SourceEditSimple::CancelButton_clicked()
{
self.reject()
}
