

#include "levelcreatepopup.h"

// Tnz6 includes
#include "menubarcommandids.h"
#include "tapp.h"

// TnzTools includes
#include "tools/toolhandle.h"

// TnzQt includes
#include "toonzqt/menubarcommand.h"
#include "toonzqt/gutil.h"
#include "toonzqt/doublefield.h"
#include "historytypes.h"

// TnzLib includes
#include "toonz/toonzscene.h"
#include "toonz/txsheet.h"
#include "toonz/txshcell.h"
#include "toonz/txshsimplelevel.h"
#include "toonz/txshleveltypes.h"
#include "toonz/levelset.h"
#include "toonz/levelproperties.h"
#include "toonz/sceneproperties.h"
#include "toonz/tcamera.h"
#include "toonz/tframehandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/tscenehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/tpalettehandle.h"
#include "toonz/preferences.h"
#include "toonz/palettecontroller.h"
#include "toonz/tproject.h"

// TnzCore includes
#include "tsystem.h"
#include "tpalette.h"
#include "tvectorimage.h"
#include "trasterimage.h"
#include "ttoonzimage.h"
#include "timagecache.h"
#include "tundo.h"
#include "filebrowsermodel.h"

// Qt includes
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMainWindow>

using namespace DVGui;

//=============================================================================
namespace
{
//-----------------------------------------------------------------------------

const QString VectorLevel("Toonz Vector Level");
const QString ToonzLevel("Toonz Raster Level");
const QString RasterLevel("Raster Level");
const QString ScanLevel("Scan Level");

//=============================================================================
// CreateLevelUndo
//-----------------------------------------------------------------------------

class CreateLevelUndo : public TUndo
{
	int m_rowIndex;
	int m_columnIndex;
	int m_frameCount;
	int m_oldLevelCount;
	int m_step;
	TXshSimpleLevelP m_sl;
	bool m_areColumnsShifted;

public:
	CreateLevelUndo(int row, int column, int frameCount, int step, bool areColumnsShifted)
		: m_rowIndex(row), m_columnIndex(column), m_frameCount(frameCount), m_step(step), m_sl(0), m_areColumnsShifted(areColumnsShifted)
	{
		TApp *app = TApp::instance();
		ToonzScene *scene = app->getCurrentScene()->getScene();
		m_oldLevelCount = scene->getLevelSet()->getLevelCount();
	}
	~CreateLevelUndo()
	{
		m_sl = 0;
	}

	void onAdd(TXshSimpleLevelP sl)
	{
		m_sl = sl;
	}

	void undo() const
	{
		TApp *app = TApp::instance();
		ToonzScene *scene = app->getCurrentScene()->getScene();
		TXsheet *xsh = scene->getXsheet();
		if (m_areColumnsShifted)
			xsh->removeColumn(m_columnIndex);
		else if (m_frameCount > 0)
			xsh->removeCells(m_rowIndex, m_columnIndex, m_frameCount);

		TLevelSet *levelSet = scene->getLevelSet();
		if (levelSet) {
			int m = levelSet->getLevelCount();
			while (m > 0 && m > m_oldLevelCount) {
				--m;
				TXshLevel *level = levelSet->getLevel(m);
				if (level)
					levelSet->removeLevel(level);
			}
		}
		app->getCurrentScene()->notifySceneChanged();
		app->getCurrentScene()->notifyCastChange();
		app->getCurrentXsheet()->notifyXsheetChanged();
	}

	void redo() const
	{
		if (!m_sl.getPointer())
			return;
		TApp *app = TApp::instance();
		ToonzScene *scene = app->getCurrentScene()->getScene();
		scene->getLevelSet()->insertLevel(m_sl.getPointer());
		TXsheet *xsh = scene->getXsheet();
		if (m_areColumnsShifted)
			xsh->insertColumn(m_columnIndex);
		std::vector<TFrameId> fids;
		m_sl->getFids(fids);
		int i = m_rowIndex;
		int f = 0;
		while (i < m_frameCount + m_rowIndex) {
			TFrameId fid = (fids.size() != 0) ? fids[f] : i;
			TXshCell cell(m_sl.getPointer(), fid);
			f++;
			xsh->setCell(i, m_columnIndex, cell);
			int appo = i++;
			while (i < m_step + appo)
				xsh->setCell(i++, m_columnIndex, cell);
		}
		app->getCurrentScene()->notifySceneChanged();
		app->getCurrentScene()->notifyCastChange();
		app->getCurrentXsheet()->notifyXsheetChanged();
	}

	int getSize() const
	{
		return sizeof *this;
	}
	QString getHistoryString()
	{
		return QObject::tr("Create Level %1  at Column %2")
			.arg(QString::fromStdWString(m_sl->getName()))
			.arg(QString::number(m_columnIndex + 1));
	}
};

//-----------------------------------------------------------------------------
} //anonymous namespace
//-----------------------------------------------------------------------------

//=============================================================================
/*! \class LevelCreatePopup
		\brief The LevelCreatePopup class provides a modal dialog to create a new level.

		Inherits \b Dialog.
*/
//-----------------------------------------------------------------------------

LevelCreatePopup::LevelCreatePopup()
	: Dialog(TApp::instance()->getMainWindow(), true, true, "LevelCreate")
{
	setWindowTitle(tr("New Level"));

	m_nameFld = new LineEdit(this);
	m_fromFld = new DVGui::IntLineEdit(this);
	m_toFld = new DVGui::IntLineEdit(this);
	m_stepFld = new DVGui::IntLineEdit(this);
	m_incFld = new DVGui::IntLineEdit(this);
	m_levelTypeOm = new QComboBox();

	m_pathFld = new FileField(0);
	m_widthLabel = new QLabel(tr("Width:"));
	m_widthFld = new DVGui::MeasuredDoubleLineEdit(0);
	m_heightLabel = new QLabel(tr("Height:"));
	m_heightFld = new DVGui::MeasuredDoubleLineEdit(0);
	m_dpiLabel = new QLabel(tr("DPI:"));
	m_dpiFld = new DoubleLineEdit(0, 66.76);

	QPushButton *okBtn = new QPushButton(tr("OK"), this);
	QPushButton *cancelBtn = new QPushButton(tr("Cancel"), this);
	QPushButton *applyBtn = new QPushButton(tr("Apply"), this);

	//Exclude all character which cannot fit in a filepath (Win).
	//Dots are also prohibited since they are internally managed by Toonz.
	QRegExp rx("[^\\\\/:?*.\"<>|]+");
	m_nameFld->setValidator(new QRegExpValidator(rx, this));

	m_levelTypeOm->addItem(VectorLevel);
	m_levelTypeOm->addItem(ToonzLevel);
	m_levelTypeOm->addItem(RasterLevel);
	m_levelTypeOm->addItem(ScanLevel);

	m_widthFld->setMeasure("level.lx");
	m_heightFld->setMeasure("level.ly");
	m_widthFld->setRange(0.1, (std::numeric_limits<double>::max)());
	m_heightFld->setRange(0.1, (std::numeric_limits<double>::max)());
	m_dpiFld->setRange(0.1, (std::numeric_limits<double>::max)());

	okBtn->setDefault(true);

	//--- layout
	m_topLayout->setMargin(0);
	m_topLayout->setSpacing(0);
	{
		QGridLayout *guiLay = new QGridLayout();
		guiLay->setMargin(10);
		guiLay->setVerticalSpacing(10);
		guiLay->setHorizontalSpacing(5);
		{
			//Name
			guiLay->addWidget(new QLabel(tr("Name:")), 0, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_nameFld, 0, 1, 1, 4);

			//From-To
			guiLay->addWidget(new QLabel(tr("From:")), 1, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_fromFld, 1, 1);
			guiLay->addWidget(new QLabel(tr("To:")), 1, 2, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_toFld, 1, 3);

			//Step-Inc
			guiLay->addWidget(new QLabel(tr("Step:")), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_stepFld, 2, 1);
			guiLay->addWidget(new QLabel(tr("Increment:")), 2, 2, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_incFld, 2, 3);

			//Type
			guiLay->addWidget(new QLabel(tr("Type:")), 3, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_levelTypeOm, 3, 1, 1, 3);

			//Save In
			guiLay->addWidget(new QLabel(tr("Save In:")), 4, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_pathFld, 4, 1, 1, 4);

			//Width - Height
			guiLay->addWidget(m_widthLabel, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_widthFld, 5, 1);
			guiLay->addWidget(m_heightLabel, 5, 2, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_heightFld, 5, 3);

			//DPI
			guiLay->addWidget(m_dpiLabel, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
			guiLay->addWidget(m_dpiFld, 6, 1, 1, 3);
		}
		guiLay->setColumnStretch(0, 0);
		guiLay->setColumnStretch(1, 0);
		guiLay->setColumnStretch(2, 0);
		guiLay->setColumnStretch(3, 0);
		guiLay->setColumnStretch(4, 1);

		m_topLayout->addLayout(guiLay, 1);
	}

	m_buttonLayout->setMargin(0);
	m_buttonLayout->setSpacing(30);
	{
		m_buttonLayout->addStretch(1);
		m_buttonLayout->addWidget(okBtn, 0);
		m_buttonLayout->addWidget(applyBtn, 0);
		m_buttonLayout->addWidget(cancelBtn, 0);
		m_buttonLayout->addStretch(1);
	}

	//---- signal-slot connections
	bool ret = true;
	ret = ret && connect(m_levelTypeOm, SIGNAL(currentIndexChanged(const QString &)),
						 SLOT(onLevelTypeChanged(const QString &)));
	ret = ret && connect(okBtn, SIGNAL(clicked()), this, SLOT(onOkBtn()));
	ret = ret && connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
	ret = ret && connect(applyBtn, SIGNAL(clicked()), this, SLOT(onApplyButton()));

	setSizeWidgetEnable(false);
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::updatePath()
{
	ToonzScene *scene = TApp::instance()->getCurrentScene()->getScene();

	/*--- 初期Pathを入れる。Xsheet Roomのときのみ、分岐 ---*/
	QString roomName = TApp::instance()->getCurrentRoomName();
	TFilePath defaultPath;
	if (roomName == "Xsheet") {
		/*--- 名称未設定シーンのとき、+satsuei直下に ---*/
		if (scene->isUntitled())
			defaultPath = TFilePath("+" + TProject::Scenes);
		/*--- 保存済みシーンのとき、そのシーンファイルの入っているフォルダの1階層上のフォルダにする ---*/
		else
			defaultPath = scene->codeFilePath(scene->getScenePath().getParentDir().getParentDir());
	} else
		defaultPath = scene->getDefaultLevelPath(getLevelType()).getParentDir();

	m_pathFld->setPath(toQString(defaultPath));
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::showEvent(QShowEvent *)
{
	update();
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::setSizeWidgetEnable(bool isEnable)
{
	m_widthLabel->setEnabled(isEnable);
	m_heightLabel->setEnabled(isEnable);
	m_widthFld->setEnabled(isEnable);
	m_heightFld->setEnabled(isEnable);
	m_dpiLabel->setEnabled(isEnable);
	m_dpiFld->setEnabled(isEnable);
}

//-----------------------------------------------------------------------------

int LevelCreatePopup::getLevelType() const
{
	QString levelTypeStr = m_levelTypeOm->currentText();
	if (levelTypeStr == ScanLevel)
		return TZI_XSHLEVEL;
	else if (levelTypeStr == VectorLevel)
		return PLI_XSHLEVEL;
	else if (levelTypeStr == ToonzLevel)
		return TZP_XSHLEVEL;
	else if (levelTypeStr == RasterLevel)
		return OVL_XSHLEVEL;
	return TZP_XSHLEVEL;
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::onLevelTypeChanged(const QString &text)
{
	if (text == "Raster Level" || text == "Toonz Raster Level")
		setSizeWidgetEnable(true);
	else
		setSizeWidgetEnable(false);
	updatePath();
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::onOkBtn()
{
	apply();
	close();
	/*if(apply())
    this->accept();*/
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::onApplyButton()
{
	apply();
}

//-----------------------------------------------------------------------------

bool LevelCreatePopup::apply()
{

	TApp *app = TApp::instance();
	int row = app->getCurrentFrame()->getFrame();
	int col = app->getCurrentColumn()->getColumnIndex();
	int i, j;

	ToonzScene *scene = app->getCurrentScene()->getScene();
	TXsheet *xsh = scene->getXsheet();

	int from = (int)m_fromFld->getValue();
	int to = (int)m_toFld->getValue();
	int inc = (int)m_incFld->getValue();
	int step = (int)m_stepFld->getValue();
	double w = m_widthFld->getValue();
	double h = m_heightFld->getValue();
	double dpi = m_dpiFld->getValue();
	int xres = tround(w * dpi), yres = tround(h * dpi);
	int lType = getLevelType();

	std::wstring levelName = m_nameFld->text().toStdWString();
	// tolgo i blanks prima e dopo

	i = levelName.find_first_not_of(L' ');
	if (i == (int)std::wstring::npos)
		levelName = L"";
	else {
		int j = levelName.find_last_not_of(L' ');
		assert(j != (int)std::wstring::npos);
		levelName = levelName.substr(i, j - i + 1);
	}
	if (levelName.empty()) {
		error(tr("No level name specified: please choose a valid level name"));
		return false;
	}

	if (from > to) {
		error(tr("Invalid frame range"));
		return false;
	}
	if (inc <= 0) {
		error(tr("Invalid increment value"));
		return false;
	}
	if (step <= 0) {
		error(tr("Invalid step value"));
		return false;
	}

	int numFrames = step * (((to - from) / inc) + 1);

	if (scene->getLevelSet()->getLevel(levelName)) {
		error(tr("The level name specified is already used: please choose a different level name"));
		return false;
	}

	TFilePath parentDir(m_pathFld->getPath().toStdWString());
	TFilePath fp = scene->getDefaultLevelPath(lType, levelName).withParentDir(parentDir);

	TFilePath actualFp = scene->decodeFilePath(fp);
	if (TSystem::doesExistFileOrLevel(actualFp)) {
		error(tr("The level name specified is already used: please choose a different level name"));
		return false;
	}
	parentDir = scene->decodeFilePath(parentDir);
	if (!TFileStatus(parentDir).doesExist()) {
		QString question;
		/*question = "Folder " +toQString(parentDir) +
								 " doesn't exist.\nDo you want to create it?";*/
		question = tr("Folder %1 doesn't exist.\nDo you want to create it?").arg(toQString(parentDir));
		int ret = DVGui::MsgBox(question, QObject::tr("Yes"), QObject::tr("No"));
		if (ret == 0 || ret == 2)
			return false;
		try {
			TSystem::mkDir(parentDir);
			DvDirModel::instance()->refreshFolder(parentDir.getParentDir());
		} catch (...) {
			error(tr("Unable to create") + toQString(parentDir));
			return false;
		}
	}

	TXshLevel *level = scene->createNewLevel(lType, levelName, TDimension(), 0, fp);
	TXshSimpleLevel *sl = dynamic_cast<TXshSimpleLevel *>(level);
	assert(sl);
	sl->setPath(fp, true);
	if (lType == TZP_XSHLEVEL || lType == OVL_XSHLEVEL) {
		sl->getProperties()->setDpiPolicy(LevelProperties::DP_ImageDpi);
		sl->getProperties()->setDpi(dpi);
		sl->getProperties()->setImageDpi(TPointD(dpi, dpi));
		sl->getProperties()->setImageRes(TDimension(xres, yres));
	}

	/*-- これからLevelを配置しようとしているセルが空いているかどうかのチェック --*/
	bool areColumnsShifted = false;
	TXshCell cell = xsh->getCell(row, col);
	bool isInRange = true;
	for (i = row; i < row + numFrames; i++) {
		if (!cell.isEmpty()) {
			isInRange = false;
			break;
		}
		cell = xsh->getCell(i, col);
	}
	/*-- 別のLevelに占有されていた場合、Columnを1つ右に移動 --*/
	if (!isInRange) {
		col += 1;
		TApp::instance()->getCurrentColumn()->setColumnIndex(col);
		areColumnsShifted = true;
		xsh->insertColumn(col);
	}

	CreateLevelUndo *undo = new CreateLevelUndo(row, col, numFrames, step, areColumnsShifted);
	TUndoManager::manager()->add(undo);

	for (i = from; i <= to; i += inc) {
		TFrameId fid(i);
		TXshCell cell(sl, fid);
		if (lType == PLI_XSHLEVEL)
			sl->setFrame(fid, new TVectorImage());
		else if (lType == TZP_XSHLEVEL) {
			TRasterCM32P raster(xres, yres);
			raster->fill(TPixelCM32());
			TToonzImageP ti(raster, TRect());
			ti->setDpi(dpi, dpi);
			sl->setFrame(fid, ti);
			ti->setSavebox(TRect(0, 0, xres - 1, yres - 1));
		} else if (lType == OVL_XSHLEVEL) {
			TRaster32P raster(xres, yres);
			raster->clear();
			TRasterImageP ri(raster);
			ri->setDpi(dpi, dpi);
			sl->setFrame(fid, ri);
		}
		for (j = 0; j < step; j++)
			xsh->setCell(row++, col, cell);
	}

	if (lType == TZP_XSHLEVEL || lType == OVL_XSHLEVEL) {
		sl->save(fp);
		DvDirModel::instance()->refreshFolder(fp.getParentDir());
	}

	undo->onAdd(sl);

	app->getCurrentScene()->notifySceneChanged();
	app->getCurrentScene()->notifyCastChange();
	app->getCurrentXsheet()->notifyXsheetChanged();

	//Cambia l'immagine corrente ma non cambiano ne' il frame ne' la colonna corrente
	// (entrambi notificano il cambiamento dell'immagine al tool).
	// devo verfificare che sia settato il tool giusto.
	app->getCurrentTool()->onImageChanged((TImage::Type)app->getCurrentImageType());
	return true;
}

//-----------------------------------------------------------------------------

void LevelCreatePopup::update()
{
	updatePath();
	Preferences *pref = Preferences::instance();

	m_widthFld->setValue(pref->getDefLevelWidth());
	m_heightFld->setValue(pref->getDefLevelHeight());
	m_dpiFld->setValue(pref->getDefLevelDpi());
	int levelType = pref->getDefLevelType();
	int index = -1;
	switch (levelType) {
	case TZI_XSHLEVEL:
		index = m_levelTypeOm->findText(ScanLevel);
		break;
	case PLI_XSHLEVEL:
		index = m_levelTypeOm->findText(VectorLevel);
		break;
	case TZP_XSHLEVEL:
		index = m_levelTypeOm->findText(ToonzLevel);
		break;
	case OVL_XSHLEVEL:
		index = m_levelTypeOm->findText(RasterLevel);
		break;
	}
	if (index >= 0)
		m_levelTypeOm->setCurrentIndex(index);

	/*
  (old behaviour)
  TCamera* camera = scene->getCurrentCamera();
	TDimensionD cameraSize = camera->getSize();
	m_widthFld->setValue(cameraSize.lx);
	m_heightFld->setValue(cameraSize.ly);
  if(camera->isXPrevalence())
    m_dpiFld->setValue(camera->getDpi().x);
  else
    m_dpiFld->setValue(camera->getDpi().y);
    */
}

//-----------------------------------------------------------------------------

OpenPopupCommandHandler<LevelCreatePopup> openLevelCreatePopup(MI_NewLevel);
