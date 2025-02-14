

#include "toonzqt/spreadsheetviewer.h"
#include "toonzqt/gutil.h"

#include "toonz/tframehandle.h"

#include <QKeyEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QScrollBar>
#include <QPainter>
#include <QGridLayout>
#include <QPaintEvent>
#include <QToolTip>
#include <assert.h>

#include <QMainWindow>
#include <QDockWidget>

namespace Spreadsheet
{

//=============================================================================
FrameScroller::FrameScroller()
{
}

FrameScroller::~FrameScroller()
{
}

void FrameScroller::connectScroller(FrameScroller *scroller)
{
	if (scroller != this && !m_connectedScrollers.contains(scroller)) {
		m_connectedScrollers.append(scroller);
		scroller->connectScroller(this);
		QScrollBar *sb0 = getFrameScrollArea()->verticalScrollBar();
		QScrollBar *sb1 = scroller->getFrameScrollArea()->verticalScrollBar();
		QObject::connect(sb0, SIGNAL(valueChanged(int)), sb1, SLOT(setValue(int)));
		QObject::connect(sb1, SIGNAL(valueChanged(int)), sb0, SLOT(setValue(int)));
	}
}

void FrameScroller::disconnectScroller(FrameScroller *scroller)
{
	if (m_connectedScrollers.contains(scroller)) {
		m_connectedScrollers.removeAll(scroller);
		scroller->disconnectScroller(this);
		QScrollBar *sb0 = getFrameScrollArea()->verticalScrollBar();
		QScrollBar *sb1 = scroller->getFrameScrollArea()->verticalScrollBar();
		QObject::disconnect(sb0, SIGNAL(valueChanged(int)), sb1, SLOT(setValue(int)));
		QObject::disconnect(sb1, SIGNAL(valueChanged(int)), sb0, SLOT(setValue(int)));
	}
}

bool FrameScroller::isScrollerConnected(FrameScroller *scroller)
{
	return m_connectedScrollers.contains(scroller);
}

static QList<FrameScroller *> frameScrollers;

void FrameScroller::registerFrameScroller()
{
	if (!frameScrollers.contains(this)) {
		for (int i = 0; i < frameScrollers.size(); i++)
			connectScroller(frameScrollers[i]);
		frameScrollers.append(this);
	}
}

void FrameScroller::unregisterFrameScroller()
{
	if (frameScrollers.contains(this)) {
		frameScrollers.removeAll(this);
		for (int i = 0; i < frameScrollers.size(); i++)
			disconnectScroller(frameScrollers[i]);
	}
}

void FrameScroller::prepareToScroll(int dy)
{
	if (dy == 0)
		return;
	for (int i = 0; i < frameScrollers.size(); i++)
		if (frameScrollers[i] != this)
			frameScrollers[i]->onPrepareToScroll(dy);
}

//=============================================================================

void SetFrameDragTool::click(int row, int col, QMouseEvent *e)
{
	m_frameHandle->setFrame(row);
}

void SetFrameDragTool::drag(int row, int col, QMouseEvent *e)
{
	if (row < 0)
		row = 0;
	m_frameHandle->setFrame(row);
}

void SetFrameDragTool::release(int row, int col, QMouseEvent *e)
{
}

//=============================================================================
//
// SelectionDragTool
//
//-----------------------------------------------------------------------------

SelectionDragTool::SelectionDragTool(SpreadsheetViewer *viewer)
	: m_viewer(viewer), m_firstRow(-1), m_firstCol(-1)
{
}

void SelectionDragTool::click(int row, int col, QMouseEvent *e)
{
	m_firstCol = col;
	m_firstRow = row;
	QRect selectedCells(col, row, 1, 1);
	m_viewer->selectCells(selectedCells);
}

void SelectionDragTool::drag(int row, int col, QMouseEvent *e)
{
	int r0 = qMin(row, m_firstRow);
	int r1 = qMax(row, m_firstRow);
	int c0 = qMin(col, m_firstCol);
	int c1 = qMax(col, m_firstCol);
	QRect selectedCells(c0, r0, c1 - c0 + 1, r1 - r0 + 1);
	m_viewer->selectCells(selectedCells);
}

void SelectionDragTool::release(int row, int col, QMouseEvent *e)
{
}

//=============================================================================

PanTool::PanTool(Spreadsheet::GenericPanel *panel)
	: m_panel(panel), m_viewer(panel->getViewer()), m_lastPos()
{
}

void PanTool::click(int row, int col, QMouseEvent *e)
{
	m_lastPos = e->pos(); // m_panel->mapToGlobal(e->pos());
}
void PanTool::drag(int row, int col, QMouseEvent *e)
{
	QPoint pos = e->pos(); // m_panel->mapToGlobal(e->pos());
	//QPoint delta = p - m_lastPos;
	//m_lastPos = p;
	// QToolTip::showText(p,"delta="+QString::number(delta.x())+","+QString::number(delta.y()));
	m_viewer->scroll(m_lastPos - pos);
}
void PanTool::release(int row, int col, QMouseEvent *e)
{
	//QToolTip::hideText();
}

//=============================================================================

#if QT_VERSION >= 0x050500
ScrollArea::ScrollArea(QWidget *parent, Qt::WindowFlags flags)
#else
ScrollArea::ScrollArea(QWidget *parent, Qt::WFlags flags)
#endif
	: QScrollArea(parent)
{
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setLineWidth(6);
	setContentsMargins(10, 10, 10, 10);
}

ScrollArea::~ScrollArea()
{
}

void ScrollArea::keyPressEvent(QKeyEvent *e)
{
	e->ignore();
}

void ScrollArea::wheelEvent(QWheelEvent *e)
{
	e->ignore();
}

//=============================================================================

GenericPanel::GenericPanel(SpreadsheetViewer *viewer)
	: QWidget(viewer), m_viewer(viewer), m_dragTool(0)
{
	setFocusPolicy(Qt::NoFocus);
}

GenericPanel::~GenericPanel()
{
}

void GenericPanel::paintEvent(QPaintEvent *e)
{
	QPainter p(this);
	p.setPen(m_viewer->getLightLineColor());
	for (int c = 0;; c++) {
		int x = getViewer()->columnToX(c);
		if (x > width())
			break;
		p.drawLine(x, 0, x, height());
	}
	for (int r = 0;; r++) {
		int y = getViewer()->rowToY(r);
		if (y > height())
			break;
		p.drawLine(0, y, width(), y);
	}
	p.setPen(Qt::magenta);
	p.drawLine(e->rect().topLeft(), e->rect().bottomRight());
}

void GenericPanel::mousePressEvent(QMouseEvent *e)
{
	assert(!m_dragTool);
	if (e->button() == Qt::MidButton)
		m_dragTool = new PanTool(this);
	else
		m_dragTool = createDragTool(e);

	int row = getViewer()->yToRow(e->pos().y());
	int col = getViewer()->xToColumn(e->pos().x());
	if (m_dragTool)
		m_dragTool->click(row, col, e);
}

void GenericPanel::mouseReleaseEvent(QMouseEvent *e)
{
	int row = getViewer()->yToRow(e->pos().y());
	int col = getViewer()->xToColumn(e->pos().x());
	m_viewer->stopAutoPan();
	if (m_dragTool) {
		m_dragTool->release(row, col, e);
		delete m_dragTool;
		m_dragTool = 0;
	}
}

void GenericPanel::mouseMoveEvent(QMouseEvent *e)
{
	int row = getViewer()->yToRow(e->pos().y());
	int col = getViewer()->xToColumn(e->pos().x());
	if (e->buttons() != 0 && m_dragTool != 0) {
		if ((e->buttons() & Qt::LeftButton) != 0 && !visibleRegion().contains(e->pos())) {
			QRect bounds = visibleRegion().boundingRect();
			m_viewer->setAutoPanSpeed(bounds, e->pos());
		} else
			m_viewer->stopAutoPan();
		m_dragTool->drag(row, col, e);
	}
}

//=============================================================================

RowPanel::RowPanel(SpreadsheetViewer *viewer)
	: GenericPanel(viewer), m_xa(12)
{
}

//-----------------------------------------------------------------------------

DragTool *RowPanel::createDragTool(QMouseEvent *)
{
	TFrameHandle *frameHandle = getViewer()->getFrameHandle();
	if (frameHandle)
		return new SetFrameDragTool(frameHandle);
	else
		return 0;
}

//-----------------------------------------------------------------------------

void RowPanel::drawRows(QPainter &p, int r0, int r1)
{
#ifdef _WIN32
	static QFont font("Arial", 9, QFont::Bold);
#else
	static QFont font("Helvetica", 9, QFont::Bold);
#endif
	p.setFont(font);

	QRect visibleRect = visibleRegion().boundingRect();
	int x0 = visibleRect.left();
	int x1 = visibleRect.right();
	int y0 = visibleRect.top();
	int y1 = visibleRect.bottom();

	int r;
	for (r = r0; r <= r1; r++) {
		int y = getViewer()->rowToY(r);
		//draw horizontal line
		QColor color = (getViewer()->isMarkRow(r)) ? getViewer()->getMarkerLineColor() : getViewer()->getLightLineColor();
		p.setPen(color);
		p.drawLine(x0, y, x1, y);

		//draw numbers
		p.setPen(getViewer()->getTextColor());

		QString number = QString::number(r + 1);
		p.drawText(QRect(x0, y + 1, width(), 18), Qt::AlignHCenter | Qt::AlignBottom, number);
	}
	// erase the marker interval at upper-end
	if (r0 == 0) {
		p.setPen(getViewer()->getLightLineColor());
		p.drawLine(x0, getViewer()->rowToY(0), x1, getViewer()->rowToY(0));
	}
}

//-----------------------------------------------------------------------------

void RowPanel::drawCurrentRowGadget(QPainter &p, int r0, int r1)
{
	int currentRow = getViewer()->getCurrentRow();
	int y = getViewer()->rowToY(currentRow);
	if (currentRow < r0 || r1 < currentRow)
		return;
	p.fillRect(1, y + 1, width() - 2, 19, getViewer()->getCurrentRowBgColor());
}

//-----------------------------------------------------------------------------

void RowPanel::paintEvent(QPaintEvent *e)
{
	QRect toBeUpdated = e->rect();
	QPainter p(this);

	// range di righe visibili
	int r0 = getViewer()->yToRow(toBeUpdated.top());
	int r1 = getViewer()->yToRow(toBeUpdated.bottom());

	p.setClipRect(toBeUpdated);
	p.fillRect(toBeUpdated, QBrush(getViewer()->getLightLightBGColor()));

	drawCurrentRowGadget(p, r0, r1);
	drawRows(p, r0, r1);
}

//=============================================================================

ColumnPanel::ColumnPanel(SpreadsheetViewer *viewer)
	: GenericPanel(viewer)
{
}

//=============================================================================

CellPanel::CellPanel(SpreadsheetViewer *viewer)
	: GenericPanel(viewer)
{
}

DragTool *CellPanel::createDragTool(QMouseEvent *)
{
	// FunctionSheetCellViewer::createDragTool is called instead
	// when clicking on the CellPanel in NumericalColumns
	return new SelectionDragTool(getViewer());
}

void CellPanel::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);

	QRect toBeUpdated = e->rect();
	painter.setClipRect(toBeUpdated);

	int x0 = toBeUpdated.left() - 1;
	int y0 = toBeUpdated.top();
	int x1 = toBeUpdated.right(), y1 = toBeUpdated.bottom();

	// visible rows range
	int r0 = getViewer()->yToRow(y0);
	int r1 = getViewer()->yToRow(y1);

	// visible columns range
	int c0 = getViewer()->xToColumn(x0);
	int c1 = getViewer()->xToColumn(x1);

	// cambia colore alle celle prima di rowCount()
	int rowCount = getViewer()->getRowCount();

	//fill with bg color
	painter.fillRect(toBeUpdated, getViewer()->getLightLightBGColor());

	//scene range bg
	int yLast = getViewer()->rowToY(rowCount);
	if (yLast < y1)
		painter.fillRect(x0, y0, x1 - x0, yLast - y0, getViewer()->getBGColor());
	else
		painter.fillRect(toBeUpdated, getViewer()->getBGColor());

	//draw cells
	drawCells(painter, r0, c0, r1, c1);

	// draw columns
	painter.setPen(getViewer()->getVerticalLineColor());
	for (int col = c0; col <= c1; col++) {
		int x = getViewer()->columnToX(col);
		painter.drawLine(x, y0, x, y1);
	}

	// draw rows
	int currentRow = getViewer()->getCurrentRow();
	for (int r = r0; r <= r1; r++) {
		int y = getViewer()->rowToY(r);
		QColor color = getViewer()->isMarkRow(r) ? getViewer()->getMarkerLineColor() : getViewer()->getLightLineColor();
		painter.setPen(color);
		painter.drawLine(x0, y, x1, y);
	}
	// erase the marker interval at upper-end
	painter.setPen(getViewer()->getLightLineColor());
	painter.drawLine(x0, 0, x1, 0);
}

} // namespace Spreadsheet

//=============================================================================

SpreadsheetViewer::SpreadsheetViewer(QWidget *parent)
	: QFrame(parent), m_columnScrollArea(0), m_rowScrollArea(0), m_cellScrollArea(0), m_frameHandle(0), m_columnWidth(50), m_rowHeight(20), m_timerId(0), m_autoPanSpeed(0, 0), m_lastAutoPanPos(0, 0), m_rowCount(0), m_columnCount(0), m_currentRow(0), m_markRowDistance(6), m_markRowOffset(0), m_isComputingSize(false)
{
	setFocusPolicy(Qt::NoFocus);

	setFrameStyle(QFrame::StyledPanel);
	setObjectName("Viewer");

	//column header
	m_columnScrollArea = new Spreadsheet::ScrollArea;
	m_columnScrollArea->setObjectName("ScrollArea");
	m_columnScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_columnScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_columnScrollArea->setFocusPolicy(Qt::NoFocus);

	//row area
	m_rowScrollArea = new Spreadsheet::ScrollArea;
	m_rowScrollArea->setObjectName("ScrollArea");
	m_rowScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_rowScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_rowScrollArea->setFocusPolicy(Qt::NoFocus);

	//cell area
	m_cellScrollArea = new Spreadsheet::ScrollArea;
	m_cellScrollArea->setObjectName("ScrollArea");
	m_cellScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_cellScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_cellScrollArea->horizontalScrollBar()->setObjectName("XsheetScrollBar");
	m_cellScrollArea->verticalScrollBar()->setObjectName("XsheetScrollBar");
	m_cellScrollArea->setFocusPolicy(Qt::NoFocus);

	m_columnScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed));
	m_rowScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored));

	m_rowScrollArea->setFixedWidth(30);
	m_columnScrollArea->setFixedHeight(m_rowHeight * 3 - 3);
	//m_columnScrollArea->setFixedHeight(m_rowHeight * 3 + 60 - 63);

	//---- layout
	QGridLayout *layout = new QGridLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	{
		layout->addWidget(m_columnScrollArea, 0, 1);
		layout->addWidget(m_rowScrollArea, 1, 0);
		layout->addWidget(m_cellScrollArea, 1, 1, 2, 2);

		//upper-right
		QWidget *w = new QWidget(this);
		w->setFixedSize(QSize(20, m_rowHeight * 3 + 60 - 63));
		layout->addWidget(w, 0, 2);

		//lower-left
		w = new QWidget(this);
		w->setFixedSize(QSize(30, 20));
		layout->addWidget(w, 2, 0);

		layout->setColumnStretch(0, 0);
		layout->setColumnStretch(1, 1);
		layout->setColumnStretch(2, 0);
		layout->setRowStretch(0, 0);
		layout->setRowStretch(1, 1);
		layout->setRowStretch(2, 0);
	}
	setLayout(layout);

	//---signal-slot connections

	// vertical slider: cell <=> row
	connect(m_rowScrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
			m_cellScrollArea->verticalScrollBar(), SLOT(setValue(int)));
	connect(m_cellScrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
			m_rowScrollArea->verticalScrollBar(), SLOT(setValue(int)));

	// horizontal slider: cell <=> column
	connect(m_columnScrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
			m_cellScrollArea->horizontalScrollBar(), SLOT(setValue(int)));
	connect(m_cellScrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
			m_columnScrollArea->horizontalScrollBar(), SLOT(setValue(int)));

	connect(m_cellScrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
			SLOT(onVSliderChanged(int)));
	connect(m_cellScrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
			SLOT(onHSliderChanged(int)));
}

SpreadsheetViewer::~SpreadsheetViewer()
{
}

void SpreadsheetViewer::setFrameHandle(TFrameHandle *frameHandle)
{
	if (m_frameHandle == frameHandle)
		return;
	if (m_frameHandle)
		m_frameHandle->disconnect(this);
	m_frameHandle = frameHandle;

	if (isVisible() && m_frameHandle) {
		connect(m_frameHandle, SIGNAL(frameSwitched()), this, SLOT(onFrameSwitched()));
		update();
	}
}

void SpreadsheetViewer::setRowsPanel(Spreadsheet::RowPanel *rows)
{
	m_rowScrollArea->setWidget(rows);
}

void SpreadsheetViewer::setColumnsPanel(Spreadsheet::ColumnPanel *columns)
{
	m_columnScrollArea->setWidget(columns);
	// columns->setFixedHeight(200);
}

void SpreadsheetViewer::setCellsPanel(Spreadsheet::CellPanel *cells)
{
	m_cellScrollArea->setWidget(cells);
}

void SpreadsheetViewer::setRowCount(int rowCount)
{
	if (m_rowCount != rowCount) {
		m_rowCount = rowCount;
		refreshContentSize(0, 0);
	}
}

void SpreadsheetViewer::setColumnCount(int columnCount)
{
	if (m_columnCount != columnCount) {
		m_columnCount = columnCount;
		refreshContentSize(0, 0);
	}
}

void SpreadsheetViewer::scroll(QPoint delta)
{
	refreshContentSize(delta.x(), delta.y());
	prepareToScroll(delta.y());
	if (delta.y() != 0) {
		QScrollBar *vSc = m_cellScrollArea->verticalScrollBar();
		vSc->setValue(vSc->value() + delta.y());
	}
	if (delta.x() != 0) {
		QScrollBar *hSc = m_cellScrollArea->horizontalScrollBar();
		hSc->setValue(hSc->value() + delta.x());
	}
}

void SpreadsheetViewer::setAutoPanSpeed(const QPoint &speed)
{
	bool wasAutoPanning = isAutoPanning();
	m_autoPanSpeed = speed;
	if (isAutoPanning() && !wasAutoPanning && m_timerId == 0)
		m_timerId = startTimer(40);
	else if (!isAutoPanning() && wasAutoPanning && m_timerId != 0) {
		killTimer(m_timerId);
		m_timerId = 0;
	}
}

//-----------------------------------------------------------------------------

int getAutoPanSpeed(int pixels)
{
	int f = 40;
	return tmin(100, (f - 1 + pixels * f) / 100);
}

//-----------------------------------------------------------------------------

void SpreadsheetViewer::setAutoPanSpeed(const QRect &widgetBounds, const QPoint &mousePos)
{
	QPoint speed;
	int limit = 100, factor = 30;
	if (mousePos.x() < widgetBounds.left())
		speed.setX(-getAutoPanSpeed(widgetBounds.left() - mousePos.x()));
	else if (mousePos.x() > widgetBounds.right())
		speed.setX(getAutoPanSpeed(mousePos.x() - widgetBounds.right()));
	if (mousePos.y() < widgetBounds.top())
		speed.setY(-getAutoPanSpeed(widgetBounds.top() - mousePos.y()));
	else if (mousePos.y() > widgetBounds.bottom())
		speed.setY(getAutoPanSpeed(mousePos.y() - widgetBounds.bottom()));
	setAutoPanSpeed(speed);
	m_lastAutoPanPos = mousePos;
}

/*!Shift is a consequence of style sheet border.*/
int SpreadsheetViewer::xToColumn(int x) const
{
	return (x + 1) / m_columnWidth;
}

/*!Shift is a consequence of style sheet border.*/
int SpreadsheetViewer::columnToX(int col) const
{
	return (col * m_columnWidth) - 1;
}

/*!Shift is a consequence of style sheet border.*/
int SpreadsheetViewer::yToRow(int y) const
{
	return (y + 1) / m_rowHeight;
}

/*!Shift is a consequence of style sheet border.*/
int SpreadsheetViewer::rowToY(int row) const
{
	return (row * m_rowHeight) - 1;
}

bool SpreadsheetViewer::refreshContentSize(int scrollDx, int scrollDy)
{
	QSize viewportSize = m_cellScrollArea->viewport()->size();
	QPoint offset = m_cellScrollArea->widget()->pos();
	offset = QPoint(qMin(0, offset.x() - scrollDx), qMin(0, offset.y() - scrollDy));

	QSize contentSize(columnToX(m_columnCount + 1), rowToY(m_rowCount + 1));

	QSize actualSize(contentSize);
	int x = viewportSize.width() - offset.x();
	int y = viewportSize.height() - offset.y();
	if (x > actualSize.width())
		actualSize.setWidth(x);
	if (y > actualSize.height())
		actualSize.setHeight(y);

	if (actualSize == m_cellScrollArea->widget()->size())
		return false;
	else {
		m_isComputingSize = true;
		m_cellScrollArea->widget()->setFixedSize(actualSize);
		m_rowScrollArea->widget()->setFixedSize(
			m_rowScrollArea->viewport()->width(),
			actualSize.height());
		m_columnScrollArea->widget()->setFixedSize(
			actualSize.width(),
			m_columnScrollArea->viewport()->height());
		m_isComputingSize = false;
		return true;
	}
}

void SpreadsheetViewer::showEvent(QShowEvent *)
{
	int viewportHeight = m_cellScrollArea->height();
	int contentHeight = rowToY(m_rowCount * 0 + 50);
	QScrollBar *vSc = m_cellScrollArea->verticalScrollBar();
	int actualContentHeight = qMax(contentHeight, vSc->value() + viewportHeight);
	m_rowScrollArea->widget()->setFixedHeight(actualContentHeight);
	m_cellScrollArea->widget()->setFixedHeight(actualContentHeight);
	if (m_frameHandle)
		connect(m_frameHandle, SIGNAL(frameSwitched()), this, SLOT(onFrameSwitched()));

	// updateAreasSize();
}

void SpreadsheetViewer::hideEvent(QHideEvent *)
{
	if (m_frameHandle)
		m_frameHandle->disconnect(this);
}

void SpreadsheetViewer::resizeEvent(QResizeEvent *e)
{
	QFrame::resizeEvent(e);
	refreshContentSize(0, 0);
	/*
	int w = width();
	int h = height();

  int hSpacing = 4;
  int vSpacing = 4;

  int x = m_rowScrollAreaWidth + hSpacing;
  int y = m_columnScrollAreaHeight + vSpacing;

	m_cellScrollArea->setGeometry(x,y, w-x, h-y);

  int sh = m_cellScrollArea->horizontalScrollBar()->height();
  int sw = m_cellScrollArea->verticalScrollBar()->width();

	m_columnScrollArea->setGeometry(x, 0, w-x-sw, m_columnScrollAreaHeight);
  m_rowScrollArea->setGeometry(0, y, m_rowScrollAreaWidth, h-y-sh);

  updateSizeToScroll(0,0); //Non updateAreeSize() perche' si deve tener conto degli scrollbar.
*/
}

void SpreadsheetViewer::wheelEvent(QWheelEvent *e)
{
	scroll(QPoint(0, -e->delta()));
}

void SpreadsheetViewer::timerEvent(QTimerEvent *e)
{
	if (!isAutoPanning())
		return;
	scroll(m_autoPanSpeed);
	m_lastAutoPanPos += m_autoPanSpeed;
	/*
  if(m_dragTool)
  {
    QMouseEvent mouseEvent(QEvent::MouseMove, m_lastAutoPanPos, Qt::NoButton, 0, 0);
    m_dragTool->onDrag(&mouseEvent);
  }
  */
}

void SpreadsheetViewer::keyPressEvent(QKeyEvent *e)
{
	int frameCount = m_rowCount;
	int row = m_frameHandle->getFrame();

	if (e->key() == Qt::Key_Up && row > 0) { //Row = frame precedente a quello settato
		m_frameHandle->setFrame(row - 1);
		return;
	} else if (e->key() == Qt::Key_Down) { //Row = frame successivo a quello settato
		m_frameHandle->setFrame(row + 1);
		return;
	} else if (e->key() == '0') {
		QWidget *panel = parentWidget();
		QWidget *panelParent = panel->parentWidget();
		while (panelParent != 0 && dynamic_cast<QMainWindow *>(panelParent) == 0) {
			panel = panelParent;
			panelParent = panel->parentWidget();
		}
		if (panelParent) {
			QList<QDockWidget *> panels = panelParent->findChildren<QDockWidget *>();
			for (int i = 0; i < panels.size(); i++) {
				QWidget *w = panels[i];
			}
		}
		return;
	}

	int y = 0;
	QRect visibleRect = m_cellScrollArea->widget()->visibleRegion().boundingRect();
	int visibleRowCount = visibleRect.height() / m_rowHeight;
	if (e->key() == Qt::Key_PageUp) //Setto la visualizzazione della pagina precedente
		y = visibleRect.top() - (visibleRowCount + 1) * m_rowHeight;
	else if (e->key() == Qt::Key_PageDown) //Setto la visualizzazione della pagina successiva
		y = visibleRect.bottom() + (visibleRowCount + 1) * m_rowHeight;
	else if (e->key() == Qt::Key_Home)
		y = 0;
	else if (e->key() == Qt::Key_End)
		y = (frameCount + 1) * m_rowHeight;
	else
		return;

	int deltaY = 0;
	if (y < visibleRect.top())
		deltaY = y - visibleRect.top();
	else
		deltaY = y - visibleRect.bottom();
	scroll(QPoint(0, deltaY));
}

void SpreadsheetViewer::frameSwitched()
{
}

/*
void SpreadsheetViewer::updateAllAree()
{
}

void SpreadsheetViewer::updateCellColumnAree()
{
}

void SpreadsheetViewer::updateCellRowAree()
{
}

*/

void SpreadsheetViewer::updateAreas()
{
}

void SpreadsheetViewer::onVSliderChanged(int)
{
	if (!m_isComputingSize)
		refreshContentSize(0, 0);
	/*
  QScrollBar *vSc = m_cellScrollArea->verticalScrollBar();
  int h = qMax(vSc->value() + m_cellScrollArea->height(), rowToY(getRowCount()));
  if(m_cellScrollArea->widget())
    m_cellScrollArea->widget()->setFixedHeight(h);
  if(m_rowScrollArea->widget())
    m_rowScrollArea->widget()->setFixedHeight(h);
    */

	/*
  int viewportHeight = m_cellScrollArea->height();
  int contentHeight = rowToY(m_rowCount*0 + 50);
  QScrollBar *vSc = m_cellScrollArea->verticalScrollBar();
  int actualContentHeight = qMax(contentHeight, vSc->value() + viewportHeight);
  m_rowScrollArea->widget()->setFixedHeight(actualContentHeight);
  m_cellScrollArea->widget()->setFixedHeight(actualContentHeight);
*/
}

void SpreadsheetViewer::onHSliderChanged(int)
{
	if (!m_isComputingSize)
		refreshContentSize(0, 0);
}

void SpreadsheetViewer::ensureVisibleCol(int col)
{
	int x = columnToX(col) + m_columnWidth / 2;

	int vertValue = m_cellScrollArea->verticalScrollBar()->value();
	m_cellScrollArea->ensureVisible(x, vertValue, m_columnWidth / 2, 0);
}
