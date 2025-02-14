

// Tnz6 includes
#include "mainwindow.h"
#include "flipbook.h"
#include "tapp.h"
#include "iocommand.h"
#include "previewfxmanager.h"
#include "cleanupsettingspopup.h"
#include "filebrowsermodel.h"

#ifdef LINETEST
#include "licensegui.h"
#include "licensecontroller.h"
#endif

// TnzTools includes
#include "tools/tool.h"
#include "tools/toolcommandids.h"

// TnzQt includes
#include "toonzqt/dvdialog.h"
#include "toonzqt/menubarcommand.h"
#include "toonzqt/tmessageviewer.h"
#include "toonzqt/icongenerator.h"
#include "toonzqt/gutil.h"
#include "toonzqt/pluginloader.h"
//iwsw commented out temporarily
//#include "toonzqt/ghibli_3dlut_util.h"

// TnzStdfx includes
#include "stdfx/shaderfx.h"

// TnzLib includes
#include "toonz/preferences.h"
#include "toonz/toonzfolders.h"
#include "toonz/tproject.h"
#include "toonz/studiopalette.h"
#include "toonz/stylemanager.h"
#include "toonz/tscenehandle.h"
#include "toonz/txshsimplelevel.h"

// TnzSound includes
#include "tnzsound.h"

// TnzImage includes
#include "tnzimage.h"

// TnzBase includes
#include "permissionsmanager.h"
#include "tenv.h"
#include "tcli.h"

// TnzCore includes
#include "tsystem.h"
#include "tthread.h"
#include "tthreadmessage.h"
#include "tundo.h"
#include "tconvert.h"
#include "tiio_std.h"
#include "timagecache.h"
#include "tofflinegl.h"
#include "tpluginmanager.h"
#include "tsimplecolorstyles.h"
#include "toonz/imagestyles.h"
#include "tvectorbrushstyle.h"

#ifdef MACOSX
#include "tipc.h"
#endif

// Qt includes
#include <QApplication>
#include <QSplashScreen>
#include <QGLPixelBuffer>
#include <QTranslator>
#include <QFileInfo>
#include <QSettings>

using namespace DVGui;
#if defined LINETEST
const char *applicationName = "Toonz LineTest";
const char *applicationVersion = "6.4";
const char *dllRelativePath = "./linetest.app/Contents/Frameworks";
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "MS Sans Serif");
TEnv::IntVar EnvSoftwareCurrentFontSize("SoftwareCurrentFontSize", 12);
const char *applicationFullName = "LineTest 6.4 Beta";
const char *rootVarName = "LINETESTROOT";
const char *systemVarPrefix = "LINETEST";
#elif defined BRAVODEMO
const char *applicationName = "Toonz Bravo";
const char *applicationVersion = "6.5";
const char *dllRelativePath = "./bravo6.app/Contents/Frameworks";
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "MS Sans Serif");
TEnv::IntVar EnvSoftwareCurrentFontSize("SoftwareCurrentFontSize", 12);
const char *applicationFullName = "Bravo 6.5 (Demo) Aperta";
const char *rootVarName = "BRAVOROOT";
const char *systemVarPrefix = "BRAVO";
#elif defined BRAVO
#ifdef BRAVOEXPRESS
const char *applicationName = "Toonz Bravo Express";
const char *applicationVersion = "7.1";
const char *dllRelativePath = "./bravo6.app/Contents/Frameworks";
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "MS Sans Serif");
TEnv::IntVar EnvSoftwareCurrentFontSize("SoftwareCurrentFontSize", 12);
const char *applicationFullName = "Bravo Express 7.1";
const char *rootVarName = "BRAVOEXPRESSROOT";
const char *systemVarPrefix = "BRAVOEXPRESS";
#else
const char *applicationName = "Toonz Bravo";
const char *applicationVersion = "7.1";
const char *dllRelativePath = "./bravo6.app/Contents/Frameworks";
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "MS Sans Serif");
TEnv::IntVar EnvSoftwareCurrentFontSize("SoftwareCurrentFontSize", 12);
const char *applicationFullName = "Bravo 7.1";
const char *rootVarName = "BRAVOROOT";
const char *systemVarPrefix = "BRAVO";
#endif
#elif defined XPRESS
const char *applicationName = "Toonz Xpress";
const char *applicationVersion = "6.5";
const char *dllRelativePath = "./xpress.app/Contents/Frameworks";
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "MS Sans Serif");
TEnv::IntVar EnvSoftwareCurrentFontSize("SoftwareCurrentFontSize", 12);
const char *applicationFullName = "Express 6.5 Aperta";
const char *rootVarName = "XPRESSROOT";
const char *systemVarPrefix = "XPRESS";

#else

const char *applicationName = "OpenToonz";
const char *applicationVersion = "1.0";
const char *dllRelativePath = "./toonz6.app/Contents/Frameworks";

#ifdef _WIN32
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "Arial");
#else
TEnv::StringVar EnvSoftwareCurrentFont("SoftwareCurrentFont", "Hervetica");
#endif
TEnv::IntVar EnvSoftwareCurrentFontSize("SoftwareCurrentFontSize", 12);
TEnv::StringVar EnvSoftwareCurrentFontWeight("SoftwareCurrentFontWeightIsBold", "Yes");

const char *applicationFullName = "OpenToonz 1.0";
const char *rootVarName = "TOONZROOT";
const char *systemVarPrefix = "TOONZ";
#endif
TCli::StringQualifier selectedFeature("-license", "");

#ifdef MACOSX
#include "tthread.h"
void postThreadMsg(TThread::Message *)
{
}
void qt_mac_set_menubar_merge(bool enable);
#endif

//Modifica per toonz (non servono questo tipo di licenze)
#define NO_LICENSE
//-----------------------------------------------------------------------------

void fatalError(QString msg)
{
	DVGui::MsgBoxInPopup(CRITICAL, msg + "\n" + QObject::tr("Installing %1 again could fix the problem.").arg(applicationFullName));
	exit(0);
}
//-----------------------------------------------------------------------------

void lastWarningError(QString msg)
{
	DVGui::error(msg);
	//exit(0);
}
//-----------------------------------------------------------------------------

class LicenseMsg : public TThread::Message
{
	QString m_msg;

public:
	LicenseMsg(char *msg) : m_msg(msg) {}
	void onDeliver()
	{
		lastWarningError(m_msg);
	}

	TThread::Message *clone() const
	{
		return new LicenseMsg(*this);
	}
};

extern "C" void licenseObserverCB(char *msg)
{
	std::cout << __FUNCTION__ << " Begin message>" << std::endl;
	std::cout << msg << std::endl
			  << "<End Message" << std::endl;
	LicenseMsg(msg).send();
}

//-----------------------------------------------------------------------------

void toonzRunOutOfContMemHandler(unsigned long size)
{
#ifdef _WIN32
	static bool firstTime = true;
	if (firstTime) {
		MessageBox(NULL, (LPCWSTR)L"Run out of contiguous physical memory: please save all and restart Toonz!",
				   (LPCWSTR)L"Warning", MB_OK | MB_SYSTEMMODAL);
		firstTime = false;
	}
#endif
}

//-----------------------------------------------------------------------------

// todo.. da mettere in qualche .h
#ifndef LINETEST
DV_IMPORT_API void initStdFx();
#endif
DV_IMPORT_API void initColorFx();

//-----------------------------------------------------------------------------

//! Inizializzaza l'Environment di Toonz
/*! In particolare imposta la projectRoot e
    la stuffDir, controlla se la directory di outputs esiste (e provvede a
    crearla in caso contrario) verifica inoltre che stuffDir esista.
*/
void initToonzEnv()
{
	StudioPalette::enable(true);

#ifdef MACOSX

// Stuff dir deve essere assoluta (la rendo tale)

#if defined BRAVODEMO
	QFileInfo infoStuff(QString("Toonz 7.1 Bravo stuff"));
#elif defined BRAVO
#ifdef BRAVOEXPRESS
	QFileInfo infoStuff(QString("Toonz 7.1 Bravo Express stuff"));
#else
	QFileInfo infoStuff(QString("Toonz 7.1 Bravo stuff"));
#endif
#elif defined XPRESS
	QFileInfo infoStuff(QString("Toonz 7.1 Xpress stuff"));
#elif defined LINETEST
	QFileInfo infoStuff(QString("Toonz 6.4 LineTest stuff"));
#else
	QFileInfo infoStuff(QString("Toonz 7.1 stuff"));
#endif

#endif

	TEnv::setApplication(applicationName, applicationVersion);
	TEnv::setRootVarName(rootVarName);
	TEnv::setSystemVarPrefix(systemVarPrefix);
	TEnv::setDllRelativeDir(TFilePath(dllRelativePath));

	QCoreApplication::setOrganizationName("OpenToonz");
	QCoreApplication::setOrganizationDomain("");
	QString fullApplicationNameQStr = QString(applicationName) + " " + applicationVersion;
	QCoreApplication::setApplicationName(fullApplicationNameQStr);

	/*-- TOONZROOTのPathの確認 --*/
	// controllo se la xxxroot e' definita e corrisponde ad un folder esistente
	TFilePath stuffDir = TEnv::getStuffDir();
	if (stuffDir == TFilePath() || !TFileStatus(stuffDir).isDirectory()) {
		if (stuffDir == TFilePath())
			fatalError("Undefined or empty: \"" + toQString(TEnv::getRootVarPath()) + "\"");
		else
			fatalError("Folder \"" + toQString(stuffDir) + "\" not found or not readable");
	}

	//  TProjectManager::instance()->enableSimpleProject(true);

	//! Inizializzazione Librerie di Toonz.
	/*! Inizializza le plugins di toonz, imposta la rootDir per
    ImagePatternStrokeStyle e per ImageCache, setta i folder del progetto,
    imposta lo Stencil Buffer Context (QT), e l'offlineGL da utilizzare
*/

	Tiio::defineStd();
	initImageIo();
	initSoundIo();
#ifndef LINETEST
	initStdFx();
#endif
	initColorFx();

	// TPluginManager::instance()->loadStandardPlugins();

	TFilePath library = ToonzFolder::getLibraryFolder();

	TRasterImagePatternStrokeStyle::setRootDir(library);
	TVectorImagePatternStrokeStyle::setRootDir(library);
	TVectorBrushStyle::setRootDir(library);

	CustomStyleManager::setRootPath(library);

	// sembra indispensabile nella lettura dei .tab 2.2:
	TPalette::setRootDir(library);
	TImageStyle::setLibraryDir(library);

	//TProjectManager::instance()->enableTabMode(true);

	TProjectManager *projectManager = TProjectManager::instance();

	/*-- TOONZPROJECTSのパスセットを取得する。（TOONZPROJECTSはセミコロンで区切って複数設定可能） --*/
	TFilePathSet projectsRoots = ToonzFolder::getProjectsFolders();
	TFilePathSet::iterator it;
	for (it = projectsRoots.begin(); it != projectsRoots.end(); ++it)
		projectManager->addProjectsRoot(*it);

	/*-- もしまだ無ければ、TOONZROOT/sandboxにsandboxプロジェクトを作る --*/
	projectManager->createSandboxIfNeeded();

	/*
  TProjectP project = projectManager->getCurrentProject();
  Non dovrebbe servire per Tab:
  project->setFolder(TProject::Drawings, TFilePath("$scenepath"));
  project->setFolder(TProject::Extras, TFilePath("$scenepath"));
  project->setUseScenePath(TProject::Drawings, false);
  project->setUseScenePath(TProject::Extras, false);
*/
	// Imposto la rootDir per ImageCache

	/*-- TOONZCACHEROOTの設定  --*/
	TFilePath cacheDir = ToonzFolder::getCacheRootFolder();
	if (cacheDir.isEmpty())
		cacheDir = TEnv::getStuffDir() + "cache";
	TImageCache::instance()->setRootDir(cacheDir);

	DV_IMPORT_API void initializeImageRasterizer();
	initializeImageRasterizer();
}

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
	//  Enable standard input/output on Windows Platform for debug
	BOOL consoleAttached = ::AttachConsole(ATTACH_PARENT_PROCESS);
	if (consoleAttached) {
		freopen("CON", "r", stdin);
		freopen("CON", "w", stdout);
		freopen("CON", "w", stderr);
	}
#endif

	/*-- "-layout [レイアウト設定ファイル名]" で、必要なモジュールのPageだけのレイアウトで起動することを可能にする --*/
	QString argumentLayoutFileName = "";
	TFilePath loadScenePath;
	if (argc > 1) {
		for (int a = 1; a < argc; a++) {
			if (QString(argv[a]) == "-layout") {
				argumentLayoutFileName = QString(argv[a + 1]);
				a++;
			} else
				loadScenePath = TFilePath(argv[a]);
		}
	}

	QApplication a(argc, argv);

#ifdef Q_OS_WIN
	//	Since currently OpenToonz does not work with OpenGL of software or angle,
	//	force Qt to use desktop OpenGL
	a.setAttribute(Qt::AA_UseDesktopOpenGL, true);
#endif

	// Some Qt objects are destroyed badly withouth a living qApp. So, we must enforce a way to either
	// postpone the application destruction until the very end, OR ensure that sensible objects are
	// destroyed before.

	// Using a static QApplication only worked on Windows, and in any case C++ respects the statics destruction
	// order ONLY within the same library. On MAC, it made the app crash on exit o_o. So, nope.

	std::auto_ptr<QObject> mainScope(new QObject(&a)); // A QObject destroyed before the qApp is therefore explicitly
	mainScope->setObjectName("mainScope");			   // provided. It can be accessed by looking in the qApp's children.

#ifdef _WIN32
#ifndef x64
	//Store the floating point control word. It will be re-set before Toonz initialization
	//has ended.
	unsigned int fpWord = 0;
	_controlfp_s(&fpWord, 0, 0);
#endif
#endif

#ifdef _WIN32
	//At least on windows, Qt's 4.5.2 native windows feature tend to create
	//weird flickering effects when dragging panel separators.
	a.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif

	//Set the app's locale for numeric stuff to standard C. This is important for atof() and similar
	//calls that are locale-dependant.
	setlocale(LC_NUMERIC, "C");

// Set current directory to the bundle/application path - this is needed to have correct relative paths
#ifdef MACOSX
	{
		QDir appDir(QApplication::applicationDirPath());
		appDir.cdUp(), appDir.cdUp(), appDir.cdUp();

		bool ret = QDir::setCurrent(appDir.absolutePath());
		assert(ret);
	}
#endif

	// splash screen
	QPixmap splashPixmap(":Resources/splash.png");
#ifdef _WIN32
	a.setFont(QFont("Arial", 10));
#else
	a.setFont(QFont("Helvetica", 10));
#endif

	QString offsetStr("\n\n\n\n\n\n\n\n");

	TSystem::hasMainLoop(true);

	TMessageRepository::instance();

	QSplashScreen splash(splashPixmap);
	splash.show();
	a.processEvents();

	splash.showMessage(offsetStr + "Initializing QGLFormat...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	// OpenGL
	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setStencil(true);
	QGLFormat::setDefaultFormat(fmt);

	splash.showMessage(offsetStr + "Initializing Toonz environment ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	//Install run out of contiguous memory callback
	TBigMemoryManager::instance()->setRunOutOfContiguousMemoryHandler(&toonzRunOutOfContMemHandler);

	// Toonz environment
	initToonzEnv();

	// Initialize thread components
	TThread::init();

	std::string feature = selectedFeature.getValue();

	TProjectManager *projectManager = TProjectManager::instance();
	if (Preferences::instance()->isSVNEnabled()) {
		// Read Version Control repositories and add it to project manager as "special" svn project root
		VersionControl::instance()->init();
		QList<SVNRepository> repositories = VersionControl::instance()->getRepositories();
		int count = repositories.size();
		for (int i = 0; i < count; i++) {
			SVNRepository r = repositories.at(i);

			TFilePath localPath(r.m_localPath.toStdWString());
			if (!TFileStatus(localPath).doesExist()) {
				try {
					TSystem::mkDir(localPath);
				} catch (TException &e) {
					fatalError(QString::fromStdWString(e.getMessage()));
				}
			}
			projectManager->addSVNProjectsRoot(localPath);
		}
	}

#if defined(MACOSX) && defined(__LP64__)

	//Load the shared memory settings
	int shmmax = Preferences::instance()->getShmMax();
	int shmseg = Preferences::instance()->getShmSeg();
	int shmall = Preferences::instance()->getShmAll();
	int shmmni = Preferences::instance()->getShmMni();

	if (shmall < 0) //Make sure that at least 100 MB of shared memory are available
		shmall = (tipc::shm_maxSharedPages() < (100 << 8)) ? (100 << 8) : -1;

	tipc::shm_set(shmmax, shmseg, shmall, shmmni);

#endif

	// DVDirModel must be instantiated after Version Control initialization...
	FolderListenerManager::instance()->addListener(DvDirModel::instance());

	splash.showMessage(offsetStr + "Loading Translator ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	// Carico la traduzione contenuta in toonz.qm (se ï¿½ presente)
	QString languagePathString = QString::fromStdString(toString(TEnv::getConfigDir() + "loc"));
#ifdef MACOSX
	//the merge of menu on osx can cause problems with different languages with the Preferences menu
	//qt_mac_set_menubar_merge(false);
	languagePathString += "/" + Preferences::instance()->getCurrentLanguage();
#else
	languagePathString += "\\" + Preferences::instance()->getCurrentLanguage();
#endif
	QTranslator translator;
#ifdef LINETEST
	translator.load("linetest", languagePathString);
#else
	translator.load("toonz", languagePathString);
#endif
	// La installo
	a.installTranslator(&translator);

	// Carico la traduzione contenuta in toonzqt.qm (se e' presente)
	QTranslator translator2;
	translator2.load("toonzqt", languagePathString);
	a.installTranslator(&translator2);

	// Carico la traduzione contenuta in tnzcore.qm (se e' presente)
	QTranslator tnzcoreTranslator;
	tnzcoreTranslator.load("tnzcore", languagePathString);
	qApp->installTranslator(&tnzcoreTranslator);

	// Carico la traduzione contenuta in toonzlib.qm (se e' presente)
	QTranslator toonzlibTranslator;
	toonzlibTranslator.load("toonzlib", languagePathString);
	qApp->installTranslator(&toonzlibTranslator);

	// Carico la traduzione contenuta in colorfx.qm (se e' presente)
	QTranslator colorfxTranslator;
	colorfxTranslator.load("colorfx", languagePathString);
	qApp->installTranslator(&colorfxTranslator);

	// Carico la traduzione contenuta in tools.qm
	QTranslator toolTranslator;
	toolTranslator.load("tnztools", languagePathString);
	qApp->installTranslator(&toolTranslator);

	// Aggiorno la traduzione delle properties di tutti i tools
	TTool::updateToolsPropertiesTranslation();

	splash.showMessage(offsetStr + "Loading styles ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	// stile
	QApplication::setStyle("windows");

	IconGenerator::setFilmstripIconSize(Preferences::instance()->getIconSize());

	splash.showMessage(offsetStr + "Loading shaders ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	loadShaderInterfaces(ToonzFolder::getLibraryFolder() + TFilePath("shaders"));

	splash.showMessage(offsetStr + "Initializing Toonz application ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	TTool::setApplication(TApp::instance());
	TApp::instance()->init();

//iwsw commented out temporarily
#if 0 
  QStringList monitorNames;
  /*-- 接続モニタがPVM-2541の場合のみLUTを読み込む --*/
  if (Preferences::instance()->isDoColorCorrectionByUsing3DLutEnabled())
  {
	  /*-- 接続モニタがPVM-2541の場合のみLUTを読み込む --*/
	  monitorNames = Ghibli3DLutUtil::getMonitorName();
	  if (monitorNames.contains(QString::fromStdWString(L"PVM-2541")))
		  /*-- 3DLUTファイルを読み込む --*/
		  Ghibli3DLutUtil::loadLutFile(Preferences::instance()->get3DLutPath());
  }
  /*-- 接続モニタをスプラッシュ画面にも表示 --*/
  if (!monitorNames.isEmpty())
  {
	  lastUpdateStr += QString("Monitor Name : ");
	  for (int mn = 0; mn < monitorNames.size(); mn++)
	  {
		  if (mn != 0)
			  lastUpdateStr += QString(", ");
		  lastUpdateStr += monitorNames.at(mn);
	  }
	  lastUpdateStr += QString("\n");
  }
#endif

	splash.showMessage(offsetStr + "Loading Plugins...", Qt::AlignCenter, Qt::white);
	a.processEvents();
	/* poll the thread ends: 
	 絶対に必要なわけではないが PluginLoader は中で setup ハンドラが常に固有のスレッドで呼ばれるよう main thread queue の blocking をしているので
	 processEvents を行う必要がある
   */
	while (!PluginLoader::load_entries("")) {
		a.processEvents();
	}

	splash.showMessage(offsetStr + "Creating main window ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	/*-- Layoutファイル名をMainWindowのctorに渡す --*/
	MainWindow w(argumentLayoutFileName);

	splash.showMessage(offsetStr + "Loading style sheet ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	// Carico lo styleSheet
	QString currentStyle = Preferences::instance()->getCurrentStyleSheet();
	a.setStyleSheet(currentStyle);

	TApp::instance()->setMainWindow(&w);
	w.setWindowTitle(applicationFullName);

	splash.showMessage(offsetStr + "Starting main window ...", Qt::AlignCenter, Qt::white);
	a.processEvents();

	TFilePath fp = ToonzFolder::getModuleFile("mainwindow.ini");
	QSettings settings(toQString(fp), QSettings::IniFormat);
	w.restoreGeometry(settings.value("MainWindowGeometry").toByteArray());

#ifndef MACOSX
	//Workaround for the maximized window case: Qt delivers two resize events, one in the normal geometry, before
	//maximizing (why!?), the second afterwards - all inside the following show() call. This makes troublesome for
	//the docking system to correctly restore the saved geometry. Fortunately, MainWindow::showEvent(..) gets called
	//just between the two, so we can disable the currentRoom layout right before showing and re-enable it after
	//the normal resize has happened.
	if (w.isMaximized())
		w.getCurrentRoom()->layout()->setEnabled(false);
#endif

	QRect splashGeometry = splash.geometry();
	splash.finish(&w);

	a.setQuitOnLastWindowClosed(false);
// a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
/*
	int *check;
	check = License::checkLicense();
	*check = 3;*/

#ifdef LINETEST
	std::string license = License::getInstalledLicense();
	License::writeMacAddress();
	//Controllo che la data della prima installazione contenuta nella sentinella sia minore della data corrente.
	if (License::isTemporaryLicense(license) && !License::isValidSentinel(license)) {
		DVGui::error(QObject::tr("System date tampered."));
		return -1;
	}
	if (License::checkLicense(license) == License::INVALID_LICENSE ||
		(License::isTemporaryLicense(license) && License::getDaysLeft(license) < 31)) {
		LicenseWizard licenseWizard(0);
		licenseWizard.setGeometry(splashGeometry);
		//UpgradeLicense upgradeLicense(&w);
		//int upgraded = upgradeLicense.exec();
		int ret = licenseWizard.exec();
		if (ret == QDialog::Rejected)
			return -1;
	}

	w.checkForLicense();
#endif
	w.checkForUpdates();

	w.show();

	//Show floating panels only after the main window has been shown
	w.startupFloatingPanels();

	CommandManager::instance()->execute(T_Hand);
	if (!loadScenePath.isEmpty()) {
		splash.showMessage(QString("Loading file '") + loadScenePath.getQString() + "'...", Qt::AlignCenter, Qt::white);

		loadScenePath = loadScenePath.withType("tnz");
		if (TFileStatus(loadScenePath).doesExist())
			IoCmd::loadScene(loadScenePath);
	}

	QFont *myFont;

	std::string family = EnvSoftwareCurrentFont;
	myFont = new QFont(QString(family.c_str()));

	myFont->setPixelSize(EnvSoftwareCurrentFontSize);
	/*-- フォントのBoldの指定 --*/
	std::string weight = EnvSoftwareCurrentFontWeight;
	if (strcmp(weight.c_str(), "Yes") == 0)
		myFont->setBold(true);
	else
		myFont->setBold(false);
	a.setFont(*myFont);

	QAction *action = CommandManager::instance()->getAction("MI_OpenTMessage");
	if (action)
		QObject::connect(TMessageRepository::instance(), SIGNAL(openMessageCenter()), action, SLOT(trigger()));

	QObject::connect(
		TUndoManager::manager(), SIGNAL(somethingChanged()),
		TApp::instance()->getCurrentScene(), SLOT(setDirtyFlag()));

#ifdef _WIN32
#ifndef x64
	//On 32-bit architecture, there could be cases in which initialization could alter the
	//FPU floating point control word. I've seen this happen when loading some AVI coded (VFAPI),
	//where 80-bit internal precision was used instead of the standard 64-bit (much faster and
	//sufficient - especially considering that x86 truncates to 64-bit representation anyway).
	//IN ANY CASE, revert to the original control word.
	//In the x64 case these precision changes simply should not take place up to _controlfp_s
	//documentation.
	_controlfp_s(0, fpWord, -1);
#endif
#endif

	a.installEventFilter(TApp::instance());

	int ret = a.exec();

	TUndoManager::manager()->reset();
	PreviewFxManager::instance()->reset();

#ifdef _WIN32
	if (consoleAttached) {
		::FreeConsole();
	}
#endif

	return ret;
}
