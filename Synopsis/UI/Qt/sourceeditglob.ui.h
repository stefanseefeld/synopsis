/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/


void SourceEditGlob::AddDirectoryButton_clicked()
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

void SourceEditGlob::RemoveDirectoryButton_clicked()
{
item = self.DirList.firstChild()
while item:
	next = item.nextSibling()
	if self.DirList.isSelected(item):
		self.DirList.takeItem(item)
	item = next
}


void SourceEditGlob::DirList_selectionChanged()
{
item = self.DirList.firstChild()
while item:
	if self.DirList.isSelected(item):
		self.RemoveDirectoryButton.setEnabled(1)
		self.MakeRelativeButton.setEnabled(1)
		return
	item = item.nextSibling()
self.RemoveDirectoryButton.setEnabled(0)
self.MakeRelativeButton.setEnabled(0)
}
	    
void SourceEditGlob::MakeRelativeButton_clicked()
{
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
}

void SourceEditGlob::OkButton_clicked()
{
self.accept()
}


void SourceEditGlob::CancelButton_clicked()
{
self.reject()
}
