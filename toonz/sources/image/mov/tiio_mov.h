

#ifndef TIIO_MOV_H
#define TIIO_MOV_H

#ifdef x64
#include "tiio_mov_proxy.h"
#else

//Windows include
#include <windows.h>

//QuickTime includes
namespace QuickTime
{
#define list List
#define map Map
#define iterator Iterator
#define float_t Float_t

#include "QTML.h"
#include "Movies.h"
#include "Script.h"
#include "FixMath.h"
#include "Sound.h"
#include "QuickTimeComponents.h"

#include "tquicktime.h"

#undef list
#undef map
#undef iterator
#undef float_t
} //namespace QuickTime

//Toonz includes
#include "tlevel_io.h"
#include "tthreadmessage.h"
#include "tcommon.h"

#undef DVAPI
#undef DVVAR
#ifdef IMAGE_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

using namespace QuickTime;

//-----------------------------------------------------------------------------

//  Forward declarations

class TImageWriterMov;
class TImageReaderMov;

//-----------------------------------------------------------------------------

//  Global functions

bool IsQuickTimeInstalled();

//***********************************************************************************
//    Mov TLevelWriter class
//***********************************************************************************

class TLevelWriterMov : public TLevelWriter
{
	std::vector<std::pair<int, TimeValue>> m_savedFrames;
	int m_IOError;

	Movie m_movie;
	Track m_videoTrack;
	Track m_soundTrack;
	Media m_videoMedia;
	Media m_soundMedia;
	GWorldPtr m_gworld;
	PixMapHandle m_pixmap;
	short m_refNum;
	PixelXRGB *m_buf;
	int buf_lx;
	int buf_ly;
	int m_firstFrame;
	TThread::Mutex m_mutex;
	ComponentInstance m_ci;

public:
	TLevelWriterMov(const TFilePath &path, TPropertyGroup *winfo);
	~TLevelWriterMov();

	TImageWriterP getFrameWriter(TFrameId fid);

	void save(const TImageP &img, int frameIndex);
	void saveSoundTrack(TSoundTrack *st);

public:
	static TLevelWriter *create(const TFilePath &f, TPropertyGroup *winfo)
	{
		return new TLevelWriterMov(f, winfo);
	}
};

//***********************************************************************************
//    Mov TLevelReader class
//***********************************************************************************

class DVAPI TLevelReaderMov : public TLevelReader
{
	bool m_readAsToonzOutput; //default: false
	bool m_yMirror;			  //default: true
	bool m_loadTimecode;	  //default: false
	int m_IOError;

	short m_refNum;
	Movie m_movie;
	Track m_track;
	MediaHandler m_timecodeHandler;
	long m_depth;
	std::map<int, TimeValue> currentTimes;
	int m_lx, m_ly;
	int m_hh, m_mm, m_ss, m_ff;
	TThread::Mutex m_mutex;

public:
	TLevelReaderMov(const TFilePath &path);
	~TLevelReaderMov();

	TImageReaderP getFrameReader(TFrameId fid);
	TLevelP loadInfo();
	void load(const TRasterP &rasP, int frameIndex, const TPoint &pos, int shrinkX = 1, int shrinkY = 1);

	void timecode(int frame, UCHAR &hh, UCHAR &mm, UCHAR &ss, UCHAR &ff);
	void loadedTimecode(UCHAR &hh, UCHAR &mm, UCHAR &ss, UCHAR &ff);

	const TImageInfo *getImageInfo(TFrameId fid) { return m_info; }
	const TImageInfo *getImageInfo() { return m_info; }

	void setYMirror(bool enabled);
	void setLoadTimecode(bool enabled);
	void enableRandomAccessRead(bool enable);

	TDimension getSize() const { return TDimension(m_lx, m_ly); }
	TRect getBBox() const { return TRect(0, 0, m_lx - 1, m_ly - 1); }

public:
	static TLevelReader *create(const TFilePath &f);

private:
	TLevelP loadToonzOutputFormatInfo();
};

//===========================================================================

//  Mov Properties

namespace Tiio
{

class MovWriterProperties : public TPropertyGroup
{
public:
	MovWriterProperties();
};

} //namespace Tiio

//===========================================================================

#endif //!x64

#endif //TIIO_MOV_H
