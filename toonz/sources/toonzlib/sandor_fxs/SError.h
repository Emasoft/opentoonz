

// SError.h: interface for the SError class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERROR_H__25953AF0_0B0B_11D6_B96F_0040F674BE6A__INCLUDED_)
#define AFX_SERROR_H__25953AF0_0B0B_11D6_B96F_0040F674BE6A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <vector>

#include "SDef.h"

class SError
{
protected:
	std::string m_msg;

public:
	SError() : m_msg(""){};
	SError(char *s) : m_msg(s){};
	virtual ~SError(){};
	virtual void debug_print() const
	{
		/*if ( !m_msg.empty() )
			smsg_error("Error %s!\n",m_msg.c_str()); 
		else
			smsg_error("\n"); */
	}
};

class SMemAllocError : public SError
{
public:
	SMemAllocError() : SError(""){};
	SMemAllocError(char *s) : SError(s){};
	virtual ~SMemAllocError(){};
	void debug_print() const
	{
		/*	if ( !m_msg.empty() )
			smsg_error("Error in Memory Allocation %s!\n",m_msg.c_str()); 
		else
			smsg_error("Error in Memory Allocation\n"); */
	}
};

class SWriteRasterError : public SError
{
public:
	SWriteRasterError() : SError(""){};
	SWriteRasterError(char *s) : SError(s){};
	virtual ~SWriteRasterError(){};
	void debug_print() const
	{
		/*if ( !m_msg.empty() )
			smsg_error("Error in Writing Raster %s!\n",m_msg.c_str()); 
		else
			smsg_error("Error in Writing Raster\n"); */
	}
};

class SBlurMatrixError : public SError
{
public:
	SBlurMatrixError() : SError(""){};
	SBlurMatrixError(char *s) : SError(s){};
	virtual ~SBlurMatrixError(){};
	void debug_print() const
	{
		/*	if ( !m_msg.empty() ) {
			smsg_error("Error in Generating BlurMatrix %s!\n",m_msg.c_str()); 
		} else
			smsg_error("Error in Generating BlurMatrix!\n"); */
	}
};

class SFileReadError : public SError
{
public:
	SFileReadError() : SError(""){};
	SFileReadError(char *s) : SError(s){};
	virtual ~SFileReadError(){};
	void debug_print() const
	{
		/*if ( !m_msg.empty() ) {
			smsg_error("Error in Reading File %s!\n",m_msg.c_str()); 
		} else
			smsg_error("Error in Reading File!\n"); */
	}
};

#endif // !defined(AFX_SERROR_H__25953AF0_0B0B_11D6_B96F_0040F674BE6A__INCLUDED_)
