#include "Inpaint.h"
#include <algorithm>
#include "assert.h" 
//#include <QtGui>

CInpaint::CInpaint()
{
	m_edgePots	= nullptr;
	m_edgeAlloc	= 0;
	m_edgeCount	= 0;
}


CInpaint::~CInpaint()
{
	if ( m_edgePots ) free( m_edgePots );
}

bool CInpaint::inpaint( uint8_t* imgBuf, int32_t pitch, int32_t inpType )
{
	if ( nullptr == imgBuf || pitch <= 0 ) return false;
	if ( m_picChips.empty() && !m_inpRect.isValid() ) return false;
	m_imgOrg.create( (SOrgPixel*)imgBuf, m_maskImage.width, m_maskImage.height, pitch, true );
	m_imgProc.create( ( uint8_t*)m_maskImage.pixel( 0, 0 ), m_maskImage.width, m_maskImage.height, m_maskImage.pitch, true );

	//如果有通过水印检测得到的修补区域，就要在修补前检查图像的水印区域是否有水印。
	if ( m_wmarkChips.size() )
	{
		for ( int32_t i = 0; i < m_wmarkChips.size(); ++i )
		{
			if ( m_wmarkChips[i] >= m_currOper ) continue;
			SChip*		chip	= m_picChips[m_wmarkChips[i]];
			int32_t		validPix	= 0;
			int32_t		allPix		= 0;
			for ( int32_t y = max( 1, chip->rect.top() ), endY = min( chip->rect.bottom(), m_maskImage.height - 2 ); y <= endY; ++y )
			{
				int32_t		staX		=  max( 1, chip->rect.left() );
				SOrgPixel*	imgPix	= m_imgOrg.pixel( staX, y );
				SOrgPixel*	imgPixT	= m_imgOrg.pixel( staX, y - 1 );
				SOrgPixel*	imgPixB	= m_imgOrg.pixel( staX, y + 1 );
				SDetectPixel*	detPix	= m_watermark.pixel( staX, y );
				for ( int32_t x = staX, endX = min( chip->rect.right(), m_maskImage.width - 2 ); x <= endX; ++x )
				{
					if ( detPix->mark2 )
					{
						int32_t		sx	= ( ( imgPixT->b + imgPix->b * 2 + imgPixB->b ) - ( imgPixT->b + imgPix->b * 2 + imgPixB->b ) );
						int32_t		sy	= ( ( imgPixT->b + imgPixT->b * 2 + imgPixT->b ) - ( imgPixB->b + imgPixB->b * 2 + imgPixB->b ) );
						uint32_t	sobelB	= uint32_t( sqrt( sx * sx + sy * sy ) );
						sx	= ( ( imgPixT->g + imgPix->g * 2 + imgPixB->g ) - ( imgPixT->g + imgPix->g * 2 + imgPixB->g ) );
						sy	= ( ( imgPixT->g + imgPixT->g * 2 + imgPixT->g ) - ( imgPixB->g + imgPixB->g * 2 + imgPixB->g ) );
						uint32_t	sobelG	= uint32_t( sqrt( sx * sx + sy * sy ) );
						sx	= ( ( imgPixT->r + imgPix->r * 2 + imgPixB->r ) - ( imgPixT->r + imgPix->r * 2 + imgPixB->r ) );
						sy	= ( ( imgPixT->r + imgPixT->r * 2 + imgPixT->r ) - ( imgPixB->r + imgPixB->r * 2 + imgPixB->r ) );
						uint32_t	sobelR	= uint32_t( sqrt( sx * sx + sy * sy ) );
						if ( sobelB > 150 || sobelG > 150 || sobelR > 150 || ( sobelB + sobelG + sobelR ) > 300 )
						{
							++validPix;
						}
						++allPix;
					}
					++imgPix;
					++imgPixT;
					++imgPixB;
					++detPix;
				}
			}
			//qDebug() << "Ind:" << i << ", all:" << allPix << ", valid:" << validPix << ", " << float( validPix ) / allPix;
			if ( validPix * 3 < allPix )
			{
				undoOper( chip );
			}
			else
			{
				redoOper( chip );
			}
		}
	}
	if ( inpType <= 0 )
	{
		if ( 0 == findEdgePots() ) return false;
		fastInpaint();

	}
	else if ( inpType == 1 )
	{
		if ( 0 == findBorderPots() ) return false;
		fastInpaint2();
	}
	else
	{
		if ( 0 == findBorderPots() ) return false;
		expendBorder();
		for ( auto pt = m_borderPtFirsts.begin(); pt != m_borderPtFirsts.end(); ++pt )
		{
			structuralRepair( pt->x(), pt->y() );
		}
		drawbackBorder();

		for ( int32_t y = m_inpRect.top(); y <= m_inpRect.bottom(); ++y )
		{
			uint8_t*	prcPix	= m_imgProc.pixel( m_inpRect.left(), y );
			for ( int32_t x = m_inpRect.left(); x <= m_inpRect.right(); ++x )
			{
				if ( IS_STATIC_PIXEL( *prcPix ) )
				{
					*prcPix	= 0;
				}
				++prcPix;
			}
		}

	}
	m_maskReset	= true;
	return true;
}

int32_t CInpaint::findEdgePots()
{
	m_edgeCount	= 0;
	if ( !getUnited() ) return 0;

	for ( auto rt = m_rtAbsolutes.begin(); rt != m_rtAbsolutes.end(); ++rt )
	{
		int32_t	left		= rt->left();
		int32_t	right		= rt->right();
		int32_t	top			= rt->top();
		int32_t	bottom		= rt->bottom();
		int32_t	stepLen		= rt->width();
		for ( int32_t y = top; y <= bottom; ++y )
		{
			uint8_t*	prcRev	= m_imgProc.pixel( left, y );
			for ( int32_t d = 0; d < stepLen; ++d )
			{
				*prcRev++	= PIXEL_MISSING;
			}
			int32_t	index0	= checkFirstEdge( left, y );
			int32_t	index1	= stepLen > 1 ? checkFirstEdge( right, y ) : index0;
			m_edgePots[index0].xLength	= stepLen;
			SOrgPixel*	imgRev	= m_imgOrg.pixel( left, y );
			for ( int32_t d = 0; d < stepLen - 1; ++d )
			{
				*( (int32_t*)imgRev )	= index0;
				++imgRev;
			}
			m_edgePots[index1].xLength	= 1;
			m_edgePots[index1].yLength	= 1;
			m_edgePots[index0].xEndColor	= m_edgePots[index1].avgColor;
			*( (int32_t*)imgRev )	= index1;
		}
		stepLen	= rt->height();
		for ( int32_t x = left + 1; x < right; ++x )
		{
			int32_t	yIndex0	= checkFirstEdge( x, top );
			int32_t	yIndex1	= checkFirstEdge( x, bottom );
			if ( top == 0 || top == m_imgOrg.height - 1 )
			{
				*( (int32_t*)m_imgOrg.pixel( x, top ) )	= yIndex0;
			}
			m_edgePots[yIndex0].xLength	= 1;
			m_edgePots[yIndex0].yLength	= stepLen;
			m_edgePots[yIndex1].xLength	= 1;
			m_edgePots[yIndex1].yLength	= -stepLen;
			m_edgePots[yIndex0].yEndColor	= m_edgePots[yIndex1].avgColor;
			if ( bottom == m_imgOrg.height - 1 )
			{
				*( (int32_t*)m_imgOrg.pixel( x, bottom ) )	= yIndex1;
			}
		}

		int32_t	xIndex	= *( (int32_t*)m_imgOrg.pixel( left, top ) );
		m_edgePots[xIndex].yLength	= stepLen;
		m_edgePots[xIndex].yEndColor	= m_edgePots[*( (int32_t*)m_imgOrg.pixel( left, bottom ) )].avgColor;
		xIndex	= *( (int32_t*)m_imgOrg.pixel( right, top ) );
		m_edgePots[xIndex].yLength	= stepLen;
		m_edgePots[xIndex].yEndColor	= m_edgePots[*( (int32_t*)m_imgOrg.pixel( right, bottom ) )].avgColor;

	}

	for ( auto rt = m_rtUniteds.begin(); rt != m_rtUniteds.end(); ++rt )
	{
		for ( int32_t y = rt->top(); y <= rt->bottom(); ++y )
		{
			int32_t		ptStart;
			bool		isStart	= false;
			uint8_t*	pixPrc	= m_imgProc.pixel( rt->left(), y );
			SOrgPixel*	pixImg	= m_imgOrg.pixel( rt->left(), y );
			for ( int32_t x = rt->left(); x <= rt->right() + 1; ++x )
			{
				if ( *pixPrc && x <= rt->right() )
				{
					if ( !isStart )
					{
						isStart	= true;
						ptStart	= x;
					}
				}
				else
				{
					if ( isStart )
					{
						isStart	= false;
						int32_t	stepLen	= x - ptStart;
						uint8_t*	prcRev	= pixPrc - stepLen;
						for ( int32_t d = 0; d < stepLen; ++d )
						{
							*prcRev		= PIXEL_MISSING;
							++prcRev;
						}
						SOrgPixel*	imgRev	= pixImg - stepLen;
						int32_t	index0	= checkFirstEdge( ptStart, y );
						int32_t	index1	= stepLen > 1 ? checkFirstEdge( x - 1, y ) : index0;
						m_edgePots[index0].xLength	= stepLen;
						for ( int32_t d = 0; d < stepLen - 1; ++d )
						{
							*( (int32_t*)imgRev )	= index0;
							++imgRev;
						}
						m_edgePots[index0].xEndColor	= m_edgePots[index1].avgColor;
						*( (int32_t*)imgRev )	= index1;

					}
				}
				++pixPrc;
				++pixImg;
			}
		}

		for ( int32_t x = rt->left(); x <= rt->right(); ++x )
		{
			int32_t		ptStart;
			bool		isStart	= false;
			uint8_t*	pixPrc	= m_imgProc.pixel( x, rt->top() );
			for ( int32_t y = rt->top(); y <= rt->bottom() + 1; ++y )
			{
				if ( IS_MISSING_PIXEL( *pixPrc ) && y <= rt->bottom() )
				{
					if ( !isStart )
					{
						isStart	= true;
						ptStart	= y;
					}
				}
				else
				{
					if ( isStart )
					{
						isStart	= false;
						int32_t	stepLen	= y - ptStart;
						int32_t	xIndex	= *( (int32_t*)m_imgOrg.pixel( x, ptStart ) );
						int32_t	yIndex0	= checkFirstEdge( x, ptStart );
						int32_t	yIndex1	= checkFirstEdge( x, y - 1 );
						if ( yIndex0 >= 0 )
						{
							if ( ptStart == 0 || ptStart == m_imgOrg.height - 1 )
							{
								*( (int32_t*)m_imgOrg.pixel( x, ptStart ) )	= yIndex0;
							}
							m_edgePots[yIndex0].yLength	= stepLen;
							if ( yIndex1 >= 0 )
							{
								m_edgePots[yIndex1].yLength		= -stepLen;
								m_edgePots[yIndex0].yEndColor	= m_edgePots[yIndex1].avgColor;
								if ( y == m_imgOrg.height )
								{
									*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )	= yIndex1;
								}
							}
							else
							{
								m_edgePots[yIndex0].yEndColor	= m_edgePots[*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )].avgColor;
							}
						}
						else
						{
							m_edgePots[xIndex].yLength	= stepLen;
							if ( yIndex1 >= 0 )
							{
								m_edgePots[xIndex].yEndColor	= m_edgePots[yIndex1].avgColor;
								if ( y == m_imgOrg.height )
								{
									*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )	= yIndex1;
								}
							}
							else
							{
								m_edgePots[xIndex].yEndColor	= m_edgePots[*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )].avgColor;
							}
						}

					}
				}
				pixPrc += m_imgProc.pitch;
			}
		}
	}
	return m_edgeCount;
}

//CInpaint::SOrgPixel CInpaint::getAvgColor( int32_t x, int32_t y, int32_t radius )
//{
//	int32_t		stX	= max( x - radius, 0 );
//	int32_t		stY	= max( y - radius, 0 );
//	int32_t		enX	= min( x + radius + 1, m_imgProc.width );
//	int32_t		enY	= min( y + radius + 1, m_imgProc.height );
//	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0;
//	for ( int32_t dy = stY; dy < enY; ++dy )
//	{
//		uint8_t*	prcRev	= m_imgProc.pixel( stX, dy );
//		SOrgPixel*	imgRev	= m_imgOrg.pixel( stX, dy );
//		for ( int32_t dx = stX; dx < enX; ++dx )
//		{
//			if ( IS_VALID_PIXEL(*prcRev) )
//			{
//				b += imgRev->b;
//				g += imgRev->g;
//				r += imgRev->r;
//				a += imgRev->a;
//				++c;
//			}
//			++prcRev;
//			++imgRev;
//		}
//	}
//	return SOrgPixel( uint8_t( b / c ), uint8_t( g / c ), uint8_t( r / c ), uint8_t( a / c ) );
//}

void CInpaint::fastInpaint()
{
	if ( m_edgeCount == 0 ) return;
	auto	funAngularPoint	= [this]( int32_t x, int32_t y )->void {
		uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
		if ( IS_MISSING_PIXEL( *pixPrc ) )
		{
			uint32_t	distanceMin	= UINT_MAX;
			SEdgePoint*	ptFind = nullptr;
			for ( int32_t i = 0; i < m_edgeCount; ++i )
			{
				SEdgePoint*	pt	= m_edgePots + i;
				if ( pt->pixCount == 0 ) continue;
				uint32_t	distance	= ( x - pt->x() ) * ( x - pt->x() ) + ( y - pt->y() ) * ( y - pt->y() );
				if ( distance < distanceMin )
				{
					distanceMin	= distance;
					ptFind		= pt;
				}
			}
			SOrgPixel*	pixOrg	= m_imgOrg.pixel( x, y );
			m_edgePots[*( (int32_t*)pixOrg )].avgColor	= ptFind->avgColor;
		}
	};

	funAngularPoint( 0, 0 );
	funAngularPoint( m_imgProc.width - 1, 0 );
	funAngularPoint( m_imgProc.width - 1, m_imgProc.height - 1 );
	funAngularPoint( 0, m_imgProc.height - 1 );

	auto	funEdgeLine	= [this]( int32_t x, int32_t y, bool isX )->void {
		int32_t		ptStart;
		bool		isStart	= false;
		uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
		int32_t		loop	= isX ? m_imgProc.width : m_imgProc.height;
		int32_t		dByPrc	= isX ? 1 : m_imgProc.pitch;
		int32_t		dByImg	= isX ? 1 : m_imgOrg.pitch / m_imgOrg.bytesPerPixel;
		for ( int32_t i = 0; i <= loop; ++i )
		{
			if ( IS_UNREPAIR_PIXEL(*pixPrc) && i < loop )
			{
				if ( !isStart )
				{
					isStart	= true;
					ptStart	= i;
				}
			}
			else
			{
				if ( isStart )
				{
					isStart	= false;
					SOrgPixel*	imgRev	= m_imgOrg.pixel( isX ? ptStart : x, isX ? y : ptStart );
					SOrgPixel*	imgRevE	= m_imgOrg.pixel( isX ? x - 1 : x, isX ? y : y - 1 );
					SOrgPixel	rgbaSta = m_edgePots[*( (int32_t*)imgRev )].avgColor;
					SOrgPixel	rgbaEnd = m_edgePots[*( (int32_t*)imgRevE )].avgColor;
					int32_t		stepLen	= i - ptStart;
					uint8_t*	prcRev	= m_imgProc.pixel( isX ? ptStart : x, isX ? y : ptStart );
					for ( int32_t d = 0; d < stepLen; ++d )
					{
						int32_t	ied	= *( (int32_t*)imgRev );
						m_edgePots[ied].avgColor	= SOrgPixel( ( rgbaSta.b * ( stepLen - d ) + rgbaEnd.b * d ) / stepLen
							, ( rgbaSta.g * ( stepLen - d ) + rgbaEnd.g * d ) / stepLen
							, ( rgbaSta.r * ( stepLen - d ) + rgbaEnd.r * d ) / stepLen
							, ( rgbaSta.a * ( stepLen - d ) + rgbaEnd.a * d ) / stepLen );
						if ( IS_UNREPAIR_PIXEL( prcRev[-1] ) )
						{
							int32_t	ied1	= *( (int32_t*)( imgRev - 1 ) );
							m_edgePots[ied1].xEndColor	= m_edgePots[ied].avgColor;
							m_edgePots[ied].xEndColor		= m_edgePots[ied].avgColor;
						}
						imgRev	+= dByImg;
						prcRev	+= dByPrc;
					}
				}
			}
			isX ? ++x : ++y;
			pixPrc	+= dByPrc;
		}
	};

	if ( m_inpRect.x() == 0 ) funEdgeLine( 0, 0, false );
	if ( m_inpRect.right() == m_imgProc.width - 1 ) funEdgeLine( m_imgProc.width - 1, 0, false );
	if ( m_inpRect.y() == 0 ) funEdgeLine( 0, 0, true );
	if ( m_inpRect.bottom() == m_imgProc.height - 1 ) funEdgeLine( 0, m_imgProc.height - 1, true );

#if 1
	for ( int32_t i = 0; i < m_edgeCount; ++i )
	{

		auto ptY	= m_edgePots + i;
		uint8_t*	prcRev	= m_imgProc.pixel( ptY->x(), ptY->y() );
		if ( IS_PATCHED_PIXEL( *prcRev ) ) continue;
		SOrgPixel*	imgRev	= m_imgOrg.pixel( ptY->x(), ptY->y() );
		if ( ptY->y() + ptY->yLength == m_imgProc.height )
		{
			int32_t	ied1	= *( (int32_t*)( imgRev + ( ptY->yLength - 1 ) * ( m_imgOrg.pitch / m_imgOrg.bytesPerPixel ) ) );
			ptY->yEndColor	= m_edgePots[ied1].avgColor;
		}
		for ( int32_t dy = 0; dy < ptY->yLength; ++dy )
		{
			if ( IS_PATCHED_PIXEL( *prcRev ) )
			{
				imgRev	= (SOrgPixel*)( ( (uint8_t*)imgRev ) + m_imgOrg.pitch );
				prcRev	+= m_imgProc.pitch;
				continue;
			}
			//assert( *(int32_t*)imgRev < m_edgeCount && *(int32_t*)imgRev >= 0 );
			auto		ptX		= m_edgePots + *( (int32_t*)imgRev );
			float	xStaT	= ptY->x() - ptX->x() + 1;
			float	xEndT	= ptX->xLength - xStaT + 1;
			float	yStaT	= dy + 1;
			float	yEndT	= ptY->yLength - dy;
	
			float	xSta	= ( xEndT * yStaT * yEndT );
			float	xEnd	= ( xStaT * yStaT * yEndT );
			float	ySta	= ( xStaT * xEndT * yEndT );
			float	yEnd	= ( xStaT * xEndT * yStaT );
			float	totLen	= xSta + xEnd + ySta + yEnd;

			//float	totLen	= xStaT + xEndT + yStaT + yEndT;

			//float	xSta	= totLen / xStaT;
			//float	xEnd	= totLen / xEndT;
			//float	ySta	= totLen / yStaT;
			//float	yEnd	= totLen / yEndT;

			//totLen	= xSta + xEnd + ySta + yEnd;

			imgRev->b	= ( ptX->avgColor.b * xSta + ptX->xEndColor.b * xEnd + ptY->avgColor.b * ySta + ptY->yEndColor.b * yEnd ) / totLen;
			imgRev->g	= ( ptX->avgColor.g * xSta + ptX->xEndColor.g * xEnd + ptY->avgColor.g * ySta + ptY->yEndColor.g * yEnd ) / totLen;
			imgRev->r	= ( ptX->avgColor.r * xSta + ptX->xEndColor.r * xEnd + ptY->avgColor.r * ySta + ptY->yEndColor.r * yEnd ) / totLen;
			imgRev->a	= ( ptX->avgColor.a * xSta + ptX->xEndColor.a * xEnd + ptY->avgColor.a * ySta + ptY->yEndColor.a * yEnd ) / totLen;

			imgRev	= (SOrgPixel*)( ( (uint8_t*)imgRev ) + m_imgOrg.pitch );
			*prcRev	|= PIXEL_PATCHED;
			prcRev	+= m_imgProc.pitch;
		}
	}
#else
	for ( int32_t i = 0; i < m_edgeCount; ++i )
	{
		auto ptY	= m_edgePots + i;
		uint8_t*	prcRev	= m_imgProc.pixel( ptY->x(), ptY->y() );
		if ( IS_PATCHED_PIXEL( *prcRev ) ) continue;
		SOrgPixel*	imgRev	= m_imgOrg.pixel( ptY->x(), ptY->y() );
		if ( ptY->y() + ptY->yLength == m_imgProc.height )
		{
			int32_t	ied1	= *( (int32_t*)( imgRev + ( ptY->yLength - 1 ) * ( m_imgOrg.pitch / m_imgOrg.bytesPerPixel ) ) );
			ptY->yEndColor	= m_edgePots[ied1].avgColor;
		}
		for ( int32_t dy = 0; dy < ptY->yLength; ++dy )
		{
			if ( IS_PATCHED_PIXEL( *prcRev ) )
			{
				imgRev	= (SOrgPixel*)( ( (uint8_t*)imgRev ) + m_imgOrg.pitch );
				prcRev	+= m_imgProc.pitch;
				continue;
			}
			auto		ptX		= m_edgePots + *( (int32_t*)imgRev );
			uint32_t	xSta	= ptY->x() - ptX->x() + 1;
			uint32_t	xEnd	= ptX->xLength - xSta + 1;
			uint32_t	ySta	= dy + 1;
			uint32_t	yEnd	= ptY->yLength - dy;

			uint32_t	totLen	= xSta + xEnd + ySta + yEnd;
			xSta	= totLen / xSta;
			xEnd	= totLen / xEnd;
			ySta	= totLen / ySta;
			yEnd	= totLen / yEnd;
			totLen	= xSta + xEnd + ySta + yEnd;

			imgRev->b	= ( ptX->avgColor.b * xSta + ptX->xEndColor.b * xEnd + ptY->avgColor.b * ySta + ptY->yEndColor.b * yEnd ) / totLen;
			imgRev->g	= ( ptX->avgColor.g * xSta + ptX->xEndColor.g * xEnd + ptY->avgColor.g * ySta + ptY->yEndColor.g * yEnd ) / totLen;
			imgRev->r	= ( ptX->avgColor.r * xSta + ptX->xEndColor.r * xEnd + ptY->avgColor.r * ySta + ptY->yEndColor.r * yEnd ) / totLen;
			imgRev->a	= ( ptX->avgColor.a * xSta + ptX->xEndColor.a * xEnd + ptY->avgColor.a * ySta + ptY->yEndColor.a * yEnd ) / totLen;
			imgRev	= (SOrgPixel*)( ( (uint8_t*)imgRev ) + m_imgOrg.pitch );
			*prcRev	|= PIXEL_PATCHED;
			prcRev	+= m_imgProc.pitch;
		}
	}
#endif
}

void CInpaint::fastInpaint2()
{
	sort( m_borderPots.begin(), m_borderPots.end(),
		[]( const SBorderPoint& p1, const SBorderPoint& p2 ) {
		return p1.sum > p2.sum; } );

	int32_t		edgePtMax	= m_borderPots.size() * 2 + BLOCK_RADIUS * 8;
	int32_t		edgePtCount	= 0;
	int32_t		edgePtIndex	= 0;
	GPoint*		edgePots	= (GPoint*)malloc( edgePtMax * sizeof( GPoint ) );

	for ( auto pt = m_borderPots.begin(); pt != m_borderPots.end(); ++pt )
	{
		int32_t		r = 0, g = 0, b = 0, a = 0, c = 0;
		for ( int32_t j = 0; j < 8; ++j )
		{
			GPoint		pt2	= *pt + poOctree8[j];
			uint8_t*	prcRev	= m_imgProc.pixel( pt2.x(), pt2.y() );
			if ( IS_MISSING_PIXEL( *prcRev ) && ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) == 0 )
			{
				edgePots[edgePtCount++].setPos( pt2.x(), pt2.y() );
				*prcRev	|= 1;
			}
			else if ( IS_STATIC_PIXEL( *prcRev ) || IS_PATCHED_PIXEL( *prcRev ) )
			{
				SOrgPixel*	pixImg	= m_imgOrg.pixel( pt2.x(), pt2.y() );
				b	+= pixImg->b;
				g	+= pixImg->g;
				r	+= pixImg->r;
				a	+= pixImg->a;
				++c;
			}
		}
		*m_imgProc.pixel( pt->x(), pt->y() )	|= PIXEL_PATCHED;
		SOrgPixel*	pixImg	= m_imgOrg.pixel( pt->x(), pt->y() );
		pixImg->b	= b / c;
		pixImg->g	= g / c;
		pixImg->r	= r / c;
		pixImg->a	= a / c;
	}

	while ( edgePtIndex < edgePtCount )
	{
		GPoint		pt	= edgePots[( edgePtIndex++ ) % edgePtMax];
		uint8_t*	prcRev	= m_imgProc.pixel( pt.x(), pt.y() );

		int32_t		r = 0, g = 0, b = 0, a = 0, c = 0;
		for ( int32_t i = 0; i < 8; ++i )
		{
			GPoint		pt2	= pt + poOctree8[i];
			uint8_t*	prcRev	= m_imgProc.pixel( pt2.x(), pt2.y() );
			if ( IS_MISSING_PIXEL( *prcRev ) && ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) == 0 )
			{
				edgePots[( edgePtCount++ ) % edgePtMax].setPos( pt2.x(), pt2.y() );
				assert( ( edgePtCount % edgePtMax ) != ( edgePtIndex % edgePtMax ) );
				*prcRev	|= 1;
			}
			else if ( IS_STATIC_PIXEL( *prcRev ) || IS_PATCHED_PIXEL( *prcRev ) )
			{
				SOrgPixel*	pixImg	= m_imgOrg.pixel( pt2.x(), pt2.y() );
				b	+= pixImg->b;
				g	+= pixImg->g;
				r	+= pixImg->r;
				a	+= pixImg->a;
				++c;
			}
		}
		*m_imgProc.pixel( pt.x(), pt.y() )	|= PIXEL_PATCHED;
		SOrgPixel*	pixImg	= m_imgOrg.pixel( pt.x(), pt.y() );
		pixImg->b	= b / c;
		pixImg->g	= g / c;
		pixImg->r	= r / c;
		pixImg->a	= a / c;
	}
	free( edgePots );
}

inline int32_t CInpaint::checkFirstEdge( int32_t x, int32_t y )
{
	uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0, s = 0;

	if ( ( *pixPrc & ( PIXEL_MISSING | PIXEL_BORDER ) ) == PIXEL_MISSING )
	{
		if ( m_edgeCount + 1 >= m_edgeAlloc )
		{
			m_edgeAlloc	+= 8192;
			m_edgePots	= (SEdgePoint*)realloc( m_edgePots, m_edgeAlloc * sizeof( SEdgePoint ) );
			if ( m_edgePots == nullptr )
			{
				m_edgeCount	= 0;
				m_edgeAlloc	= 0;
				return -1;
			}
		}
		for ( int32_t i = 0; i < 8; ++i )
		{
			uint8_t	pix	= pixPrc[poOctree8[i].x() + poOctree8[i].y() * m_imgProc.pitch];
			if ( pix == 0 )
			{
				SOrgPixel*	pixImg	= m_imgOrg.pixel( x + poOctree8[i].x(), y + poOctree8[i].y() );
				b	+= pixImg->b;
				g	+= pixImg->g;
				r	+= pixImg->r;
				a	+= pixImg->a;
				++c;
			}
		}
		if ( c )
		{
			m_edgePots[m_edgeCount++]	= SEdgePoint( x, y, c, SOrgPixel( uint8_t( b / c ), uint8_t( g / c ), uint8_t( r / c ), uint8_t( a / c ) ) );
		}
		else
		{
			m_edgePots[m_edgeCount++]	= SEdgePoint( x, y );
		}
		*pixPrc	|= PIXEL_BORDER;
		return m_edgeCount - 1;
	}
	return -1;
}
