

#ifndef TXSH_ZERARYFX_COLUMN_INCLUDED
#define TXSH_ZERARYFX_COLUMN_INCLUDED

#include "toonz/txshcolumn.h"

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//=============================================================================
//forward declarations
class TFx;
class TZeraryColumnFx;
class TXshZeraryFxLevel;
class TXshCell;

//=============================================================================
//!The TXshZeraryFxColumn class provides a zerary fx column in xsheet and allows its management.
/*!Inherits \b TXshCellColumn.
\n The class defines zerary fx column, more than \b TXshCellColumn has a pointer to
   \b TZeraryColumnFx getZeraryColumnFx() and a pointer to \b TXshZeraryFxLevel.
*/
//=============================================================================

class DVAPI TXshZeraryFxColumn : public TXshCellColumn
{
	PERSIST_DECLARATION(TXshZeraryFxColumn)
	TZeraryColumnFx *m_zeraryColumnFx;
	TXshZeraryFxLevel *m_zeraryFxLevel;

public:
	/*!
    Constructs a TXshZeraryFxColumn with default value.
  */
	TXshZeraryFxColumn(int frameCount = 100);
	/*!
    Constructs a TXshZeraryFxColumn object that is a copy of the TXshZeraryFxColumn
    object \a src.
    \sa clone()
  */
	TXshZeraryFxColumn(const TXshZeraryFxColumn &src);
	/*!
    Destroys the TXshZeraryFxColumn object.
  */
	~TXshZeraryFxColumn();

	TXshColumn::ColumnType getColumnType() const;

	/*! Return true if level which cell \b cell belongs is equal to \b m_zeraryFxLevel. */
	bool canSetCell(const TXshCell &cell) const;

	/*! Clone column and return a pointer to the new \b TXshColumn cloned. */
	TXshColumn *clone() const;

	/*!
    Return \b TZeraryColumnFx.
  */
	TZeraryColumnFx *getZeraryColumnFx() const { return m_zeraryColumnFx; }

	TXshZeraryFxColumn *getZeraryFxColumn() { return this; }

	bool setCell(int row, const TXshCell &cell);
	/*! Return false if cannot set cells.*/
	bool setCells(int row, int rowCount, const TXshCell cells[]);

	/*! Return a pointer to \b TFx \b m_zeraryColumnFx. */
	TFx *getFx() const;

	void loadData(TIStream &is);
	void saveData(TOStream &os);

	/*! Implement method isn't necessary because TXshZeraryFxColumn doesn't have icon. */
	void updateIcon() {}

private:
	// not implemented
	TXshZeraryFxColumn &operator=(const TXshZeraryFxColumn &);
};

#ifdef _WIN32
template class DVAPI TSmartPointerT<TXshZeraryFxColumn>;
#endif

typedef TSmartPointerT<TXshZeraryFxColumn> TXshZeraryFxColumnP;

#endif
