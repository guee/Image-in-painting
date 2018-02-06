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
	//初始化要修补的图像的掩模。
	//width		: 图像的宽
	//height	: 图像的高
	bool regionReset( int32_t width, int32_t height ) { return resetSize( width, height ); }

	//以鼠标涂抹的方式设置要修补的区域。
	//开始绘制，设置画笔的半径，画笔的形状是圆形。
	//radius	: 半径，圆的实际直径是 radius * 2 + 1，因此当半径可以为 0。
	//通常在鼠标按下键时，调用 regionPencilBegin。
	bool regionPencilBegin( int32_t radius ) { return pencilBegin( radius ); }
	//输入鼠标移动的坐标。
	//x,y		: 坐标。
	//当输入第一个坐标时，会在坐标位置绘制指定半径的实心圆。
	//输入后续坐标时，以指定半径的实心圆把鼠标移动的轨迹连成成线。
	//通常在鼠标移动时，调用 regionPencilPos。
	bool regionPencilPos( int32_t x, int32_t y ) { return pencilPos( x, y ); }
	//结束当前的绘制。
	//通常在鼠标松开按键时，调用 regionPencilEnd。
	bool regionPencilEnd() { return pencilEnd(); }

	//设置要修补的矩形区域，可以多次调用设置多个区域。
	//x,y		: 区域的左上角坐标（图像的左上角坐标为(0,0)）
	//width, height	: 区域的宽度和高度。
	bool regionRect( int32_t x, int32_t y, int32_t width, int32_t height ) { return fillRect( x, y, width, height ); }

	//把线段连接成的闭合多边形设置为要修补的区域。
	//pots		: 多边形每个角的坐标，数组中每两个 int32_t 分别为 x, y。
	//				每组 x,y 与它前一组坐标连接成线，最终组成闭合的多边形。
	//potCount	: 坐标的数量。因为每组坐标(x,y)需要两个 int32_t，所以 potCount 是 pots 数组长度的一半。
	bool regionPath( const int32_t pots[], int32_t potCount ) { return pathClosed( pots, potCount ); }

	//水印侦测。输入多张在相同位置上有水印的相同分辨率的图像，自动检测水印的区域。
	//imgBuf	: 图像的数据，只能是32位像素格式。
	//pitch		: 图像每行的字节数。图像的宽高与 regionReset 设置的一致。
	//函数返回 false 表示还没有成功检测到水印，需要继续输入其它图像。
	//函数返回 true 表示已经成功检测到水印。可以使用 getRegionImage 得取掩模图像查看检测到的水印区域。
	//				检测到水印后，会自动使用 regionPath 函数标记出水印区域。
	//输入的图像越多，检测到的水印区域越准确，即使函数已经返回了 true，仍然可以继续输入图像。
	//通常如果输入的每张图片画面有较大的区别，那么 5 到 10 张就可以准确地检测到水印，否则就需要更多的图片。
	//如果检测视频中的水印，因为临近的帧差别很小，最好是每间隔几秒的画面输入一帧，以减少计算量。
	//如果视频本身很短，例如 10 秒，那么也可以间隔几帧输入一次。
	bool regionDetection( uint8_t* imgBuf, int32_t pitch ) {
		return CMaskDrawer::watermarkDetection( imgBuf, pitch ); }

	//取得设置修补区域的操作数量（包含已经撤消的操作）。
	//current	: 返回当前操作的索引。
	//因为可以撤消、重做操作，所以 current 可能小于函数的返回值，表示撤消了一些操作。
	int32_t	regionOperNum( int32_t& current ) { return operNum( current ); }

	//撤消一步操作。
	//每次撤消操作后，使用 regionOperNum 函数取得 current 值会比之前少1，直到为0。
	bool regionUndo() { return undo(); }
	//重做被撤消的操作。
	//每次重做操作后，使用 regionOperNum 函数取得 current 值会比之前加1，直到与 regionOperNum 的返回值相等。
	bool regionRedo() { return redo(); }
	
	//每次进行了设置修补区域的操作(包含撤消和重做)后，可以取得掩模受到影响的矩形区域。
	//如果进行多次操作后才调用本函数取得掩模受到影响的矩形区域，则得到的是累积合并的矩形区域。
	bool getRegionChangedRect( int32_t& x, int32_t& y, int32_t& width, int32_t& height ) { return getChangedRect( x, y, width, height ); }
	//取得掩模的图像。
	//获得的是一个 8bit 的位图，即每个字节表示一个像素。
	//pitch		: 掩模图像每行的字节数。图像的宽高与 regionReset 设置的相同。
	//返回的图像数据中，如果字节值不为0，表示当前位置的像素已经被设置为待修补。
	const uint8_t* getRegionImage( int32_t& pitch ) { return getMaskImage( pitch ); }

	//开始修补图像。只能是32位像素格式。
	//imgBuf	: 图像的数据
	//pitch		: 图像每行的字节数。图像的宽高与 regionReset 设置的一致。
	//inpType	: 修补的方式。 0 或 1 或 2
	//如果要对多张图片进行修补，当每张图片的水印位置和图像分辨率相同时，不需要重复设置待修补区域。
	//当需要修补的图片中水印位置或分辨率与上次不同时，必须调用 regionReset，然后重新设置待修补区域。
	//修补了图像之后，对修补区域掩模(mask)的操作就不能撤消了，如果再次设置修补区域，则使用新设置的修补区域。
	bool inpaint( uint8_t* imgBuf, int32_t pitch, int32_t inpType );
private:

#define	PIXEL_DIFF_MIN		16
#define	PIXEL_DIFF_MAX		128


#define	BLOCK_RADIUS	7
#define	MAX_BLOCK_FIND_RADIUS	50//( MAX_EDGE_BLOCK_RADIUS * 10 )
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
#define	IS_UNREPAIR_PIXEL( pix )	( ( (pix) & ( PIXEL_MISSING | PIXEL_PATCHED ) ) == PIXEL_MISSING )	//未修复
#define	IS_MISSING_PIXEL( pix )	( (pix) & ( PIXEL_MISSING ) )	//待修复


#define	PIXEL_BOREDR_RADIUS_MASK		0x0F
#define	IS_SEARCH_PIXEL( pix )	( ( (pix) & ( PIXEL_INVALID | PIXEL_MISSING | PIXEL_BOREDR_RADIUS_MASK ) ) == 0 )	//图像上一直存在的
#define	IS_VALID_PIXEL( pix )	( IS_STATIC_PIXEL( pix ) || ( (pix) & ( PIXEL_BOREDR_RADIUS_MASK ) ) )
#define	IS_BORDER_PIXEL( pix )	( (pix) & ( PIXEL_BORDER ) )	//修复区域边界

#pragma pack(pop)
	CImage<SOrgPixel>	m_imgOrg;
	CImage<uint8_t>		m_imgProc;

	GPoint			m_boundTopLeft;
	GRect			m_findRect;

	struct SEdgePoint : public GPoint
	{
		int32_t		pixCount;
		int32_t		xLength;
		int32_t		yLength;
		SOrgPixel	avgColor;
		SOrgPixel	xEndColor;
		SOrgPixel	yEndColor;
		SEdgePoint() { pixCount = 0; avgColor = SOrgPixel(); xLength = yLength = 0; }
		SEdgePoint( const GPoint& pt, int32_t _pixCount = 0, SOrgPixel _color = SOrgPixel() )
			:GPoint( pt )
		{
			pixCount	= _pixCount;
			xEndColor = yEndColor = avgColor = _color;
			xLength = yLength = 1;
		}
		SEdgePoint( int32_t _x, int32_t _y, int32_t _pixCount = 0, SOrgPixel _color = SOrgPixel() )
			:GPoint( _x, _y )
		{
			pixCount	= _pixCount;
			xEndColor = yEndColor = avgColor = _color;
			xLength = yLength = 1;
		}
	};
	struct SBorderPoint : public GPoint
	{
		int32_t		sum;
		GPoint		offset;
		SBorderPoint() { sum = 0; }
		SBorderPoint( const GPoint& pt, int32_t _sum = 0 )
			:GPoint( pt )	{ sum	= _sum;	}
		SBorderPoint( int32_t _x, int32_t _y, int32_t _sum = 0 )
			:GPoint( _x, _y ) {	sum	= _sum;	}
	};
	SEdgePoint*		m_edgePots;
	int32_t			m_edgeAlloc;
	int32_t			m_edgeCount;
	vector<GPoint>	m_borderPtFirsts;
	vector<SBorderPoint>	m_borderPots;

	int32_t findEdgePots();
	int32_t findBorderPots();
	void	fastInpaint();
	void	fastInpaint2();
	void	expendBorder();
	void	drawbackBorder();
	//SOrgPixel getAvgColor( int32_t x, int32_t y, int32_t radius );
	inline int32_t checkFirstEdge( int32_t x, int32_t y );
	inline int32_t checkFirstEdge2( int32_t x, int32_t y );
	inline bool checkFirstBorder( int32_t x, int32_t y, int32_t radius = PIXEL_BOREDR_RADIUS_MASK );
		
	void structuralRepair( int32_t x, int32_t y );

	const	GPoint	poOctree8[8]	= { { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
};

