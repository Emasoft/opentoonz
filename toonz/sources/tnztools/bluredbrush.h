

#ifndef T_BLUREDBRUSH
#define T_BLUREDBRUSH

#include "traster.h"
#include "trastercm.h"
#include "tcurves.h"
#include <QPainter>
#include <QImage>

//=======================================================
//
//BluredBrush
//
//=======================================================

class BluredBrush
{
	TRaster32P m_ras;
	QImage m_rasImage;
	int m_size;
	QRadialGradient m_gradient;
	TThickPoint m_lastPoint;
	double m_oldOpacity;
	bool m_enableDinamicOpacity;

	double getNextPadPosition(const TThickQuadratic &q, double t) const;

public:
	BluredBrush(const TRaster32P &ras, int size, const QRadialGradient &gradient, bool doDinamicOpacity);
	~BluredBrush();

	void addPoint(const TThickPoint &p, double opacity);
	void addArc(const TThickPoint &pa, const TThickPoint &pb, const TThickPoint &pc, double opacityA, double opacityC);
	TRect getBoundFromPoints(const std::vector<TThickPoint> &points) const;
	//colormapped
	void updateDrawing(const TRasterCM32P rasCM, const TRasterCM32P rasBackupCM, const TRect &bbox,
					   int styleId, bool selective) const;
	void eraseDrawing(const TRasterCM32P rasCM, const TRasterCM32P rasBackupCM, const TRect &bbox,
					  bool selective, int selectedStyleId, const std::wstring &mode) const;

	//fullcolor
	void updateDrawing(const TRasterP ras, const TRasterP rasBackup, const TPixel32 &color,
					   const TRect &bbox, double opacity) const;
	void eraseDrawing(const TRasterP ras, const TRasterP rasBackup, const TRect &bbox, double opacity) const;
};

#endif //T_BLUREDBRUSH
