
// TnzCore includes
#include "tconvert.h"
#include "tgeometry.h"
#include "tgl.h"
#include "trop.h"
#include "tstopwatch.h"
#include "tsystem.h"

// TnzLib includes
#include "toonz/tscenehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/tframehandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/toonzscene.h"
#include "toonz/sceneproperties.h"
#include "toonz/txsheet.h"
#include "toonz/stage.h"
#include "toonz/stage2.h"
#include "toonz/txshlevel.h"
#include "toonz/txshcell.h"
#include "toonz/tcamera.h"
#include "toonz/tstageobjecttree.h"
#include "toonz/tobjecthandle.h"
#include "toonz/tpalettehandle.h"
#include "toonz/tonionskinmaskhandle.h"
#include "toutputproperties.h"
#include "toonz/palettecontroller.h"
#include "toonz/toonzfolders.h"
#include "toonz/preferences.h"

// TnzQt includes
#include "toonzqt/menubarcommand.h"
#include "toonzqt/dvdialog.h"
#include "toonzqt/gutil.h"
#include "toonzqt/imageutils.h"
#include "toonzqt/flipconsole.h"

// TnzTools includes
#include "tools/toolhandle.h"
#include "tools/tooloptions.h"

// Tnz6 includes
#include "tapp.h"
#include "mainwindow.h"
#include "sceneviewer.h"
#include "xsheetdragtool.h"
#include "ruler.h"
#include "menubarcommandids.h"
#include "toolbar.h"

// Qt includes
#include <QPainter>
#include <QVBoxLayout>
#include <QAction>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QSlider>
#include <QButtonGroup>
#include <QMenu>
#include <QToolBar>
#include <QMainWindow>
#include <QSettings>

#include "comboviewerpane.h"

using namespace DVGui;

//=============================================================================
//
// ComboViewerPanel
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

#if QT_VERSION >= 0x050500
ComboViewerPanel::ComboViewerPanel(QWidget *parent, Qt::WindowFlags flags)
#else
ComboViewerPanel::ComboViewerPanel(QWidget *parent, Qt::WFlags flags)
#endif
	: TPanel(parent)
{
	TApp *app = TApp::instance();

	QFrame *hbox = new QFrame(this);
	hbox->setFrameStyle(QFrame::StyledPanel);
	hbox->setObjectName("ComboViewerPanel");

	//ToolBar
	m_toolbar = new Toolbar(this, false);
	//Tool Options
	m_toolOptions = new ToolOptions();
	m_toolOptions->setObjectName("ComboViewerToolOptions");

	//Viewer
	ImageUtils::FullScreenWidget *fsWidget = new ImageUtils::FullScreenWidget(this);
	fsWidget->setWidget(m_sceneViewer = new SceneViewer(fsWidget));

#ifdef Q_OS_WIN
	//  Workaround for QTBUG-48288
	//  This code should be removed after updating Qt.
	//  Qt may crash in handling WM_SIZE of m_sceneViewer in splash.finish(&w)
	//  in main.cpp. To suppress sending WM_SIZE, set window position here.
	//  WM_SIZE will not be sent if window poistion is not changed.
	::SetWindowPos(reinterpret_cast<HWND>(m_sceneViewer->winId()),
		HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#endif

	m_vRuler = new Ruler(this, m_sceneViewer, true);
	m_hRuler = new Ruler(this, m_sceneViewer, false);

	m_sceneViewer->setRulers(m_vRuler, m_hRuler);

	m_keyFrameButton = new ViewerKeyframeNavigator(this, app->getCurrentFrame());
	m_keyFrameButton->setObjectHandle(app->getCurrentObject());
	m_keyFrameButton->setXsheetHandle(app->getCurrentXsheet());

	//FlipConsole
	int buttons = FlipConsole::cFullConsole;
	buttons &= (~FlipConsole::eSound);
	buttons &= (~FlipConsole::eFilledRaster);
	buttons &= (~FlipConsole::eDefineLoadBox);
	buttons &= (~FlipConsole::eUseLoadBox);

	/* --- layout --- */
	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->setMargin(0);
	mainLayout->setSpacing(0);
	{
		mainLayout->addWidget(m_toolbar, 0);
		mainLayout->addWidget(m_toolOptions, 0);

		QGridLayout *viewerL = new QGridLayout();
		viewerL->setMargin(0);
		viewerL->setSpacing(0);
		{
			viewerL->addWidget(m_vRuler, 1, 0);
			viewerL->addWidget(m_hRuler, 0, 1);
			viewerL->addWidget(fsWidget, 1, 1);
		}
		mainLayout->addLayout(viewerL, 1);
		m_flipConsole = new FlipConsole(mainLayout, buttons, false, m_keyFrameButton, "SceneViewerConsole", this, true);
	}
	hbox->setLayout(mainLayout);
	setWidget(hbox);

	m_flipConsole->enableButton(FlipConsole::eMatte, false, false);
	m_flipConsole->enableButton(FlipConsole::eSave, false, false);
	m_flipConsole->enableButton(FlipConsole::eCompare, false, false);
	m_flipConsole->enableButton(FlipConsole::eSaveImg, false, false);
	m_flipConsole->enableButton(FlipConsole::eGRed, false, false);
	m_flipConsole->enableButton(FlipConsole::eGGreen, false, false);
	m_flipConsole->enableButton(FlipConsole::eGBlue, false, false);
	m_flipConsole->enableButton(FlipConsole::eSound, false, true);

	m_flipConsole->setFrameRate(app->getCurrentScene()->getScene()->getProperties()->getOutputProperties()->getFrameRate());
	m_flipConsole->setFrameHandle(TApp::instance()->getCurrentFrame());

	bool ret = true;

	// When zoom changed, only if the viewer is active, change window title.
	ret = connect(m_sceneViewer, SIGNAL(onZoomChanged()), SLOT(changeWindowTitle()));
	ret = ret && connect(m_sceneViewer, SIGNAL(previewToggled()), SLOT(changeWindowTitle()));
	ret = ret && connect(m_flipConsole, SIGNAL(playStateChanged(bool)),
						 TApp::instance()->getCurrentFrame(), SLOT(setPlaying(bool)));
	ret = ret && connect(m_flipConsole, SIGNAL(buttonPressed(FlipConsole::EGadget)),
						 m_sceneViewer, SLOT(onButtonPressed(FlipConsole::EGadget)));
	ret = ret && connect(m_sceneViewer, SIGNAL(previewStatusChanged()),
						 this, SLOT(update()));
	ret = ret && connect(app->getCurrentScene(), SIGNAL(sceneSwitched()),
						 this, SLOT(onSceneSwitched()));
	assert(ret);

	// note: initializeTitleBar() refers to m_sceneViewer
	initializeTitleBar(getTitleBar());

	//initial state of the parts
	m_visibleFlag[CVPARTS_TOOLBAR] = true;
	m_visibleFlag[CVPARTS_TOOLOPTIONS] = true;
	m_visibleFlag[CVPARTS_FLIPCONSOLE] = true;
	updateShowHide();
}

//-----------------------------------------------------------------------------
/*! toggle show/hide of the widgets according to m_visibleFlag
*/

void ComboViewerPanel::updateShowHide()
{
	//toolbar
	m_toolbar->setVisible(m_visibleFlag[CVPARTS_TOOLBAR]);
	//tool options bar
	m_toolOptions->setVisible(m_visibleFlag[CVPARTS_TOOLOPTIONS]);
	//flip console
	m_flipConsole->showHideAllParts(m_visibleFlag[CVPARTS_FLIPCONSOLE]);
	update();
}

//-----------------------------------------------------------------------------
/*! showing the show/hide commands
*/

void ComboViewerPanel::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = new QMenu(this);
	addShowHideContextMenu(menu);
	menu->exec(event->globalPos());
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::addShowHideContextMenu(QMenu *menu)
{

	QMenu *showHideMenu = menu->addMenu(tr("GUI Show / Hide"));
	//actions
	QAction *toolbarSHAct = showHideMenu->addAction(tr("Toolbar"));
	QAction *toolOptionsSHAct = showHideMenu->addAction(tr("Tool Options Bar"));
	QAction *flipConsoleSHAct = showHideMenu->addAction(tr("Console"));

	toolbarSHAct->setCheckable(true);
	toolbarSHAct->setChecked(m_visibleFlag[CVPARTS_TOOLBAR]);
	toolbarSHAct->setData((int)CVPARTS_TOOLBAR);

	toolOptionsSHAct->setCheckable(true);
	toolOptionsSHAct->setChecked(m_visibleFlag[CVPARTS_TOOLOPTIONS]);
	toolOptionsSHAct->setData((int)CVPARTS_TOOLOPTIONS);

	flipConsoleSHAct->setCheckable(true);
	flipConsoleSHAct->setChecked(m_visibleFlag[CVPARTS_FLIPCONSOLE]);
	flipConsoleSHAct->setData((int)CVPARTS_FLIPCONSOLE);

	QActionGroup *showHideActGroup = new QActionGroup(this);
	showHideActGroup->setExclusive(false);
	showHideActGroup->addAction(toolbarSHAct);
	showHideActGroup->addAction(toolOptionsSHAct);
	showHideActGroup->addAction(flipConsoleSHAct);

	connect(showHideActGroup, SIGNAL(triggered(QAction *)), this, SLOT(onShowHideActionTriggered(QAction *)));

	showHideMenu->addSeparator();
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewCamera));
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewTable));
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_FieldGuide));
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_SafeArea));
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewBBox));
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewColorcard));
	showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewRuler));
}

//-----------------------------------------------------------------------------
/*! slot function for show/hide the parts 
*/

void ComboViewerPanel::onShowHideActionTriggered(QAction *act)
{
	CV_Parts parts = (CV_Parts)act->data().toInt();
	assert(parts < CVPARTS_COUNT);

	if (m_visibleFlag[parts])
		m_visibleFlag[parts] = false;
	else
		m_visibleFlag[parts] = true;

	updateShowHide();
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::onDrawFrame(int frame, const ImagePainter::VisualSettings &settings)
{
	TApp *app = TApp::instance();
	m_sceneViewer->setVisual(settings);
	TFrameHandle *frameHandle = app->getCurrentFrame();

	if (m_sceneViewer->isPreviewEnabled()) {
		class Previewer *pr = Previewer::instance(m_sceneViewer->getPreviewMode() == SceneViewer::SUBCAMERA_PREVIEW);

		pr->getRaster(frame - 1); //the 'getRaster' starts the render of the frame is not already started
		int curFrame = frame;
		if (frameHandle->isPlaying() && !pr->isFrameReady(frame - 1)) //stops on last rendered frame until current is ready!
		{
			while (frame > 0 && !pr->isFrameReady(frame - 1))
				frame--;
			if (frame == 0)
				frame = curFrame; //if no frame is ready, I stay on current...no use to rewind
			m_flipConsole->setCurrentFrame(frame);
		}
	}

	assert(frame >= 0);
	if (frame != frameHandle->getFrameIndex() + 1) {
		if (frameHandle->isEditingScene()) {
			TXshColumn *column = app->getCurrentXsheet()->getXsheet()->getColumn(app->getCurrentColumn()->getColumnIndex());
			if (column) {
				TXshSoundColumn *soundColumn = column->getSoundColumn();
				if (soundColumn && !soundColumn->isPlaying())
					app->getCurrentFrame()->scrubColumn(frame, frame, soundColumn);
			}
		}
		frameHandle->setCurrentFrame(frame);
	}

	else if (settings.m_blankColor != TPixel::Transparent)
		m_sceneViewer->update();
}

//-----------------------------------------------------------------------------

ComboViewerPanel::~ComboViewerPanel()
{
	if (TApp::instance()->getInknPaintViewerPanel() == this)
		TApp::instance()->setInknPaintViewerPanel(0);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::showEvent(QShowEvent *)
{
	TApp *app = TApp::instance();
	TFrameHandle *frameHandle = app->getCurrentFrame();
	TSceneHandle *sceneHandle = app->getCurrentScene();
	TXshLevelHandle *levelHandle = app->getCurrentLevel();
	TObjectHandle *objectHandle = app->getCurrentObject();
	TXsheetHandle *xshHandle = app->getCurrentXsheet();

	bool ret = true;

	/*!
	onSceneChanged(): called when the scene changed
	- set new scene's FPS
	- update the range of frame slider with a new framehandle
	- set the marker
	- update key frames
	*/
	ret = connect(xshHandle, SIGNAL(xsheetChanged()), this, SLOT(onSceneChanged()));
	ret = ret && connect(sceneHandle, SIGNAL(sceneSwitched()), this, SLOT(onSceneChanged()));
	ret = ret && connect(sceneHandle, SIGNAL(sceneChanged()), this, SLOT(onSceneChanged()));

	/*!
	changeWindowTitle(): called when the scene / level / frame is changed
	- chenge the title text
	*/
	ret = ret && connect(sceneHandle, SIGNAL(nameSceneChanged()), this, SLOT(changeWindowTitle()));
	ret = ret && connect(levelHandle, SIGNAL(xshLevelChanged()), this, SLOT(changeWindowTitle()));
	ret = ret && connect(frameHandle, SIGNAL(frameSwitched()), this, SLOT(changeWindowTitle()));
	// onXshLevelSwitched(TXshLevel*)： changeWindowTitle() + updateFrameRange()
	ret = ret && connect(levelHandle, SIGNAL(xshLevelSwitched(TXshLevel *)),
						 this, SLOT(onXshLevelSwitched(TXshLevel *)));

	//updateFrameRange(): update the frame slider's range
	ret = ret && connect(levelHandle, SIGNAL(xshLevelChanged()), this, SLOT(updateFrameRange()));

	//onFrameTypeChanged(): reset the marker positions in the flip console
	ret = ret && connect(frameHandle, SIGNAL(frameTypeChanged()), this, SLOT(onFrameTypeChanged()));

	//onFrameChanged(): update the flipconsole according to the current frame
	ret = ret && connect(frameHandle, SIGNAL(frameSwitched()), this, SLOT(onFrameChanged()));

	ret = ret && connect(app->getCurrentTool(), SIGNAL(toolSwitched()), m_sceneViewer, SLOT(onToolSwitched()));
	ret = ret && connect(sceneHandle, SIGNAL(preferenceChanged()), m_flipConsole, SLOT(onPreferenceChanged()));
	m_flipConsole->onPreferenceChanged();

	assert(ret);

	m_flipConsole->setActive(true);

	//refresh
	onSceneChanged();
	changeWindowTitle();
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::hideEvent(QHideEvent *)
{
	TApp *app = TApp::instance();
	disconnect(app->getCurrentScene());
	disconnect(app->getCurrentLevel());
	disconnect(app->getCurrentFrame());
	disconnect(app->getCurrentObject());
	disconnect(app->getCurrentXsheet());
	disconnect(app->getCurrentTool());

	m_flipConsole->setActive(false);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::initializeTitleBar(TPanelTitleBar *titleBar)
{
	bool ret = true;

	TPanelTitleBarButtonSet *viewModeButtonSet;
	m_referenceModeBs = viewModeButtonSet = new TPanelTitleBarButtonSet();
	int x = -232;
	int iconWidth = 17;
	TPanelTitleBarButton *button;

	// buttons for show / hide toggle for the field guide and the safe area
	TPanelTitleBarButtonForSafeArea *safeAreaButton = new TPanelTitleBarButtonForSafeArea(
		titleBar,
		":Resources/safearea.png",
		":Resources/safearea_over.png",
		":Resources/safearea_on.png");
	safeAreaButton->setToolTip("Safe Area (Right Click to Select)");
	titleBar->add(QPoint(x, 1), safeAreaButton);
	ret = ret && connect(safeAreaButton, SIGNAL(toggled(bool)), CommandManager::instance()->getAction(MI_SafeArea), SLOT(trigger()));
	ret = ret && connect(CommandManager::instance()->getAction(MI_SafeArea), SIGNAL(triggered(bool)), safeAreaButton, SLOT(setPressed(bool)));
	//initialize state
	safeAreaButton->setPressed(CommandManager::instance()->getAction(MI_SafeArea)->isChecked());

	button = new TPanelTitleBarButton(
		titleBar,
		":Resources/fieldguide.png",
		":Resources/fieldguide_over.png",
		":Resources/fieldguide_on.png");
	button->setToolTip("Field Guide");
	x += 5 + iconWidth;
	titleBar->add(QPoint(x, 1), button);
	ret = ret && connect(button, SIGNAL(toggled(bool)), CommandManager::instance()->getAction(MI_FieldGuide), SLOT(trigger()));
	ret = ret && connect(CommandManager::instance()->getAction(MI_FieldGuide), SIGNAL(triggered(bool)), button, SLOT(setPressed(bool)));
	//initialize state
	button->setPressed(CommandManager::instance()->getAction(MI_FieldGuide)->isChecked());

	//view mode toggles
	button = new TPanelTitleBarButton(
		titleBar,
		":Resources/standard.png",
		":Resources/standard_over.png",
		":Resources/standard_on.png");
	button->setToolTip("Camera Stand View");
	x += 10 + iconWidth;
	titleBar->add(QPoint(x, 1), button);
	button->setButtonSet(viewModeButtonSet, SceneViewer::NORMAL_REFERENCE);
	button->setPressed(true);

	button = new TPanelTitleBarButton(
		titleBar,
		":Resources/3D.png",
		":Resources/3D_over.png",
		":Resources/3D_on.png");
	button->setToolTip("3D View");
	x += 19; // width of standard.png = 18pixels
	titleBar->add(QPoint(x, 1), button);
	button->setButtonSet(viewModeButtonSet, SceneViewer::CAMERA3D_REFERENCE);

	button = new TPanelTitleBarButton(
		titleBar,
		":Resources/view_camera.png",
		":Resources/view_camera_over.png",
		":Resources/view_camera_on.png");
	button->setToolTip("Camera View");
	x += 18; // width of 3D.png = 18pixels
	titleBar->add(QPoint(x, 1), button);
	button->setButtonSet(viewModeButtonSet, SceneViewer::CAMERA_REFERENCE);
	ret = ret && connect(viewModeButtonSet, SIGNAL(selected(int)), m_sceneViewer, SLOT(setReferenceMode(int)));

	//freeze button
	button = new TPanelTitleBarButton(
		titleBar,
		":Resources/freeze.png",
		":Resources/freeze_over.png",
		":Resources/freeze_on.png");
	x += 10 + 19; // width of viewcamera.png = 18pixels

	button->setToolTip("Freeze"); //RC1
	titleBar->add(QPoint(x, 1), button);
	ret = ret && connect(button, SIGNAL(toggled(bool)), m_sceneViewer, SLOT(freeze(bool)));

	//preview toggles
	m_previewButton = new TPanelTitleBarButton(
		titleBar,
		":Resources/viewpreview.png",
		":Resources/viewpreview_over.png",
		":Resources/viewpreview_on.png");
	x += 5 + iconWidth;
	titleBar->add(QPoint(x, 1), m_previewButton);
	m_previewButton->setToolTip(tr("Preview"));
	ret = ret && connect(m_previewButton, SIGNAL(toggled(bool)), SLOT(enableFullPreview(bool)));

	m_subcameraPreviewButton = new TPanelTitleBarButton(
		titleBar,
		":Resources/subcamera_preview.png",
		":Resources/subcamera_preview_over.png",
		":Resources/subcamera_preview_on.png");
	x += 28; // width of viewpreview.png =28pixels

	titleBar->add(QPoint(x, 1), m_subcameraPreviewButton);
	m_subcameraPreviewButton->setToolTip(tr("Sub-camera Preview"));
	ret = ret && connect(m_subcameraPreviewButton, SIGNAL(toggled(bool)), SLOT(enableSubCameraPreview(bool)));

	assert(ret);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::enableFullPreview(bool enabled)
{
	m_subcameraPreviewButton->setPressed(false);
	m_sceneViewer->enablePreview(enabled ? SceneViewer::FULL_PREVIEW : SceneViewer::NO_PREVIEW);
	m_flipConsole->setProgressBarStatus(&Previewer::instance(false)->getProgressBarStatus());
	enableFlipConsoleForCamerastand(enabled);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::enableSubCameraPreview(bool enabled)
{
	m_previewButton->setPressed(false);
	m_sceneViewer->enablePreview(enabled ? SceneViewer::SUBCAMERA_PREVIEW : SceneViewer::NO_PREVIEW);
	m_flipConsole->setProgressBarStatus(&Previewer::instance(true)->getProgressBarStatus());
	enableFlipConsoleForCamerastand(enabled);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::enableFlipConsoleForCamerastand(bool on)
{
	m_flipConsole->enableButton(FlipConsole::eMatte, on, false);
	m_flipConsole->enableButton(FlipConsole::eSave, on, false);
	m_flipConsole->enableButton(FlipConsole::eCompare, on, false);
	m_flipConsole->enableButton(FlipConsole::eSaveImg, on, false);
	m_flipConsole->enableButton(FlipConsole::eGRed, on, false);
	m_flipConsole->enableButton(FlipConsole::eGGreen, on, false);
	m_flipConsole->enableButton(FlipConsole::eGBlue, on, false);
	m_flipConsole->enableButton(FlipConsole::eBlackBg, on, false);
	m_flipConsole->enableButton(FlipConsole::eWhiteBg, on, false);
	m_flipConsole->enableButton(FlipConsole::eCheckBg, on, false);

	m_flipConsole->enableProgressBar(on);
	update();
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::onXshLevelSwitched(TXshLevel *)
{
	changeWindowTitle();
	m_sceneViewer->update();
	// If the level switched by using the level choose combo box in the film strip,
	// the current level switches without change in the frame type (level or scene).
	// For such case, update the frame range of the console here.
	if (TApp::instance()->getCurrentFrame()->isEditingLevel())
		updateFrameRange();
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::changeWindowTitle()
{
	TApp *app = TApp::instance();
	ToonzScene *scene = app->getCurrentScene()->getScene();
	if (!scene)
		return;
	int frame = app->getCurrentFrame()->getFrame();

	// put the titlebar texts in this string
	QString name;

	// if the frame type is "scene editing"
	if (app->getCurrentFrame()->isEditingScene()) {
		QString sceneName = QString::fromStdWString(scene->getSceneName());
		if (sceneName.isEmpty())
			sceneName = tr("Untitled");

		if (app->getCurrentScene()->getDirtyFlag())
			sceneName += QString(" *");
		name = tr("Scene: ") + sceneName;
		if (frame >= 0)
			name = name + tr("   ::   Frame: ") + tr(toString(frame + 1).c_str());
		int col = app->getCurrentColumn()->getColumnIndex();
		if (col < 0) {
			setWindowTitle(name);
			return;
		}
		TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
		TXshCell cell = xsh->getCell(frame, col);
		if (cell.isEmpty()) {
			setWindowTitle(name);
			return;
		}
		assert(cell.m_level.getPointer());
		TFilePath fp(cell.m_level->getName());
		QString imageName = QString::fromStdWString(fp.withFrame(cell.m_frameId).getWideString());
		name = name + tr("   ::   Level: ") + imageName;

		if (m_sceneViewer->isPreviewEnabled()) {
			name = name + "  ::  Zoom : " + QString::number((int)(100.0 * sqrt(m_sceneViewer->getViewMatrix().det()) * m_sceneViewer->getDpiFactor())) + "%";
		}

		// If the current level exists and some option is set in the preference,
		// set the zoom value to the current level's dpi
		else if (Preferences::instance()->isActualPixelViewOnSceneEditingModeEnabled() && TApp::instance()->getCurrentLevel()->getSimpleLevel() && !CleanupPreviewCheck::instance()->isEnabled() // cleanup preview must be OFF
				 && !CameraTestCheck::instance()->isEnabled())																																	 // camera test mode must be OFF neither
		{
			name = name + "  ::  Zoom : " + QString::number((int)(100.0 * sqrt(m_sceneViewer->getViewMatrix().det()) * m_sceneViewer->getDpiFactor())) + "%";
		}

	}
	// if the frame type is "level editing"
	else {
		TXshLevel *level = app->getCurrentLevel()->getLevel();
		if (level) {
			TFilePath fp(level->getName());
			QString imageName = QString::fromStdWString(fp.withFrame(app->getCurrentFrame()->getFid()).getWideString());

			name = name + tr("Level: ") + imageName;

			name = name + "  ::  Zoom : " + QString::number((int)(100.0 * sqrt(m_sceneViewer->getViewMatrix().det()) * m_sceneViewer->getDpiFactor())) + "%";
		}
	}
	setWindowTitle(name);
}

//-----------------------------------------------------------------------------
/*! update the frame range according to the current frame type
*/
void ComboViewerPanel::updateFrameRange()
{
	TFrameHandle *fh = TApp::instance()->getCurrentFrame();
	int frameIndex = fh->getFrameIndex();
	int maxFrameIndex = fh->getMaxFrameIndex();
	if (frameIndex > maxFrameIndex)
		frameIndex = maxFrameIndex;
	m_flipConsole->setFrameRange(1, maxFrameIndex + 1, 1, frameIndex + 1);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::onSceneChanged()
{
	TFrameHandle *fh = TApp::instance()->getCurrentFrame();
	int frameIndex = fh->getFrameIndex();
	int maxFrameIndex = fh->getMaxFrameIndex();
	if (frameIndex > maxFrameIndex)
		maxFrameIndex = frameIndex;

	// set the FPS for new scene
	m_flipConsole->setFrameRate(TApp::instance()->getCurrentScene()->getScene()->getProperties()->getOutputProperties()->getFrameRate());
	// update the frame slider's range with new frameHandle
	m_flipConsole->setFrameRange(1, maxFrameIndex + 1, 1, frameIndex + 1);

	// set the markers
	int fromIndex, toIndex, dummy;
	XsheetGUI::getPlayRange(fromIndex, toIndex, dummy);
	m_flipConsole->setMarkers(fromIndex, toIndex);

	// update the key frames
	if (m_keyFrameButton &&
		(m_keyFrameButton->getCurrentFrame() != frameIndex))
		m_keyFrameButton->setCurrentFrame(frameIndex);
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::onSceneSwitched()
{
	m_previewButton->setPressed(false);
	m_subcameraPreviewButton->setPressed(false);
	enableFlipConsoleForCamerastand(false);
	m_sceneViewer->enablePreview(SceneViewer::NO_PREVIEW);
	m_flipConsole->setChecked(FlipConsole::eDefineSubCamera, false);
	m_sceneViewer->setEditPreviewSubcamera(false);
	onSceneChanged();
}

//-----------------------------------------------------------------------------

void ComboViewerPanel::onFrameChanged()
{
	int frameIndex = TApp::instance()->getCurrentFrame()->getFrameIndex();
	m_flipConsole->setCurrentFrame(frameIndex + 1);
	if (m_keyFrameButton &&
		(m_keyFrameButton->getCurrentFrame() != frameIndex))
		m_keyFrameButton->setCurrentFrame(frameIndex);
}

//-----------------------------------------------------------------------------
/*! reset the marker positions in the flip console
*/
void ComboViewerPanel::onFrameTypeChanged()
{
	if (TApp::instance()->getCurrentFrame()->getFrameType() == TFrameHandle::LevelFrame &&
		m_sceneViewer->isPreviewEnabled()) {
		m_previewButton->setPressed(false);
		m_subcameraPreviewButton->setPressed(false);
		enableFlipConsoleForCamerastand(false);
		m_sceneViewer->enablePreview(SceneViewer::NO_PREVIEW);
	}
	m_flipConsole->setChecked(FlipConsole::eDefineSubCamera, false);
	m_sceneViewer->setEditPreviewSubcamera(false);

	updateFrameRange();

	// if in the scene editing mode, get the preview marker positions
	if (TApp::instance()->getCurrentFrame()->isEditingScene()) {
		//set the markers
		int fromIndex, toIndex, dummy;
		XsheetGUI::getPlayRange(fromIndex, toIndex, dummy);
		m_flipConsole->setMarkers(fromIndex, toIndex);
	}
	// if in the level editing mode, ignore the preview marker
	else
		m_flipConsole->setMarkers(0, -1);
}

//-----------------------------------------------------------------------------

bool ComboViewerPanel::isFrameAlreadyCached(int frame)
{
	if (m_sceneViewer->isPreviewEnabled()) {
		class Previewer *pr = Previewer::instance();
		return pr->isFrameReady(frame - 1);
	} else
		return true;
}

//-----------------------------------------------------------------------------
