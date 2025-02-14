

#ifndef FXSCHEMATICNODE_H
#define FXSCHEMATICNODE_H

// TnzQt includes
#include "schematicnode.h"

// Qt includes
#include <QGraphicsItem>
#include <QList>

//==============================================================

//    Forward declarations

class TFx;
class TOutputFx;
class TXsheetFx;
class TZeraryColumnFx;
class TPaletteColumnFx;
class TLevelColumnFx;
class TStageObjectId;

class FxSchematicNode;
class FxSchematicScene;
class FxSchematicDock;
class FxSchematicColumnNode;
class FxSchematicPaletteNode;
class FxSchematicNormalFxNode;
class FxSchematicXSheetNode;
class FxSchematicOutputNode;

//==============================================================

enum eFxSchematicPortType {
	eFxOutputPort = 200,
	eFxInputPort = 201,
	eFxLinkPort = 202,
	eFxGroupedInPort = 203,
	eFxGroupedOutPort = 204
};

enum eFxType {
	eNormalFx = 100,
	eZeraryFx = 101,
	eMacroFx = 102,
	eColumnFx = 103,
	eOutpuFx = 104,
	eXSheetFx = 106,
	eGroupedFx = 107,
	eNormalImageAdjustFx = 108,
	eNormalLayerBlendingFx = 109,
	eNormalMatteFx = 110
};

//*****************************************************
//    FxColumnPainter
//*****************************************************

class FxColumnPainter : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	FxSchematicColumnNode *m_parent;
	double m_width, m_height;
	QString m_name;
	int m_type;

public:
	FxColumnPainter(FxSchematicColumnNode *parent, double width, double height, const QString &name);
	virtual ~FxColumnPainter();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	void setName(const QString &name) { m_name = name; }

	QLinearGradient getGradientByLevelType(int type);

public slots:

	void onIconGenerated();

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
};

//*****************************************************
//    FxPalettePainter
//*****************************************************

class FxPalettePainter : public QGraphicsItem, public QObject
{
	FxSchematicPaletteNode *m_parent;
	double m_width, m_height;
	QString m_name;

public:
	FxPalettePainter(FxSchematicPaletteNode *parent, double width, double height, const QString &name);
	~FxPalettePainter();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	void setName(const QString &name) { m_name = name; }

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
};

//*****************************************************
//    FxPainter
//*****************************************************

class FxPainter : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	FxSchematicNode *m_parent;
	double m_width, m_height;
	QString m_name, m_label;
	eFxType m_type;

	//to obtain the fx icons for zoom out view of the schematic
	std::string m_fxType;

	QLinearGradient getGradientByLevelType(eFxType);
	void paint_small(QPainter *painter);

public:
	FxPainter(FxSchematicNode *parent, double width, double height, const QString &name, eFxType type, std::string fxType);
	~FxPainter();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	void setName(const QString &name) { m_name = name; }

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
};

//*****************************************************
//    FxXSheetPainter
//*****************************************************

class FxXSheetPainter : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	double m_width, m_height;

	FxSchematicXSheetNode *m_parent;

public:
	FxXSheetPainter(FxSchematicXSheetNode *parent, double width, double height);
	~FxXSheetPainter();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
};

//*****************************************************
//    FxOutputPainter
//*****************************************************

class FxOutputPainter : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	double m_width, m_height;
	bool m_isActive;
	FxSchematicOutputNode *m_parent;

public:
	FxOutputPainter(FxSchematicOutputNode *parent, double width, double height);
	~FxOutputPainter();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
};

//*****************************************************
//    FxSchematicLink
//*****************************************************

class FxSchematicLink : public SchematicLink
{
	Q_OBJECT

public:
	FxSchematicLink(QGraphicsItem *parent, QGraphicsScene *scene);
	~FxSchematicLink();

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
};

//*****************************************************
//    FxSchematicPort
//*****************************************************

class FxSchematicPort : public SchematicPort
{
	TFx *m_ownerFx;
	FxSchematicPort *m_currentTargetPort;
	QList<SchematicLink *> m_hiddenLinks;
	QList<SchematicLink *> m_ghostLinks;

public:
	FxSchematicPort(FxSchematicDock *parent, int type);
	~FxSchematicPort();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	bool linkTo(SchematicPort *port, bool checkOnly = false);
	FxSchematicDock *getDock() const;
	SchematicLink *makeLink(SchematicPort *port);

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *cme);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *me);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *me);
	TFx *getOwnerFx() const;

private:
	void linkEffects(TFx *inputFx, TFx *fx, int inputId);
	SchematicPort *searchPort(const QPointF &scenePos);

	//!Handles hiding of existing link and showing of ghost links for snapping after creation link only for fx having
	//!dynamic ports.
	//!If \b secondIndex is -1 consider the last port in the groupedPorts of the node.
	void handleSnappedLinksOnDynamicPortFx(const std::vector<TFxPort *> &groupedPorts, int targetIndex, int startIndex = -1);

	void resetSnappedLinksOnDynamicPortFx();

	void hideSnappedLinks();
	void showSnappedLinks();
};

//*****************************************************
//    FxSchematicDock
//*****************************************************

class FxSchematicDock : public QGraphicsItem, public QObject
{
	QString m_name;
	double m_width;
	FxSchematicPort *m_port;

public:
	FxSchematicDock(FxSchematicNode *parent, const QString &string, double width, eFxSchematicPortType type);
	~FxSchematicDock();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	FxSchematicNode *getNode();
	FxSchematicPort *getPort() { return m_port; }
};

//*****************************************************
//    FxSchematicNode
//*****************************************************

class FxSchematicNode : public SchematicNode
{
	Q_OBJECT

protected:
	enum eDropActionMode {
		eShift,
		eNone
	};

protected:
	QString m_name;  //!< Node's name (displayed on top, editable)
	TFxP m_fx;		 //!< The node's associated fx (could be a wrapper to the actual fx)
	TFxP m_actualFx; //!< The actual node's associated fx

	FxSchematicNode *m_linkedNode;
	QList<FxSchematicDock *> m_inDocks;
	FxSchematicDock *m_outDock;
	FxSchematicDock *m_linkDock;
	SchematicName *m_nameItem;
	eFxType m_type;
	bool m_isCurrentFxLinked;

	bool m_isLargeScaled;

protected:
	//! If the fx has dynamic port groups, ensures that each group always has at least one unlinked port
	//! that users can attach to, while keeping the number of unlinked ports to the minimum allowed by the
	//! group's specifics.
	virtual void checkDynamicInputPortSize() const;

	//! Adds a dynamic group port to the associated fx.
	void addDynamicInputPort(int groupIndex) const;

	//! Removes the port with specified name from the associated fx. Returns false (ie operation refused)
	//! if the specified port does not exists, is connected, or is not in a dynamic port group.
	bool removeDynamicInputPort(const std::string &name) const;

	//! Moves the port group links to the front ports.
	void shiftLinks() const;

	void updateOutputDockToolTips(const QString &name);

public:
	FxSchematicNode(FxSchematicScene *parent, TFx *fx, qreal width, qreal height, eFxType type);

	void setWidth(const qreal &width) { m_width = width; }
	void setHeight(const qreal &height) { m_height = height; }
	void setSchematicNodePos(const QPointF &pos) const;

	TFx *getFx() const { return m_fx.getPointer(); }
	bool isA(eFxType type) { return m_type == type; }

	int getInputPortCount() { return m_inDocks.size(); }
	int getInputDockId(FxSchematicDock *dock);

	FxSchematicPort *getInputPort(int i) { return m_inDocks[i] ? m_inDocks[i]->getPort() : 0; }
	FxSchematicPort *getOutputPort() { return m_outDock ? m_outDock->getPort() : 0; }
	FxSchematicPort *getLinkPort() { return m_linkDock ? m_linkDock->getPort() : 0; }

	bool isNameEditing() { return m_nameItem->isVisible(); }
	void onClicked();
	bool isCurrentFxLinked(SchematicNode *comingNode) { return m_isCurrentFxLinked; }
	void setIsCurrentFxLinked(bool value, FxSchematicNode *comingNode);
	void setPosition(const QPointF &newPos);

	void updatePortsPosition();

	virtual bool isOpened() { return true; }
	virtual bool isEnabled() const;
	virtual bool isCached() const;
	virtual void resize(bool maximizeNode) {}

	bool isLargeScaled() { return m_isLargeScaled; }
signals:

	void switchCurrentFx(TFx *fx);
	void currentColumnChanged(int index);

	void fxNodeDoubleClicked();
};

//*****************************************************
//    FxSchematicOutputNode
//*****************************************************

class FxSchematicOutputNode : public FxSchematicNode
{
	FxOutputPainter *m_outputPainter;

public:
	FxSchematicOutputNode(FxSchematicScene *scene, TOutputFx *fx);
	~FxSchematicOutputNode();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);
};

//*****************************************************
//    FxSchematicXSheetNode
//*****************************************************

class FxSchematicXSheetNode : public FxSchematicNode
{
	FxXSheetPainter *m_xsheetPainter;

public:
	FxSchematicXSheetNode(FxSchematicScene *scene, TXsheetFx *fx);
	~FxSchematicXSheetNode();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);
};

//*****************************************************
//    FxSchematicNormalFxNode
//*****************************************************

class FxSchematicNormalFxNode : public FxSchematicNode
{
	Q_OBJECT

	FxPainter *m_painter;
	SchematicToggle *m_renderToggle;

public:
	FxSchematicNormalFxNode(FxSchematicScene *scene, TFx *fx);
	~FxSchematicNormalFxNode();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

	void resize(bool maximizeNode);

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);

protected slots:

	void onNameChanged();
	void onRenderToggleClicked(bool);
};

//*****************************************************
//    FxSchematicZeraryNode
//*****************************************************

class FxSchematicZeraryNode : public FxSchematicNode
{
	Q_OBJECT

	FxPainter *m_painter;
	int m_columnIndex;
	SchematicToggle *m_renderToggle, *m_cameraStandToggle;

public:
	FxSchematicZeraryNode(FxSchematicScene *scene, TZeraryColumnFx *fx);
	~FxSchematicZeraryNode();

	QRectF boundingRect() const;
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

	void resize(bool maximizeNode);

	int getColumnIndex() { return m_columnIndex; }
	bool isCached() const;

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);

protected slots:

	void onRenderToggleClicked(bool);
	void onNameChanged();
};

//*****************************************************
//    FxSchematicColumnNode
//*****************************************************

class FxSchematicColumnNode : public FxSchematicNode
{
	Q_OBJECT

	SchematicThumbnailToggle *m_resizeItem;
	SchematicToggle *m_renderToggle, *m_cameraStandToggle;
	FxColumnPainter *m_columnPainter;
	int m_columnIndex;
	bool m_isOpened;

public:
	FxSchematicColumnNode(FxSchematicScene *scene, TLevelColumnFx *fx);
	~FxSchematicColumnNode();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	QPixmap getPixmap();
	bool isOpened() { return m_isOpened; }

	void getLevelTypeAndName(int &, QString &);

	void resize(bool maximizeNode);
	int getColumnIndex() { return m_columnIndex; }

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);

private:
	void renameObject(const TStageObjectId &id, std::string name);

protected slots:

	void onRenderToggleClicked(bool);
	void onCameraStandToggleClicked(int);
	void onChangedSize(bool);
	void onNameChanged();
};

//*****************************************************
//    FxSchematicPaletteNode
//*****************************************************

class FxSchematicPaletteNode : public FxSchematicNode
{
	Q_OBJECT

	SchematicToggle *m_renderToggle;
	FxPalettePainter *m_palettePainter;
	int m_columnIndex;

public:
	FxSchematicPaletteNode(FxSchematicScene *scene, TPaletteColumnFx *fx);
	~FxSchematicPaletteNode();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	QPixmap getPixmap();
	bool isOpened() { return false; }
	int getColumnIndex() { return m_columnIndex; }

	QString getPaletteName();

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);

protected slots:

	void onRenderToggleClicked(bool);
	void onNameChanged();

private:
	void renameObject(const TStageObjectId &id, std::string name);
};

//*****************************************************
//    FxGroupNode
//*****************************************************

class FxGroupNode : public FxSchematicNode
{
	Q_OBJECT

	QList<TFxP> m_groupedFxs;
	QList<TFxP> m_roots;
	int m_groupId;
	FxPainter *m_painter;
	SchematicToggle *m_renderToggle;
	bool m_isOpened;

public:
	FxGroupNode(FxSchematicScene *scene, const QList<TFxP> &groupedFx, const QList<TFxP> &roots,
				int groupId, const std::wstring &groupName);
	~FxGroupNode();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

	FxSchematicPort *getOutputPort() const { return m_outDock ? m_outDock->getPort() : 0; }
	bool isNameEditing() const { return m_nameItem->isVisible(); }
	QList<TFxP> getRootFxs() const { return m_roots; }
	int getRootCount() { return m_roots.size(); }
	int getFxCount() const { return m_groupedFxs.size(); }
	TFx *getFx(int i) const { return 0 <= i && i < m_groupedFxs.size() ? m_groupedFxs[i].getPointer() : 0; }

	QList<TFxP> getGroupedFxs() const { return m_groupedFxs; }
	void updateFxsDagPosition(const TPointD &pos) const;
	bool isOpened() { return m_isOpened; }
	void resize(bool maximized);
	bool contains(TFxP fx);
	//returns the number of ports that take the group in input... it consider also the node xsheet
	int getOutputConnectionsCount() const;
	bool isEnabled() const;
	virtual bool isCached() const;

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *me);
	QPointF computePos() const;

protected slots:

	void onNameChanged();
	void onRenderToggleClicked(bool);
};

#endif // FXSCHEMATICNODE_H
