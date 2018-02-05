#include "MaskDrawer.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))


CMaskDrawer::CMaskDrawer()
{
	m_curPencil	= nullptr;
	m_currOper	= 0;
	m_width		= 0;
	m_height	= 0;
}


CMaskDrawer::~CMaskDrawer()
{
}

bool CMaskDrawer::resetSize( int32_t width, int32_t height )
{
	if ( m_width != width || m_height != height )
	{
		if ( !m_maskImage_exp.create( nullptr, width + 2, height + 2 ) )
			return false;
		m_maskImage.create( m_maskImage_exp.pixel( 1, 1 ), width, height, m_maskImage_exp.pitch, true );
		SMaskPixel*	pixT	= m_maskImage_exp.pixel( 0, 0 );
		SMaskPixel*	pixB	= m_maskImage_exp.pixel( 0, height + 1 );
		for ( int32_t x = 0; x < m_maskImage_exp.width; ++x )
		{
			pixT->curDraw = pixB->curDraw = 1;
			++pixT;
			++pixB;
		}
		SMaskPixel*	pixL	= m_maskImage_exp.pixel( 0, 0 );
		SMaskPixel*	pixR	= m_maskImage_exp.pixel( width + 1, 0 );
		for ( int32_t y = 0; y < m_maskImage_exp.height; ++y )
		{
			pixL->curDraw = pixR->curDraw = 1;
			pixL	+= m_maskImage_exp.pitch;
			pixR	+= m_maskImage_exp.pitch;
		}
		m_width		= width;
		m_height	= height;
	}

	m_changedRect.clear();
	m_picChips.clear();
	m_currOper	= 0;
	m_curPencil	= nullptr;
	return true;
}

bool CMaskDrawer::fillRect( int32_t x, int32_t y, int32_t width, int32_t height )
{
	if ( !m_maskImage.isVaild() ) return false;
	GRect	rtFill( x, y, width, height );
	rtFill	= m_maskImage.rtImage.intersected( rtFill );
	if ( rtFill.isEmpty() ) return false;
	if ( m_curPencil ) pencilEnd();

	SChip*	chip	= new SChip;
	chip->isValid	= true;
	chip->eType		= eRect;
	chip->radius	= 1;
	chip->rect		= rtFill;
	if ( m_currOper < m_picChips.size() )
	{
		m_picChips.erase( m_picChips.begin() + m_currOper, m_picChips.end() );
	}
	m_picChips.push_back( chip );
	if ( !redo() )
	{
		m_picChips.pop_back();
		delete chip;
		return false;
	}
	return true;
}

bool CMaskDrawer::pencilBegin( int32_t radius )
{
	if ( !m_maskImage.isVaild() || radius < 0 ) return false;
	if ( m_curPencil ) pencilEnd();
	m_curPencil		= new SChip;
	m_curPencil->eType	= ePencil;
	m_curPencil->radius	= radius;
	return true;
}

bool CMaskDrawer::pencilPos( int32_t x, int32_t y )
{
	if ( nullptr == m_curPencil ) return false;
	if ( m_curPencil->pots.empty() )
	{

	}
	else
	{
		//drawLine( m_curPencil->pots.back(), GPoint( x, y ), m_curPencil->radius );
	}
	m_curPencil->pots.push_back( GPoint( x, y ) );
	return true;
}

bool CMaskDrawer::pencilEnd()
{
	if ( nullptr == m_curPencil ) return false;
	if ( m_curPencil->pots.size() )
	{
		m_picChips.push_back( m_curPencil );
	}
	else
	{
		delete m_curPencil;
	}
	m_curPencil	= nullptr;
	return true;
}

bool CMaskDrawer::pathClosed( const int32_t pots[], int32_t potCount )
{
	if ( !m_maskImage.isVaild() ) return false;
	if ( potCount < 3 || nullptr == pots ) return false;
	if ( m_curPencil ) pencilEnd();

	SChip*	chip	= new SChip;
	chip->eType		= ePath;
	chip->radius	= 0;
	int32_t		l = INT_MAX, t = INT_MAX, r = INT_MIN, b = INT_MIN;
	for ( int32_t i = 0; i < potCount; ++i )
	{
		GPoint	pot( pots[i * 2 + 0], pots[i * 2 + 1] );
		l	= min( pot.x(), l );
		t	= min( pot.y(), t );
		r	= max( pot.x(), r );
		b	= max( pot.y(), b );
		chip->pots.push_back( pot );
	}
	chip->rect.setRect( l, t, r - l + 1, b - t + 1 );
	if ( m_currOper < m_picChips.size() )
	{
		m_picChips.erase( m_picChips.begin() + m_currOper, m_picChips.end() );
	}
	m_picChips.push_back( chip );
	if ( !redo() )
	{
		m_picChips.pop_back();
		delete chip;
		return false;
	}
	return true;
}

int32_t CMaskDrawer::operNum( int32_t & current )
{
	current	= m_currOper;
	return int32_t( m_picChips.size() );
}

bool CMaskDrawer::redo()
{
	if ( m_currOper == m_picChips.size() ) return false;
	SChip*	chip	= m_picChips[m_currOper];

	if ( chip->eType == eRect )
	{
		if ( chip->rect.isNull() ) return false;
		for ( int32_t y = chip->rect.top(); y <= chip->rect.bottom(); ++y )
		{
			SMaskPixel*	pix	= m_maskImage.pixel( chip->rect.left(), y );
			for ( int32_t x = chip->rect.left(); x <= chip->rect.right(); ++x )
			{
				++( pix->drawCount );
				++pix;
			}
		}
		m_changedRect.push_back( chip->rect );
	}
	else if ( chip->eType == ePath )
	{
		if ( chip->rect.isNull() ) return false;
		fillPath( chip->pots );
		for ( int32_t y = chip->rect.top(); y <= chip->rect.bottom(); ++y )
		{
			SMaskPixel*	pix	= m_maskImage.pixel( chip->rect.left(), y );
			for ( int32_t x = chip->rect.left(); x <= chip->rect.right(); ++x )
			{
				if ( pix->curDraw )
				{
					++( pix->drawCount );
					pix->curDraw	= 0;
				}
				++pix;
			}
		}
		m_changedRect.push_back( chip->rect );
	}
	++m_currOper;
	return true;
}

bool CMaskDrawer::undo()
{
	if ( m_currOper == 0 ) return false;
	SChip*	chip	= m_picChips[m_currOper - 1];

	if ( chip->eType == eRect )
	{
		for ( int32_t y = chip->rect.top(); y <= chip->rect.bottom(); ++y )
		{
			SMaskPixel*	pix	= m_maskImage.pixel( chip->rect.left(), y );
			for ( int32_t x = chip->rect.left(); x <= chip->rect.right(); ++x )
			{
				--( pix->drawCount );
				++pix;
			}
		}
		m_changedRect.push_back( chip->rect );
	}
	--m_currOper;
	return true;
}

bool CMaskDrawer::getChangedRect( int32_t & x, int32_t & y, int32_t & width, int32_t & height )
{
	if ( m_changedRect.empty() )
		return false;
	GRect	rtChanged;
	for ( auto rt = m_changedRect.begin(); rt != m_changedRect.end(); ++rt )
	{
		rtChanged	|= *rt;
	}
	m_changedRect.clear();
	x	= rtChanged.x();
	y	= rtChanged.y();
	width	= rtChanged.width();
	height	= rtChanged.height();
	return true;
}
//
//void CMaskDrawer::drawLine( const GPoint& poStart, const GPoint& poEnd, float radius )
//{
//	auto	rightLine	= [&]( float stX, float stY, float enX, float enY )->void {
//		float	stepCount	= max( abs( enX - stX ), abs( enY - stY ) );
//		float	dx		= float( enX - stX ) / stepCount;
//		float	dy		= float( enY - stY ) / stepCount;
//		if ( stX < 0 || stY < 0 )
//		{
//			( 0 - stX ) / dx
//		}
//		for ( float i = 0; i <= stepCount; ++i )
//		{
//			int32_t		ix	= int( stX );
//			int32_t		iy	= int( stY );
//			if ( ix >= 0 && iy >= 0 && ix < m_maskImage.width && iy < m_maskImage.height )
//			{
//				SMaskPixel*	pix	= m_maskImage.pixel( ix, iy );
//				pix->curDraw	= 1;
//			}
//			stX	+= dx;
//			stY	+= dy;
//		}
//	};
//	auto	leftLine	= [&]( float stX, float stY, float enX, float enY )->void {
//		float	stepCount	= max( abs( enX - stX ), abs( enY - stY ) );
//		float	dx		= float( enX - stX ) / stepCount;
//		float	dy		= float( enY - stY ) / stepCount;
//		for ( float i = 1; i < stepCount; ++i )
//		{
//			stX	+= dx;
//			stY	+= dy;
//			SMaskPixel*	pix	= m_maskImage.pixel( stX, stY );
//			pix->curDraw	= 1;
//			while ( ( ++pix )->curDraw == 0 )
//			{
//				pix->curDraw	= 1;
//			}
//		}
//	};
//
//	if ( radius == 0.0f )
//	{
//		rightLine( poStart.x(), poStart.y(), poEnd.x(), poEnd.y() );
//		return;
//	}
//	float	angle		= atan2( poEnd.y() - poStart.y(), poEnd.x() - poStart.x() );
//	float	diameter	= radius * 2.0f + 1.0f;
//
//	QPointF	pts[4];
//	int32_t	maxRight	= INT_MIN;
//	int32_t	rightInd	= -1;
//
//	pts[0].setY( sin( angle + M_PI * 0.5 ) * radius + poStart.y() );
//	pts[0].setX( cos( angle + M_PI * 0.5 ) * radius + poStart.x() );
//	if ( maxRight < pts[0].x() ) { maxRight = pts[0].x(); rightInd = 0; }
//	pts[1].setY( sin( angle + M_PI * 0.5 ) * -radius + poStart.y() );
//	pts[1].setX( cos( angle + M_PI * 0.5 ) * -radius + poStart.x() );
//	if ( maxRight < pts[1].x() ) { maxRight = pts[1].x(); rightInd = 1; }
//
//	pts[2]	= pts[1] + ( poEnd - poStart );
//	if ( maxRight < pts[2].x() ) { maxRight = pts[2].x(); rightInd = 2; }
//	pts[3]	= pts[0] + ( poEnd - poStart );
//	if ( maxRight < pts[3].x() ) { maxRight = pts[3].x(); rightInd = 3; }
//
//	rightLine( pts[rightInd].x(), pts[rightInd].y(), pts[( rightInd + 4 - 1 ) % 4].x(), pts[( rightInd + 4 - 1 ) % 4].y() );
//	rightLine( pts[rightInd].x(), pts[rightInd].y(), pts[( rightInd + 1 ) % 4].x(), pts[( rightInd + 1 ) % 4].y() );
//
//	rightInd	= ( rightInd + 2 ) % 4;
//	leftLine( pts[rightInd].x(), pts[rightInd].y(), pts[( rightInd + 4 - 1 ) % 4].x(), pts[( rightInd + 4 - 1 ) % 4].y() );
//	leftLine( pts[rightInd].x(), pts[rightInd].y(), pts[( rightInd + 1 ) % 4].x(), pts[( rightInd + 1 ) % 4].y() );
//}
//
void CMaskDrawer::fillPath( const vector<GPoint>& pots )
{
	
	GPoint	poLast	= pots.back();
	for ( auto pt = pots.begin(); pt != pots.end(); ++pt )
	{
		int32_t	stepCount	= abs( pt->y() - poLast.y() );

		//		float	dx		= float( enX - stX ) / stepCount;
		//		float	dy		= float( enY - stY ) / stepCount;
		poLast	= *pt;
	}
}