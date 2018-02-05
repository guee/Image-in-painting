#include "Inpaint.h"



CInpaint::CInpaint()
{
	m_cbFunction	= nullptr;
	m_cbParam		= nullptr;
	m_borderPots	= nullptr;
	m_borderAlloc	= 0;
	m_borderCount	= 0;
}


CInpaint::~CInpaint()
{
	if ( m_borderPots ) free( m_borderPots );
}

bool CInpaint::setDamagedImage( uint8_t * imgBuf, int32_t width, int32_t height, int32_t pitch )
{
	if ( !resetSize( width, height ) )
		return false;
	if ( !m_imgOrg.create( (SOrgPixel*)imgBuf, width, height, pitch, true ) )
		return false;
	return true;
}

bool CInpaint::inpaint()
{
	if ( !m_imgOrg.isVaild() ) return false;
	if ( m_picChips.empty() ) return false;
	//m_cbFunction	= cbFunction;
	//m_cbParam		= cbParam;
	m_imgProc.create( ( uint8_t*)m_maskImage.pixel( 0, 0 ), m_maskImage.width, m_maskImage.height, m_maskImage.pitch, true );
	//m_edgePots.clear();
	m_borderCount	= 0;
	
	if ( 0 == findEdgePots() ) return false;
	fastInpaint();

	m_changedRect.clear();
	m_picChips.clear();
	m_currOper	= 0;
	m_curPencil	= nullptr;
	for ( int32_t y = m_inpRect.top(); y <= m_inpRect.bottom(); ++y )
	{
		memset( m_imgProc.pixel( m_inpRect.left(), y ), 0, m_inpRect.width() );
	}
	return true;
}

bool CInpaint::Inpaint()
{
	fastInpaint();
	return true;
	//vector<GPoint>	blkPots;
	//while( findEdgePots() )
	//{
	//	break;
	//	for ( auto pt = m_startPots.begin(); pt != m_startPots.end(); ++pt )
	//	{
	//		//uint8_t*	pixel	= m_imgOrg.pixel( pt->x(), pt->y() );
	//		//pixel[0] = pixel[1] = 0;  pixel[1] = 255;
	//		if ( pt->x() - MAX_EDGE_BLOCK_RADIUS < 0 || pt->x() + MAX_EDGE_BLOCK_RADIUS >= m_imgProc.width ||
	//			pt->y() - MAX_EDGE_BLOCK_RADIUS < 0 || pt->y() + MAX_EDGE_BLOCK_RADIUS >= m_imgProc.height )
	//		{
	//			continue;
	//		}
	//		uint8_t*	pixEdge	= m_imgProc.pixel( pt->x(), pt->y() );
	//		if ( IS_FIRST_PIXEL(*pixEdge) )
	//		{
	//			GPoint	ptOffset;
	//			if ( bestFillBlock( *pt, ptOffset, true ) )
	//			{
	//				blkPots.push_back( ptOffset );
	//			}
	//			//break;
	//		}
	//	}
	//	break;
	//}
	//m_edgePots.clear();
	//return true;
	////for (  )
	//for ( int32_t i = 0; i < m_edgePots.size(); ++i )
	//{
	//	GPoint	pt	= m_edgePots[i];
	//	auto sel	= blkPots.begin();
	//	int32_t		sum = 0, minSum = INT_MAX;
	//	for ( auto blk = blkPots.begin(); blk != blkPots.end(); ++blk )
	//	{
	//		GPoint	offset	= pt + *blk;
	//		uint8_t*	prcExp	= m_imgProc.pixel( offset.x(), offset.y() );
	//		if ( *prcExp == STATIC_PIXEL_MASK )
	//		{
	//			sum	= 0;
	//			for ( int32_t dy = -3; dy <= 3; ++dy )
	//			{
	//				uint8_t*	prcRev	= m_imgProc.pixel( pt.x() - 3, pt.y() + dy );
	//				uint8_t*	orgRev	= m_imgOrg.pixel( pt.x() - 3, pt.y() + dy );
	//				for ( int32_t dx = -3; dx <= 3; ++dx )
	//				{
	//					if ( IS_STATIC_PIXEL( *prcRev ) )
	//					{
	//						uint8_t*	orgExp	= m_imgOrg.pixelEx( offset.x() + dx, offset.y() + dy );
	//						sum	+= ( orgExp[0] - orgRev[0] ) * ( orgExp[0] - orgRev[0] )
	//							+ ( orgExp[1] - orgRev[1] ) * ( orgExp[1] - orgRev[1] )
	//							+ ( orgExp[2] - orgRev[2] ) * ( orgExp[2] - orgRev[2] );
	//					}
	//					++prcRev;
	//					orgRev	+= 4;
	//				}
	//			}
	//			if ( sum < minSum )
	//			{
	//				minSum	= sum;
	//				sel		= blk;
	//			}
	//		}

	//	}
	//	GPoint	offset	= pt + *sel;
	//	for ( int32_t dy = -2; dy <= 2; ++dy )
	//	{
	//		uint8_t*	prcRev	= m_imgProc.pixel( pt.x() - 2, pt.y() + dy );
	//		uint8_t*	orgRev	= m_imgOrg.pixel( pt.x() - 2, pt.y() + dy );
	//		for ( int32_t dx = -2; dx <= 2; ++dx )
	//		{
	//			if ( !IS_VALID_PIXEL( *prcRev ) )
	//			{
	//				uint8_t*	orgExp	= m_imgOrg.pixelEx( offset.x() + dx, offset.y() + dy );
	//				*(uint32_t*)orgRev	= *(uint32_t*)orgExp;
	//				*prcRev		= PATCHED_PIXEL_MASK;
	//				m_edgePots.push_back( GPoint( pt.x() + dx, pt.y() + dy ) );
	//			}
	//			++prcRev;
	//			orgRev	+= 4;
	//		}
	//	}
	//}


	return true;
}

CInpaint::SOrgPixel CInpaint::getAvgColor( int32_t x, int32_t y, int32_t radius )
{
	int32_t		stX	= max( x - radius, 0 );
	int32_t		stY	= max( y - radius, 0 );
	int32_t		enX	= min( x + radius + 1, m_imgProc.width );
	int32_t		enY	= min( y + radius + 1, m_imgProc.height );
	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0;
	for ( int32_t dy = stY; dy < enY; ++dy )
	{
		uint8_t*	prcRev	= m_imgProc.pixel( stX, dy );
		SOrgPixel*	imgRev	= m_imgOrg.pixel( stX, dy );
		for ( int32_t dx = stX; dx < enX; ++dx )
		{
			if ( IS_VALID_PIXEL(*prcRev) )
			{
				b += imgRev->b;
				g += imgRev->g;
				r += imgRev->r;
				a += imgRev->a;
				++c;
			}
			++prcRev;
			++imgRev;
		}
	}
	return SOrgPixel( uint8_t( b / c ), uint8_t( g / c ), uint8_t( r / c ), uint8_t( a / c ) );
}

void CInpaint::fastInpaint()
{
	if ( m_borderCount == 0 ) return;
	auto	funAngularPoint	= [this]( int32_t x, int32_t y )->void {
		uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
		if ( IS_MISSING_PIXEL( *pixPrc ) )
		{
			uint32_t	distanceMin	= UINT_MAX;
			SEdgePoint*	ptFind = nullptr;
			for ( int32_t i = 0; i < m_borderCount; ++i )
			{
				SEdgePoint*	pt	= m_borderPots + i;
				if ( pt->sum == 0 ) continue;
				uint32_t	distance	= ( x - pt->x() ) * ( x - pt->x() ) + ( y - pt->y() ) * ( y - pt->y() );
				if ( distance < distanceMin )
				{
					distanceMin	= distance;
					ptFind		= pt;
				}
			}
			SOrgPixel*	pixOrg	= m_imgOrg.pixel( x, y );
			m_borderPots[*( (int32_t*)pixOrg )].avgColor	= ptFind->avgColor;
			m_borderPots[*( (int32_t*)pixOrg )].sum		= 1;
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
					SOrgPixel	rgbaSta = m_borderPots[*( (int32_t*)imgRev )].avgColor;
					SOrgPixel	rgbaEnd = m_borderPots[*( (int32_t*)imgRevE )].avgColor;
					int32_t		stepLen	= i - ptStart;
					uint8_t*	prcRev	= m_imgProc.pixel( isX ? ptStart : x, isX ? y : ptStart );
					for ( int32_t d = 0; d < stepLen; ++d )
					{
						int32_t	ied	= *( (int32_t*)imgRev );
						m_borderPots[ied].avgColor	= SOrgPixel( ( rgbaSta.b * ( stepLen - d ) + rgbaEnd.b * d ) / stepLen
							, ( rgbaSta.g * ( stepLen - d ) + rgbaEnd.g * d ) / stepLen
							, ( rgbaSta.r * ( stepLen - d ) + rgbaEnd.r * d ) / stepLen
							, ( rgbaSta.a * ( stepLen - d ) + rgbaEnd.a * d ) / stepLen );
						m_borderPots[ied].sum		= 1;
						if ( IS_UNREPAIR_PIXEL( prcRev[-1] ) )
						{
							int32_t	ied1	= *( (int32_t*)( imgRev - 1 ) );
							m_borderPots[ied1].xEndColor	= m_borderPots[ied].avgColor;
							m_borderPots[ied].xEndColor		= m_borderPots[ied].avgColor;
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
	for ( int32_t i = 0; i < m_borderCount; ++i )
	{
		auto ptY	= m_borderPots + i;
		uint8_t*	prcRev	= m_imgProc.pixel( ptY->x(), ptY->y() );
		if ( IS_PATCHED_PIXEL( *prcRev ) ) continue;
		SOrgPixel*	imgRev	= m_imgOrg.pixel( ptY->x(), ptY->y() );
		if ( ptY->y() + ptY->yLength == m_imgProc.height )
		{
			int32_t	ied1	= *( (int32_t*)( imgRev + ( ptY->yLength - 1 ) * ( m_imgOrg.pitch / m_imgOrg.bytesPerPixel ) ) );
			ptY->yEndColor	= m_borderPots[ied1].avgColor;
		}
		for ( int32_t dy = 0; dy < ptY->yLength; ++dy )
		{
			if ( IS_PATCHED_PIXEL( *prcRev ) )
			{
				imgRev	= (SOrgPixel*)( ( (uint8_t*)imgRev ) + m_imgOrg.pitch );
				prcRev	+= m_imgProc.pitch;
				continue;
			}
			auto		ptX		= m_borderPots + *( (int32_t*)imgRev );
			float	xStaT	= ptY->x() - ptX->x() + 1;
			float	xEndT	= ptX->xLength - xStaT + 1;
			float	yStaT	= dy + 1;
			float	yEndT	= ptY->yLength - dy;

	
			float	xSta	= ( xEndT * yStaT * yEndT );
			float	xEnd	= ( xStaT * yStaT * yEndT );
			float	ySta	= ( xStaT * xEndT * yEndT );
			float	yEnd	= ( xStaT * xEndT * yStaT );

			float	totLen	= xSta + xEnd + ySta + yEnd;


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
	for ( int32_t i = 0; i < m_borderCount; ++i )
	{
		auto ptY	= m_borderPots + i;
		uint8_t*	prcRev	= m_imgProc.pixel( ptY->x(), ptY->y() );
		if ( IS_PATCHED_PIXEL( *prcRev ) ) continue;
		SOrgPixel*	imgRev	= m_imgOrg.pixel( ptY->x(), ptY->y() );
		if ( ptY->y() + ptY->yLength == m_imgProc.height )
		{
			int32_t	ied1	= *( (int32_t*)( imgRev + ( ptY->yLength - 1 ) * ( m_imgOrg.pitch / m_imgOrg.bytesPerPixel ) ) );
			ptY->yEndColor	= m_borderPots[ied1].avgColor;
		}
		int32_t		shift	= ( ( ptY->yLength / 2 ) * ( ptY->yLength / 2 ) + 2048 ) >> 11;
		for ( int32_t dy = 0; dy < ptY->yLength; ++dy )
		{
			if ( IS_PATCHED_PIXEL( *prcRev ) )
			{
				imgRev	= (SOrgPixel*)( ( (uint8_t*)imgRev ) + m_imgOrg.pitch );
				prcRev	+= m_imgProc.pitch;
				continue;
			}
			auto		ptX		= m_borderPots + *( (int32_t*)imgRev );
			uint32_t	shiftRight	= shift * ( ( ( ptX->xLength / 2 ) * ( ptX->xLength / 2 ) + 2048 ) >> 11 );
			uint32_t	xStaT	= ptY->x() - ptX->x() + 1;
			uint32_t	xEndT	= ptX->xLength - xStaT + 1;
			uint32_t	yStaT	= dy + 1;
			uint32_t	yEndT	= ptY->yLength - dy;

			uint32_t	xSta	= ( xEndT * yStaT * yEndT ) / shiftRight;
			uint32_t	xEnd	= ( xStaT * yStaT * yEndT ) / shiftRight;
			uint32_t	ySta	= ( xStaT * xEndT * yEndT ) / shiftRight;
			uint32_t	yEnd	= ( xStaT * xEndT * yStaT ) / shiftRight;
			//uint32_t	xSta	= ( xEndT * yStaT * yEndT );
			//uint32_t	xEnd	= ( xStaT * yStaT * yEndT );
			//uint32_t	ySta	= ( xStaT * xEndT * yEndT );
			//uint32_t	yEnd	= ( xStaT * xEndT * yStaT );

			uint32_t	totLen	= xSta + xEnd + ySta + yEnd;


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

	//const	GPoint	poOctree8[]	= { { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
	//vector<GPoint>	edgePots;
	//int32_t		edgePtMax	= m_edgePots.size() * 2;
	//int32_t		edgePtCount	= 0;
	//int32_t		edgePtIndex	= 0;
	//edgePots.resize( edgePtMax );
	//for ( int32_t i = 0; i < m_edgePots.size(); i += 8 )
	//{
	//	GPoint	pt	= m_edgePots[i];
	//	SPrcPixel*	prcRev	= m_imgProc.pixel( pt.x(), pt.y() );
	//	edgePots[edgePtCount++]	= pt;
	//	prcRev->isPatched	= 0;
	//}
	//while ( edgePtIndex < edgePtCount )
	//{
	//	GPoint	pt	= edgePots[edgePtIndex % edgePtMax];
	//	SPrcPixel*	prcRev	= m_imgProc.pixel( pt.x(), pt.y() );
	//	SOrgPixel*	imgRev	= m_imgOrg.pixel( pt.x(), pt.y() );

	//	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0;
	//	for ( int32_t i = 0; i < 8; ++i )
	//	{
	//		GPoint		pt2	= pt + poOctree8[i];
	//		SPrcPixel*	prcRev2	= m_imgProc.pixel( pt2.x(), pt2.y() );
	//		SOrgPixel*	imgRev2	= m_imgOrg.pixel( pt2.x(), pt2.y() );
	//		if ( prcRev2->isPatched )
	//		{
	//			edgePots[edgePtCount % edgePtMax]	= pt2;
	//			++edgePtCount;
	//			prcRev2->isPatched	= 0;
	//		}
	//		if ( ( prcRev2->isMissing == 0 && prcRev2->isInvalid == 0 ) || prcRev2->edgeDistance )
	//		{
	//			b	+= imgRev2->b * 3;
	//			g	+= imgRev2->g * 3;
	//			r	+= imgRev2->r * 3;
	//			a	+= imgRev2->a * 3;
	//			c += 3;
	//		}
	//		else
	//		{
	//			b	+= imgRev2->b;
	//			g	+= imgRev2->g;
	//			r	+= imgRev2->r;
	//			a	+= imgRev2->a;
	//			++c;
	//		}
	//	}
	//	imgRev->b	= b / c;
	//	imgRev->g	= g / c;
	//	imgRev->r	= r / c;
	//	imgRev->a	= a / c;
	//	prcRev->edgeDistance	= 1;
	//	++edgePtIndex;
	//}
	//int32_t		ptStart;
	//for ( int32_t y = 0; y < m_imgProc.height; ++y )
	//{
	//	uint8_t*	prcRev	= m_imgProc.pixel( 0, y );
	//	bool		isStart	= false;
	//	for ( int32_t x = 0; x < m_imgProc.width; ++x )
	//	{
	//		if ( IS_VALID_PIXEL( *prcRev ) )
	//		{
	//			if ( isStart )
	//			{
	//				isStart	= false;
	//				int16_t*	imgRev	= (int16_t*)m_imgOrg.pixel( ptStart, y );
	//				int32_t		stepLen	= x - ptStart;

	//				for ( int32_t dx = 0; dx < stepLen; ++dx )
	//				{
	//					imgRev[0]	= dx + 1;
	//					imgRev[1]	= stepLen - dx;
	//					imgRev	+= 2;
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if ( !isStart )
	//			{
	//				isStart	= true;
	//				ptStart	= x;
	//			}
	//		}
	//		++prcRev;
	//	}
	//}
	//uint8_t		rgbaStaX[4];
	//uint8_t		rgbaEndX[4];
	//uint8_t		rgbaStaY[4];
	//uint8_t		rgbaEndY[4];
	//for ( int32_t x = 0; x < m_imgProc.width; ++x )
	//{
	//	uint8_t*	prcRev	= m_imgProc.pixel( x, 0 );
	//	bool		isStart	= false;
	//	for ( int32_t y = 0; y < m_imgProc.height; ++y )
	//	{
	//		if ( IS_VALID_PIXEL( *prcRev ) )
	//		{
	//			if ( isStart )
	//			{
	//				isStart	= false;
	//				*( (uint32_t*)rgbaStaY ) = getAvgColor( x, ptStart - 1, 1 );			
	//				*( (uint32_t*)rgbaEndY ) = getAvgColor( x, y, 1 );

	//				int32_t	stepLen	= y - ptStart;
	//				uint8_t*	imgRev	= m_imgOrg.pixel( x, ptStart );
	//				for ( int32_t dy = 0; dy < stepLen; ++dy )
	//				{
	//					int32_t		xSta	= ( (int16_t*)imgRev )[0];
	//					int32_t		xEnd	= ( (int16_t*)imgRev )[1];

	//					*( (uint32_t*)rgbaStaX ) = getAvgColor( x - xSta, ptStart + dy, 1 );
	//					*( (uint32_t*)rgbaEndX ) = getAvgColor( x + xEnd, ptStart + dy, 1 );
	//					int32_t		ySta	= dy + 1;
	//					int32_t		yEnd	= stepLen - dy;

	//					int64_t		iTotLen	= xSta * xEnd * ySta * yEnd;

	//					xSta	= iTotLen / xSta;
	//					xEnd	= iTotLen / xEnd;
	//					ySta	= iTotLen / ySta;
	//					yEnd	= iTotLen / yEnd;

	//					iTotLen	= xSta + xEnd + ySta + yEnd;

	//					imgRev[0]	= ( rgbaStaX[0] * xSta + rgbaEndX[0] * xEnd + rgbaStaY[0] * ySta + rgbaEndY[0] * yEnd ) / iTotLen;
	//					imgRev[1]	= ( rgbaStaX[1] * xSta + rgbaEndX[1] * xEnd + rgbaStaY[1] * ySta + rgbaEndY[1] * yEnd ) / iTotLen;
	//					imgRev[2]	= ( rgbaStaX[2] * xSta + rgbaEndX[2] * xEnd + rgbaStaY[2] * ySta + rgbaEndY[2] * yEnd ) / iTotLen;
	//					imgRev[3]	= ( rgbaStaX[3] * xSta + rgbaEndX[3] * xEnd + rgbaStaY[3] * ySta + rgbaEndY[3] * yEnd ) / iTotLen;
	//					imgRev	+= m_imgOrg.pitch;
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if ( !isStart )
	//			{
	//				isStart	= true;
	//				ptStart	= y;
	//			}
	//		}
	//		prcRev += m_imgProc.pitch;
	//	}
	//}
}

int32_t CInpaint::findEdgePots()
{
	int32_t			chipCount	= m_currOper;
	GRect*			unitedRects	= nullptr;
	int32_t			rectCount	= 0;
	if ( chipCount == 0 ) return 0;
	if ( chipCount <= 50 )
	{
		unitedRects	= new GRect[chipCount];
		for ( int32_t i = 0; i < m_currOper; ++i )
		{
			auto ch	= m_picChips[i];
			if ( !ch->isValid ) continue;

			GRect	rtUnited	= ch->rect.adjusted( -1, -1, 1, 1 );
			bool	isAdjoin	= false;
			bool	isAdjoined	= false;
			do
			{
				isAdjoin	= false;
				for ( int32_t j = i + 1; j < m_currOper; ++j )
				{
					auto ch2	= m_picChips[j];
					if ( !ch2->isValid ) continue;
					if ( rtUnited.intersects( ch2->rect ) )
					{
						rtUnited	|= ch2->rect.adjusted( -1, -1, 1, 1 );
						ch2->isValid	= false;
						isAdjoined	= isAdjoin	= true;
						--chipCount;
					}
				}
			} while ( isAdjoin );

			if ( isAdjoined || ch->eType != eRect )
			{
				unitedRects[rectCount++]	= rtUnited.adjusted( 1, 1, -1, -1 );
			}
			else
			{
				int32_t	left		= ch->rect.left();
				int32_t	right		= ch->rect.right();
				int32_t	top			= ch->rect.top();
				int32_t	bottom		= ch->rect.bottom();
				int32_t	stepLen		= ch->rect.width();
				for ( int32_t y = top; y <= bottom; ++y )
				{
					uint8_t*	prcRev	= m_imgProc.pixel( left, y );
					for ( int32_t d = 0; d < stepLen; ++d )
					{
						*prcRev		= PIXEL_MISSING;
						++prcRev;
					}
					int32_t	index0	= checkFirstEdge_force( left, y );
					int32_t	index1	= checkFirstEdge_force( right, y );
					m_borderPots[index0].xLength	= stepLen;
					SOrgPixel*	imgRev	= m_imgOrg.pixel( left, y );
					for ( int32_t d = 0; d < stepLen - 1; ++d )
					{
						*( (int32_t*)imgRev )	= index0;
						++imgRev;
					}
					m_borderPots[index1].xLength	= 1;
					m_borderPots[index1].yLength	= 1;
					m_borderPots[index0].xEndColor	= m_borderPots[index1].avgColor;
					m_borderPots[index1].xEndColor	= m_borderPots[index1].avgColor;
					*( (int32_t*)imgRev )	= index1;
				}
				stepLen	= ch->rect.height();
				for ( int32_t x = left + 1; x < right; ++x )
				{
					int32_t	yIndex0	= checkFirstEdge_force( x, top );
					int32_t	yIndex1	= checkFirstEdge_force( x, bottom );
					if ( top == 0 || top == m_imgOrg.height - 1 )
					{
						*( (int32_t*)m_imgOrg.pixel( x, top ) )	= yIndex0;
					}
					m_borderPots[yIndex0].xLength	= 1;
					m_borderPots[yIndex0].yLength	= stepLen;
					m_borderPots[yIndex1].xLength	= 1;
					m_borderPots[yIndex1].yLength	= -stepLen;
					m_borderPots[yIndex0].yEndColor	= m_borderPots[yIndex1].avgColor;
					if ( bottom == m_imgOrg.height - 1 )
					{
						*( (int32_t*)m_imgOrg.pixel( x, bottom ) )	= yIndex1;
					}
				}

				int32_t	xIndex	= *( (int32_t*)m_imgOrg.pixel( left, top ) );
				m_borderPots[xIndex].yLength	= stepLen;
				m_borderPots[xIndex].yEndColor	= m_borderPots[*( (int32_t*)m_imgOrg.pixel( left, bottom ) )].avgColor;
				xIndex	= *( (int32_t*)m_imgOrg.pixel( right, top ) );
				m_borderPots[xIndex].yLength	= stepLen;
				m_borderPots[xIndex].yEndColor	= m_borderPots[*( (int32_t*)m_imgOrg.pixel( right, bottom ) )].avgColor;

				if ( m_inpRect.isValid() )
				{
					m_inpRect	|= ch->rect;
				}
				else
				{
					m_inpRect	= ch->rect;
				}
			}
			--chipCount;
			ch->isValid	= false;
		}
	}
	else
	{
		chipCount	= 0;
		unitedRects	= new GRect[1];
		//unitedRects.push_back( GRect( 0, 0, m_maskImage.width, m_maskImage.height ) );
		unitedRects[rectCount++].setRect( 0, 0, m_maskImage.width, m_maskImage.height );
	}

	for ( int32_t i = 0; i < rectCount; ++i )
	{
		GRect*	rt	= unitedRects + i;
		if ( m_inpRect.isValid() )
			m_inpRect	|= *rt;
		else
			m_inpRect	= *rt;

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
						int32_t	index0	= checkFirstEdge_force( ptStart, y );
						int32_t	index1	= checkFirstEdge_force( x - 1, y );
						m_borderPots[index0].xLength	= stepLen;
						for ( int32_t d = 0; d < stepLen - 1; ++d )
						{
							*( (int32_t*)imgRev )	= index0;
							++imgRev;
						}

						m_borderPots[index1].xLength	= 1;
						m_borderPots[index1].yLength	= 1;
						m_borderPots[index0].xEndColor	= m_borderPots[index1].avgColor;
						m_borderPots[index1].xEndColor	= m_borderPots[index1].avgColor;
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
			int32_t	bottom= rt->bottom();
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
						int32_t	xIndex	= *( (int32_t*)m_imgOrg.pixel( x, ptStart ));
						int32_t	yIndex0	= checkFirstEdge_force( x, ptStart );
						int32_t	yIndex1	= checkFirstEdge_force( x, y - 1 );
						if ( yIndex0 >= 0 )
						{
							if ( ptStart == 0 || ptStart == m_imgOrg.height - 1 )
							{
								*( (int32_t*)m_imgOrg.pixel( x, ptStart ) )	= yIndex0;
							}
							m_borderPots[yIndex0].xLength	= 1;
							m_borderPots[yIndex0].yLength	= stepLen;
							if ( yIndex1 >= 0 )
							{
								m_borderPots[yIndex1].xLength		= 1;
								m_borderPots[yIndex1].yLength		= -stepLen;
								m_borderPots[yIndex0].yEndColor	= m_borderPots[yIndex1].avgColor;
								if ( y == m_imgOrg.height )
								{
									*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )	= yIndex1;
								}
							}
							else
							{
								m_borderPots[yIndex0].yEndColor	= m_borderPots[*( ( int32_t* )m_imgOrg.pixel( x, y - 1 ))].avgColor;
							}
						}
						else
						{
							m_borderPots[xIndex].yLength	= stepLen;
							if ( yIndex1 >= 0 )
							{
								m_borderPots[xIndex].yEndColor	= m_borderPots[yIndex1].avgColor;
								if ( y == m_imgOrg.height )
								{
									*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )	= yIndex1;
								}
							}
							else
							{
								m_borderPots[xIndex].yEndColor	= m_borderPots[*( (int32_t*)m_imgOrg.pixel( x, y - 1 ) )].avgColor;
							}
						}

					}
				}
				pixPrc += m_imgProc.pitch;
			}
		}
	}
	//sort( m_edgePots.begin(), m_edgePots.end(),
	//	[]( const SEdgePoint& p1, const SEdgePoint& p2 ) {
	//	return p1.sum > p2.sum; } );
	if ( unitedRects ) delete[]unitedRects;
	return m_borderCount;
}

//
//int32_t	CInpaint::otsuSplit( vector<SEdgePoint>::iterator begin, vector<SEdgePoint>::iterator end )
//{
//	int64_t totSum	= 0;
//
//	for ( auto pt = begin; pt != end; ++pt )
//	{
//		totSum	+= pt->sum;
//	}
//	
//	int32_t		sumLeft		= 0;
//	int32_t		sumRight	= totSum;
//	int32_t		countRight	= end - begin;
//	int32_t		countLeft	= 0;
//	int32_t		splitCount	= 0;
//	double		variance	= 0;
//	double		maxVariance	= -1;
//	for ( auto pt = begin; pt != end; ++pt )
//	{
//		if ( pt->sum == 0 ) return countLeft;
//		++countLeft;
//		--countRight;
//		sumLeft		+= pt->sum;
//		sumRight	-= pt->sum;
//		variance	= ( double( sumLeft ) / countLeft ) - ( double( sumRight ) / countRight );
//		variance	= variance * variance * countLeft * countRight;
//		if ( variance > maxVariance )
//		{
//			maxVariance	= variance;
//			splitCount	= countLeft;
//		}
//	}
//	return splitCount;
//}
//
//inline int32_t CInpaint::blockDiffSum( const GPoint& poCenter, const GPoint& ptMatch, int32_t radius )
//{
//	uint8_t*	pixExpS	= m_imgProc.pixel( ptMatch.x() - radius, ptMatch.y() - radius );
//	uint8_t*	pixRevS	= m_imgProc.pixel( poCenter.x() - radius, poCenter.y() - radius );
//	uint8_t*	orgRevS	= m_imgOrg.pixel( poCenter.x() - radius, poCenter.y() - radius );
//	uint8_t*	orgExpS	= m_imgOrg.pixel( ptMatch.x() - radius, ptMatch.y() - radius );
//	int32_t	sumDiff	= 0, sumPix = 0, diameter = ( radius * 2 + 1 );
//	int32_t	Sobel	= 0;
//	for ( int32_t y = 0; y < diameter; ++y )
//	{
//		uint8_t*	pixExp	= pixExpS + y * m_imgProc.pitch;
//		uint8_t*	pixRev	= pixRevS + y * m_imgProc.pitch;
//		uint8_t*	orgRev	= orgRevS + y * m_imgOrg.pitch;
//		uint8_t*	orgExp	= orgExpS + y * m_imgOrg.pitch;
//		for ( int32_t x = 0; x < diameter; ++x )
//		{
//			if ( IS_VALID_PIXEL( *pixRev ) )
//			{
//				int32_t	b	= ( orgExp[0] - orgRev[0] );
//				int32_t	g	= ( orgExp[1] - orgRev[1] );
//				int32_t	r	= ( orgExp[2] - orgRev[2] );
//				int32_t	a	= ( orgExp[3] - orgRev[3] );
//				sumDiff	+= ( b * b + g * g + r * r + a * a );
//				++sumPix;
//			}
//			++pixExp;
//			++pixRev;
//			orgRev	+= 4;
//			orgExp	+= 4;
//		}
//	}
//	return sumDiff / sumPix;
//}
//
//bool CInpaint::bestFillBlock( const GPoint& ptCenter, GPoint& ptOffset, bool adjacent )
//{
//	GPoint	ptMatch;
//	int32_t	minDiff	= INT_MAX;
//	const	int32_t	radius	= MAX_EDGE_BLOCK_RADIUS;
//
//	for ( int32_t y = m_imgRect.top(); y < m_imgRect.bottom(); ++y )
//	{
//		uint8_t*	pixProc		= m_imgProc.pixel( m_imgRect.left(), y );
//		for ( int32_t x = m_imgRect.left(); x < m_imgRect.right(); ++x )
//		{
//			if ( *pixProc != STATIC_PIXEL_MASK )
//			{
//				++pixProc;
//				continue;
//			}
//			int32_t	intDiff	= blockDiffSum( ptCenter, GPoint( x, y ), radius );
//			if ( intDiff <= minDiff )
//			{
//				ptMatch.setX( x );
//				ptMatch.setY( y );
//				minDiff	= intDiff;
//				if ( minDiff <= PIXEL_DIFF_MIN )	break;
//			}
//			++pixProc;
//		}
//		if ( minDiff <= PIXEL_DIFF_MIN )	break;
//	}
//	if ( minDiff == INT_MAX )
//	{
//		return false;
//	}
//	ptOffset	= ptMatch - ptCenter;
//	qDebug() << "first (" << ptCenter.x() << "," << ptCenter.y() << ") from (" << ptMatch.x() << "," << ptMatch.y()
//		<< ") diff=" << minDiff;
//
//	fillBlock( ptCenter, ptMatch, radius );
//
////	vector<GPoint> pots;
////	pots.push_back( ptCenter );
////	expendFillBlock( ptOffset, PIXEL_DIFF_MAX, pots );
//	//int32_t	oldSize	= m_edgePots.size();
//	//for ( auto pt = pots.begin(); pt != pots.end(); ++pt )
//	//{
//	//	for ( int32_t i = -( radius + 1 ); i <= radius; ++i )
//	//	{
//	//		checkFirstEdge( pt->x() + i, pt->y() - radius - 1 );
//	//		checkFirstEdge( pt->x() + i + 1, pt->y() + radius + 1 );
//	//		checkFirstEdge( pt->x() - radius - 1, pt->y() + i + 1 );
//	//		checkFirstEdge( pt->x() + radius + 1, pt->y() + i );
//	//	}
//		//if ( adjacent && m_edgePots.size() > oldSize )
//		//{
//		//	adjacent	= false;
//		//	sort( m_edgePots.begin() + oldSize, m_edgePots.end(),
//		//		[]( const SEdgePoint& p1, const SEdgePoint& p2 ) {
//		//		return p1.sum > p2.sum; } );
//
//		//	for ( auto pt = m_edgePots.begin() + oldSize; pt != m_edgePots.end(); ++pt )
//		//	{
//		//		if ( pt->x() - radius < 0 || pt->x() + radius >= m_imgProc.width ||
//		//			pt->y() - radius < 0 || pt->y() + radius >= m_imgProc.height )
//		//		{
//		//			continue;
//		//		}
//		//		uint8_t*	pixEdge	= m_imgProc.pixel( pt->x(), pt->y() );
//		//		if ( IS_FIRST_PIXEL( *pixEdge ) )
//		//		{
//		//			GPoint	ptOffset;
//		//			if ( bestFillBlock( *pt, ptOffset, false ) )
//		//			{
//		//			}
//		//		}
//		//	}
//		//}
//	//}
//	return true;
//}
//
//inline void CInpaint::fillBlock( const GPoint& ptCenter, const GPoint& ptMatch, int32_t radius )
//{
//	int32_t	diameter	= radius * 2 + 1;
//	int32_t	cc		= 0;
//	for ( int32_t dy = 0; dy < diameter; ++dy )
//	{
//		uint8_t*	prcRev	= m_imgProc.pixel( ptCenter.x() - radius, ptCenter.y() - radius + dy );
//		uint8_t*	orgRev	= m_imgOrg.pixel( ptCenter.x() - radius, ptCenter.y() - radius + dy );
//		uint8_t*	prcExp	= m_imgProc.pixel( ptMatch.x() - radius, ptMatch.y() - radius + dy );
//		uint8_t*	orgExp	= m_imgOrg.pixel( ptMatch.x() - radius, ptMatch.y() - radius + dy );
//		for ( int32_t dx = 0; dx < diameter; ++dx )
//		{
//			if ( !IS_VALID_PIXEL( *prcRev ) )
//			{
//				*(uint32_t*)orgRev	= *(uint32_t*)orgExp;
//				*prcRev = PATCHED_PIXEL_MASK;
//				++cc;
//			}
//			++prcRev;
//			++prcExp;
//			orgRev	+= 4;
//			orgExp	+= 4;
//		}
//	}
//	if ( cc == 0 )
//	{
//		qDebug() << "Empty Block!";
//	}
//}
//
//
inline int32_t CInpaint::checkFirstEdge( int32_t x, int32_t y )
{
	const	GPoint	poOctree8[]	= { { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
	uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
	SOrgPixel	pixBak[8];
	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0, s = 0;
	if ( ( *pixPrc & ( PIXEL_MISSING | PIXEL_BORDER ) ) == PIXEL_MISSING )
	{
		if ( m_borderCount + 1 >= m_borderAlloc )
		{
			m_borderAlloc	+= 8192;
			m_borderPots	= (SEdgePoint*)realloc( m_borderPots, m_borderAlloc * sizeof( SEdgePoint ) );
			if ( m_borderPots == nullptr )
			{
				m_borderCount	= 0;
				m_borderAlloc	= 0;
				return -1;
			}

		}

		for ( int32_t i = 0; i < 8; ++i )
		{
			uint8_t	pix	= pixPrc[poOctree8[i].x() + poOctree8[i].y() * m_imgProc.pitch];
			if ( IS_VALID_PIXEL( pix ) )
			{
				SOrgPixel*	pixImg	= m_imgOrg.pixel( x + poOctree8[i].x(), y + poOctree8[i].y() );
				b	+= pixImg->b;
				g	+= pixImg->g;
				r	+= pixImg->r;
				a	+= pixImg->a;
				pixBak[c]	= *pixImg;
				++c;
			}
		}
		if ( c )
		{
			b	/= c;
			g	/= c;
			r	/= c;
			a	/= c;
			for ( int32_t i = 0; i < c; ++i )
			{
				s	+= ( b - pixBak[i].b ) * ( b - pixBak[i].b ) + ( g - pixBak[i].g ) * ( g - pixBak[i].g )
					+ ( r - pixBak[i].r ) * ( r - pixBak[i].r ) + ( a - pixBak[i].a ) * ( a - pixBak[i].a );
			}
			*pixPrc	|= PIXEL_BORDER;
			m_borderPots[m_borderCount++]	= SEdgePoint( x, y, s + 1, SOrgPixel( uint8_t( b ), uint8_t( g ), uint8_t( r ), uint8_t( a ) ) );
			return m_borderCount - 1;
		}
	}
	return -1;
}

inline int32_t CInpaint::checkFirstEdge_force( int32_t x, int32_t y )
{
	const	GPoint	poOctree8[]	= { { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
	uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
	SOrgPixel	pixBak[8];
	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0, s = 0;



	if ( ( *pixPrc & ( PIXEL_MISSING | PIXEL_BORDER ) ) == PIXEL_MISSING )
	{
		if ( m_borderCount + 1 >= m_borderAlloc )
		{
			m_borderAlloc	+= 8192;
			m_borderPots	= (SEdgePoint*)realloc( m_borderPots, m_borderAlloc * sizeof( SEdgePoint ) );
			if ( m_borderPots == nullptr )
			{
				m_borderCount	= 0;
				m_borderAlloc	= 0;
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
				pixBak[c]	= *pixImg;
				++c;
			}
		}
		if ( c )
		{
			b	/= c;
			g	/= c;
			r	/= c;
			a	/= c;
			for ( int32_t i = 0; i < c; ++i )
			{
				s	+= ( b - pixBak[i].b ) * ( b - pixBak[i].b ) + ( g - pixBak[i].g ) * ( g - pixBak[i].g )
					+ ( r - pixBak[i].r ) * ( r - pixBak[i].r ) + ( a - pixBak[i].a ) * ( a - pixBak[i].a );
			}
			m_borderPots[m_borderCount++]	= SEdgePoint( x, y, s + 1, SOrgPixel( uint8_t( b ), uint8_t( g ), uint8_t( r ), uint8_t( a ) ) );
		}
		else
		{
			m_borderPots[m_borderCount++]	= SEdgePoint( x, y );
		}
		*pixPrc	|= PIXEL_BORDER;
		return m_borderCount - 1;
	}
	return -1;
}

//void CInpaint::expendFillBlock( const GPoint& ptOffset, int32_t maxDiff, vector<GPoint>& pots )
//{
//	for ( auto poCenter = m_edgePots.begin(); poCenter != m_edgePots.end(); ++poCenter )
//	{
//		GPoint	ptMatch		= *poCenter + ptOffset;
//		if ( ptMatch.x() - MAX_EDGE_BLOCK_RADIUS < 0 || ptMatch.x() + MAX_EDGE_BLOCK_RADIUS >= m_imgOrg.width
//			|| ptMatch.y() - MAX_EDGE_BLOCK_RADIUS < 0 || ptMatch.y() + MAX_EDGE_BLOCK_RADIUS >= m_imgOrg.height )
//		{
//			continue;
//		}
//		uint8_t*	pixProc		= m_imgProc.pixel( ptMatch.x(), ptMatch.y() );
//		if ( *pixProc != STATIC_PIXEL_MASK
//			|| !IS_FIRST_PIXEL( *m_imgProc.pixel( poCenter->x(), poCenter->y() ) ) )
//		{
//			continue;
//		}
//
//		int32_t	intDiff	= blockDiffSum( *poCenter, ptMatch, MAX_EDGE_BLOCK_RADIUS );
//		if ( intDiff <= maxDiff )
//		{
//			pots.push_back( *poCenter );
//			qDebug() << "second (" << poCenter->x() << "," << poCenter->y() << ") from (" << ptMatch.x() << "," << ptMatch.y()
//				<< ") radius=" << MAX_EDGE_BLOCK_RADIUS << ", diff=" << intDiff;
//			fillBlock( *poCenter, ptMatch, MAX_EDGE_BLOCK_RADIUS );
//		}
//	}
//}