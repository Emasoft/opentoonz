

#include "toonz/palettecmd.h"

// TnzLib includes
#include "toonz/tpalettehandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/studiopalette.h"
#include "toonz/txsheet.h"
#include "toonz/txshcolumn.h"
#include "toonz/txshcell.h"
#include "toonz/txshlevelcolumn.h"
#include "toonz/txshsimplelevel.h"
#include "toonz/cleanupcolorstyles.h"
#include "toonz/studiopalette.h"
#include "toonz/txshlevel.h"
#include "toonz/toonzscene.h"
#include "toonz/toonzimageutils.h"
#include "toonz/cleanupcolorstyles.h"
#include "toonz/preferences.h"

// TnzCore includes
#include "tpalette.h"
#include "tcolorstyles.h"
#include "tundo.h"
#include "tvectorimage.h"
#include "ttoonzimage.h"
#include "trasterimage.h"
#include "tconvert.h"
#include "tcolorutils.h"
#include "tropcm.h"
#include "tstroke.h"
#include "tregion.h"
#include "tlevel_io.h"

#include "historytypes.h"

// tcg includes
#include "tcg/boost/range_utility.h"

// boost includes
#include <boost/bind.hpp>
#include <boost/range/counting_range.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>

//===================================================================

void findPaletteLevels(set<TXshSimpleLevel *> &levels, int &rowIndex, int &columnIndex,
					   TPalette *palette, TXsheet *xsheet)
{
	rowIndex = columnIndex = -1;
	int columnCount = xsheet->getColumnCount();
	int c;
	for (c = 0; c < columnCount; c++) {
		TXshColumn *column = xsheet->getColumn(c);
		if (!column || column->isEmpty())
			continue;

		TXshLevelColumn *levelColumn = column->getLevelColumn();
		if (!levelColumn || levelColumn->isEmpty())
			continue;

		int r0, r1;
		if (!column->getRange(r0, r1))
			continue;
		int r;
		for (r = r0; r <= r1; r++) {
			TXshCell cell = levelColumn->getCell(r);
			if (cell.isEmpty())
				continue;
			TXshSimpleLevel *level = cell.getSimpleLevel();
			if (!level || level->getPalette() != palette)
				continue;
			levels.insert(level);
			if (rowIndex < 0) {
				rowIndex = r;
				columnIndex = c;
			}
		}
	}
}

//===================================================================

namespace
{

bool isStyleUsed(const TVectorImageP vi, int styleId)
{
	int strokeCount = vi->getStrokeCount();
	int i;
	for (i = strokeCount - 1; i >= 0; i--) {
		TStroke *stroke = vi->getStroke(i);
		if (stroke && stroke->getStyle() == styleId)
			return true;
	}
	int regionCount = vi->getRegionCount();
	for (i = 0; i < regionCount; i++) {
		TRegion *region = vi->getRegion(i);
		if (region || region->getStyle() != styleId)
			return true;
	}
	return false;
}

//-------------------------------------------------------------------

bool isStyleUsed(const TToonzImageP vi, int styleId)
{
	TRasterCM32P ras = vi->getRaster();
	for (int y = 0; y < ras->getLy(); y++) {
		TPixelCM32 *pix = ras->pixels(y), *endPix = pix + ras->getLx();
		while (pix < endPix) {
			if (pix->getPaint() == styleId)
				return true;
			if (pix->getInk() == styleId)
				return true;
			pix++;
		}
	}
	return false;
}

//-------------------------------------------------------------------

/*! Return true if one style is used. */
bool areStylesUsed(const TImageP image, const std::vector<int> styleIds)
{
	int j;
	for (j = 0; j < (int)styleIds.size(); j++)
		if (isStyleUsed(image, styleIds[j]))
			return true;
	return false;
}

} //namespace

//===================================================================

bool isStyleUsed(const TImageP image, int styleId)
{
	TVectorImageP vi = image;
	TToonzImageP ti = image;
	if (vi)
		return isStyleUsed(vi, styleId);
	if (ti)
		return isStyleUsed(ti, styleId);
	return false;
}

//===================================================================

/*! Return true if one style is used. */
bool areStylesUsed(const set<TXshSimpleLevel *> levels, const std::vector<int> styleIds)
{
	set<TXshSimpleLevel *>::const_iterator it = levels.begin();
	for (it; it != levels.end(); it++) {
		TXshSimpleLevel *level = *it;
		std::vector<TFrameId> fids;
		level->getFids(fids);
		int i;
		for (i = 0; i < (int)fids.size(); i++) {
			TImageP image = level->getFrame(fids[i], true);
			if (areStylesUsed(image, styleIds))
				return true;
		}
	}
	return false;
}

//===================================================================
//
// arrangeStyles
// srcPage : {a0,a1,...an } ==> dstPage : {b,b+1,...b+n-1}
//
//-------------------------------------------------------------------

/*! \namespace PaletteCmd
		\brief Provides a collection of methods to manage \b TPalette.
*/

namespace
{

class ArrangeStylesUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle;
	TPaletteP m_palette;
	int m_dstPageIndex;
	int m_dstIndexInPage;
	int m_srcPageIndex;
	std::set<int> m_srcIndicesInPage;

public:
	ArrangeStylesUndo(TPaletteHandle *paletteHandle,
					  int dstPageIndex,
					  int dstIndexInPage,
					  int srcPageIndex,
					  const std::set<int> &srcIndicesInPage)
		: m_paletteHandle(paletteHandle), m_dstPageIndex(dstPageIndex), m_dstIndexInPage(dstIndexInPage), m_srcPageIndex(srcPageIndex), m_srcIndicesInPage(srcIndicesInPage)
	{
		m_palette = m_paletteHandle->getPalette();
		assert(m_palette);
		assert(0 <= dstPageIndex && dstPageIndex < m_palette->getPageCount());
		assert(0 <= srcPageIndex && srcPageIndex < m_palette->getPageCount());
		TPalette::Page *dstPage = m_palette->getPage(dstPageIndex);
		assert(dstPage);
		assert(0 <= dstIndexInPage && dstIndexInPage <= dstPage->getStyleCount());
		assert(!srcIndicesInPage.empty());
		TPalette::Page *srcPage = m_palette->getPage(srcPageIndex);
		assert(srcPage);
		assert(0 <= *srcIndicesInPage.begin() && *srcIndicesInPage.rbegin() < srcPage->getStyleCount());
	}
	void undo() const
	{
		TPalette::Page *srcPage = m_palette->getPage(m_srcPageIndex);
		assert(srcPage);
		TPalette::Page *dstPage = m_palette->getPage(m_dstPageIndex);
		assert(dstPage);
		std::vector<int> styles;
		int count = m_srcIndicesInPage.size();
		int h = m_dstIndexInPage;
		std::set<int>::const_iterator i;
		for (i = m_srcIndicesInPage.begin(); i != m_srcIndicesInPage.end(); ++i) {
			if (srcPage == dstPage && (*i) <= h)
				h--;
			else
				break;
		}
		assert(h + count - 1 <= dstPage->getStyleCount());
		int k;
		for (k = 0; k < count; k++) {
			styles.push_back(dstPage->getStyleId(h));
			dstPage->removeStyle(h);
		}
		k = 0;
		for (i = m_srcIndicesInPage.begin(); i != m_srcIndicesInPage.end(); ++i, ++k)
			srcPage->insertStyle(*i, styles[k]);

		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		TPalette::Page *srcPage = m_palette->getPage(m_srcPageIndex);
		assert(srcPage);
		TPalette::Page *dstPage = m_palette->getPage(m_dstPageIndex);
		assert(dstPage);

		std::vector<int> styles;
		std::set<int>::const_reverse_iterator i;
		std::vector<int>::iterator j;
		int k = m_dstIndexInPage;
		for (i = m_srcIndicesInPage.rbegin(); i != m_srcIndicesInPage.rend(); ++i) {
			int index = *i;
			if (m_dstPageIndex == m_srcPageIndex && index < k)
				k--;
			styles.push_back(srcPage->getStyleId(index));
			srcPage->removeStyle(index);
		}
		for (j = styles.begin(); j != styles.end(); ++j)
			dstPage->insertStyle(k, *j);
		m_palette->setDirtyFlag(true);
		m_paletteHandle->notifyPaletteChanged();
	}

	int getSize() const { return sizeof *this + m_srcIndicesInPage.size() * sizeof(int); }

	QString getHistoryString()
	{
		return QObject::tr("Arrange Styles  in Palette %1")
			.arg(QString::fromStdWString(m_palette->getPaletteName()));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

//-----------------------------------------------------------------------------

void PaletteCmd::arrangeStyles(TPaletteHandle *paletteHandle,
							   int dstPageIndex, int dstIndexInPage,
							   int srcPageIndex, const std::set<int> &srcIndicesInPage)
{
	if (dstPageIndex == srcPageIndex && dstIndexInPage == *srcIndicesInPage.begin())
		return;
	ArrangeStylesUndo *undo =
		new ArrangeStylesUndo(paletteHandle, dstPageIndex,
							  dstIndexInPage, srcPageIndex,
							  srcIndicesInPage);
	undo->redo();
	TUndoManager::manager()->add(undo);
}

//=============================================================================
//CreateStyle
//-----------------------------------------------------------------------------
namespace
{
//-----------------------------------------------------------------------------

class CreateStyleUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle;
	TPaletteP m_palette;
	int m_pageIndex;
	int m_styleId;
	TColorStyle *m_style;

public:
	CreateStyleUndo(
		TPaletteHandle *paletteHandle,
		int pageIndex,
		int styleId)
		: m_paletteHandle(paletteHandle), m_pageIndex(pageIndex), m_styleId(styleId)
	{
		m_palette = m_paletteHandle->getPalette();
		m_style = m_palette->getStyle(m_styleId)->clone();
		assert(m_palette);
		assert(0 <= pageIndex && pageIndex < m_palette->getPageCount());
		assert(0 <= styleId && styleId < m_palette->getStyleCount());
	}
	void undo() const
	{
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		int indexInPage = page->search(m_styleId);
		assert(indexInPage >= 0);
		page->removeStyle(indexInPage);
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		assert(m_palette->getStylePage(m_styleId) == 0);
		int indexInPage = page->addStyle(m_styleId);
		if (indexInPage == -1) {
			indexInPage = page->getStyleCount();
			page->insertStyle(indexInPage, m_style->getMainColor());
			assert(m_styleId == page->getStyleId(indexInPage));
		}
		m_palette->getStyle(m_styleId)->setMainColor(m_style->getMainColor());
		m_palette->getStyle(m_styleId)->setName(m_style->getName());
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const { return sizeof *this; }
	QString getHistoryString()
	{
		return QObject::tr("Create Style#%1  in Palette %2")
			.arg(QString::number(m_styleId))
			.arg(QString::fromStdWString(m_palette->getPaletteName()));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

//-----------------------------------------------------------------------------

void PaletteCmd::createStyle(
	TPaletteHandle *paletteHandle,
	TPalette::Page *page)
{
	int index = paletteHandle->getStyleIndex();
	TPalette *palette = paletteHandle->getPalette();
	int newIndex;

	int UnpagedId = palette->getFirstUnpagedStyle();
	if (UnpagedId != -1 && !palette->isCleanupPalette()) {
		if (index == -1)
			palette->getStyle(UnpagedId)->setMainColor(TPixel32::Black);
		else
			palette->getStyle(UnpagedId)->setMainColor(palette->getStyle(index)->getMainColor());
		newIndex = page->addStyle(UnpagedId);
	} else if (!palette->isCleanupPalette()) {
		if (index == -1)
			newIndex = page->addStyle(TPixel32::Black);
		else {
			TColorStyle *style = palette->getStyle(index);
			TCleanupStyle *cleanupStyle = dynamic_cast<TCleanupStyle *>(style);
			if ((cleanupStyle || index == 0) && palette->isCleanupPalette()) {
				TColorCleanupStyle *newCleanupStyle = new TColorCleanupStyle();
				if (cleanupStyle) {
					int i;
					for (i = 0; i < cleanupStyle->getParamCount(); i++)
						newCleanupStyle->setColorParamValue(i, cleanupStyle->getColorParamValue(i));
				}
				newIndex = page->addStyle(newCleanupStyle);
			} else
				newIndex = page->addStyle(style->getMainColor());
		}
	} else /*- CleanupPaletteの場合 -*/
	{
		newIndex = page->addStyle(new TColorCleanupStyle(TPixel32::Red));
	}
	int newStyleId = page->getStyleId(newIndex);
	/*-  StudioPalette上でStyleを追加した場合、GlobalNameを自動で割り振る -*/
	if (palette->getGlobalName() != L"") {
		TColorStyle *cs = palette->getStyle(newStyleId);
		std::wstring gname = L"-" + palette->getGlobalName() + L"-" + toWideString(newStyleId);
		cs->setGlobalName(gname);
	}

	page->getStyle(newIndex)->setName(QString("color_%1").arg(newStyleId).toStdWString());
	paletteHandle->setStyleIndex(newStyleId);

	palette->setDirtyFlag(true);
	paletteHandle->notifyPaletteChanged();
	TUndoManager::manager()->add(new CreateStyleUndo(paletteHandle, page->getIndex(), page->getStyleId(newIndex)));
}

//=============================================================================
// addStyles
//-----------------------------------------------------------------------------

namespace
{

//=============================================================================
// AddStylesUndo
//-----------------------------------------------------------------------------

class AddStylesUndo : public TUndo
{
	TPaletteP m_palette;
	int m_pageIndex;
	int m_indexInPage;
	std::vector<std::pair<TColorStyle *, int>> m_styles;
	TPaletteHandle *m_paletteHandle;

public:
	// creare DOPO l'inserimento
	AddStylesUndo(
		const TPaletteP &palette,
		int pageIndex,
		int indexInPage,
		int count,
		TPaletteHandle *paletteHandle)
		: m_palette(palette), m_pageIndex(pageIndex), m_indexInPage(indexInPage), m_paletteHandle(paletteHandle)
	{
		assert(m_palette);
		assert(0 <= m_pageIndex && m_pageIndex < m_palette->getPageCount());
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		assert(0 <= indexInPage && indexInPage + count <= page->getStyleCount());
		for (int i = 0; i < count; i++) {
			std::pair<TColorStyle *, int> p;
			p.second = page->getStyleId(m_indexInPage + i);
			p.first = m_palette->getStyle(p.second)->clone();
			m_styles.push_back(p);
		}
	}
	~AddStylesUndo()
	{
		for (int i = 0; i < (int)m_styles.size(); i++)
			delete m_styles[i].first;
	}
	void undo() const
	{
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		int count = m_styles.size();
		int i;
		for (i = count - 1; i >= 0; i--)
			page->removeStyle(m_indexInPage + i);
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		for (int i = 0; i < (int)m_styles.size(); i++) {
			TColorStyle *cs = m_styles[i].first->clone();
			int styleId = m_styles[i].second;
			assert(m_palette->getStylePage(styleId) == 0);
			m_palette->setStyle(styleId, cs);
			page->insertStyle(m_indexInPage + i, styleId);
		}
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const { return sizeof *this + m_styles.size() * sizeof(TColorStyle); }

	QString getHistoryString()
	{
		return QObject::tr("Add Style  to Palette %1")
			.arg(QString::fromStdWString(m_palette->getPaletteName()));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

//-----------------------------------------------------------------------------
/*- ドラッグ＆ドロップ時に呼ばれる -*/
void PaletteCmd::addStyles(TPaletteHandle *paletteHandle, int pageIndex,
						   int indexInPage, const std::vector<TColorStyle *> &styles)
{
	TPalette *palette = paletteHandle->getPalette();
	assert(0 <= pageIndex && pageIndex < palette->getPageCount());
	TPalette::Page *page = palette->getPage(pageIndex);
	assert(page);
	assert(0 <= indexInPage && indexInPage <= page->getStyleCount());
	int count = styles.size();
	for (int i = 0; i < count; i++) {
		page->insertStyle(indexInPage + i, styles[i]->clone());

		/*-- StudioPaletteから持ち込んだ場合、その元の名前をoriginalNameに入れる --*/
		if (styles[i]->getGlobalName() != L"") {
			/*-- もし元のStyleにOriginalNameが無ければ（コピー元がStudioPaletteからということ）--*/
			if (styles[i]->getOriginalName() == L"") {
				/*-- 元のStyleの名前をペースト先のOriginalNameに入れる --*/
				page->getStyle(indexInPage + i)->setOriginalName(styles[i]->getName());
			}
		}
		/*-- それ以外の場合は、clone()でそれぞれの名前をコピーしているので、そのままでOK --*/
	}
	TUndoManager::manager()->add(new AddStylesUndo(palette, pageIndex, indexInPage, count, paletteHandle));
	palette->setDirtyFlag(true);
}

//=============================================================================
// eraseStyles
//-----------------------------------------------------------------------------

namespace
{

void eraseStylesInLevels(const set<TXshSimpleLevel *> &levels, const std::vector<int> styleIds)
{
	set<TXshSimpleLevel *>::const_iterator it = levels.begin();
	for (it; it != levels.end(); it++) {
		TXshSimpleLevel *level = *it;
		std::vector<TFrameId> fids;
		level->getFids(fids);
		int i;
		for (i = 0; i < (int)fids.size(); i++) {
			TImageP image = level->getFrame(fids[i], true);
			TVectorImageP vi = image;
			TToonzImageP ti = image;
			if (vi)
				vi->eraseStyleIds(styleIds);
			else if (ti)
				TRop::eraseStyleIds(ti.getPointer(), styleIds);
		}
	}
}

} // namespace

//-----------------------------------------------------------------------------

void PaletteCmd::eraseStyles(const std::set<TXshSimpleLevel *> &levels,
							 const std::vector<int> &styleIds)
{
	typedef std::pair<const TXshSimpleLevelP, std::vector<TVectorImageP>> LevelImages;

	struct Undo : public TUndo {
		std::set<TXshSimpleLevel *> m_levels;
		std::vector<int> m_styleIds;

		mutable std::map<TXshSimpleLevelP, std::vector<TVectorImageP>> m_imagesByLevel;

	public:
		Undo(const std::set<TXshSimpleLevel *> &levels, const std::vector<int> &styleIds)
			: m_levels(levels), m_styleIds(styleIds)
		{
			tcg::substitute(m_imagesByLevel, levels | boost::adaptors::filtered(isVector) | boost::adaptors::transformed(toEmptyLevelImages));
		}

		bool isValid() const { return !m_levels.empty(); }

		void redo() const
		{
			std::for_each(m_imagesByLevel.begin(), m_imagesByLevel.end(), cloneImages);
			eraseStylesInLevels(m_levels, m_styleIds);
		}

		void undo() const
		{
			std::for_each(m_imagesByLevel.begin(), m_imagesByLevel.end(), restoreImages);
		}

		int getSize() const { return 10 << 20; } // At max 10 per 100 MB

	private:
		static bool isVector(const TXshSimpleLevel *level)
		{
			return (assert(level), level->getType() == PLI_XSHLEVEL);
		}

		static LevelImages toEmptyLevelImages(TXshSimpleLevel *level)
		{
			return LevelImages(level, std::vector<TVectorImageP>());
		}

		static void copyStrokeIds(const TVectorImage &src, TVectorImage &dst)
		{
			UINT s, sCount = src.getStrokeCount();
			for (s = 0; s != sCount; ++s)
				dst.getStroke(s)->setId(src.getStroke(s)->getId());
		}

		static TVectorImageP cloneImage(const TXshSimpleLevel &level, int f)
		{
			TVectorImageP src = static_cast<TVectorImageP>(level.getFrame(level.getFrameId(f), false));
			TVectorImageP dst = src->clone();

			copyStrokeIds(*src, *dst);
			return dst;
		}

		static void cloneImages(LevelImages &levelImages)
		{
			tcg::substitute(levelImages.second, boost::counting_range(0, levelImages.first->getFrameCount()) | boost::adaptors::transformed(boost::bind(cloneImage, boost::cref(*levelImages.first), _1)));
		}

		static void restoreImage(const TXshSimpleLevelP &level, int f, const TVectorImageP &vi)
		{
			level->setFrame(level->getFrameId(f), vi.getPointer());
		}

		static void restoreImages(LevelImages &levelImages)
		{
			int f, fCount = tmin(levelImages.first->getFrameCount(), int(levelImages.second.size()));

			for (f = 0; f != fCount; ++f)
				restoreImage(levelImages.first, f, levelImages.second[f]);
		}

	}; // Undo

	if (levels.empty() || styleIds.empty())
		return;

	std::auto_ptr<TUndo> undo(new Undo(levels, styleIds));
	if (static_cast<Undo &>(*undo).isValid()) {
		undo->redo();
		TUndoManager::manager()->add(undo.release());
	}
}

//=============================================================================
// addPage
//-----------------------------------------------------------------------------

namespace
{

class AddPageUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle;
	TPaletteP m_palette;
	int m_pageIndex;
	std::wstring m_pageName;
	std::vector<std::pair<TColorStyle *, int>> m_styles;

public:
	// creare DOPO l'inserimento
	AddPageUndo(
		TPaletteHandle *paletteHandle,
		int pageIndex,
		std::wstring pageName)
		: m_paletteHandle(paletteHandle), m_pageIndex(pageIndex), m_pageName(pageName)
	{
		m_palette = m_paletteHandle->getPalette();
		assert(m_palette);
		assert(0 <= m_pageIndex && m_pageIndex < m_palette->getPageCount());
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		for (int i = 0; i < page->getStyleCount(); i++) {
			std::pair<TColorStyle *, int> p;
			p.first = page->getStyle(i)->clone();
			p.second = page->getStyleId(i);
			m_styles.push_back(p);
		}
	}
	~AddPageUndo()
	{
		for (int i = 0; i < (int)m_styles.size(); i++)
			delete m_styles[i].first;
	}
	void undo() const
	{
		m_palette->erasePage(m_pageIndex);
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		TPalette::Page *page = m_palette->addPage(m_pageName);
		assert(page);
		assert(page->getIndex() == m_pageIndex);
		for (int i = 0; i < (int)m_styles.size(); i++) {
			TColorStyle *cs = m_styles[i].first->clone();
			int styleId = m_styles[i].second;
			assert(m_palette->getStylePage(styleId) == 0);
			m_palette->setStyle(styleId, cs);
			page->addStyle(styleId);
		};
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const { return sizeof *this + m_styles.size() * sizeof(TColorStyle); }

	QString getHistoryString()
	{
		return QObject::tr("Add Page %1 to Palette %2")
			.arg(QString::fromStdWString(m_pageName))
			.arg(QString::fromStdWString(m_palette->getPaletteName()));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

//-----------------------------------------------------------------------------

void PaletteCmd::addPage(TPaletteHandle *paletteHandle, std::wstring name, bool withUndo)
{
	TPalette *palette = paletteHandle->getPalette();
	if (name == L"")
		name = L"page " + toWideString(palette->getPageCount() + 1);
	TPalette::Page *page = palette->addPage(name);

	palette->setDirtyFlag(true);

	paletteHandle->notifyPaletteChanged();
	if (withUndo)
		TUndoManager::manager()->add(new AddPageUndo(paletteHandle, page->getIndex(), name));
}

//=============================================================================
// destroyPage
//-----------------------------------------------------------------------------

namespace
{

class DestroyPageUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle;
	TPaletteP m_palette;
	int m_pageIndex;
	std::wstring m_pageName;
	std::vector<int> m_styles;

public:
	DestroyPageUndo(TPaletteHandle *paletteHandle, int pageIndex)
		: m_paletteHandle(paletteHandle), m_pageIndex(pageIndex)
	{
		m_palette = m_paletteHandle->getPalette();
		assert(m_palette);
		assert(0 <= pageIndex && pageIndex < m_palette->getPageCount());
		assert(m_palette->getPageCount() > 1);
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		m_pageName = page->getName();
		m_styles.resize(page->getStyleCount());
		for (int i = 0; i < page->getStyleCount(); i++)
			m_styles[i] = page->getStyleId(i);
	}
	void undo() const
	{
		TPalette::Page *page = m_palette->addPage(m_pageName);
		m_palette->movePage(page, m_pageIndex);
		for (int i = 0; i < (int)m_styles.size(); i++)
			page->addStyle(m_styles[i]);
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		m_palette->erasePage(m_pageIndex);
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const { return sizeof *this; }
	QString getHistoryString()
	{
		return QObject::tr("Delete Page %1 from Palette %2")
			.arg(QString::fromStdWString(m_pageName))
			.arg(QString::fromStdWString(m_palette->getPaletteName()));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

//-----------------------------------------------------------------------------

void PaletteCmd::destroyPage(TPaletteHandle *paletteHandle, int pageIndex)
{
	TPalette *palette = paletteHandle->getPalette();
	assert(0 <= pageIndex && pageIndex < palette->getPageCount());

	TUndoManager::manager()->add(new DestroyPageUndo(paletteHandle, pageIndex));
	palette->erasePage(pageIndex);

	palette->setDirtyFlag(true);

	paletteHandle->notifyPaletteChanged();
}

//===================================================================
// ReferenceImage
//-------------------------------------------------------------------

namespace
{

//===================================================================
// SetReferenceImageUndo
//-------------------------------------------------------------------

class SetReferenceImageUndo : public TUndo
{
	TPaletteP m_palette;
	TPaletteP m_oldPalette, m_newPalette;

	TPaletteHandle *m_paletteHandle;

public:
	SetReferenceImageUndo(TPaletteP palette, TPaletteHandle *paletteHandle)
		: m_palette(palette), m_oldPalette(palette->clone()), m_paletteHandle(paletteHandle)
	{
	}
	void onAdd()
	{
		m_newPalette = m_palette->clone();
	}
	void undo() const
	{
		m_palette->assign(m_oldPalette.getPointer());
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		m_palette->assign(m_newPalette.getPointer());
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const
	{
		return sizeof(*this) + sizeof(TPalette) * 2;
	}
	QString getHistoryString()
	{
		return QObject::tr("Load Color Model %1  to Palette %2")
			.arg(QString::fromStdString(m_newPalette->getRefImgPath().getLevelName()))
			.arg(QString::fromStdWString(m_palette->getPaletteName()));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

//===================================================================
// loadRefImage
//-------------------------------------------------------------------

int loadRefImage(TPaletteHandle *paletteHandle, bool replace,
				 TPaletteP levelPalette, const TFilePath &_fp, int &frame, ToonzScene *scene,
				 const std::vector<int> &frames)
{
	bool paletteAlreadyLoaded = false;
	TFilePath fp = scene->decodeFilePath(_fp);
	if (_fp == TFilePath()) {
		paletteAlreadyLoaded = true;
		fp = levelPalette->getRefImgPath();
	}

	TImageP img;
	try {
		TLevelReaderP lr(fp);
		if (fp != TFilePath() && lr) {
			TLevelP level = lr->loadInfo();
			if (level && level->getFrameCount() > 0) {
				TLevel::Iterator it;
				if (!paletteAlreadyLoaded) {
					std::vector<TFrameId> fids;
					for (it = level->begin(); it != level->end(); ++it) {
						if (it->first == -1 || it->first == -2) {
							assert(level->getFrameCount() == 1);
							fids.push_back(0);
							break;
						}
						// if the frame list is empty, store all fids of the level
						if (frames.empty()) {
							fids.push_back(it->first);
							continue;
						}
						// if the frame list is specified, load only the frames matches with the list
						else {
							std::vector<int>::const_iterator framesIt;
							for (framesIt = frames.begin(); framesIt != frames.end(); framesIt++) {
								if (it->first.getNumber() == *framesIt) {
									fids.push_back(it->first);
									break;
								}
							}
						}
					}
					levelPalette->setRefLevelFids(fids);
				}

				if (frame <= 0)
					frame = level->begin()->first.getNumber();

				const TLevel::Table *table = level->getTable();
				TFrameId fid(frame);

				if (table->find(fid) != table->end()) {
					img = lr->getFrameReader(fid)->load();
					if (img && img->getPalette() == 0) {
						if (paletteAlreadyLoaded || level->getPalette() != 0)
							img->setPalette(level->getPalette());
						else if ((fp.getType() == "tzp" || fp.getType() == "tzu"))
							img->setPalette(ToonzImageUtils::loadTzPalette(fp.withType("plt").withNoFrame()));
					}
				}
			}
		} else
			img = levelPalette->getRefImg();
	} catch (TException &e) {
		std::wcout << L"error: " << e.getMessage() << std::endl;
	} catch (...) {
		std::cout << "error for other reasons" << std::endl;
	}

	if (!img)
		return 1;

	if (paletteAlreadyLoaded) {
		img->setPalette(0);
		levelPalette->setRefImgPath(_fp);
		return 0;
	}

	TUndo *undo = new SetReferenceImageUndo(levelPalette, paletteHandle);

	if (!replace) //ret==1)
	{
		TPaletteP imagePalette;
		if (TRasterImageP ri = img) {
			TRaster32P raster = ri->getRaster();
			if (raster) {
				std::set<TPixel32> colors;
				int colorCount = 256;
				if (Preferences::instance()->getPaletteTypeOnLoadRasterImageAsColorModel() == 0)
					/*-- 全ての異なるピクセルの色を別のStyleにする --*/
					TColorUtils::buildPrecisePalette(colors, raster, colorCount);
				else
					/*-- 似ている色をまとめて1つのStyleにする --*/
					TColorUtils::buildPalette(colors, raster, colorCount);
				colors.erase(TPixel::Black); //il nero viene messo dal costruttore della TPalette
				imagePalette = new TPalette();
				std::set<TPixel32>::const_iterator it = colors.begin();
				for (; it != colors.end(); ++it)
					imagePalette->getPage(0)->addStyle(*it);
			}
		} else
			imagePalette = img->getPalette();

		if (imagePalette) {
			std::wstring gName = levelPalette->getGlobalName();
			//Se sto caricando un reference image su una studio palette
			if (!gName.empty()) {
				imagePalette->setGlobalName(gName);
				StudioPalette::instance()->setStylesGlobalNames(imagePalette.getPointer());
			}
			// voglio evitare di sostituire una palette con pochi colori ad una con tanti colori
			/*-- ColorModelの色数が少ないのにOverwriteしようとした場合は、余分の分だけStyleが追加される --*/
			while (imagePalette->getStyleCount() < levelPalette->getStyleCount()) {
				int index = imagePalette->getStyleCount();
				assert(index < levelPalette->getStyleCount());
				TColorStyle *style = levelPalette->getStyle(index)->clone();
				imagePalette->addStyle(style);
			}
			levelPalette->assign(imagePalette.getPointer());
		}
	}

	img->setPalette(0);

	levelPalette->setRefImgPath(_fp);
	if (!replace)
		levelPalette->setDirtyFlag(true);

	TUndoManager::manager()->add(undo);
	paletteHandle->notifyPaletteChanged();

	return 0;
}

//-------------------------------------------------------------------
} // namespace
//-------------------------------------------------------------------

//===================================================================
// loadReferenceImage
// load frames specified in frames. if frames is empty, then load all frames of the level.
// return values -- 2: failed_to_get_palette, 1: failed_to_get_image, 0: OK
//-------------------------------------------------------------------

int PaletteCmd::loadReferenceImage(TPaletteHandle *paletteHandle, bool replace,
								   const TFilePath &_fp, int &frame, ToonzScene *scene,
								   const std::vector<int> &frames)
{
	TPaletteP levelPalette = paletteHandle->getPalette();
	if (!levelPalette)
		return 2;

	int ret = loadRefImage(paletteHandle, replace, levelPalette, _fp, frame, scene, frames);
	if (ret != 0)
		return ret;

	// when choosing replace(Keep the destination palette), dirty flag is unchanged
	if (!replace)
		levelPalette->setDirtyFlag(true);

	return 0;
}

//===================================================================
// removeReferenceImage
//-------------------------------------------------------------------

void PaletteCmd::removeReferenceImage(TPaletteHandle *paletteHandle)
{
	TPaletteP levelPalette = paletteHandle->getPalette();
	if (!levelPalette)
		return;
	TUndo *undo = new SetReferenceImageUndo(levelPalette, paletteHandle);

	levelPalette->setRefImg(TImageP());
	levelPalette->setRefImgPath(TFilePath());

	std::vector<TFrameId> fids;
	levelPalette->setRefLevelFids(fids);

	levelPalette->setDirtyFlag(true);
	paletteHandle->notifyPaletteChanged();

	TUndoManager::manager()->add(undo);
}

//=============================================================================
// MovePage
//-----------------------------------------------------------------------------

namespace
{

class MovePageUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle;
	TPaletteP m_palette;
	int m_srcIndex;
	int m_dstIndex;

public:
	MovePageUndo(TPaletteHandle *paletteHandle, int srcIndex, int dstIndex)
		: m_paletteHandle(paletteHandle), m_srcIndex(srcIndex), m_dstIndex(dstIndex)
	{
		m_palette = m_paletteHandle->getPalette();
	}
	void undo() const
	{
		m_palette->movePage(m_palette->getPage(m_dstIndex), m_srcIndex);
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		m_palette->movePage(m_palette->getPage(m_srcIndex), m_dstIndex);
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const { return sizeof *this; }
	QString getHistoryString()
	{
		return QObject::tr("Move Page");
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

void PaletteCmd::movePalettePage(TPaletteHandle *paletteHandle, int srcIndex, int dstIndex)
{
	TPaletteP palette = paletteHandle->getPalette();
	palette->movePage(palette->getPage(srcIndex), dstIndex);
	TUndoManager::manager()->add(new MovePageUndo(paletteHandle, srcIndex, dstIndex));
	paletteHandle->notifyPaletteChanged();
}

//=============================================================================
// RenamePage
//-----------------------------------------------------------------------------

namespace
{

class RenamePageUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle;
	TPaletteP m_palette;
	int m_pageIndex;
	std::wstring m_newName;
	std::wstring m_oldName;

public:
	RenamePageUndo(TPaletteHandle *paletteHandle, int pageIndex, const std::wstring &newName)
		: m_paletteHandle(paletteHandle), m_pageIndex(pageIndex), m_newName(newName)
	{
		m_palette = m_paletteHandle->getPalette();
		m_oldName = m_palette->getPage(pageIndex)->getName();
	}
	void undo() const
	{
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		page->setName(m_oldName);
		m_paletteHandle->notifyPaletteChanged();
	}
	void redo() const
	{
		TPalette::Page *page = m_palette->getPage(m_pageIndex);
		assert(page);
		page->setName(m_newName);
		m_paletteHandle->notifyPaletteChanged();
	}
	int getSize() const { return sizeof *this; }
	QString getHistoryString()
	{
		return QObject::tr("Rename Page  %1 > %2")
			.arg(QString::fromStdWString(m_oldName))
			.arg(QString::fromStdWString(m_newName));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

void PaletteCmd::renamePalettePage(TPaletteHandle *paletteHandle, int pageIndex, const std::wstring &newName)
{
	if (!paletteHandle)
		return;
	TPalette *palette = paletteHandle->getPalette();
	if (!palette)
		return;
	if (pageIndex < 0 || pageIndex >= palette->getPageCount())
		return;
	RenamePageUndo *undo = new RenamePageUndo(paletteHandle, pageIndex, newName);
	paletteHandle->notifyPaletteChanged();
	TPalette::Page *page = palette->getPage(pageIndex);
	assert(page);
	page->setName(newName);
	palette->setDirtyFlag(true);
	paletteHandle->notifyPaletteChanged();
	TUndoManager::manager()->add(undo);
}

//=============================================================================
// RenamePaletteStyle
//-----------------------------------------------------------------------------

namespace
{

class RenamePaletteStyleUndo : public TUndo
{
	TPaletteHandle *m_paletteHandle; //Usato nell'undo e nel redo per lanciare la notifica di cambiamento
	int m_styleId;
	TPaletteP m_palette;
	std::wstring m_newName;
	std::wstring m_oldName;

public:
	RenamePaletteStyleUndo(TPaletteHandle *paletteHandle, const std::wstring &newName)
		: m_paletteHandle(paletteHandle), m_newName(newName)
	{
		m_palette = paletteHandle->getPalette();
		assert(m_palette);
		m_styleId = paletteHandle->getStyleIndex();
		TColorStyle *style = m_palette->getStyle(m_styleId);
		assert(style);
		m_oldName = style->getName();
	}
	void undo() const
	{
		TColorStyle *style = m_palette->getStyle(m_styleId);
		assert(style);
		style->setName(m_oldName);
		m_paletteHandle->notifyColorStyleChanged(false);
	}
	void redo() const
	{
		TColorStyle *style = m_palette->getStyle(m_styleId);
		assert(style);
		style->setName(m_newName);
		m_paletteHandle->notifyColorStyleChanged(false);
	}
	int getSize() const { return sizeof *this; }
	QString getHistoryString()
	{
		return QObject::tr("Rename Style#%1 in Palette%2  : %3 > %4")
			.arg(QString::number(m_styleId))
			.arg(QString::fromStdWString(m_palette->getPaletteName()))
			.arg(QString::fromStdWString(m_oldName))
			.arg(QString::fromStdWString(m_newName));
	}
	int getHistoryType()
	{
		return HistoryType::Palette;
	}
};

} // namespace

void PaletteCmd::renamePaletteStyle(TPaletteHandle *paletteHandle, const std::wstring &newName)
{
	if (!paletteHandle)
		return;
	TPalette *palette = paletteHandle->getPalette();
	if (!palette)
		return;
	TColorStyle *style = paletteHandle->getStyle();
	if (!style)
		return;
	if (style->getName() == newName)
		return;

	RenamePaletteStyleUndo *undo = new RenamePaletteStyleUndo(paletteHandle, newName);
	style->setName(newName);
	palette->setDirtyFlag(true);
	paletteHandle->notifyColorStyleChanged(false);
	TUndoManager::manager()->add(undo);
}
