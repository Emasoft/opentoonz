#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#endif

#include <memory>

#include "tsystem.h"
//#include "tunicode.h"
#include "tfilepath_io.h"
#include "tconvert.h"

#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <set>
#include <tenv.h>

#undef PLATFORM

#ifdef _WIN32
#pragma warning(disable : 4996)
#define PLATFORM WIN32
#include <process.h>
#include <psapi.h>
#include <io.h>
#include <stdlib.h>
#include <direct.h>
// gmt: sulla mia macchina cosi' non compila!!!
// #include "winsock2.h"
// #include "lmcons.h"
#include <sys/utime.h>
#include <lm.h>
#endif

#ifdef LINUX
#define PLATFORM LINUX
#include <grp.h>
#include <utime.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/sysinfo.h>
#include <sys/swap.h>
#include <sys/statfs.h>
#include <pwd.h>
#include <mntent.h>
#include <dlfcn.h>
#include <utime.h>
#include <sys/time.h>

#endif

#if defined(MACOSX)
#define PLATFORM MACOSX
#include <grp.h>
#include <utime.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h> // for ftime
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/param.h> // for getfsstat
#include <sys/ucred.h>
#include <sys/mount.h>
#include <pwd.h>
#include <dlfcn.h>

#include "Carbon/Carbon.h"

#endif

#ifdef __sgi
#define PLATFORM SGI
#include <sys/param.h>
#include <unistd.h>
#include <grp.h>
#include <sys/dir.h> // dirent.h
#include <sys/utime.h>
#include <sys/swap.h>
#include <sys/statfs.h>
#include <pwd.h>
#include <mntent.h>

#include <dlfcn.h>

#endif

#ifndef PLATFORM
PLATFORM_NOT_SUPPORTED
#endif

using namespace std;

#ifdef _WIN32

wstring getFormattedMessage(DWORD lastError)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		lastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	int wSize = MultiByteToWideChar(0, 0, (char *)lpMsgBuf, -1, 0, 0);
	if (!wSize)
		return wstring();

	std::unique_ptr<wchar_t[]> wBuffer(new wchar_t[wSize + 1]);
	MultiByteToWideChar(0, 0, (char *)lpMsgBuf, -1, wBuffer.get(), wSize);
	wBuffer[wSize] = '\0';
	wstring wmsg(wBuffer.get());

	LocalFree(lpMsgBuf);
	return wmsg;
}

#endif
//------------------------------------------------------------

void TSystem::outputDebug(string s)
{
#ifdef TNZCORE_LIGHT
#ifdef _WIN32
	OutputDebugString((LPCWSTR)s.c_str());
#else
	cerr << s << endl;
#endif
#else
	qDebug(s.c_str());
#endif
}

//------------------------------------------------------------

int TSystem::getProcessId()
{
	return getpid();
}

//------------------------------------------------------------

bool TSystem::memoryShortage()
{
#ifdef _WIN32

	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memStatus);

	assert(memStatus.ullAvailPhys <= memStatus.ullTotalPhys);

	if (memStatus.ullAvailPhys < memStatus.ullTotalPhys * 0.20) //if available memory is less then 20% of total memory
		return true;

	PROCESS_MEMORY_COUNTERS c;
	c.cb = sizeof(PROCESS_MEMORY_COUNTERS);
	BOOL ret = GetProcessMemoryInfo(GetCurrentProcess(), &c, sizeof(PROCESS_MEMORY_COUNTERS));
	assert(ret);

	return c.WorkingSetSize > memStatus.ullTotalVirtual * 0.6; //if total memory used by this process(WorkingSetSize) is
															   //half of max allocatable memory
															   //(ullTotalVirtual: on 32bits machines, tipically it's 2GB)
															   //It's better "to stay large"; for values >0.6 this function may
															   //returns that there is memory, but for fragmentation the malloc fails the same!

#elif defined(MACOSX)

	// to be done...
	return false;

#elif defined(LINUX)

	// to be done...
	return false;

#else

	@ @ @ERROR : PLATFORM NOT SUPPORTED

#endif
}

//------------------------------------------------------------

TINT64 TSystem::getFreeMemorySize(bool onlyPhisicalMemory)
{

	TINT64 totalFree = 0;

#ifdef _WIN32

	MEMORYSTATUSEX buff;
	buff.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&buff);

	if (onlyPhisicalMemory)
		return buff.ullAvailPhys >> 10;
	else
		return buff.ullAvailPageFile >> 10;

#elif defined(__sgi)

	//check for virtual memory
	int numberOfResources = swapctl(SC_GETNSWP, 0); /* get number of swapping resources configued */

	if (numberOfResources == 0)
		return 0;

	//avrei voluto fare: struct swaptable *table = new struct swaptable[...]
	struct swaptable *table = (struct swaptable *)calloc(1, sizeof(struct swapent) * numberOfResources + sizeof(int));

	table->swt_n = numberOfResources;
	swapctl(SC_LIST, table); /* list all the swapping resources */

	TINT64 virtualFree = 0;
	TINT64 physicalFree = 0;

	for (int i = 0; i < table->swt_n; i++) {
		virtualFree += table->swt_ent[i].ste_free;
	}

	free(table);
	totalFree = virtualFree << 4 + physicalFree;

#elif defined(LINUX)

	struct sysinfo *sysInfo = (struct sysinfo *)calloc(1, sizeof(struct sysinfo));

	if (!sysinfo(sysInfo)) {
		if (onlyPhisicalMemory)
			totalFree = sysInfo->freeram;
		else
			totalFree = sysInfo->freeram + sysInfo->freeswap;
	} else {
		assert(!"sysinfo function failed");
	}
	free(sysInfo);

#elif defined(MACOSX)

	// to be done...
	totalFree = 512 * 1024;

#else
	@ @ @ERROR : PLATFORM NOT SUPPORTED
#endif

#ifndef _WIN32
#else
#endif

	return totalFree;
}

//------------------------------------------------------------
/*
ostream& operator<<(ostream&out, const TTime  &t)
{
  return out<<t.getDate()<<" "<<t.getTime(); 
}
*/

//------------------------------------------------------------

TINT64 TSystem::getDiskSize(const TFilePath &diskName)
{
	TINT64 size = 0;
	if (!diskName.isAbsolute()) {
		assert(0);
		return 0;
	}
#ifndef _WIN32
	struct statfs buf;
	wstring str_diskname = diskName.getWideString();
#ifdef __sgi
	statfs(toString(str_diskname).c_str(), &buf, sizeof(struct statfs), 0);
#else
	statfs(toString(str_diskname).c_str(), &buf);
#endif
	size = (TINT64)((buf.f_blocks * buf.f_bsize) >> 10);
#else
	DWORD sectorsPerCluster;	// sectors per cluster
	DWORD bytesPerSector;		// bytes per sector
	DWORD numberOfFreeClusters; // free clusters
	DWORD totalNumberOfClusters;

	BOOL rc = GetDiskFreeSpaceW(
		diskName.getWideString().c_str(), // root path
		&sectorsPerCluster,				  // sectors per cluster
		&bytesPerSector,				  // bytes per sector
		&numberOfFreeClusters,			  // free clusters
		&totalNumberOfClusters			  // total clusters
		);

	if (!rc)
		throw TSystemException(diskName, getFormattedMessage(GetLastError()));
	else
		size = (totalNumberOfClusters * sectorsPerCluster * bytesPerSector) >> 10;
#endif
	return size;
}

//------------------------------------------------------------

TINT64 TSystem::getFreeDiskSize(const TFilePath &diskName)
{
	TINT64 size = 0;
	if (!diskName.isAbsolute()) {
		assert(0);
		return 0;
	}
#ifndef _WIN32
	struct statfs buf;
	wstring str_diskname = diskName.getWideString();
#ifdef __sgi
	statfs(str_diskname.c_str(), &buf, sizeof(struct statfs), 0);
#else
	statfs(toString(str_diskname).c_str(), &buf);
#endif
	size = (TINT64)(buf.f_bfree * buf.f_bsize) >> 10;
#else
	DWORD sectorsPerCluster;	// sectors per cluster
	DWORD bytesPerSector;		// bytes per sector
	DWORD numberOfFreeClusters; // free clusters
	DWORD totalNumberOfClusters;

	BOOL rc = GetDiskFreeSpaceW(diskName.getWideString().c_str(), // root path
								&sectorsPerCluster,				  // sectors per cluster
								&bytesPerSector,				  // bytes per sector
								&numberOfFreeClusters,			  // free clusters
								&totalNumberOfClusters			  // total clusters
								);

	if (!rc) //eccezione... getLastError etc...
		throw TSystemException(diskName, "cannot get disk info!");
	else
		size = (numberOfFreeClusters * sectorsPerCluster * bytesPerSector) >> 10;
#endif
	return size;
}

//------------------------------------------------------------

TINT64 TSystem::getMemorySize(bool onlyPhisicalMemory)
{
#ifdef _WIN32

	MEMORYSTATUS buff;
	GlobalMemoryStatus(&buff);
	if (onlyPhisicalMemory)
		return buff.dwTotalPhys >> 10;
	else
		return buff.dwTotalPageFile >> 10;

#elif defined(__sgi)

	int physicalMemory;

	if (swapctl(SC_GETSWAPMAX, &physicalMemory))
		return ((size_t)0);
	else
		return logSwapLibero >> 1;
#elif defined(LINUX)

	struct sysinfo *sysInfo = (struct sysinfo *)calloc(1, sizeof(struct sysinfo));
	TINT64 ret = 0;

	if (!sysinfo(sysInfo))
		ret = sysInfo->totalram;
	else
		assert(!"sysinfo function failed");

	free(sysInfo);
	return ret;

#elif defined(MACOSX)

	// to be done...
	return 512 * 1024;

#else
	@ @ @ERROR : PLATFORM NOT SUPPORTED
#endif

#ifndef _WIN32
#else
#endif
}

//------------------------------------------------------------

void TSystem::moveFileToRecycleBin(const TFilePath &fp)
{
#if defined(_WIN32)
	//
	// from http://msdn.microsoft.com/msdnmag/issues/01/04/c/default.aspx
	//
	// Copy pathname to double-NULL-terminated string.
	//
	wchar_t buf[_MAX_PATH + 1];				 // allow one more character
	wcscpy(buf, fp.getWideString().c_str()); // copy caller's path name
	buf[wcslen(buf) + 1] = 0;				 // need two NULLs at end

	SHFILEOPSTRUCTW data;
	memset(&data, 0, sizeof(SHFILEOPSTRUCTW));
	data.fFlags |= FOF_SILENT;		   // don't report progress
	data.fFlags |= FOF_NOERRORUI;	  // don't report errors
	data.fFlags |= FOF_NOCONFIRMATION; // don't confirm delete

	data.wFunc = FO_DELETE;			   // REQUIRED: delete operation
	data.pFrom = buf;				   // REQUIRED: which file(s)
	data.pTo = NULL;				   // MUST be NULL
	data.fFlags |= FOF_ALLOWUNDO;	  // ..send to Recycle Bin
	int ret = SHFileOperationW(&data); // do it!

#elif defined(MACOSX)
	FSRef foundRef;
	OSErr err = FSFindFolder(kOnSystemDisk,
							 kTrashFolderType, kDontCreateFolder,
							 &foundRef);

	if (err) {
		assert(false);
		deleteFile(fp);
		return;
	}
	UInt8 path[255];
	err = FSRefMakePath(&foundRef, path, 254);
	if (err) {
		assert(false);
		deleteFile(fp);
		return;
	}
	//TFilePath dest = TFilePath(path)+(fp.getName()+fp.getDottedType());
	string fullNameWithExt = toString(fp.getWideString());
	int i = fullNameWithExt.rfind("/");
	string nameWithExt = fullNameWithExt.substr(i + 1);
	TFilePath dest = TFilePath((char *)path) + nameWithExt;

	try {
		renameFile(dest, fp);
	} catch (...) {
		try {
			copyFile(dest, fp);
			deleteFile(fp);
		} catch (...) {
		}
	}

#else
	assert(!"Not implemented yet");
#endif
}

//------------------------------------------------------------

TString TSystemException::getMessage() const
{
	wstring msg;
	switch (m_err) {
	case -1:
		// nothing
		msg = m_msg;
		CASE EEXIST : msg = L": Directory was not created because filename is the name of an existing file, directory, or device";
		CASE ENOENT : msg = L": Path was not found, or the named file does not exist or is a null pathname.";
		CASE ENOTEMPTY : msg = L": Given path is not a directory; directory is not empty; or directory is either current working directory or root directory";
		CASE EACCES : msg = L": Search permission is denied by a component of the path prefix, or write permission on the file named by path is denied, or times is NULL, and write access is denied";
		CASE EFAULT : msg = L": Times is not NULL and, or points outside the process's allocated address space.";
		CASE EINTR : msg = L": A signal was caught during the utime system call.";
		CASE ENAMETOOLONG : msg = L": The length of the path argument exceeds {PATH_MAX}, or the length of a path component exceeds {NAME_MAX} while _POSIX_NO_TRUNC is in effect.";
		CASE ENOTDIR : msg = L": A component of the path prefix is not a directory.";
		CASE EPERM : msg = L": The calling process does not have the super-user privilege, the effective user ID is not the owner of the file, and times is not NULL, or the file system containing the file is mounted read-only";
		CASE EROFS : msg = L": The current file system level range does not envelop the level of the file named by path, and the calling process does not have the super-user privilege.";
		CASE ENOSYS : msg = L": When the named file cannot have its time reset.  The file is on a file system that doesn't have this operation.";
		CASE EMFILE : msg = L": The maximum number of file descriptors are currently open.";
		CASE ENFILE : msg = L": The system file table is full.";
		CASE EBADF : msg = L": The file descriptor determined by the DIR stream is no longer valid.  This result occurs if the DIR stream has been closed.";
		CASE EINVAL : msg = L": 64-bit and non-64-bit calls were mixed in a sequence of calls.";
	DEFAULT:
		msg = L": Unknown error";

#ifndef _WIN32
		CASE ELOOP : msg = L": Too many symbolic links were encountered in translating path.";
#ifndef MACOSX
		CASE EMULTIHOP : msg = L": Components of path require hopping to multiple remote machines and the file system does not allow it.";
		CASE ENOLINK : msg = L": Path points to a remote machine and the link to that machine is no longer active.";
#endif
#if defined(__sgi)
		CASE EDIRCORRUPTED : msg = L": The directory is corrupted on disk.";
#endif
		CASE EOVERFLOW : msg = L": One of the inode number values or offset values did not fit in 32 bits, and the 64-bit interfaces were not used.";
#endif
	}
	return m_fname.getWideString() + L"\n" + msg;
}

//------------------------------------------------------------

void TSystem::touchFile(const TFilePath &path)
{
#ifndef TNZCORE_LIGHT

	// string filename = path.getFullPath();
	if (TFileStatus(path).doesExist()) {
		int ret;
#if defined(MACOSX) || defined(LINUX)
		wstring str_file = path.getWideString();
		ret = utimes(toString(str_file).c_str(), 0);
#else
		ret = _wutime(path.getWideString().c_str(), 0);
#endif
		if (0 != ret)
			throw TSystemException(path, errno);
	} else {
		Tofstream file(path);
		if (!file) {
			throw TSystemException(path, errno);
		}
		file.close(); // altrimenti il compilatore da' un warning:
					  // variabile non utilizzata
	}

#endif
}

//------------------------------------------------------------
