#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <memory>

// TnzCore includes
#include "tcommon.h"
#include "tgeometry.h"
#include "tpixel.h"

// TnzLib includes
#include "toonz/levelproperties.h"

// Qt includes
#include <QString>
#include <QObject>
#include <QMap>
#include <QRegExp>

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//==============================================================

//    Forward declarations

class TFilePath;
class QSettings;

//==============================================================

//**********************************************************************************
//    Preferences  declaration
//**********************************************************************************

/*!
  \brief    Stores application-wide preferences used throughout Toonz.
*/

class DVAPI Preferences : public QObject // singleton
{
	Q_OBJECT

public:
	struct LevelFormat {
		QString m_name;			//!< Name displayed for the format.
		QRegExp m_pathFormat;   //!< <TT>[default: ".*"]</TT>Used to recognize levels in
								//!  the format. It's case <I>in</I>sensitive.
		LevelOptions m_options; //!< Options associated to levels in the format.
		int m_priority;			//!< <TT>[default: 1]</TT> Priority value for the format.
								//!  Higher priority means that the format is matched first.
	public:
		LevelFormat(const QString &name = QString())
			: m_name(name), m_pathFormat(".*", Qt::CaseInsensitive), m_priority(1) {}

		bool matches(const TFilePath &fp) const;
	};

	enum ColumnIconLoadingPolicy {
		LoadAtOnce,  /**< it tells to load and display column icon
					\b at \b once when the scene is opened;      */
		LoadOnDemand /**< it tells to load display icon \b on
					\b demand (generally by clicking on the column header)     */
	};

public:
	static Preferences *instance();

	// General settings  tab

	void setUndoMemorySize(int memorySize);
	int getUndoMemorySize() const { return m_undoMemorySize; }

	void setDefaultTaskChunkSize(int chunkSize);
	int getDefaultTaskChunkSize() const
	{
		return m_chunkSize;
	}

	void enableDefaultViewer(bool on);
	bool isDefaultViewerEnabled() const
	{
		return m_defaultViewerEnabled;
	}

	void enableRasterOptimizedMemory(bool on);
	bool isRasterOptimizedMemory() const
	{
		return m_rasterOptimizedMemory;
	}

	void enableAutosave(bool on);
	bool isAutosaveEnabled() const
	{
		return m_autosaveEnabled;
	}

	void setAutosavePeriod(int minutes);
	int getAutosavePeriod() const
	{
		return m_autosavePeriod;
	} // minutes

	void enableLevelsBackup(bool enabled);
	bool isLevelsBackupEnabled() const { return m_levelsBackupEnabled; }

	void enableSceneNumbering(bool enabled);
	bool isSceneNumberingEnabled() const { return m_sceneNumberingEnabled; }

	void enableReplaceAfterSaveLevelAs(bool on);
	bool isReplaceAfterSaveLevelAsEnabled() const
	{
		return m_replaceAfterSaveLevelAs;
	}

	// Interface  tab

	void setCurrentLanguage(int currentLanguage);
	QString getCurrentLanguage() const;
	QString getLanguage(int index) const;
	int getLanguageCount() const;

	void setCurrentStyleSheet(int currentStyleSheet);
	QString getCurrentStyleSheet() const;
	QString getStyleSheet(int index) const;
	int getStyleSheetCount() const;

	void setUnits(std::string s);
	QString getUnits() const { return m_units; }

	void setCameraUnits(std::string s);
	QString getCameraUnits() const { return m_cameraUnits; }

	void enableGeneratedMovieView(bool on);
	bool isGeneratedMovieViewEnabled() const
	{
		return m_generatedMovieViewEnabled;
	}

	void setViewValues(int shrink, int step);
	void getViewValues(int &shrink, int &step) const
	{
		shrink = m_shrink, step = m_step;
	}

	void setIconSize(const TDimension &dim);
	TDimension getIconSize() const { return m_iconSize; }

	void setViewerBGColor(const TPixel32 &color, bool isDragging);
	TPixel getViewerBgColor() const { return m_viewerBGColor; }

	void setPreviewBGColor(const TPixel32 &color, bool isDragging);
	TPixel getPreviewBgColor() const { return m_previewBGColor; }

	void setChessboardColor1(const TPixel32 &color, bool isDragging);
	void setChessboardColor2(const TPixel32 &color, bool isDragging);
	void getChessboardColors(TPixel32 &col1, TPixel32 &col2) const
	{
		col1 = m_chessboardColor1;
		col2 = m_chessboardColor2;
	}

	void enableShowRasterImagesDarkenBlendedInViewer(bool on);
	bool isShowRasterImagesDarkenBlendedInViewerEnabled() const
	{
		return m_showRasterImagesDarkenBlendedInViewer;
	}

	void enableActualPixelViewOnSceneEditingMode(bool on);
	bool isActualPixelViewOnSceneEditingModeEnabled() const
	{
		return m_actualPixelViewOnSceneEditingMode;
	}

	void setViewerZoomCenter(int type);
	int getViewerZoomCenter() const
	{
		return m_viewerZoomCenter;
	}

	void enableShowFrameNumberWithLetters(bool on);
	bool isShowFrameNumberWithLettersEnabled() const
	{
		return m_showFrameNumberWithLetters;
	}
	void enableLevelNameOnEachMarker(bool on);
	bool isLevelNameOnEachMarkerEnabled() const
	{
		return m_levelNameOnEachMarker;
	}
	void setColumnIconLoadingPolicy(ColumnIconLoadingPolicy cilp);
	ColumnIconLoadingPolicy getColumnIconLoadingPolicy() const
	{
		return (ColumnIconLoadingPolicy)m_columnIconLoadingPolicy;
	}
	void enableMoveCurrent(bool on);
	bool isMoveCurrentEnabled() const
	{
		return m_moveCurrentFrameByClickCellArea;
	}

	// Visualization  tab

	void setShow0ThickLines(bool on);
	bool getShow0ThickLines() const
	{
		return m_show0ThickLines;
	}

	void setRegionAntialias(bool on);
	bool getRegionAntialias() const
	{
		return m_regionAntialias;
	}

	// Loading  tab

	void enableAutoExpose(bool on);
	bool isAutoExposeEnabled() const
	{
		return m_autoExposeEnabled;
	}

	void enableSubsceneFolder(bool on);
	bool isSubsceneFolderEnabled() const
	{
		return m_subsceneFolderEnabled;
	}

	int addLevelFormat(const LevelFormat &format); //!< Inserts a new level format.  \return  The associated format index.
	void removeLevelFormat(int formatIdx);		   //!< Removes a level format.

	const LevelFormat &levelFormat(int formatIdx) const; //!< Retrieves a level format.
	int levelFormatsCount() const;						 //!< Returns the number of stored level formats.

	/*! \return     Either the index of a matching format, or \p -1 if none
                  was found.                                                        */

	int matchLevelFormat(const TFilePath &fp) const; //!< Returns the \a nonnegative index of the first level format
													 //!  matching the specified file path, <I>or \p -1 if none</I>.

	void setInitialLoadTlvCachingBehavior(int type);
	int getInitialLoadTlvCachingBehavior() const
	{
		return m_initialLoadTlvCachingBehavior;
	}

	void enableRemoveSceneNumberFromLoadedLevelName(bool on);
	bool isRemoveSceneNumberFromLoadedLevelNameEnabled() const
	{
		return m_removeSceneNumberFromLoadedLevelName;
	}

	void setPaletteTypeOnLoadRasterImageAsColorModel(int type);
	int getPaletteTypeOnLoadRasterImageAsColorModel() const
	{
		return m_paletteTypeOnLoadRasterImageAsColorModel;
	}

	// Drawing  tab

	void setScanLevelType(std::string s);
	QString getScanLevelType() const { return m_scanLevelType; }

	void setDefLevelType(int levelType);
	int getDefLevelType() const { return m_defLevelType; }

	void setDefLevelWidth(double width);
	double getDefLevelWidth() const { return m_defLevelWidth; }

	void setDefLevelHeight(double height);
	double getDefLevelHeight() const { return m_defLevelHeight; }

	void setDefLevelDpi(double dpi);
	double getDefLevelDpi() const { return m_defLevelDpi; }

	void setAutocreationType(int s);
	int getAutocreationType() const { return m_autocreationType; }

	bool isAutoCreateEnabled() const
	{
		return m_autocreationType > 0;
	}
	bool isAnimationSheetEnabled() const
	{
		return m_autocreationType == 2;
	}

	void enableSaveUnpaintedInCleanup(bool on);
	bool isSaveUnpaintedInCleanupEnable() const
	{
		return m_saveUnpaintedInCleanup;
	}

	void enableMinimizeSaveboxAfterEditing(bool on);
	bool isMinimizeSaveboxAfterEditing() const
	{
		return m_minimizeSaveboxAfterEditing;
	}

	void setFillOnlySavebox(bool on);
	bool getFillOnlySavebox() const { return m_fillOnlySavebox; }

	void enableMultiLayerStylePicker(bool on);
	bool isMultiLayerStylePickerEnabled() const
	{
		return m_multiLayerStylePickerEnabled;
	}

	// Xsheet  tab

	void setXsheetStep(int step);					   //!< Sets the step used for the <I>next/prev step</I> commands.
	int getXsheetStep() const { return m_xsheetStep; } //!< Returns the step used for the <I>next/prev step</I> commands.

	void enableXsheetAutopan(bool on); //!< Enables/disables xsheet panning during playback.
	bool isXsheetAutopanEnabled() const
	{
		return m_xsheetAutopanEnabled;
	} //!< Returns whether xsheet pans during playback.

	void enableIgnoreAlphaonColumn1(bool on); //!< Enables/disables xsheet panning during playback.
	bool isIgnoreAlphaonColumn1Enabled() const
	{
		return m_ignoreAlphaonColumn1Enabled;
	} //!< Returns whether xsheet pans during playback.

	void setDragCellsBehaviour(int dragCellsBehaviour);
	int getDragCellsBehaviour() const { return m_dragCellsBehaviour; }

	// Animation  tab

	void setKeyframeType(int s);
	int getKeyframeType() const { return m_keyframeType; }

	void setAnimationStep(int s);
	int getAnimationStep() const { return m_animationStep; }

	// Preview  tab

	void setBlankValues(int blanksCount, TPixel32 blankColor);
	void getBlankValues(int &blanksCount, TPixel32 &blankColor) const
	{
		blanksCount = m_blanksCount, blankColor = m_blankColor;
	}

	void enablePreviewAlwaysOpenNewFlip(bool on);
	bool previewAlwaysOpenNewFlipEnabled() const
	{
		return m_previewAlwaysOpenNewFlipEnabled;
	}

	void enableRewindAfterPlayback(bool on);
	bool rewindAfterPlaybackEnabled() const
	{
		return m_rewindAfterPlaybackEnabled;
	}

	void enableFitToFlipbook(bool on);
	bool fitToFlipbookEnabled() const
	{
		return m_fitToFlipbookEnabled;
	}

	// Onion Skin  tab

	void enableOnionSkin(bool on);
	bool isOnionSkinEnabled() const
	{
		return m_onionSkinEnabled;
	}
	void setOnionPaperThickness(int thickness);
	int getOnionPaperThickness() const { return m_onionPaperThickness; }

	void setOnionData(const TPixel &frontOnionColor, const TPixel &backOnionColor, bool inksOnly);
	void getOnionData(TPixel &frontOnionColor, TPixel &backOnionColor, bool &inksOnly) const
	{
		frontOnionColor = m_frontOnionColor, backOnionColor = m_backOnionColor, inksOnly = m_inksOnly;
	}

	// Transparency Check  tab

	void setTranspCheckData(const TPixel &bg, const TPixel &ink, const TPixel &paint);
	void getTranspCheckData(TPixel &bg, TPixel &ink, TPixel &paint) const
	{
		bg = m_transpCheckBg;
		ink = m_transpCheckInk;
		paint = m_transpCheckPaint;
	}

	// Version Control  tab

	void enableSVN(bool on);
	bool isSVNEnabled() const { return m_SVNEnabled; }

	void enableAutomaticSVNFolderRefresh(bool on);
	bool isAutomaticSVNFolderRefreshEnabled() const
	{
		return m_automaticSVNFolderRefreshEnabled;
	}

	// Uncategorized - internals

	void setAskForOverrideRender(bool on);
	bool askForOverrideRender() const { return m_askForOverrideRender; }

	void setLineTestFpsCapture(int lineTestFpsCapture);
	int getLineTestFpsCapture() const { return m_lineTestFpsCapture; }

	int getTextureSize() const { return m_textureSize; }
	bool useDrawPixel() { return m_textureSize == 0; }

	int getShmMax() const { return m_shmmax; } //! \sa The \p sysctl unix command.
	int getShmSeg() const { return m_shmseg; } //! \sa The \p sysctl unix command.
	int getShmAll() const { return m_shmall; } //! \sa The \p sysctl unix command.
	int getShmMni() const { return m_shmmni; } //! \sa The \p sysctl unix command.

	Q_SIGNALS :

		void
		stopAutoSave();
	void startAutoSave();

private:
	std::unique_ptr<QSettings> m_settings;

	QMap<int, QString> m_languageMaps,
		m_styleSheetMaps;

	std::vector<LevelFormat> m_levelFormats;

	QString m_units,
		m_cameraUnits,
		m_scanLevelType;

	double m_defLevelWidth,
		m_defLevelHeight,
		m_defLevelDpi;

	TDimension m_iconSize;

	TPixel32 m_blankColor,
		m_frontOnionColor,
		m_backOnionColor,
		m_transpCheckBg,
		m_transpCheckInk,
		m_transpCheckPaint;

	int m_autosavePeriod, // minutes
		m_chunkSize,
		m_blanksCount,
		m_onionPaperThickness,
		m_step,
		m_shrink,
		m_textureSize,
		m_autocreationType,
		m_keyframeType,
		m_animationStep;
	int m_currentLanguage,
		m_currentStyleSheet,
		m_undoMemorySize, // in megabytes
		m_dragCellsBehaviour,
		m_lineTestFpsCapture,
		m_defLevelType,
		m_xsheetStep,
		m_shmmax,
		m_shmseg,
		m_shmall,
		m_shmmni;

	bool m_autoExposeEnabled,
		m_autoCreateEnabled,
		m_subsceneFolderEnabled,
		m_generatedMovieViewEnabled,
		m_xsheetAutopanEnabled,
		m_ignoreAlphaonColumn1Enabled,
		m_previewAlwaysOpenNewFlipEnabled,
		m_rewindAfterPlaybackEnabled,
		m_fitToFlipbookEnabled,
		m_autosaveEnabled,
		m_defaultViewerEnabled;
	bool m_rasterOptimizedMemory,
		m_saveUnpaintedInCleanup,
		m_askForOverrideRender,
		m_automaticSVNFolderRefreshEnabled,
		m_SVNEnabled,
		m_levelsBackupEnabled,
		m_minimizeSaveboxAfterEditing,
		m_sceneNumberingEnabled,
		m_animationSheetEnabled,
		m_inksOnly;
	bool m_fillOnlySavebox,
		m_show0ThickLines,
		m_regionAntialias;

	TPixel32 m_viewerBGColor,
		m_previewBGColor,
		m_chessboardColor1,
		m_chessboardColor2;
	bool m_showRasterImagesDarkenBlendedInViewer,
		m_actualPixelViewOnSceneEditingMode;
	int m_viewerZoomCenter; // MOUSE_CURSOR = 0, VIEWER_CENTER = 1
	// used in the load level popup. ON_DEMAND = 0, ALL_ICONS = 1, ALL_ICONS_AND_IMAGES = 2
	int m_initialLoadTlvCachingBehavior;
	// automatically remove 6 letters of scene number from the level name ("c0001_A.tlv" -> "A")
	bool m_removeSceneNumberFromLoadedLevelName;
	// after save level as command, replace the level with "save-as"ed level
	bool m_replaceAfterSaveLevelAs;
	// convert the last one digit of the frame number to alphabet
	// Ex.  12 -> 1B    21 -> 2A   30 -> 3
	bool m_showFrameNumberWithLetters;
	// display level name on each marker in the xsheet cell area
	bool m_levelNameOnEachMarker;
	// whether to load the column icon(thumbnail) at once / on demand
	int m_columnIconLoadingPolicy;
	bool m_moveCurrentFrameByClickCellArea;
	bool m_onionSkinEnabled;
	bool m_multiLayerStylePickerEnabled;
	/*-- 
	Color Modelにラスタ画像を読み込んだとき、パレットをどのように作るか
	0 : 全ての異なるピクセルの色を別のStyleにする, 1 : 似ている色をまとめて1つのStyleにする
  --*/
	int m_paletteTypeOnLoadRasterImageAsColorModel;

private:
	Preferences();
	~Preferences();
};

#endif // PREFERENCES_H
