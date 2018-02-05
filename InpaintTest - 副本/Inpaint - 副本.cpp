#include "Inpaint.h"



CInpaint::CInpaint()
{
}


CInpaint::~CInpaint()
{
}

bool CInpaint::InpaintBGRA( uint8_t* imgBuf, int32_t width, int32_t height, int32_t pitch, uint32_t colorKey )
{
	if ( !m_imgOrg.create( 4, imgBuf, width, height, pitch, true ) )
		return false;
	if ( !m_imgExpend.create( 1, NULL, width + 2, height + 2, 0, false ) )
		return false;
	m_imgProc.create( 1, m_imgExpend.pixel( 1, 1 ), width, height, m_imgExpend.pitch, true );

	int32_t		left = INT_MAX, top = INT_MAX;
	int32_t		right = INT_MIN, bottom = INT_MIN;
	for ( int32_t y = 0; y < height; ++y )
	{
		uint8_t*	pixSour	= m_imgOrg.pixel( 0, y );
		uint8_t*	pixProc	= m_imgProc.pixel( 0, y );
		pixProc[-1]	= 0x4F;
		for ( int32_t x = 0; x < width; ++x )
		{
			if ( *( (uint32_t*)pixSour ) == colorKey )
			{
				*pixProc	= MAX_EDGE_BLOCK_RADIUS + 1;
				if ( x < left ) left = x;
				if ( x > right ) right = x;
				if ( y < top ) top = y;
				if ( y > bottom ) bottom = y;
			}
			++pixProc;
			pixSour	+= 4;
		}
		*pixProc	= 0x4F;
	}
	if ( left == INT_MAX || top == INT_MAX || right == INT_MIN || bottom == INT_MIN )
		return false;
	m_inpaintBbounds.setRect( left, top, right - left + 1, bottom - top + 1 );
	memset( m_imgExpend.pixel( 0, 0 ), 0x4F, m_imgExpend.pitch );
	memset( m_imgExpend.pixel( 0, height + 1 ), 0x4F, m_imgExpend.pitch );
	return Inpaint();
}

bool CInpaint::InpaintGray( uint8_t * imgBuf, int32_t width, int32_t height, int32_t pitch, uint8_t colorKey )
{
	if ( !m_imgOrg.create( 1, imgBuf, width, height, pitch, true ) )
		return false;
	if ( !m_imgExpend.create( 1, NULL, width + 2, height + 2, 0, false ) )
		return false;
	m_imgProc.create( 1, m_imgExpend.pixel( 1, 1 ), width, height, m_imgExpend.pitch, true );
	int32_t		left = INT_MAX, top = INT_MAX;
	int32_t		right = INT_MIN, bottom = INT_MIN;
	for ( int32_t y = 0; y < height; ++y )
	{
		uint8_t*	pixSour	= m_imgOrg.pixel( 0, y );
		uint8_t*	pixProc	= m_imgProc.pixel( 0, y );
		for ( int32_t x = 0; x < width; ++x )
		{
			if ( pixSour[0] == colorKey )
			{
				*pixProc	= MAX_EDGE_BLOCK_RADIUS;
				if ( x < left ) left = x;
				if ( x > right ) right = x;
				if ( y < top ) top = y;
				if ( y > bottom ) bottom = y;
			}
			++pixProc;
			pixSour	+= 1;
		}
	}
	m_inpaintBbounds.setRect( left, top, right - left, bottom - top );
	findEdgePots();
	return Inpaint();
}


bool CInpaint::Inpaint()
{
//	QRect	rtRevamap( poRevamp.x() - MAX_EDGE_BLOCK_RADIUS, poRevamp.y() - MAX_EDGE_BLOCK_RADIUS, blockDiameter, blockDiameter );
	//if ( findEdgePots() == 0 ) return true;
	//return true;
	vector<pair<QPoint, QPoint>>	blkPots;
	do
	{
		m_edgePots.clear();
		blkPots.clear();
		if ( findEdgePots() == 0 ) return true;
		for ( auto pt = m_edgePots.begin(); pt != m_edgePots.end(); ++pt )
		{
			if ( pt->x() - MAX_EDGE_BLOCK_RADIUS < 0 || pt->x() + MAX_EDGE_BLOCK_RADIUS >= m_imgProc.width ||
				pt->y() - MAX_EDGE_BLOCK_RADIUS < 0 || pt->y() + MAX_EDGE_BLOCK_RADIUS >= m_imgProc.height )
			{
				continue;
			}
			uint8_t*	pixEdge	= m_imgProc.pixel( pt->x(), pt->y() );
			if ( *pixEdge & 0x80 )
			{
				if ( findBestFillBlock( *pt, blkPots ) )
				{

				}
			}
			//break;
		}

		for ( auto pt = blkPots.begin(); pt != blkPots.end(); ++pt )
		{
			QPoint		ptR		= pt->first;
			QPoint		ptE		= pt->second;
			for ( int32_t y = 0; y < MAX_BLOCK_DIAMETER; ++y )
			{
				uint8_t*	pixEdge	= m_imgProc.pixel( ptR.x(), ptR.y() + y );
				uint8_t*	pixOrgR	= m_imgOrg.pixel( ptR.x(), ptR.y() + y );
				uint8_t*	pixOrgE	= m_imgOrg.pixel( ptE.x(), ptE.y() + y );
				for ( int32_t x = 0; x < MAX_BLOCK_DIAMETER; ++x )
				{
					int32_t	alpha	= ( *pixEdge & 0x0F );
					alpha	= alpha * 240 / ( ( MAX_EDGE_BLOCK_RADIUS + 1 ) );
					if ( alpha )
					{
						alpha	= 0;
						//if ( *pixEdge & 0x10 )
						//{
						//	pixOrgR[0]	= ( pixOrgR[-4] + pixOrgR[4] + pixOrgR[m_imgOrg.pitch] + pixOrgR[-m_imgOrg.pitch] + pixOrgE[0] ) / 5;
						//	pixOrgR[1]	= ( pixOrgR[-3] + pixOrgR[5] + pixOrgR[m_imgOrg.pitch + 1] + pixOrgR[-m_imgOrg.pitch + 1] + pixOrgE[1] ) / 5;
						//	pixOrgR[2]	= ( pixOrgR[-2] + pixOrgR[6] + pixOrgR[m_imgOrg.pitch + 2] + pixOrgR[-m_imgOrg.pitch + 2] + pixOrgE[2] ) / 5;
						//}
						//else
						{
							*pixEdge	|= 0x10;
							pixOrgR[0]	= ( pixOrgE[0] * ( 256 - alpha ) + pixOrgR[0] * alpha ) >> 8;
							pixOrgR[1]	= ( pixOrgE[1] * ( 256 - alpha ) + pixOrgR[1] * alpha ) >> 8;
							pixOrgR[2]	= ( pixOrgE[2] * ( 256 - alpha ) + pixOrgR[2] * alpha ) >> 8;
						}
						//alpha	= 0;
					}

					++pixEdge;
					pixOrgR	+= 4;
					pixOrgE	+= 4;

				}
			}
			//clearBlockEdge2( ptR );
		}


		for ( int32_t y = m_inpaintBbounds.top(); y <= m_inpaintBbounds.bottom(); ++y )
		{
			uint8_t*	pixel	= m_imgProc.pixel( m_inpaintBbounds.left(), y );
			uint8_t*	pixOrgR	= m_imgOrg.pixel( m_inpaintBbounds.left(), y );
			for ( int32_t x = m_inpaintBbounds.left(); x <= m_inpaintBbounds.right(); ++x )
			{
				if ( *pixel & 0x10 )
				{
					*pixel	= 0;
				}
				else if ( *pixel & 0x0F )
				{
					*pixel	= MAX_EDGE_BLOCK_RADIUS + 1;
					pixOrgR[0]= pixOrgR[2] = 0;
					pixOrgR[1]= 255;
				}
				++pixel;
				pixOrgR	+= 4;
			}
		}
		qDebug() << blkPots.size();
		//break;
	} while ( blkPots.size() );

	return true;
}

int32_t CInpaint::findEdgePots()
{
	const	QPoint	poOctree8[]	= { { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
	int32_t		fillPotCount	= 0;
	m_edgePots.clear();
	//寻找最边缘的像素
	for ( int32_t y = m_inpaintBbounds.top(); y <= m_inpaintBbounds.bottom(); ++y )
	{
		uint8_t*	pixel	= m_imgProc.pixel( m_inpaintBbounds.left(), y );
		for ( int32_t x = m_inpaintBbounds.left(); x <= m_inpaintBbounds.right(); ++x )
		{
			if ( *pixel == MAX_EDGE_BLOCK_RADIUS + 1 )
			{
				int32_t	count	= 0;
				for ( int32_t i = 0; i < 8; ++i )
				{
					uint8_t*	pixEdge	= m_imgProc.pixel( x + poOctree8[i].x(), y + poOctree8[i].y() );
					if ( ( *pixEdge & 0x0F ) == 0 )
					{
						*pixEdge	|= 0x20;
						++count;
					}
				}
				if ( count > 0 )
				{
					*pixel		|= ( 0x80 | 0x40 );
					m_edgePots.push_back( SEdgeBlock( x, y, count ) );
				}
			}
			++pixel;
		}
	}
	sort( m_edgePots.begin(), m_edgePots.end(), 
		[]( const SEdgeBlock& p1, const SEdgeBlock& p2 ) {
		return p1.sum > p2.sum; } );
	//快速填充修补区域
	int32_t	ptCount	= m_edgePots.size();
	for ( int32_t ind = 0; ind < m_edgePots.size(); ++ind )
	{
		QPoint		pt		= m_edgePots[ind];
		uint8_t*	pixel	= m_imgProc.pixel( pt.x(), pt.y() );
		int32_t		count	= 0;
		int32_t		r = 0, g = 0, b = 0;
		for ( int32_t i = 0; i < 8; ++i )
		{
			QPoint		ptDest( pt + poOctree8[i] );
			uint8_t*	pixEdge	= m_imgProc.pixel( ptDest.x(), ptDest.y() );
			if ( *pixEdge & 0x20 )
			{
				uint8_t*	pixOrg	= m_imgOrg.pixel( ptDest.x(), ptDest.y() );
				b	+= pixOrg[0];
				g	+= pixOrg[1];
				r	+= pixOrg[2];
				++count;
			}
			else if ( ( *pixEdge & 0x40 ) == 0 )
			{
				BYTE	edge	= *pixel & 0x0F;
				if ( edge > 1 )
				{
					*pixEdge	= ( edge - 1 ) | 0x40;
					m_edgePots.push_back( ptDest );
				}
			}
		}
		if ( count )
		{
			uint8_t*	pixOrg	= m_imgOrg.pixel( pt.x(), pt.y() );
			pixOrg[0]	= b / count;
			pixOrg[1]	= g / count;
			pixOrg[2]	= r / count;
			*pixel	|= 0x20;
		}
	}
	m_edgePots.erase( m_edgePots.begin() + ptCount, m_edgePots.end() );
	return m_edgePots.size();
}

int32_t	CInpaint::otsuSplit( vector<SEdgeBlock>& list, int32_t totSum )
{
	if ( totSum <= 0 )
	{
		totSum	= 0;
		for ( auto pt = list.begin(); pt != list.end(); ++pt )
		{
			totSum	+= pt->sum;
		}
	}
	int32_t		sumLeft		= 0;
	int32_t		sumRight	= totSum;
	int32_t		countRight	= list.size();
	int32_t		countLeft	= 0;
	int32_t		splitCount	= 0;
	double		variance	= 0;
	double		maxVariance	= -1;
	for ( auto pt = list.begin(); pt != list.end(); ++pt )
	{
		if ( pt->sum == 0 ) return countLeft;
		++countLeft;
		--countRight;
		sumLeft		+= pt->sum;
		sumRight	-= pt->sum;
		variance	= ( double( sumLeft ) / countLeft ) - ( double( sumRight ) / countRight );
		variance	= variance * variance * countLeft * countRight;
		if ( variance > maxVariance )
		{
			maxVariance	= variance;
			splitCount	= countLeft;
		}
	}
	return splitCount;
}

inline int32_t	CInpaint::blockSubSum( const QPoint& poRevamp, const QPoint& ptExplant )
{
	int32_t	intSub	= 0;
	for ( int32_t dy = 0; dy < MAX_BLOCK_DIAMETER; ++dy )
	{
		uint8_t*	pixExp	= m_imgProc.pixel( ptExplant.x(), ptExplant.y() + dy );
		uint8_t*	pixRev	= m_imgProc.pixel( poRevamp.x(), poRevamp.y() + dy );

		uint8_t*	orgExp	= m_imgOrg.pixel( ptExplant.x(), ptExplant.y() + dy );
		uint8_t*	orgRev	= m_imgOrg.pixel( poRevamp.x(), poRevamp.y() + dy );
		for ( int32_t dx = 0; dx < MAX_BLOCK_DIAMETER; ++dx )
		{
			if ( *pixExp & 0x0F ) return INT_MAX;
			if ( ( *pixRev & 0x0F ) == 0 )
			{
				//intSub	+= ( pixExp[2] - pixEdge[2] ) * ( pixExp[2] - pixEdge[2] );
				//intSub	+= ( pixExp[0] - pixEdge[0] ) * ( pixExp[0] - pixEdge[0] );
				//orgExp[0]	= 255;
				int32_t	b	= ( orgExp[0] - orgRev[0] );
				int32_t	g	= ( orgExp[1] - orgRev[1] );
				int32_t	r	= ( orgExp[2] - orgRev[2] );
				intSub	+= b * b + g * g + r * r;

			}
			++pixExp;
			++pixRev;

			orgExp	+= 4;
			orgRev	+= 4;

		}
	}
	return intSub;
}

inline void CInpaint::clearBlockEdge( const QPoint& poRevamp )
{
	for ( int32_t dy = 0; dy < MAX_BLOCK_DIAMETER; ++dy )
	{
		uint8_t*	pixEdge	= m_imgProc.pixel( poRevamp.x(), poRevamp.y() + dy );
		for ( int32_t dx = 0; dx < MAX_BLOCK_DIAMETER; ++dx )
		{
			*pixEdge++	&= 0x7F;
		}
	}
}
inline void CInpaint::clearBlockEdge2( const QPoint& poRevamp )
{
	for ( int32_t dy = 0; dy < MAX_BLOCK_DIAMETER; ++dy )
	{
		uint8_t*	pixEdge	= m_imgProc.pixel( poRevamp.x(), poRevamp.y() + dy );
		for ( int32_t dx = 0; dx < MAX_BLOCK_DIAMETER; ++dx )
		{
			*pixEdge++	= 0;
		}
	}
}

bool CInpaint::findBestFillBlock( const SEdgeBlock& poRevamp, vector<pair<QPoint, QPoint>>& pots )
{
	QPoint	ptOffsetLT( -MAX_EDGE_BLOCK_RADIUS, -MAX_EDGE_BLOCK_RADIUS );
	QPoint	ptOffsetLT_Outside( -MAX_EDGE_BLOCK_RADIUS - 1, -MAX_EDGE_BLOCK_RADIUS - 1 );
	QPoint	ptLeftTop	= poRevamp + ptOffsetLT;
	QPoint	ptExplant;
	QRect	rtFind;
	const int32_t	blockDiameter	= MAX_BLOCK_DIAMETER;
	rtFind.setX( max( 0, ptLeftTop.x() - MAX_BLOCK_FIND_RADIUS ) );
	rtFind.setY( max( 0, ptLeftTop.y() - MAX_BLOCK_FIND_RADIUS ) );
	rtFind.setRight( min( poRevamp.x() + MAX_BLOCK_FIND_RADIUS, m_imgOrg.width - blockDiameter ) );
	rtFind.setBottom( min( poRevamp.y() + MAX_BLOCK_FIND_RADIUS, m_imgOrg.height - blockDiameter ) );

	int32_t		minThreshold	= MAX_EDGE_BLOCK_RADIUS * MAX_EDGE_BLOCK_RADIUS * 48;
	int32_t		minSub	= INT_MAX;
	int32_t		blockOffset[4]	= { 0 };
	blockOffset[0]	= 0;
	blockOffset[1]	= ( blockDiameter - 1 );
	blockOffset[2]	= ( blockDiameter - 1 ) * m_imgProc.pitch;
	blockOffset[3]	= blockOffset[2] + blockOffset[1];
	for ( int32_t y = rtFind.top(); y <= rtFind.bottom(); ++y )
	{
		uint8_t*	pixProc		= m_imgProc.pixel( rtFind.left(), y );
		for ( int32_t x = rtFind.left(); x <= rtFind.right(); ++x )
		{
			if ( pixProc[blockOffset[0]] || pixProc[blockOffset[1]] ||
				pixProc[blockOffset[2]] || pixProc[blockOffset[3]] )
			{
				++pixProc;
				continue;
			}
			int32_t	intSub	= blockSubSum( ptLeftTop, QPoint( x, y ) );
			if ( minSub >= intSub )
			{
				ptExplant.setX( x );
				ptExplant.setY( y );
				minSub	= intSub;
			}
			++pixProc;
		}
		if ( minSub < minThreshold ) break;
	}

	if ( minSub == INT_MAX ) return false;
	clearBlockEdge( ptLeftTop );
	////寻找图块边缘上邻接的其它边缘点,对图块的矩形进行扩展。
	//QPoint	ptOffsetEx	= ptExplant - ptLeftTop;
	pots.push_back( pair<QPoint, QPoint>( ptLeftTop, ptExplant ) );
	//for ( int32_t ind = pots.size() - 1; ind < pots.size(); ++ind )
	//{
	//	QPoint		pt		= pots[ind].first - ptOffsetLT;
	//	QPoint		ptLT	= pt + ptOffsetLT_Outside;
	//	uint8_t*	pixEdge	= m_imgProc.pixel( ptLT.x(), ptLT.y() );
	//	for ( int32_t x = 0; x <= MAX_BLOCK_DIAMETER; ++x )
	//	{
	//		if ( *pixEdge & 0x80 )
	//		{
	//			ptLT.setX( ptLT.x() + x );
	//			ptLT	+= ptOffsetLT;
	//			int32_t	intSub	= blockSubSum( ptLT, ptLT + ptOffsetEx );
	//			
	//			if ( intSub < minThreshold || intSub < minSub * 4 / 3 )
	//			{
	//				clearBlockEdge( ptLT );
	//				pots.push_back( pair<QPoint, QPoint>( ptLT, ptLT + ptOffsetEx ) );
	//			}
	//			break;
	//		}
	//		++pixEdge;
	//	}
	//	ptLT	= pt + ptOffsetLT_Outside;
	//	pixEdge	= m_imgProc.pixel( ptLT.x(), ptLT.y() + 1 );
	//	for ( int32_t y = 1; y <= MAX_BLOCK_DIAMETER + 1; ++y )
	//	{
	//		if ( *pixEdge & 0x80 )
	//		{
	//			ptLT.setY( ptLT.y() + y );
	//			ptLT	+= ptOffsetLT;
	//			int32_t	intSub	= blockSubSum( ptLT, ptLT + ptOffsetEx );
	//			if ( intSub < minThreshold || intSub < minSub * 4 / 3 )
	//			{
	//				clearBlockEdge( ptLT );
	//				pots.push_back( pair<QPoint, QPoint>( ptLT, ptLT + ptOffsetEx ) );
	//			}
	//			break;
	//		}
	//		pixEdge	+= m_imgProc.pitch;
	//	}

	//	ptLT	= pt - ptOffsetLT_Outside;
	//	pixEdge	= m_imgProc.pixel( ptLT.x(), ptLT.y() );
	//	for ( int32_t x = 0; x <= MAX_BLOCK_DIAMETER; ++x )
	//	{
	//		if ( *pixEdge & 0x80 )
	//		{
	//			ptLT.setX( ptLT.x() - x );
	//			ptLT	+= ptOffsetLT;
	//			int32_t	intSub	= blockSubSum( ptLT, ptLT + ptOffsetEx );
	//			if ( intSub < minThreshold || intSub < minSub * 4 / 3 )
	//			{
	//				clearBlockEdge( ptLT );
	//				pots.push_back( pair<QPoint, QPoint>( ptLT, ptLT + ptOffsetEx ) );
	//			}
	//			break;
	//		}
	//		--pixEdge;
	//	}
	//	ptLT	= pt - ptOffsetLT_Outside;
	//	pixEdge	= m_imgProc.pixel( ptLT.x(), ptLT.y() - 1 );
	//	for ( int32_t y = 1; y <= blockDiameter + 1; ++y )
	//	{
	//		if ( *pixEdge & 0x80 )
	//		{
	//			ptLT.setY( ptLT.y() - y );
	//			ptLT	+= ptOffsetLT;
	//			int32_t	intSub	= blockSubSum( ptLT, ptLT + ptOffsetEx );
	//			if ( intSub < minThreshold || intSub < minSub * 4 / 3 )
	//			{
	//				clearBlockEdge( ptLT );
	//				pots.push_back( pair<QPoint, QPoint>( ptLT, ptLT + ptOffsetEx ) );
	//			}
	//			break;
	//		}
	//		pixEdge	-= m_imgProc.pitch;
	//	}
	//}
	return true;
}
//
//inline void CInpaint::fillBlockEdge( const QPoint& poRevamp, const QPoint& poOffset )
//{
//
//}