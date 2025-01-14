

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "traster.h"

/*
#include "img.h"
#include "file.h"
#include "toonz.h"
#include "raster.h"
#include "tmsg.h"
#include "copP.h"*/

#if defined(MACOSX) || defined(LINUX)
#define TRUE 1
#define FALSE 0
#endif

/*------------------------------------------------------------------------*/
namespace
{
UCHAR First_preseed_table[256] =
	{8, 0, 1, 0, 2, 2, 1, 0, 3, 3, 3, 3, 2, 2, 3, 3,
	 4, 4, 1, 0, 2, 2, 1, 0, 4, 4, 3, 3, 2, 2, 3, 3,
	 5, 5, 1, 5, 2, 2, 1, 5, 5, 5, 5, 5, 2, 2, 5, 5,
	 4, 4, 1, 5, 2, 2, 1, 5, 4, 4, 5, 5, 2, 2, 5, 5,
	 6, 6, 1, 6, 2, 2, 1, 6, 6, 6, 6, 6, 2, 2, 6, 6,
	 4, 4, 1, 0, 2, 2, 1, 0, 4, 4, 8, 8, 2, 2, 8, 8,
	 6, 6, 1, 6, 2, 2, 1, 6, 6, 6, 6, 6, 2, 2, 6, 6,
	 4, 4, 1, 0, 2, 2, 1, 0, 4, 4, 8, 8, 2, 2, 8, 8,
	 7, 7, 1, 7, 2, 2, 1, 7, 7, 7, 7, 7, 2, 2, 7, 7,
	 4, 4, 1, 0, 2, 2, 1, 0, 4, 4, 3, 3, 2, 2, 3, 3,
	 7, 7, 1, 7, 2, 2, 1, 7, 7, 7, 7, 7, 2, 2, 7, 7,
	 4, 4, 1, 5, 2, 2, 1, 5, 4, 4, 5, 5, 2, 2, 5, 5,
	 7, 7, 1, 7, 2, 2, 1, 7, 7, 7, 7, 7, 2, 2, 7, 7,
	 4, 4, 1, 0, 2, 2, 1, 0, 4, 4, 8, 8, 2, 2, 8, 8,
	 7, 7, 1, 7, 2, 2, 1, 7, 7, 7, 7, 7, 2, 2, 7, 7,
	 4, 4, 1, 0, 2, 2, 1, 0, 4, 4, 8, 8, 2, 2, 8, 8};

static UCHAR Next_point_table[2048] =
	{8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2, 2,
	 2, 2, 1, 2, 2, 2, 2, 2, 2, 0, 1, 2, 2, 2, 2, 2,
	 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0,
	 3, 3, 1, 1, 1, 1, 1, 1, 3, 0, 1, 1, 1, 1, 1, 1,
	 3, 3, 3, 2, 2, 2, 2, 2, 3, 0, 0, 2, 2, 2, 2, 2,
	 3, 3, 1, 2, 2, 2, 2, 2, 3, 0, 1, 2, 2, 2, 2, 2,
	 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 4, 0, 4, 4, 4,
	 4, 4, 1, 4, 1, 4, 4, 4, 4, 0, 1, 4, 1, 4, 4, 4,
	 4, 4, 4, 4, 2, 4, 4, 4, 4, 0, 0, 4, 2, 4, 4, 4,
	 4, 4, 1, 4, 2, 4, 4, 4, 4, 0, 1, 4, 2, 4, 4, 4,
	 3, 3, 3, 4, 3, 4, 4, 4, 3, 0, 0, 4, 0, 4, 4, 4,
	 3, 3, 1, 4, 1, 4, 4, 4, 3, 0, 1, 4, 1, 4, 4, 4,
	 3, 3, 3, 4, 2, 4, 4, 4, 3, 0, 0, 4, 2, 4, 4, 4,
	 3, 3, 1, 4, 2, 4, 4, 4, 3, 0, 1, 4, 2, 4, 4, 4,
	 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 5, 0, 0, 0, 0,
	 5, 5, 1, 5, 1, 1, 1, 1, 5, 0, 1, 5, 1, 1, 1, 1,
	 5, 5, 5, 5, 2, 2, 2, 2, 5, 0, 0, 5, 2, 2, 2, 2,
	 5, 5, 1, 5, 2, 2, 2, 2, 5, 0, 1, 5, 2, 2, 2, 2,
	 3, 3, 3, 5, 3, 3, 3, 3, 3, 0, 0, 5, 0, 0, 0, 0,
	 3, 3, 1, 5, 1, 1, 1, 1, 3, 0, 1, 5, 1, 1, 1, 1,
	 3, 3, 3, 5, 2, 2, 2, 2, 3, 0, 0, 5, 2, 2, 2, 2,
	 3, 3, 1, 5, 2, 2, 2, 2, 3, 0, 1, 5, 2, 2, 2, 2,
	 5, 5, 5, 5, 5, 4, 4, 4, 5, 0, 0, 5, 0, 4, 4, 4,
	 5, 5, 1, 5, 1, 4, 4, 4, 5, 0, 1, 5, 1, 4, 4, 4,
	 5, 5, 5, 5, 2, 4, 4, 4, 5, 0, 0, 5, 2, 4, 4, 4,
	 5, 5, 1, 5, 2, 4, 4, 4, 5, 0, 1, 5, 2, 4, 4, 4,
	 3, 3, 3, 5, 3, 4, 4, 4, 3, 0, 0, 5, 0, 4, 4, 4,
	 3, 3, 1, 5, 1, 4, 4, 4, 3, 0, 1, 5, 1, 4, 4, 4,
	 3, 3, 3, 5, 2, 4, 4, 4, 3, 0, 0, 5, 2, 4, 4, 4,
	 3, 3, 1, 5, 2, 4, 4, 4, 3, 0, 1, 5, 2, 4, 4, 4,
	 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 6, 0, 6, 0, 0,
	 6, 6, 1, 6, 1, 6, 1, 1, 6, 0, 1, 6, 1, 6, 1, 1,
	 6, 6, 6, 6, 2, 6, 2, 2, 6, 0, 0, 6, 2, 6, 2, 2,
	 6, 6, 1, 6, 2, 6, 2, 2, 6, 0, 1, 6, 2, 6, 2, 2,
	 3, 3, 3, 6, 3, 6, 3, 3, 3, 0, 0, 6, 0, 6, 0, 0,
	 3, 3, 1, 6, 1, 6, 1, 1, 3, 0, 1, 6, 1, 6, 1, 1,
	 3, 3, 3, 6, 2, 6, 2, 2, 3, 0, 0, 6, 2, 6, 2, 2,
	 3, 3, 1, 6, 2, 6, 2, 2, 3, 0, 1, 6, 2, 6, 2, 2,
	 6, 6, 6, 6, 6, 6, 4, 4, 6, 0, 0, 6, 0, 6, 4, 4,
	 6, 6, 1, 6, 1, 6, 4, 4, 6, 0, 1, 6, 1, 6, 4, 4,
	 6, 6, 6, 6, 2, 6, 4, 4, 6, 0, 0, 6, 2, 6, 4, 4,
	 6, 6, 1, 6, 2, 6, 4, 4, 6, 0, 1, 6, 2, 6, 4, 4,
	 3, 3, 3, 6, 3, 6, 4, 4, 3, 0, 0, 6, 0, 6, 4, 4,
	 3, 3, 1, 6, 1, 6, 4, 4, 3, 0, 1, 6, 1, 6, 4, 4,
	 3, 3, 3, 6, 2, 6, 4, 4, 3, 0, 0, 6, 2, 6, 4, 4,
	 3, 3, 1, 6, 2, 6, 4, 4, 3, 0, 1, 6, 2, 6, 4, 4,
	 5, 5, 5, 5, 5, 6, 5, 5, 5, 0, 0, 5, 0, 6, 0, 0,
	 5, 5, 1, 5, 1, 6, 1, 1, 5, 0, 1, 5, 1, 6, 1, 1,
	 5, 5, 5, 5, 2, 6, 2, 2, 5, 0, 0, 5, 2, 6, 2, 2,
	 5, 5, 1, 5, 2, 6, 2, 2, 5, 0, 1, 5, 2, 6, 2, 2,
	 3, 3, 3, 5, 3, 6, 3, 3, 3, 0, 0, 5, 0, 6, 0, 0,
	 3, 3, 1, 5, 1, 6, 1, 1, 3, 0, 1, 5, 1, 6, 1, 1,
	 3, 3, 3, 5, 2, 6, 2, 2, 3, 0, 0, 5, 2, 6, 2, 2,
	 3, 3, 1, 5, 2, 6, 2, 2, 3, 0, 1, 5, 2, 6, 2, 2,
	 5, 5, 5, 5, 5, 6, 4, 4, 5, 0, 0, 5, 0, 6, 4, 4,
	 5, 5, 1, 5, 1, 6, 4, 4, 5, 0, 1, 5, 1, 6, 4, 4,
	 5, 5, 5, 5, 2, 6, 4, 4, 5, 0, 0, 5, 2, 6, 4, 4,
	 5, 5, 1, 5, 2, 6, 4, 4, 5, 0, 1, 5, 2, 6, 4, 4,
	 3, 3, 3, 5, 3, 6, 4, 4, 3, 0, 0, 5, 0, 6, 4, 4,
	 3, 3, 1, 5, 1, 6, 4, 4, 3, 0, 1, 5, 1, 6, 4, 4,
	 3, 3, 3, 5, 2, 6, 4, 4, 3, 0, 0, 5, 2, 6, 4, 4,
	 3, 3, 1, 5, 2, 6, 4, 4, 3, 0, 1, 5, 2, 6, 4, 4,
	 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0, 7, 0, 7, 7, 0,
	 7, 7, 1, 7, 1, 7, 7, 1, 7, 0, 1, 7, 1, 7, 7, 1,
	 7, 7, 7, 7, 2, 7, 7, 2, 7, 0, 0, 7, 2, 7, 7, 2,
	 7, 7, 1, 7, 2, 7, 7, 2, 7, 0, 1, 7, 2, 7, 7, 2,
	 3, 3, 3, 7, 3, 7, 7, 3, 3, 0, 0, 7, 0, 7, 7, 0,
	 3, 3, 1, 7, 1, 7, 7, 1, 3, 0, 1, 7, 1, 7, 7, 1,
	 3, 3, 3, 7, 2, 7, 7, 2, 3, 0, 0, 7, 2, 7, 7, 2,
	 3, 3, 1, 7, 2, 7, 7, 2, 3, 0, 1, 7, 2, 7, 7, 2,
	 7, 7, 7, 7, 7, 7, 7, 4, 7, 0, 0, 7, 0, 7, 7, 4,
	 7, 7, 1, 7, 1, 7, 7, 4, 7, 0, 1, 7, 1, 7, 7, 4,
	 7, 7, 7, 7, 2, 7, 7, 4, 7, 0, 0, 7, 2, 7, 7, 4,
	 7, 7, 1, 7, 2, 7, 7, 4, 7, 0, 1, 7, 2, 7, 7, 4,
	 3, 3, 3, 7, 3, 7, 7, 4, 3, 0, 0, 7, 0, 7, 7, 4,
	 3, 3, 1, 7, 1, 7, 7, 4, 3, 0, 1, 7, 1, 7, 7, 4,
	 3, 3, 3, 7, 2, 7, 7, 4, 3, 0, 0, 7, 2, 7, 7, 4,
	 3, 3, 1, 7, 2, 7, 7, 4, 3, 0, 1, 7, 2, 7, 7, 4,
	 5, 5, 5, 5, 5, 7, 7, 5, 5, 0, 0, 5, 0, 7, 7, 0,
	 5, 5, 1, 5, 1, 7, 7, 1, 5, 0, 1, 5, 1, 7, 7, 1,
	 5, 5, 5, 5, 2, 7, 7, 2, 5, 0, 0, 5, 2, 7, 7, 2,
	 5, 5, 1, 5, 2, 7, 7, 2, 5, 0, 1, 5, 2, 7, 7, 2,
	 3, 3, 3, 5, 3, 7, 7, 3, 3, 0, 0, 5, 0, 7, 7, 0,
	 3, 3, 1, 5, 1, 7, 7, 1, 3, 0, 1, 5, 1, 7, 7, 1,
	 3, 3, 3, 5, 2, 7, 7, 2, 3, 0, 0, 5, 2, 7, 7, 2,
	 3, 3, 1, 5, 2, 7, 7, 2, 3, 0, 1, 5, 2, 7, 7, 2,
	 5, 5, 5, 5, 5, 7, 7, 4, 5, 0, 0, 5, 0, 7, 7, 4,
	 5, 5, 1, 5, 1, 7, 7, 4, 5, 0, 1, 5, 1, 7, 7, 4,
	 5, 5, 5, 5, 2, 7, 7, 4, 5, 0, 0, 5, 2, 7, 7, 4,
	 5, 5, 1, 5, 2, 7, 7, 4, 5, 0, 1, 5, 2, 7, 7, 4,
	 3, 3, 3, 5, 3, 7, 7, 4, 3, 0, 0, 5, 0, 7, 7, 4,
	 3, 3, 1, 5, 1, 7, 7, 4, 3, 0, 1, 5, 1, 7, 7, 4,
	 3, 3, 3, 5, 2, 7, 7, 4, 3, 0, 0, 5, 2, 7, 7, 4,
	 3, 3, 1, 5, 2, 7, 7, 4, 3, 0, 1, 5, 2, 7, 7, 4,
	 6, 6, 6, 6, 6, 6, 7, 6, 6, 0, 0, 6, 0, 6, 7, 0,
	 6, 6, 1, 6, 1, 6, 7, 1, 6, 0, 1, 6, 1, 6, 7, 1,
	 6, 6, 6, 6, 2, 6, 7, 2, 6, 0, 0, 6, 2, 6, 7, 2,
	 6, 6, 1, 6, 2, 6, 7, 2, 6, 0, 1, 6, 2, 6, 7, 2,
	 3, 3, 3, 6, 3, 6, 7, 3, 3, 0, 0, 6, 0, 6, 7, 0,
	 3, 3, 1, 6, 1, 6, 7, 1, 3, 0, 1, 6, 1, 6, 7, 1,
	 3, 3, 3, 6, 2, 6, 7, 2, 3, 0, 0, 6, 2, 6, 7, 2,
	 3, 3, 1, 6, 2, 6, 7, 2, 3, 0, 1, 6, 2, 6, 7, 2,
	 6, 6, 6, 6, 6, 6, 7, 4, 6, 0, 0, 6, 0, 6, 7, 4,
	 6, 6, 1, 6, 1, 6, 7, 4, 6, 0, 1, 6, 1, 6, 7, 4,
	 6, 6, 6, 6, 2, 6, 7, 4, 6, 0, 0, 6, 2, 6, 7, 4,
	 6, 6, 1, 6, 2, 6, 7, 4, 6, 0, 1, 6, 2, 6, 7, 4,
	 3, 3, 3, 6, 3, 6, 7, 4, 3, 0, 0, 6, 0, 6, 7, 4,
	 3, 3, 1, 6, 1, 6, 7, 4, 3, 0, 1, 6, 1, 6, 7, 4,
	 3, 3, 3, 6, 2, 6, 7, 4, 3, 0, 0, 6, 2, 6, 7, 4,
	 3, 3, 1, 6, 2, 6, 7, 4, 3, 0, 1, 6, 2, 6, 7, 4,
	 5, 5, 5, 5, 5, 6, 7, 5, 5, 0, 0, 5, 0, 6, 7, 0,
	 5, 5, 1, 5, 1, 6, 7, 1, 5, 0, 1, 5, 1, 6, 7, 1,
	 5, 5, 5, 5, 2, 6, 7, 2, 5, 0, 0, 5, 2, 6, 7, 2,
	 5, 5, 1, 5, 2, 6, 7, 2, 5, 0, 1, 5, 2, 6, 7, 2,
	 3, 3, 3, 5, 3, 6, 7, 3, 3, 0, 0, 5, 0, 6, 7, 0,
	 3, 3, 1, 5, 1, 6, 7, 1, 3, 0, 1, 5, 1, 6, 7, 1,
	 3, 3, 3, 5, 2, 6, 7, 2, 3, 0, 0, 5, 2, 6, 7, 2,
	 3, 3, 1, 5, 2, 6, 7, 2, 3, 0, 1, 5, 2, 6, 7, 2,
	 5, 5, 5, 5, 5, 6, 7, 4, 5, 0, 0, 5, 0, 6, 7, 4,
	 5, 5, 1, 5, 1, 6, 7, 4, 5, 0, 1, 5, 1, 6, 7, 4,
	 5, 5, 5, 5, 2, 6, 7, 4, 5, 0, 0, 5, 2, 6, 7, 4,
	 5, 5, 1, 5, 2, 6, 7, 4, 5, 0, 1, 5, 2, 6, 7, 4,
	 3, 3, 3, 5, 3, 6, 7, 4, 3, 0, 0, 5, 0, 6, 7, 4,
	 3, 3, 1, 5, 1, 6, 7, 4, 3, 0, 1, 5, 1, 6, 7, 4,
	 3, 3, 3, 5, 2, 6, 7, 4, 3, 0, 0, 5, 2, 6, 7, 4,
	 3, 3, 1, 5, 2, 6, 7, 4, 3, 0, 1, 5, 2, 6, 7, 4};

/*------------------------------------------------------------------------*/

int Lx;
int Displace_vector[8];

#define E_PIX(pix) \
	(*(pix + 1))

#define W_PIX(pix) \
	(*(pix - 1))

#define N_PIX(pix) \
	(*(pix + Lx))

#define S_PIX(pix) \
	(*(pix - Lx))

#define SW_PIX(pix) \
	(*(pix - Lx - 1))

#define NW_PIX(pix) \
	(*(pix + Lx - 1))

#define NE_PIX(pix) \
	(*(pix + Lx + 1))

#define SE_PIX(pix) \
	(*(pix - Lx + 1))

#define NEIGHBOURS_CODE(pix)     \
	(((SW_PIX(pix) != 0) << 0) | \
	 ((S_PIX(pix) != 0) << 1) |  \
	 ((SE_PIX(pix) != 0) << 2) | \
	 ((W_PIX(pix) != 0) << 3) |  \
	 ((E_PIX(pix) != 0) << 4) |  \
	 ((NW_PIX(pix) != 0) << 5) | \
	 ((N_PIX(pix) != 0) << 6) |  \
	 ((NE_PIX(pix) != 0) << 7))

/*------------------------------------------------------------------------*/

#define FIRST_PRESEED(code) \
	First_preseed_table[code]

/*------------------------------------------------------------------------*/

#define NEXT_POINT(code, preseed) \
	Next_point_table[(code << 3) | preseed]

/*------------------------------------------------------------------------*/

#define UPDATE_POS(displ_code, x_pos, y_pos) \
	switch (displ_code) {                    \
		CASE 0 : x_pos--;                    \
		y_pos--;                             \
		CASE 1 : y_pos--;                    \
		CASE 2 : x_pos++;                    \
		y_pos--;                             \
		CASE 3 : x_pos--;                    \
		CASE 4 : x_pos++;                    \
		CASE 5 : x_pos--;                    \
		y_pos++;                             \
		CASE 6 : y_pos++;                    \
		CASE 7 : x_pos++;                    \
		y_pos++;                             \
	DEFAULT:                                 \
		assert(FALSE);                       \
	}

/*------------------------------------------------------------------------*/

/*=============================================================================*/

/*---------------------------------------------------------------------------*/

static int fillByteRaster(const TRaster32P &rin, TRasterGR8P &rout)
{
	bool ret = false;

	assert(rin->getLx() == rout->getLx() - 2 && rin->getLy() == rout->getLy() - 2);

	memset(rout->getRawData(), 0, rout->getLx() * rout->getLy());

	for (int i = 0; i < rin->getLy(); i++) {
		TPixel32 *bufIn = (TPixel32 *)rin->getRawData() + i * rin->getWrap();
		UCHAR *bufOut = (UCHAR *)rout->getRawData() + (i + 1) * rout->getWrap() + 1;

		for (int j = 0; j < rin->getLx(); j++, bufIn++, bufOut++)
			if (bufIn->m != 0) {
				ret = true;
				*bufOut = 1;
			}
	}
	return ret;
}

/*---------------------------------------------------------------------------*/

static void initialize_displace_vector(void)
{
	Displace_vector[0] = -Lx - 1;
	Displace_vector[1] = -Lx;
	Displace_vector[2] = -Lx + 1;
	Displace_vector[3] = -1;
	Displace_vector[4] = +1;
	Displace_vector[5] = Lx - 1;
	Displace_vector[6] = Lx;
	Displace_vector[7] = Lx + 1;
}

/*------------------------------------------------------------------------*/

static int find_next_seed(const TRasterGR8P &r, int first_seed,
						  UCHAR *&seed, int &x_pos, int &y_pos)
{
	static int Curr_x = 0, Curr_y = 0;
	UCHAR *pix;

	if (first_seed) {
		Curr_x = 0;
		Curr_y = 0;
	}

	seed = 0;

	for (; Curr_y < r->getLy(); Curr_y++) {
		pix = (UCHAR *)r->getRawData() + Curr_y * r->getWrap() + Curr_x;
		for (; Curr_x < r->getLx(); Curr_x++, pix++)
			if (*pix == 1 &&
				(!E_PIX(pix) ||
				 !W_PIX(pix) ||
				 !N_PIX(pix) ||
				 !S_PIX(pix))) {
				seed = pix;
				x_pos = Curr_x;
				y_pos = Curr_y;
				return true;
			}
		Curr_x = 0;
	}
	return false;
}

/*------------------------------------------------------------------------*/

#define COMPUTE_NUM_DEN(x_pos, y_pos, old_x_pos, old_y_pos) \
	{                                                       \
		aux = old_x_pos * y_pos - x_pos * old_y_pos;        \
		den += aux;                                         \
		x_num += (x_pos + old_x_pos) * aux;                 \
		y_num += (y_pos + old_y_pos) * aux;                 \
	}

/*------------------------------------------------------------------------*/

void doComputeCentroid(const TRasterGR8P &r, TPoint &cp)
{
	int preseed, displ, prewalker, code;
	UCHAR *seed, *walker;
	int x_num = 0, y_num = 0, den = 0;
	int x_pos, y_pos, old_x_pos, old_y_pos;
	int aux, old_displ = -1;
	int first_seed;

	Lx = r->getLx();
	Displace_vector[0] = -Lx - 1;
	Displace_vector[1] = -Lx;
	Displace_vector[2] = -Lx + 1;
	Displace_vector[3] = -1;
	Displace_vector[4] = +1;
	Displace_vector[5] = Lx - 1;
	Displace_vector[6] = Lx;
	Displace_vector[7] = Lx + 1;

	first_seed = TRUE;

	while (find_next_seed(r, first_seed, seed, x_pos, y_pos)) {
		first_seed = FALSE;

		*seed |= 0x2;

		code = NEIGHBOURS_CODE(seed);
		preseed = FIRST_PRESEED(code);

		if (preseed == 8)
			continue;

		displ = NEXT_POINT(code, preseed);

		walker = seed + Displace_vector[displ];

		old_x_pos = x_pos;
		old_y_pos = y_pos;

		UPDATE_POS(displ, x_pos, y_pos)

		COMPUTE_NUM_DEN(x_pos, y_pos, old_x_pos, old_y_pos)

		prewalker = displ ^ 0x7;

		while ((walker != seed) || (preseed != prewalker)) {
			*walker |= 0x2;

			displ = NEXT_POINT(NEIGHBOURS_CODE(walker), prewalker);

			walker += Displace_vector[displ];
			old_x_pos = x_pos;
			old_y_pos = y_pos;
			UPDATE_POS(displ, x_pos, y_pos)
			COMPUTE_NUM_DEN(x_pos, y_pos, old_x_pos, old_y_pos)
			prewalker = displ ^ 0x7;
		}
	}

	if (den)
		cp = TPoint(tround(x_num / (3.0 * den)), tround(y_num / (3.0 * den)));
}

} //namespace

TPoint computeCentroid(const TRaster32P &r)
{
	TPoint ret(1, 1);

	TRasterGR8P raux(r->getLx() + 2, r->getLy() + 2);

	if (fillByteRaster(r, raux))
		doComputeCentroid(raux, ret);

	ret.x--;
	ret.y--; /* per il bordo aggiunto */
	return ret;
}
