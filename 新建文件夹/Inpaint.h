#pragma once
#include <stdint.h>
#include <map>
#include <vector>
#include <utility>
using namespace std;

#include "MaskDrawer.h"
class CInpaint : private CMaskDrawer
{
public:
	CInpaint();
	~CInpaint();
	//设置要修补的原图像。只能是32位像素格式。
	//pitch 是图像每行的像素数。
	bool setDamagedImage( uint8_t* imgBuf, int32_t width, int32_t height, int32_t pitch );
	//bool regionPencilBegin( int32_t radius ) { return pencilBegin( radius ); }
	//bool regionPencilPos( int32_t x, int32_t y ) { return pencilPos( x, y ); }
	//bool regionPencilEnd( int32_t radius ) { return pencilEnd(); }
	//设置要修补的区域，可以设置多个区域。
	bool regionRect( int32_t x, int32_t y, int32_t width, int32_t height ) { return fillRect( x, y, width, height ); }
	//bool regionPath( const int32_t pots[], int32_t potCount ) { return pathClosed( pots, potCount ); }
	int32_t	regionOperNum( int32_t& current ) { return operNum( current ); }
	int32_t regionRedo() { return redo(); }
	int32_t regionUndo() { return undo(); }
	bool getRegionChangedRect( int32_t& x, int32_t& y, int32_t& width, int32_t& height ) { return getChangedRect( x, y, width, height ); }
	uint8_t* getRegionImage( int32_t& pitch ) { return getMaskImage( pitch ); }
	typedef	void( *OnProcProgress ) ( int32_t progress, void* cbParam );
	//开始修补。
	bool inpaint();

private:
	OnProcProgress	m_cbFunction;
	void*			m_cbParam;


#define	PIXEL_DIFF_MIN		16
#define	PIXEL_DIFF_MAX		128


#define	MAX_EDGE_BLOCK_RADIUS	15
#define	PRIORITY_BLOCK_RADIUS	2
#define	MAX_BLOCK_FIND_RADIUS	40//( MAX_EDGE_BLOCK_RADIUS * 10 )
#define	MAX_BLOCK_DIAMETER		( MAX_EDGE_BLOCK_RADIUS * 2 + 1 )


#pragma pack(push)
#pragma pack(1)
	struct SOrgPixel
	{
		uint8_t	b;
		uint8_t	g;
		uint8_t	r;
		uint8_t	a;
		SOrgPixel( uint8_t _b = 0, uint8_t _g = 0, uint8_t _r = 0, uint8_t _a = 0 )
		{
			b = _b; g = _g; r = _r; a = _a;
		}
	};

#define	PIXEL_INVALID	0x80
#define	PIXEL_MISSING	0x40
#define	PIXEL_BORDER	0x20
#define	PIXEL_PATCHED	0x10
#define	IS_STATIC_PIXEL( pix )	( ( (pix) & ( PIXEL_INVALID | PIXEL_MISSING ) ) == 0 )	//图像上一直存在的
#define	IS_PATCHED_PIXEL( pix )	( (pix) & ( PIXEL_PATCHED ) )	//已经修复
#define	IS_VALID_PIXEL( pix )	( IS_STATIC_PIXEL( pix ) || IS_PATCHED_PIXEL( pix ) )
#define	IS_MISSING_PIXEL( pix )	( (pix) & ( PIXEL_MISSING ) )	//待修复
#define	IS_UNREPAIR_PIXEL( pix )	( ( (pix) & ( PIXEL_MISSING | PIXEL_PATCHED ) ) == PIXEL_MISSING )	//未修复
#define	IS_BORDER_PIXEL( pix )	( (pix) & ( PIXEL_BORDER ) )	//修复区域边界

#pragma pack(pop)
	CImage<SOrgPixel>		m_imgOrg;
	CImage<uint8_t>		m_imgProc;

	GPoint			m_boundTopLeft;
	GRect			m_inpRect;
	struct SEdgePoint : public GPoint
	{
		int32_t	sum;
		int32_t	xLength;
		int32_t	yLength;
		SOrgPixel	avgColor;
		SOrgPixel	xEndColor;
		SOrgPixel	yEndColor;
		SEdgePoint() { sum = 0; avgColor = SOrgPixel(); xLength = yLength = 0; }
		SEdgePoint( const GPoint& pt, int32_t _sum = 0, SOrgPixel _color = SOrgPixel() )
			:GPoint( pt )
		{
			sum	= _sum;
			avgColor = _color;
			xLength = yLength = 0;
		}
		SEdgePoint( int32_t _x, int32_t _y, int32_t _sum = 0, SOrgPixel _color = SOrgPixel() )
			:GPoint( _x, _y )
		{
			sum	= _sum;
			avgColor = _color;
			xLength = yLength = 0;
		}
	};
	//vector<SEdgePoint>	m_edgePots;	//修补区域的边缘点集合。

	SEdgePoint*	m_borderPots;
	int32_t		m_borderAlloc;
	int32_t		m_borderCount;

	struct SScanLine
	{
		int32_t	ptStart;
		int32_t	lenght;
		int32_t	potIndex;
		SOrgPixel	colorSta;
		SOrgPixel	colorEnd;
	};


	//SImage	m_img
	bool	Inpaint();
	int32_t findEdgePots();
	void	fastInpaint();
	SOrgPixel getAvgColor( int32_t x, int32_t y, int32_t radius );
	//bool	bestFillBlock( const GPoint& ptCenter, GPoint& ptOffset, bool adjacent );
	//inline void fillBlock( const GPoint& ptCenter, const GPoint& ptMatch, int32_t radius );
	inline int32_t checkFirstEdge( int32_t x, int32_t y );
	inline int32_t checkFirstEdge_force( int32_t x, int32_t y );
	//void	expendFillBlock( const GPoint& ptOffset, int32_t maxDiff, vector<GPoint>& pots );
	
	//int32_t	otsuSplit( vector<SEdgePoint>::iterator begin, vector<SEdgePoint>::iterator end );
	
	//inline int32_t blockDiffSum( const GPoint& poCenter, const GPoint& ptMatch, int32_t radius );



};

