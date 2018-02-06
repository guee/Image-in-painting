#include "Inpaint.h"
#include <algorithm>
#include "assert.h" 

#define	RAND_RANGED( min, max ) ( rand() * ( (max)-( min ) + 1 ) / ( RAND_MAX + 1 ) + ( min ) )

inline int32_t CInpaint::checkFirstEdge2( int32_t x, int32_t y )
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
			if ( IS_STATIC_PIXEL( pix ) || IS_PATCHED_PIXEL( pix ) )
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
			*pixPrc	|= PIXEL_BORDER;
			return m_edgeCount - 1;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

void CInpaint::structuralRepair( int32_t x, int32_t y )
{
	struct Segment
	{
		vector<GPoint>	line;
		uint32_t		totB;
		uint32_t		totG;
		uint32_t		totR;
		uint32_t		totA;
		SOrgPixel		avgColor;
		GRect			rect;
	};
	vector<Segment*> segments;
	auto	convexClosure	= [this]( vector<GPoint>& pots )->bool {
		if ( pots.size() < 3 ) return false;
		vector<GPoint> outPots;
		sort( pots.begin(), pots.end(),
			[]( const GPoint& p1, const GPoint& p2 ) {
			return ( p1.x() < p2.x() ) || ( p1.x() == p2.x() && p1.y() < p2.y() );
		});
		outPots.push_back( pots.front() );	//最左边，上方的点。
		int32_t		index	= 0;
		int32_t		count	= pots.size();
		while ( index < count - 1 && pots[index].x() == pots[index + 1].x() ) { ++index; }
		if ( index != 0 ) outPots.push_back( pots[index] );	//最左边，下方的点。
		while ( index < count - 1 )
		{
			double_t	maxDy	= -999999999999999999.0;
			int32_t		select	= 0;
			for ( int32_t j = index + 1; j < count; ++j )
			{
				if ( j < count - 1 && pots[j].x() == pots[j + 1].x() ) continue;
				double_t	dy	= double_t( pots[j].y() - pots[index].y() ) / double_t( pots[j].x() - pots[index].x() );
				if ( dy > maxDy )
				{
					maxDy	= dy;
					select	= j;
				}
			}
			index	= select;
			if ( outPots.size() >= 3 && pots[index].y() == outPots[outPots.size() - 1].y() && pots[index].y() == outPots[outPots.size() - 2].y() )
				outPots.back()	= pots[index];
			else
				outPots.push_back( pots[index] );

		}
		while ( index > 0 && pots[index].x() == pots[index - 1].x() ) { --index; }
		if ( index != count - 1 ) outPots.push_back( pots[index] );	//最右边，上方的点。

		while ( index > 0 )
		{
			double_t	maxDy	= -999999999999999999.0;
			int32_t		select	= 0;
			for ( int32_t j = index - 1; j >= 0; --j )
			{
				if ( j > 0 && pots[j].x() == pots[j - 1].x() ) continue;
				double_t	dy	= double_t( pots[j].y() - pots[index].y() ) / double_t( pots[j].x() - pots[index].x() );
				if ( dy > maxDy )
				{
					maxDy	= dy;
					select	= j;
				}
			}
			index	= select;
			if ( outPots.size() >= 3 && pots[index].y() == outPots[outPots.size() - 1].y() && pots[index].y() == outPots[outPots.size() - 2].y() )
				outPots.back()	= pots[index];
			else
				outPots.push_back( pots[index] );
		}
		if ( outPots.size() < 4 )
			return false;
		pots.clear();
		for ( auto pt = outPots.begin(); pt < outPots.end() - 1; ++pt )
		{
			auto	pt2 = pt + 1;
			int32_t	stepCount	= max( abs( pt2->x() - pt->x() ), abs( pt2->y() - pt->y() ) );
			float	dx	= float( pt2->x() - pt->x() ) / stepCount;
			float	dy	= float( pt2->y() - pt->y() ) / stepCount;
			float	x	= pt->x();
			float	y	= pt->y();
			SOrgPixel	c0 = *m_imgOrg.pixel( pt->x(), pt->y() );
			SOrgPixel	c1 = *m_imgOrg.pixel( pt2->x(), pt2->y() );
			for ( int32_t i = 0; i <= stepCount; ++i )
			{
				int32_t		ix = x, iy = y;
				uint8_t*	prc	= m_imgProc.pixel( ix, iy );
				if ( IS_STATIC_PIXEL( *prc ) && !IS_BORDER_PIXEL( *prc ) && ( ( *prc ) & PIXEL_BOREDR_RADIUS_MASK ) == BLOCK_RADIUS )
				{
					structuralRepair( ix, iy );
				}
				if ( IS_UNREPAIR_PIXEL( *prc ) )
				{
					SOrgPixel*	pix	= m_imgOrg.pixel( x, y );
					pix->b		= ( ( stepCount - i ) * c0.b + i * c1.b ) / stepCount;
					pix->g		= ( ( stepCount - i ) * c0.g + i * c1.g ) / stepCount;
					pix->r		= ( ( stepCount - i ) * c0.r + i * c1.r ) / stepCount;
					pix->a		= ( ( stepCount - i ) * c0.a + i * c1.a ) / stepCount;
					*prc	|= PIXEL_PATCHED | PIXEL_BORDER;
				}
				pots.push_back( GPoint( ix, iy ) );
				x += dx;
				y += dy;
			}
		}

		return true;
	};

	do
	{
		uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
		if ( !IS_STATIC_PIXEL( *pixPrc ) || IS_BORDER_PIXEL( *pixPrc ) ) break;
		SOrgPixel*	pixImg	= m_imgOrg.pixel( x, y );
		Segment*	segment	= new Segment;
		segment->avgColor	= *pixImg;
		segment->totB	= pixImg->b;
		segment->totG	= pixImg->g;
		segment->totR	= pixImg->r;
		segment->totA	= pixImg->a;
		segment->line.push_back( GPoint( x, y ) );
		*pixPrc	|= PIXEL_BORDER;
		uint32_t		pixCount	= 1;
		int32_t		top = INT_MAX, left = INT_MAX;
		int32_t		right = INT_MIN, bottom = INT_MIN;
		x = y = -1;
		for ( int32_t i = 0; i < pixCount; ++i )
		{
			GPoint		ptStart	= segment->line[i];
			SOrgPixel*	locImg	= m_imgOrg.pixel( ptStart.x(), ptStart.y() );
			for ( int32_t j = 0; j < 8; ++j )
			{
				GPoint		pt		= ptStart + poOctree8[j];
				uint8_t*	pixPrc	= m_imgProc.pixel( pt.x(), pt.y() );
				if ( IS_STATIC_PIXEL(*pixPrc) && !IS_BORDER_PIXEL( *pixPrc ) && (( *pixPrc) & PIXEL_BOREDR_RADIUS_MASK) == BLOCK_RADIUS )
				{
					SOrgPixel*	pixImg	= m_imgOrg.pixel( pt.x(), pt.y() );
					if ( ( pixImg->b - locImg->b ) * ( pixImg->b - locImg->b )
						+ ( pixImg->g - locImg->g ) * ( pixImg->g - locImg->g )
						+ ( pixImg->r - locImg->r ) * ( pixImg->r - locImg->r )
						+ ( pixImg->a - locImg->a ) * ( pixImg->a - locImg->a ) < 48 )
					//if ( ( pixImg->b - segment->avgColor.b ) * ( pixImg->b - segment->avgColor.b )
					//	+ ( pixImg->g - segment->avgColor.g ) * ( pixImg->g - segment->avgColor.g )
					//	+ ( pixImg->r - segment->avgColor.r ) * ( pixImg->r - segment->avgColor.r )
					//	+ ( pixImg->a - segment->avgColor.a ) * ( pixImg->a - segment->avgColor.a ) < 128 )
					{
						segment->totB		+= pixImg->b;
						segment->totG		+= pixImg->g;
						segment->totR		+= pixImg->r;
						segment->totA		+= pixImg->a;
						++pixCount;
						segment->avgColor.b	= segment->totB / pixCount;
						segment->avgColor.g	= segment->totG / pixCount;
						segment->avgColor.r	= segment->totR / pixCount;
						segment->avgColor.a	= segment->totA / pixCount;
						*pixPrc	|= PIXEL_BORDER;
						segment->line.push_back( pt );
						if ( pt.y() < top ) top = pt.y();
						else if ( pt.y() > bottom ) bottom = pt.y();
						if ( pt.x() < left ) left = pt.x();
						else if ( pt.x() > right ) right = pt.x();
					}
					else
					{
						x	= pt.x();
						y	= pt.y();
					}
				}
			}
		}
		if ( pixCount > 4 )
		{
			segment->rect.setRect( left, top, right - left + 1, bottom - top + 1 );
			segments.push_back( segment );
		}
		else
		{
			delete segment;
		}
	} while ( x > 0 && y > 0 );
	m_edgeCount	= 0;
	for ( int32_t i = 0; i < segments.size(); ++i )
	{
		Segment*	seg	= segments[i];
		if ( seg->rect.width() > 1 && seg->rect.height() > 1 )
		{
			if ( convexClosure( seg->line ) )
			{				
				auto	findEdgePotsX	= [this]( int32_t left, int32_t right, int32_t y )->void {
					int32_t		ptStart;
					bool		isStart	= false;
					uint8_t*	pixPrc	= m_imgProc.pixel( left, y );
					SOrgPixel*	pixImg	= m_imgOrg.pixel( left, y );
					for ( int32_t x = left; x <= right + 1; ++x )
					{
						if ( IS_UNREPAIR_PIXEL(*pixPrc) && x <= right )
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
								int32_t		stepLen	= x - ptStart;
								SOrgPixel*	imgRev	= pixImg - stepLen;
								int32_t	index0	= checkFirstEdge2( ptStart, y );
								int32_t	index1	= stepLen > 1 ? checkFirstEdge2( x - 1, y ) : index0;
								assert( index0 < m_edgeCount && index0 >= 0 );
								assert( index1 < m_edgeCount && index1 >= 0 );

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
				};
				auto	findEdgePotsY	= [this]( int32_t top, int32_t bottom, int32_t x )->void {
					int32_t		ptStart;
					bool		isStart	= false;
					uint8_t*	pixPrc	= m_imgProc.pixel( x, top );
					for ( int32_t y = top; y <= bottom + 1; ++y )
					{
						if ( IS_UNREPAIR_PIXEL( *pixPrc ) && y <= bottom )
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
								assert( xIndex < m_edgeCount && xIndex >= 0 );
								int32_t	yIndex0	= checkFirstEdge2( x, ptStart );
								int32_t	yIndex1	= checkFirstEdge2( x, y - 1 );
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
										int32_t	ptInd	= *( (int32_t*)m_imgOrg.pixel( x, y - 1 ) );
										assert( ptInd < m_edgeCount && ptInd >= 0 );
										m_edgePots[yIndex0].yEndColor	= m_edgePots[ptInd].avgColor;
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
										int32_t	ptInd	= *( (int32_t*)m_imgOrg.pixel( x, y - 1 ) );
										assert( ptInd < m_edgeCount && ptInd >= 0 );
										m_edgePots[xIndex].yEndColor	= m_edgePots[ptInd].avgColor;
									}
								}

							}
						}
						pixPrc += m_imgProc.pitch;
					}
				};

				sort( seg->line.begin(), seg->line.end(),
					[]( const GPoint& p1, const GPoint& p2 ) {
					return ( p1.y() < p2.y() ) || ( p1.y() == p2.y() && p1.x() < p2.x() );
				} );
				int32_t		index	= 0;
				int32_t		count	= seg->line.size();
				while ( index < count - 1 )
				{
					int32_t		x0 = seg->line[index].x();
					int32_t		y0 = seg->line[index].y();
					while ( index < count - 1 && seg->line[index].y() == seg->line[index + 1].y() ) { ++index; }
					int32_t		x1	= seg->line[index].x();
					int32_t		y1	= seg->line[index].y();
					findEdgePotsX( x0, seg->line[index].x(), seg->line[index].y() );
					++index;
				}
				sort( seg->line.begin(), seg->line.end(),
					[]( const GPoint& p1, const GPoint& p2 ) {
					return ( p1.x() < p2.x() ) || ( p1.x() == p2.x() && p1.y() < p2.y() );
				} );
				index	= 0;
				while ( index < count - 1 )
				{
					int32_t		y0 = seg->line[index].y();
					while ( index < count - 1 && seg->line[index].x() == seg->line[index + 1].x() ) { ++index; }
					findEdgePotsY( y0, seg->line[index].y(), seg->line[index].x() );
					++index;
				}
			}
			delete seg;
			segments.erase( segments.begin() + i );
			--i;
		}
		//for ( int32_t i = 0 ;i < m_edgeCount; ++i )
		//{
		//	uint8_t*	pixPrc	= m_imgProc.pixel( m_edgePots[i].x(), m_edgePots[i].y() );
		//	SOrgPixel*	pixImg	= m_imgOrg.pixel( m_edgePots[i].x(), m_edgePots[i].y() );
		//	//*pixImg	=m_edgePots[i].avgColor;
		//	if ( IS_PATCHED_PIXEL( *pixPrc ) )
		//	{
		//		pixImg->b = pixImg->g = 0;
		//		pixImg->r = pixImg->a = 255;

		//	}
		//}
	}
	if ( m_edgeCount )
	{
		fastInpaint();
	}
	for ( int32_t i = 0; i < segments.size(); ++i )
	{
		delete segments[i];
	}
	segments.clear();
}

int32_t CInpaint::findBorderPots()
{
	vector<GRect> rtUniteds;
	vector<GRect> rtAbsolutes;
	//清除之前的修补边界点列表
	m_borderPots.clear();
	m_borderPtFirsts.clear();
	//合并修补区域
	if ( !getUnited() ) return 0;
	//独立的，矩形的修补区域可以只检查矩形边框上的像素是否是修补边界。
	for ( auto rt = m_rtAbsolutes.begin(); rt != m_rtAbsolutes.end(); ++rt )
	{
		bool	borderFinded	= false;
		for ( int32_t y = rt->top(); y <= rt->bottom(); ++y )
		{
			uint8_t*	pixPrc	= m_imgProc.pixel( rt->left(), y );
			for ( int32_t x = rt->left(); x <= rt->right(); ++x )
			{
				*pixPrc++	= PIXEL_MISSING;		//把像素标记为缺失的。
			}
		}
		for ( int32_t y = rt->top(); y <= rt->bottom(); ++y )
		{
			checkFirstBorder( rt->left(), y );
			checkFirstBorder( rt->right(), y );
			if ( !borderFinded )
			{
				for ( int32_t i = 0; i < 8; ++i )
				{
					uint8_t*	prcRev	= m_imgProc.pixel( rt->left() + poOctree8[i].x(), y + poOctree8[i].y() );
					if ( *prcRev == 0 )
					{
						borderFinded	= true;
						m_borderPtFirsts.push_back( GPoint( rt->left() + poOctree8[i].x(), y + poOctree8[i].y() ) );
						break;
					}
				}
			}
		}
		for ( int32_t x = rt->left() + 1; x < rt->right(); ++x )
		{
			checkFirstBorder( x, rt->top() );
			checkFirstBorder( x, rt->bottom() );
		}
	}
	//其它形状的区域，以及多个区域合并成的大的区域，需要扫描每个像素点检查它是否是修补边界。
	vector<pair<GPoint,GPoint>>	lines;
	for ( auto rt = m_rtUniteds.begin(); rt != m_rtUniteds.end(); ++rt )
	{
		bool	borderFinded	= false;
		for ( int32_t y = rt->top(); y <= rt->bottom(); ++y )
		{
			int32_t		ptStart;
			bool		isStart	= false;
			uint8_t*	pixPrc	= m_imgProc.pixel( rt->left(), y );
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
							*prcRev		= PIXEL_MISSING;		//把像素标记为缺失的。
							++prcRev;
						}
						lines.push_back( pair<GPoint, GPoint>( GPoint( ptStart, y ), GPoint( x - 1, y ) ) );
						if ( !borderFinded )
						{
							for ( int32_t i = 0; i < 8; ++i )
							{
								uint8_t*	prcRev	= m_imgProc.pixel( ptStart + poOctree8[i].x(), y + poOctree8[i].y() );
								if ( *prcRev == 0 )
								{
									borderFinded	= true;
									m_borderPtFirsts.push_back( GPoint( ptStart + poOctree8[i].x(), y + poOctree8[i].y() ) );
									break;
								}
							}
						}
					}
				}
				++pixPrc;
			}
		}
	}

	for ( auto line = lines.begin(); line != lines.end(); ++line )
	{
		checkFirstBorder( line->first.x(), line->first.y() );
		checkFirstBorder( line->second.x(), line->second.y() );

		if ( IS_VALID_PIXEL( *m_imgProc.pixel( line->first.x(), line->first.y() - 1 ) )
			|| IS_VALID_PIXEL( *m_imgProc.pixel( line->second.x(), line->second.y() - 1 ) )
			|| IS_VALID_PIXEL( *m_imgProc.pixel( line->first.x(), line->first.y() + 1 ) )
			|| IS_VALID_PIXEL( *m_imgProc.pixel( line->second.x(), line->second.y() + 1 ) ) )
		{
			for ( int32_t x = line->first.x(); x <= line->second.x(); ++x )
			{
				checkFirstBorder( x, line->first.y() );
			}
		}
	}

	return m_borderPots.size();
}

inline bool CInpaint::checkFirstBorder( int32_t x, int32_t y, int32_t radius )
{
	uint8_t*	pixPrc	= m_imgProc.pixel( x, y );
	SOrgPixel	pixBak[8];
	int32_t		r = 0, g = 0, b = 0, a = 0, c = 0, s = 0;
	if ( ( *pixPrc & ( PIXEL_MISSING | PIXEL_BORDER ) ) == PIXEL_MISSING && 
		( *pixPrc & PIXEL_BOREDR_RADIUS_MASK ) != radius )
	{
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
			m_borderPots.push_back( SBorderPoint( x, y, s + 1 ) );
			return true;
		}
	}
	return false;
}

void CInpaint::expendBorder()
{
	int32_t		edgePtMax	= m_borderPots.size() * 2 + BLOCK_RADIUS * 8;
	int32_t		edgePtCount	= 0;
	int32_t		edgePtIndex	= 0;
	GPoint*		edgePots	= (GPoint*)malloc( edgePtMax * sizeof( GPoint ) );
	for ( auto pt = m_borderPots.begin(); pt != m_borderPots.end(); ++pt )
	{
		*m_imgProc.pixel( pt->x(), pt->y() )	&= ~PIXEL_BORDER;
		for ( int32_t j = 0; j < 8; ++j )
		{
			uint8_t*	prcRev	= m_imgProc.pixel( pt->x() + poOctree8[j].x(), pt->y() + poOctree8[j].y() );
			if ( IS_STATIC_PIXEL( *prcRev ) && ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) == 0 )
			{
				edgePots[edgePtCount++].setPos( pt->x() + poOctree8[j].x(), pt->y() + poOctree8[j].y() );
				*prcRev	|= BLOCK_RADIUS;
			}
		}
	}
	while ( edgePtIndex < edgePtCount )
	{
		GPoint		pt	= edgePots[( edgePtIndex++ ) % edgePtMax];
		uint8_t*	prcRev	= m_imgProc.pixel( pt.x(), pt.y() );
		uint8_t		radius	= ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) - 1;

		for ( int32_t i = 0; i < 8; ++i )
		{
			GPoint		pt2	= pt + poOctree8[i];
			uint8_t*	prcRev2	= m_imgProc.pixel( pt2.x(), pt2.y() );
			if ( IS_STATIC_PIXEL( *prcRev2 ) && ( *prcRev2 & PIXEL_BOREDR_RADIUS_MASK ) == 0 )
			{
				if ( radius > 1 )
				{
					edgePots[( edgePtCount++ ) % edgePtMax].setPos( pt2.x(), pt2.y() );
					assert( ( edgePtCount % edgePtMax ) != ( edgePtIndex % edgePtMax ) );
				}
				*prcRev2	|= radius;
			}
		}
	}
	free( edgePots );
	m_findRect	= GRect( BLOCK_RADIUS, BLOCK_RADIUS, m_imgProc.width - ( BLOCK_RADIUS * 2 + 1 ), m_imgProc.height - ( BLOCK_RADIUS * 2 + 1 ) )
		.intersect( m_inpRect.adjusted( -MAX_BLOCK_FIND_RADIUS, -MAX_BLOCK_FIND_RADIUS, MAX_BLOCK_FIND_RADIUS, MAX_BLOCK_FIND_RADIUS ) );
	m_inpRect	= GRect( 0, 0, m_imgProc.width, m_imgProc.height )
		.intersect( m_inpRect.adjusted( -BLOCK_RADIUS, -BLOCK_RADIUS, BLOCK_RADIUS, BLOCK_RADIUS ) );
}


void CInpaint::drawbackBorder()
{
	int32_t		radius		= BLOCK_RADIUS;
	GRect		rtVaild;
	vector<GPoint>*		blockEdges		= new vector<GPoint>;
	vector<GPoint>*		blockEdgesLast	= new vector<GPoint>;
	auto	blockDiff	= [&]( const GPoint& pt, int32_t x, int32_t y )->int32_t
	{
		int32_t	sumDiff	= 0, sumPix = 0;
		for ( int32_t dy = -radius; dy <= radius; ++dy )
		{
			uint8_t*	revPrc	= m_imgProc.pixel( pt.x() - radius, pt.y() + dy );
			SOrgPixel*	expImg	= m_imgOrg.pixel( x - radius, y + dy );
			SOrgPixel*	revImg	= m_imgOrg.pixel( pt.x() - radius, pt.y() + dy );
			for ( int32_t dx = -radius; dx <= radius; ++dx )
			{
				if ( IS_STATIC_PIXEL( *revPrc ) || IS_PATCHED_PIXEL( *revPrc ) || ( *revPrc & PIXEL_BOREDR_RADIUS_MASK ) >= radius )
				{
					int32_t	b	= ( expImg->b - revImg->b );
					int32_t	g	= ( expImg->g - revImg->g );
					int32_t	r	= ( expImg->r - revImg->r );
					int32_t	a	= ( expImg->a - revImg->a );
					sumDiff	+= ( b * b + g * g + r * r + a * a );
					++sumPix;
				}
				++revPrc;
				++expImg;
				++revImg;
			}
		}
		return	sumDiff / sumPix;
	};

	auto	findBestBlock	= [&]( const GPoint& pt, GPoint& ptMatch )->int32_t {
		int32_t	top		= max( pt.y() + ptMatch.y() - MAX_BLOCK_FIND_RADIUS, m_findRect.top() );
		int32_t	left	= max( pt.x() + ptMatch.x() - MAX_BLOCK_FIND_RADIUS, m_findRect.left() );
		int32_t	bottom	= min( pt.y() + ptMatch.y() + MAX_BLOCK_FIND_RADIUS, m_findRect.bottom() );
		int32_t	right	= min( pt.x() + ptMatch.x() + MAX_BLOCK_FIND_RADIUS, m_findRect.right() );

		//int32_t	top		= m_findRect.top();
		//int32_t	left	= m_findRect.left();
		//int32_t	bottom	= m_findRect.bottom();
		//int32_t	right	= m_findRect.right();

		int32_t	minDiff	= INT_MAX;
		for ( int32_t y = top; y <= bottom; ++y )
		{
			uint8_t*	pixProc		= m_imgProc.pixel( left, y );
			for ( int32_t x = left; x <= right; ++x )
			{
				if ( !IS_SEARCH_PIXEL( *pixProc ) )
				{
					++pixProc;
					continue;
				}
				int32_t	intDiff	= blockDiff( pt, x, y );
				if ( intDiff <= minDiff )
				{
					ptMatch.setPos( x, y );
					minDiff	= intDiff;
					if ( minDiff <= PIXEL_DIFF_MIN )
					{
						return minDiff;
					}
				}
				++pixProc;
			}
		}
		if ( minDiff > PIXEL_DIFF_MAX )
		{
			int32_t	randCount	= ( m_findRect.width() / BLOCK_RADIUS ) * ( m_findRect.height() / BLOCK_RADIUS );
			for ( int32_t i = 0; i < randCount; ++i )
			{
				int32_t		x	= RAND_RANGED( m_findRect.left(), m_findRect.right() );
				int32_t		y	= RAND_RANGED( m_findRect.top(), m_findRect.bottom() );
				uint8_t*	pixProc		= m_imgProc.pixel( x, y );
				if ( !IS_SEARCH_PIXEL( *pixProc ) ) continue;
				int32_t	intDiff	= blockDiff( pt, x, y );
				if ( intDiff <= minDiff )
				{
					ptMatch.setPos( x, y );
					minDiff	= intDiff;
					if ( minDiff <= PIXEL_DIFF_MIN )
					{
						return minDiff;
					}
				}
			}
		}
		return minDiff;
	};

	auto	fillBlock	= [&]( const GPoint& pt, const GPoint& ptMatch )->void {
		for ( int32_t dy = -radius; dy <= radius; ++dy )
		{
			//uint8_t*	expPrc	= m_imgProc.pixel( ptMatch.x() - radius, ptMatch.y() + dy );
			uint8_t*	revPrc	= m_imgProc.pixel( pt.x() - radius, pt.y() + dy );
			SOrgPixel*	expImg	= m_imgOrg.pixel( ptMatch.x() - radius, ptMatch.y() + dy );
			SOrgPixel*	revImg	= m_imgOrg.pixel( pt.x() - radius, pt.y() + dy );
			for ( int32_t dx = -radius; dx <= radius; ++dx )
			{
				if ( IS_MISSING_PIXEL( *revPrc ) && ( *revPrc & PIXEL_BOREDR_RADIUS_MASK ) != radius )
				{
					*(uint32_t*)revImg	= *(uint32_t*)expImg;
					*revPrc	= ( *revPrc & ~PIXEL_BOREDR_RADIUS_MASK ) | radius;
				}
				//++expPrc;
				++revPrc;
				++expImg;
				++revImg;
			}
		}	
		//#define		checkBlockEdge( x, y )	{ \
		//	uint8_t*	pixPrc	= m_imgProc.pixel( (x), (y) ); \
		//	if ( IS_MISSING_PIXEL( *pixPrc ) && ( *pixPrc & ( PIXEL_BOREDR_RADIUS_MASK ) != radius ) )\
		//		blockEdges->push_back( GPoint( x, y ) );	}

		//for ( int32_t i = -radius; i < radius; ++i )
		//{
		//	checkBlockEdge( pt.x() - ( radius + 1 ), pt.y() + i );
		//	checkBlockEdge( pt.x() + ( radius + 1 ), pt.y() + i );
		//	checkBlockEdge( pt.x() + i, pt.y() - ( radius + 1 ) );
		//	checkBlockEdge( pt.x() + i, pt.y() + ( radius + 1 ) );
		//}
	};
	auto	expandFillBlock	= [&]( vector<SBorderPoint>::iterator ptStart, int32_t diff )->void {
		for ( auto pt = ptStart; pt != m_borderPots.end(); ++pt )
		{
			GPoint	ptMatch		= *pt + ptStart->offset;
			if ( !rtVaild.contains( *pt ) ) continue;
			if ( !m_findRect.contains( ptMatch ) ) continue;
			uint8_t*	pixPrc		= m_imgProc.pixel( pt->x(), pt->y() );
			if ( !IS_MISSING_PIXEL( *pixPrc ) || ( *pixPrc & PIXEL_BOREDR_RADIUS_MASK ) == radius )
			{
				continue;
			}
			pixPrc		= m_imgProc.pixel( ptMatch.x(), ptMatch.y() );
			if ( !IS_SEARCH_PIXEL( *pixPrc ) )
			{
				continue;
			}
			int32_t	intDiff	= blockDiff( *pt, ptMatch.x(), ptMatch.y() );
			if ( intDiff < PIXEL_DIFF_MAX || intDiff < PIXEL_DIFF_MIN )
			{
				fillBlock( *pt, ptMatch );
				pt->offset	= ptStart->offset;
				pt->sum		= 0;
			}
		}
	};

	//for ( auto pt = m_borderPots.begin(); pt != m_borderPots.end(); ++pt )
	//{
	//	blockEdges->push_back( *pt );
	//}

	while ( radius >= 1 )
	{
		int32_t		blockInitSize	= m_borderPots.size();
		int32_t		blockInd		= 0;
		rtVaild	= GRect( radius, radius, m_imgProc.width - ( radius * 2 + 1 ), m_imgProc.height - ( radius * 2 + 1 ) );
		while ( blockInd < m_borderPots.size() )
		{
			sort( m_borderPots.begin() + blockInd, m_borderPots.end(),
				[]( const SBorderPoint& p1, const SBorderPoint& p2 ) {
				return p1.sum > p2.sum; } );
			int32_t	blockEnd	= m_borderPots.size();
			for ( auto pt = m_borderPots.begin() + blockInd; pt != m_borderPots.end(); ++pt )
			{
				if ( !rtVaild.contains( *pt ) ) continue;
				uint8_t*	prcRev	= m_imgProc.pixel( pt->x(), pt->y() );
				//*prcRev	&= ~PIXEL_BORDER;
				if ( !IS_MISSING_PIXEL( *prcRev ) || ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) == radius )
				{
					continue;
				}
				int32_t	diff	= findBestBlock( *pt, pt->offset );
				if ( diff < INT_MAX )
				{
					pt->sum		= 0;
					fillBlock( *pt, pt->offset );
					pt->offset	-= *pt;
					expandFillBlock( pt, diff );
				}
			}
			for ( int32_t i = blockInd; i < blockEnd; ++i )
			{
				SBorderPoint	pt	= m_borderPots[i];
				if ( pt.sum ) continue;
				for ( int32_t dy = -radius; dy < radius; ++dy )
				{
					if ( checkFirstBorder( pt.x() - ( radius + 1 ), pt.y() + dy, radius ) )
					{
						m_borderPots.back().offset	= pt.offset;
					}
					if ( checkFirstBorder( pt.x() + ( radius + 1 ), pt.y() + dy, radius ) )
					{
						m_borderPots.back().offset	= pt.offset;
					}
				}
				for ( int32_t dx = -radius; dx < radius; ++dx )
				{
					if ( checkFirstBorder( pt.x() + dx, pt.y() - ( radius + 1 ), radius ) )
					{
						m_borderPots.back().offset	= pt.offset;
					}
					if ( checkFirstBorder( pt.x() + dx, pt.y() + ( radius + 1 ), radius ) )
					{
						m_borderPots.back().offset	= pt.offset;
					}
				}
			}
			blockInd	= blockEnd;
		}
		m_borderPots.erase( m_borderPots.begin() + blockInitSize, m_borderPots.end() );
		radius	/= 2;
	}



	//for ( auto pt = m_borderPots.begin(); pt != m_borderPots.end(); ++pt )
	//{
	//	blockEdges->push_back( *pt );
	//	uint8_t*	revPrc	= m_imgProc.pixel( pt->x(), pt->y() );
	//	*revPrc	= ( *revPrc & ~PIXEL_BOREDR_RADIUS_MASK ) | ( BLOCK_RADIUS -1 );
	//}

	//blockInd	= 0;
	//radius	= 1;
	//for ( int32_t rad = BLOCK_RADIUS - 1; rad > 0; --rad )
	//{
	//	GPoint	ptOffset;
	//	while ( blockInd < blockEdges->size() )
	//	{
	//		GPoint	pt	= blockEdges->at( blockInd );
	//		for ( int32_t i = 0; i < 8; ++i )
	//		{
	//			uint8_t*	revPrc	= m_imgProc.pixel( pt.x() + poOctree8[i].x(), pt.y() + poOctree8[i].y() );
	//			if ( ( *revPrc & PIXEL_BOREDR_RADIUS_MASK ) > rad )
	//			{
	//				*revPrc	= ( *revPrc & ~PIXEL_BOREDR_RADIUS_MASK ) | rad;
	//				blockEdges->push_back( GPoint( pt.x() + poOctree8[i].x(), pt.y() + poOctree8[i].y() ) );
	//			}
	//		}
	//		int32_t	diff	= findBestBlock( pt, ptOffset );
	//		if ( diff < INT_MAX )
	//		{
	//			*m_imgOrg.pixel( pt.x(), pt.y() )	= *m_imgOrg.pixel( ptOffset.x(), ptOffset.y() );
	//		}

	//		++blockInd;
	//	}
	//	blockEdges->erase( blockEdges->begin() + blockInitSize, blockEdges->end() );
	//}

	//blockInitSize

	//auto	fixSmBlock	=[&]()->void
	//{
	//	auto	sw	= blockEdgesLast;
	//	blockEdgesLast	= blockEdges;
	//	blockEdges		= sw;
	//	blockEdges->clear();
	//	GPoint	ptOffset;
	//	for ( auto pt = blockEdgesLast->begin(); pt != blockEdgesLast->end(); ++pt )
	//	{
	//		uint8_t*	prcRev	= m_imgProc.pixel( pt->x(), pt->y() );
	//		if ( !IS_MISSING_PIXEL( *prcRev ) || ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) == radius )
	//		{
	//			continue;
	//		}
	//		int32_t	diff	= findBestBlock( *pt, ptOffset );
	//		if ( diff < INT_MAX )
	//		{
	//			fillBlock( *pt, ptOffset );
	//			ptOffset	-= *pt;
	//		}
	//		//SOrgPixel*	revImg	= m_imgOrg.pixel( pt->x(), pt->y() );
	//		//revImg->b = revImg->g = 0;
	//		//revImg->r = revImg->a = 255;
	//	}
	//};

	//radius	= 1;
	//fixSmBlock();
	//fixSmBlock();
	//fixSmBlock();
	//fixSmBlock();

	//while ( radius > 1 )
	//{
	//	radius	/= 2;
	//	auto	sw	= blockEdgesLast;
	//	blockEdgesLast	= blockEdges;
	//	blockEdges		= sw;
	//	blockEdges->clear();
	//	GPoint	ptOffset;
	//	for ( auto pt = blockEdgesLast->begin(); pt != blockEdgesLast->end(); ++pt )
	//	{
	//		uint8_t*	prcRev	= m_imgProc.pixel( pt->x(), pt->y() );
	//		if ( !IS_MISSING_PIXEL( *prcRev ) || ( *prcRev & PIXEL_BOREDR_RADIUS_MASK ) == radius )
	//		{
	//			continue;
	//		}
	//		int32_t	diff	= findBestBlock( *pt, ptOffset );
	//		if ( diff < INT_MAX )
	//		{
	//			fillBlock( *pt, ptOffset );
	//			ptOffset	-= *pt;
	//		}
	//		//SOrgPixel*	revImg	= m_imgOrg.pixel( pt->x(), pt->y() );
	//		//revImg->b = revImg->g = 0;
	//		//revImg->r = revImg->a = 255;
	//	}
	//}

	//while ( edgePtIndex < edgePtCount )
	//{
	//	GPoint		pt	= edgePots[( edgePtIndex++ ) % edgePtMax];
	//	uint8_t*	prcRev	= m_imgProc.pixel( pt.x(), pt.y() );
	//	for ( int32_t i = 0; i < 8; ++i )
	//	{
	//		GPoint		pt2	= pt + poOctree8[i];
	//		uint8_t*	prcRev2	= m_imgProc.pixel( pt2.x(), pt2.y() );
	//		if ( IS_MISSING_PIXEL( *prcRev2 ) && ( *prcRev2 & PIXEL_BOREDR_RADIUS_MASK ) == 0 )
	//		{
	//			edgePots[( edgePtCount++ ) % edgePtMax].setPos( pt2.x(), pt2.y() );
	//			assert( ( edgePtCount % edgePtMax ) != ( edgePtIndex % edgePtMax ) );
	//			*prcRev2	|= PIXEL_BOREDR_RADIUS_MASK;
	//		}
	//	}
	//	bestFillBlock( pt, ptOffset, false );
	//}
}
