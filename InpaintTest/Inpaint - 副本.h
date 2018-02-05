#pragma once
#include <stdint.h>
#include <map>
#include <vector>
#include <utility>
using namespace std;
#include <QtGui>

class CInpaint
{
public:
	CInpaint();
	~CInpaint();

	bool InpaintGray( uint8_t* imgBuf, int32_t width, int32_t height, int32_t pitch, uint8_t colorKey );
	bool InpaintBGRA( uint8_t* imgBuf, int32_t width, int32_t height, int32_t pitch, uint32_t colorKey );
private:
	class CImage
	{
	public:
		int32_t		width;
		int32_t		height;
		int32_t		pitch;
		int8_t		bytesPerPixel;
		bool		useExternal;
		uint8_t*	imgBuf;
		CImage()
		{
			width = height = pitch = bytesPerPixel = 0;
			useExternal = false; imgBuf = NULL;
		}
		~CImage()
		{
			clear();
		}
		bool create( int32_t _bytesPerPixel, uint8_t* _imgBuf, int32_t _width, int32_t _height, int32_t _pitch, bool _useExternal = false )
		{
			clear();
			if ( _bytesPerPixel <= 0 || _height <= 0 || _height <= 0 ) return false;
			if ( _imgBuf == NULL && _useExternal ) return false;
			width = _width;
			height = _height;
			bytesPerPixel = _bytesPerPixel;
			useExternal = _useExternal;
			if ( _pitch <= 0 ) _pitch = ( _bytesPerPixel * _width + 3 ) / 4 * 4;
			if ( _useExternal )
			{
				pitch = _pitch;
				imgBuf = _imgBuf;
			}
			else
			{
				pitch	= _width * _bytesPerPixel;
				imgBuf	= new uint8_t[pitch * height];
				if ( _imgBuf )
				{
					for ( int32_t y = 0; y < height; ++y )
					{
						memcpy( imgBuf + y * pitch, _imgBuf + y * _pitch, min( pitch, _pitch ) );
					}
				}
				else
				{
					memset( imgBuf, 0, pitch * height );
				}
			}
			return true;
		}
		void clear()
		{
			if ( !useExternal && imgBuf )
				delete[]imgBuf;
			width = height = pitch = bytesPerPixel = 0;
			useExternal = false; imgBuf = NULL;
		}
		inline uint8_t* pixel( int32_t x, int32_t y )
		{
			return imgBuf + y * pitch + x * bytesPerPixel;
		}
		int32_t byteCount() { return pitch * height; }
	};
	#define	MAX_EDGE_BLOCK_RADIUS	3
	#define	MAX_BLOCK_FIND_RADIUS	100//( MAX_EDGE_BLOCK_RADIUS * 10 )
	#define	MAX_BLOCK_DIAMETER		( MAX_EDGE_BLOCK_RADIUS * 2 + 1 )
	CImage			m_imgOrg;
	CImage			m_imgExpend;
	CImage			m_imgProc;

	QPoint			m_boundTopLeft;
	QRect			m_inpaintBbounds;
	QRect			m_imgRect;
	struct SEdgeBlock : public QPoint
	{
		int32_t	sum;
		QPoint	ptOffset;
		SEdgeBlock( const QPoint& pt, int32_t _sum = 0 )
			:QPoint( pt )
		{
			sum	= _sum;
		}
		SEdgeBlock( int32_t _x, int32_t y, int32_t _sum = 0 )
			:QPoint( _x, y )
		{
			sum	= _sum;
		}
	};
	vector<SEdgeBlock>	m_edgePots;	//重要的边缘块(中心坐标与半径内的轮廓图像素和)。
	//SImage	m_img
	bool	Inpaint();
	int32_t findEdgePots();
	bool	findBestFillBlock( const SEdgeBlock& poRevamp, vector<pair<QPoint, QPoint>>& pots );
	int32_t	otsuSplit( vector<SEdgeBlock>& list, int32_t totSum );
	
	inline int32_t blockSubSum( const QPoint& poRevamp, const QPoint& ptExplant );
	inline void clearBlockEdge( const QPoint& poRevamp );
	inline void clearBlockEdge2( const QPoint& poRevamp );

	inline void fastFillBlock( const QPoint& poRevamp );
	//inline void fillBlockEdge( const QPoint& poRevamp, const QPoint& poOffset );

};

