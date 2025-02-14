

#include "shortcutpopup.h"

// Tnz6 includes
#include "menubarcommandids.h"
#include "tapp.h"

// TnzQt includes
#include "toonzqt/menubarcommand.h"
#include "toonzqt/dvdialog.h"

// Qt includes
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollArea>
#include <QSizePolicy>
#include <QPushButton>
#include <QPainter>
#include <QAction>
#include <QKeyEvent>
#include <QMainWindow>

// STD includes
#include <vector>

//=============================================================================
// ShortcutItem
// ------------
// Lo ShortcutTree visualizza ShortcutItem (organizzati in folder)
// ogni ShortcutItem rappresenta una QAction (e relativo Shortcut)
//-----------------------------------------------------------------------------

class ShortcutItem : public QTreeWidgetItem
{
	QAction *m_action;

public:
	ShortcutItem(QTreeWidgetItem *parent, QAction *action)
		: QTreeWidgetItem(parent, UserType), m_action(action)
	{
		setFlags(parent->flags());
		updateText();
	}
	void updateText()
	{
		QString text = m_action->text();
		text.remove("&");
		QString shortcut = m_action->shortcut().toString();
		if (shortcut != "")
			text += " (" + shortcut + ")";
		setText(0, text);
	}
	QAction *getAction() const { return m_action; }
};

//=============================================================================
// ShortcutViewer
//-----------------------------------------------------------------------------

ShortcutViewer::ShortcutViewer(QWidget *parent)
	: QWidget(parent), m_action(0)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

//-----------------------------------------------------------------------------

ShortcutViewer::~ShortcutViewer()
{
}

//-----------------------------------------------------------------------------

void ShortcutViewer::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	// sfondo azzurro se il widget ha il focus (e quindi si accorge dei tasti premuti)
	p.fillRect(1, 1, width() - 1, height() - 1, QBrush(hasFocus() ? QColor(171, 206, 255) : Qt::white));
	// bordo
	p.setPen(QColor(184, 188, 127));
	p.drawRect(0, 0, width() - 1, height() - 1);
	if (m_action) {
		// lo shortcut corrente
		p.setPen(Qt::black);
		p.drawText(10, 13, m_action->shortcut().toString());
	}
}

//-----------------------------------------------------------------------------

void ShortcutViewer::setAction(QAction *action)
{
	m_action = action;
	update();
}

//-----------------------------------------------------------------------------

bool ShortcutViewer::event(QEvent *event)
{
	// quando si vuole assegnare una combinazione che gia' assegnata bisogna
	// evitare che lo shortcut relativo agisca.
	if (event->type() == QEvent::ShortcutOverride) {
		event->accept();
		return true;
	} else
		return QWidget::event(event);
}

//-----------------------------------------------------------------------------

void ShortcutViewer::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();
	if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt) {
		event->ignore();
		return;
	}
	Qt::KeyboardModifiers modifiers = event->modifiers();

	// Tasti che non possono essere utilizzati come shortcut
	if ((modifiers | (Qt::CTRL | Qt::SHIFT | Qt::ALT)) != (Qt::CTRL | Qt::SHIFT | Qt::ALT) || key == Qt::Key_Home || key == Qt::Key_End || key == Qt::Key_PageDown || key == Qt::Key_PageUp || key == Qt::Key_Escape || key == Qt::Key_Print || key == Qt::Key_Pause || key == Qt::Key_ScrollLock) {
		if (key != Qt::Key_Plus && key != Qt::Key_Minus && key != Qt::Key_Asterisk && key != Qt::Key_Slash) {
			event->ignore();
			return;
		} else
			modifiers = 0;
	}

	if (m_action) {
		CommandManager *cm = CommandManager::instance();
		QKeySequence keySequence(key + modifiers);
		std::string shortcutString = keySequence.toString().toStdString();
		QAction *oldAction = cm->getActionFromShortcut(keySequence.toString().toStdString());
		if (oldAction == m_action)
			return;
		if (oldAction) {
			QString msg = tr("%1 is already assigned to '%2'\nAssign to '%3'?").arg(keySequence.toString()).arg(oldAction->iconText()).arg(m_action->iconText());
			int ret = DVGui::MsgBox(msg, tr("Yes"), tr("No"), 1);
			activateWindow();
			if (ret == 2 || ret == 0)
				return;
		}
		CommandManager::instance()->setShortcut(m_action, shortcutString);
		emit shortcutChanged();
	}
	event->accept();
	update();
}

//-----------------------------------------------------------------------------

void ShortcutViewer::removeShortcut()
{
	if (m_action) {
		CommandManager::instance()->setShortcut(m_action, "");
		emit shortcutChanged();
		update();
	}
}

//-----------------------------------------------------------------------------

void ShortcutViewer::enterEvent(QEvent *event)
{
	setFocus();
	update();
}

//-----------------------------------------------------------------------------

void ShortcutViewer::leaveEvent(QEvent *event)
{
	clearFocus();
	update();
}

//=============================================================================
// ShortcutTree
//-----------------------------------------------------------------------------

ShortcutTree::ShortcutTree(QWidget *parent)
	: QTreeWidget(parent)
{

	setObjectName("ShortcutTree");
	setIndentation(14);
	setAlternatingRowColors(true);

	setColumnCount(1);
	header()->close();
	//setStyleSheet("border-bottom:1px solid rgb(120,120,120); border-left:1px solid rgb(120,120,120); border-top:1px solid rgb(120,120,120)");

	QTreeWidgetItem *menuCommandFolder = new QTreeWidgetItem(this);
	menuCommandFolder->setText(0, tr("Menu Commands"));
	addFolder(tr("Fill"), FillCommandType);
	addFolder(tr("File"), MenuFileCommandType, menuCommandFolder);
	addFolder(tr("Edit"), MenuEditCommandType, menuCommandFolder);
#ifndef LINETEST
	addFolder(tr("Scan & Cleanup"), MenuScanCleanupCommandType, menuCommandFolder);
#endif
	addFolder(tr("Level"), MenuLevelCommandType, menuCommandFolder);
	addFolder(tr("Xsheet"), MenuXsheetCommandType, menuCommandFolder);
	addFolder(tr("Cells"), MenuCellsCommandType, menuCommandFolder);
	addFolder(tr("View"), MenuViewCommandType, menuCommandFolder);
	addFolder(tr("Windows"), MenuWindowsCommandType, menuCommandFolder);

	addFolder(tr("Right-click Menu Commands"), RightClickMenuCommandType);

	addFolder(tr("Tools"), ToolCommandType);
#ifndef LINETEST
	addFolder(tr("Tool Modifiers"), ToolModifierCommandType);
#endif
	addFolder(tr("Visualization"), ZoomCommandType);
	addFolder(tr("Misc"), MiscCommandType);
	addFolder(tr("Playback Controls"), PlaybackCommandType);
	addFolder(tr("RGBA Channels"), RGBACommandType);

	sortItems(0, Qt::AscendingOrder);

	connect(
		this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		this, SLOT(onCurrentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
}

//-----------------------------------------------------------------------------

ShortcutTree::~ShortcutTree()
{
}

//-----------------------------------------------------------------------------

void ShortcutTree::addFolder(const QString &title, int commandType, QTreeWidgetItem *parentFolder)
{
	QTreeWidgetItem *folder;
	if (!parentFolder)
		folder = new QTreeWidgetItem(this);
	else
		folder = new QTreeWidgetItem(parentFolder);
	assert(folder);
	folder->setText(0, title);

	std::vector<QAction *> actions;
	CommandManager::instance()->getActions((CommandType)commandType, actions);
	for (int i = 0; i < (int)actions.size(); i++) {
		ShortcutItem *item = new ShortcutItem(folder, actions[i]);
		m_items.push_back(item);
	}
}

//-----------------------------------------------------------------------------

void ShortcutTree::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	ShortcutItem *item = dynamic_cast<ShortcutItem *>(current);
	emit actionSelected(item ? item->getAction() : 0);
}

//-----------------------------------------------------------------------------

void ShortcutTree::onShortcutChanged()
{
	int i;
	for (i = 0; i < (int)m_items.size(); i++)
		m_items[i]->updateText();
}

//=============================================================================
// ShortcutPopup
//-----------------------------------------------------------------------------

ShortcutPopup::ShortcutPopup()
	: Dialog(TApp::instance()->getMainWindow(), false, false, "Shortcut")
{
	setWindowTitle(tr("Configure Shortcuts"));

	m_list = new ShortcutTree(this);
	m_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	m_sViewer = new ShortcutViewer(this);
	m_removeBtn = new QPushButton(tr("Remove"), this);

	m_topLayout->setMargin(5);
	m_topLayout->setSpacing(8);
	{
		m_topLayout->addWidget(m_list, 1);

		QHBoxLayout *bottomLayout = new QHBoxLayout();
		bottomLayout->setMargin(0);
		bottomLayout->setSpacing(1);
		{
			bottomLayout->addWidget(m_sViewer, 1);
			bottomLayout->addWidget(m_removeBtn, 0);
		}
		m_topLayout->addLayout(bottomLayout, 0);
	}

	connect(
		m_list, SIGNAL(actionSelected(QAction *)),
		m_sViewer, SLOT(setAction(QAction *)));

	connect(
		m_removeBtn, SIGNAL(clicked()),
		m_sViewer, SLOT(removeShortcut()));

	connect(
		m_sViewer, SIGNAL(shortcutChanged()),
		m_list, SLOT(onShortcutChanged()));
}

//-----------------------------------------------------------------------------

ShortcutPopup::~ShortcutPopup()
{
}

//-----------------------------------------------------------------------------

OpenPopupCommandHandler<ShortcutPopup> openShortcutPopup(MI_ShortcutPopup);
