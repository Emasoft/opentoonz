

#ifndef FILEBROWSERPOPUP_H
#define FILEBROWSERPOPUP_H

// TnzCore includes
#include "tfilepath.h"
#include "tpixel.h"

// TnzQt includes
#include "toonzqt/dvdialog.h"
#include "toonzqt/filefield.h"
#include "toonzqt/lineedit.h"
#include "toonzqt/checkbox.h"
#include "toonzqt/doublefield.h"

// Tnz6 includes
#include "cellselection.h"

//========================================================

//    Forward declaration

class FileBrowser;
class TDoubleParam;
class TCamera;
class TPropertyGroup;
class QShowEvent;
class QFrame;
class QPushButton;
class QComboBox;
class QGroupBox;
class QCheckBox;

namespace DVGui
{
class ColorField;
}

//========================================================

//********************************************************************************
//    FileBrowserPopup  declaration
//********************************************************************************

class FileBrowserPopup : public QDialog
{
	Q_OBJECT

public:
	enum Options //! Various options used to customize the popup's behavior.
	{
		STANDARD = 0x0,			//!< Standard options.
		CUSTOM_LAYOUT = 0x1,	//!< Prevents standard layout organization at construction, surrendering it
								//!  to the user. Observe that sub-widgets creation is still enforced.
		MULTISELECTION = 0x2,   //!< Enable multiple selection in the browser widget.
		WITH_APPLY_BUTTON = 0x4 //!< Enable if the filebrowser has an apply button next to the OK button
	};

public:
	FileBrowserPopup(const QString &windowTitle, Options options = STANDARD, QString applyButtonTxt = "", QWidget *customWidget = 0);
	virtual ~FileBrowserPopup() {}

	void setOkText(const QString &text);
	void setFilterTypes(const QStringList &typesList);
	void removeFilterType(const QString &type);
	void addFilterType(const QString &type);
	void setFileMode(bool isDirectoryOnly);
	void setFolder(const TFilePath &folderPath);
	void setFilename(const TFilePath &filename);

	virtual void initFolder() {}

signals:

	void filePathClicked(const TFilePath &);

public slots:

	void onOkPressed();
	void onApplyPressed();

protected:
	std::set<TFilePath> m_selectedPaths; //!< Paths in the active selection.
	TFilePath m_currentProjectPath;		 //!< Path of current project.
										 //!  \deprecated  Should not be cached...
	// keep TFrameId list in order to speed up the loading of sequencial files
	std::list<std::vector<TFrameId>> m_currentFIdsSet;
	FileBrowser *m_browser;

	QLabel *m_nameFieldLabel;
	DVGui::LineEdit *m_nameField;

	//QLabel*               m_fromFrameLabel;
	//LineEdit*	            m_fromFrame;
	//QLabel*               m_toFrameLabel;
	//LineEdit*             m_toFrame;

	QPushButton *m_okButton,
		*m_cancelButton;

	bool m_isDirectoryOnly;
	//bool                  m_checkFrameRange;
	bool m_multiSelectionEnabled;

	QSize m_dialogSize;
	QWidget *m_customWidget;

	TFilePath getCurrentPath()
	{
		if (m_selectedPaths.empty())
			return TFilePath();
		else
			return *m_selectedPaths.begin();
	}
	std::set<TFilePath> getCurrentPathSet()
	{
		return m_selectedPaths;
	}

	std::vector<TFrameId> getCurrentFIds()
	{
		std::vector<TFrameId> tmp;

		if (m_currentFIdsSet.empty())
			return tmp;
		else
			return *m_currentFIdsSet.begin();
	}
	std::list<std::vector<TFrameId>> getCurrentFIdsSet()
	{
		return m_currentFIdsSet;
	}

protected:
	/*!
    Performs some action on m_paths and returns whether the action was
    successful (accepted) - in which case the popup closes.
  */
	virtual bool execute() = 0;

	// change the "Apply" button for the browser in flipbook to "Append"
	virtual bool executeApply()
	{
		return execute();
	}

	void showEvent(QShowEvent *);
	void hideEvent(QHideEvent *);

protected slots:

	void onFilePathClicked(const TFilePath &);
	virtual void onFilePathsSelected(const std::set<TFilePath> &paths, const std::list<std::vector<TFrameId>> &fIds);
};

//********************************************************************************
//    FileBrowserPopup Heirs  declarations
//********************************************************************************

//! The GenericLoadFilePopup is a simple, standard Toonz popup that
//! asks the user for \a single file to be loaded from disk.
class GenericLoadFilePopup : public FileBrowserPopup
{
public:
	GenericLoadFilePopup(const QString &title);

	/*!
    This function shows the popup and blocks until an \a existing
    file has been selected - if the returned path is empty, the action
    was explicitly aborted by the user.
  */
	TFilePath getPath();

protected:
	bool execute();
};

//-----------------------------------------------------------------------------

//! The GenericSaveFilePopup is a simple, standard Toonz popup that
//! asks the user for a \a single file path to save something to.
class GenericSaveFilePopup : public FileBrowserPopup
{
public:
	GenericSaveFilePopup(const QString &title);

	/*!
    This function shows the popup and blocks until a suitable
    path has been selected - if the returned path is empty, the
    action was explicitly aborted by the user.

    \note The popup \a does ask for user consent to overwrite existing
          files.
  */
	TFilePath getPath();

protected:
	bool execute();
};

//-----------------------------------------------------------------------------

class LoadScenePopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	LoadScenePopup();

	bool execute();
	void initFolder();

	// change the initial path according to the current room
	void setInitialFolderByCurrentRoom();

protected:
	void showEvent(QShowEvent *);
};

//-----------------------------------------------------------------------------

class LoadSubScenePopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	LoadSubScenePopup();

	bool execute();
	void initFolder();

protected:
	void showEvent(QShowEvent *);
};

//-----------------------------------------------------------------------------

class SaveSceneAsPopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	SaveSceneAsPopup();

	bool execute();
	void initFolder();

#ifdef BRAVODEMO
	void show();
#endif
};

//-----------------------------------------------------------------------------

class SaveSubSceneAsPopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	SaveSubSceneAsPopup();

	bool execute();
	void initFolder();

#ifdef BRAVODEMO
	void show();
#endif
};

//-----------------------------------------------------------------------------

class LoadLevelPopup : public FileBrowserPopup
{
	Q_OBJECT

	QFrame *m_subsequenceFrame;
	DVGui::LineEdit *m_fromFrame;
	DVGui::LineEdit *m_toFrame;

	QFrame *m_arrangementFrame;
	DVGui::LineEdit *m_xFrom;
	DVGui::LineEdit *m_xTo;
	QComboBox *m_stepCombo;
	QComboBox *m_incCombo;
	DVGui::LineEdit *m_levelName;
	DVGui::LineEdit *m_posFrom;
	DVGui::LineEdit *m_posTo;

	QLabel *m_notExistLabel;
	QComboBox *m_loadTlvBehaviorComboBox;

public:
	LoadLevelPopup();

	bool execute();
	void initFolder();

protected:
	//reflect the current frame to GUI
	void showEvent(QShowEvent *);
	void hideEvent(QHideEvent *);

public slots:
	void onFilePathsSelected(const std::set<TFilePath> &paths, const std::list<std::vector<TFrameId>> &fIds);
	//if the frame switched, then move m_posFrom
	void onFrameSwitched();
	//if the from/to values in the subsequent box are changed, then reflect them to m_xFrom/m_xTo
	void onSubsequentFrameChanged();
	//update m_posTo
	void updatePosTo();
	//called when the file name is typed directly
	void onNameSetEditted(void);
	//if the x-sheet cells are selected, load levels at the upper-left corner of the selection
	void onSelectionChanged(TSelection *selection);

private:
	//update the fields acording to the current Path
	void updateBottomGUI(void);
	//if some option in the preferences is selected, load the level with removing
	//six letters of the scene name from the level name
	QString getLevelNameWithoutSceneNumber(std::string orgName);
};

//-----------------------------------------------------------------------------

class SaveLevelAsPopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	SaveLevelAsPopup();

	bool execute();
	void initFolder();

#ifdef BRAVODEMO
	void show();
#endif
};

//-----------------------------------------------------------------------------

class ReplaceLevelPopup : public FileBrowserPopup
{
	Q_OBJECT

	TCellSelection::Range m_range;
	bool m_replaceCells; // true : cell selection, false : column selection
	std::set<int> m_columnRange;

public:
	ReplaceLevelPopup();

	bool execute();
	void show();

	void initFolder();

protected slots:
	void onSelectionChanged(TSelection *selection);
};

//-----------------------------------------------------------------------------

class SavePaletteAsPopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	SavePaletteAsPopup();

	bool execute();

#ifdef BRAVODEMO
	void show();
#endif
	void initFolder();
};

//-----------------------------------------------------------------------------

class LoadColorModelPopup : public FileBrowserPopup
{
	Q_OBJECT

	DVGui::LineEdit *m_paletteFrame;

public:
	LoadColorModelPopup();

	bool execute();

protected:
	void showEvent(QShowEvent *);

protected slots:

	void onFilePathsSelected(const std::set<TFilePath> &paths);
};

//-----------------------------------------------------------------------------

class ImportMagpieFilePopup : public FileBrowserPopup
{
	Q_OBJECT

public:
	ImportMagpieFilePopup();

	bool execute();
	void initFolder();

protected:
	void showEvent(QShowEvent *);
};

//-----------------------------------------------------------------------------
/*! replace the parent folder path of the levels in the selected cells
*/

class ReplaceParentDirectoryPopup : public FileBrowserPopup
{
	TCellSelection::Range m_range;
	bool m_replaceCells; // true : cell selection, false : column selection
	std::set<int> m_columnRange;

public:
	ReplaceParentDirectoryPopup();
	void show();
	bool execute();
	void initFolder();
};

//-----------------------------------------------------------------------------

class BrowserPopup : public FileBrowserPopup
{
	Q_OBJECT

	TFilePath m_path;

public:
	BrowserPopup();

	const TFilePath &getPath() { return m_path; }

	bool execute();
	void initFolder(TFilePath path);
};

//-----------------------------------------------------------------------------

class BrowserPopupController : public QObject, public DVGui::FileField::BrowserPopupController
{
	Q_OBJECT

	BrowserPopup *m_browserPopup;
	bool m_isExecute;

public:
	BrowserPopupController();

	bool isExecute() { return m_isExecute; }
	void openPopup(QStringList filters, bool isDirectoryOnly, QString lastSelectedPath);
	QString getPath();
};

#endif //FILEBROWSERPOPUP_H
