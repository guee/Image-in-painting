#include "CImageDisplay.h"
#pragma comment ( lib, "winmm.lib")

CImageDisplay::CImageDisplay(QWidget *parent)
	: QWidget(parent)
{
	m_pMask		= nullptr;
	m_pImage	= nullptr;
	m_maskOperSucc	= false;
	m_pathClosed	= true;
	m_fScale	= 1.0f;
	m_bFitWnd	= true;
	m_bMouse_Press	= false;
	m_eOperType	= eOperNormal;
	m_penRadius	= 10;
	m_operPotRaidus	= 5;
	m_operPotInd	= -1;
	ui.setupUi(this);

	m_penLine.setStyle( Qt::SolidLine );
	m_penLine.setWidth( 1 );
	m_penLine.setColor( QColor( 31, 146, 255 ) );
	m_bruPot.setStyle( Qt::SolidPattern );
	m_bruPot.setColor( QColor( 255, 255, 255 ) );

	this->setMouseTracking( true );
}

CImageDisplay::~CImageDisplay()
{
	if ( m_pMask ) delete m_pMask;
}

float CImageDisplay::setFitWnd( bool bFitWnd )
{
	if ( bFitWnd )
	{
		calcScale( -1.0f, size() );
		if ( !m_bFitWnd ) update();
	}
	m_bFitWnd	= bFitWnd;
	return m_fScale;
}

void CImageDisplay::calcScale( float fScale, QSize siView, QPoint poCenter )
{
	if ( m_pImage && !m_pImage->isNull() )
	{
		if ( fScale < 0 )
		{
			QSizeF	siImg	= m_pImage->size();
			siImg.scale( siView, Qt::KeepAspectRatio );
			m_fScale	= siImg.width() * 1.0f / m_pImage->width();
			m_poLeftTop.setX( ( siImg.width() - siView.width() ) * 0.5f );
			m_poLeftTop.setY( ( siImg.height() - siView.height() ) * 0.5f );
		}
		else
		{
			QPointF	poCet	= poCenter;
			QSizeF	siOld( m_pImage->width() * m_fScale, m_pImage->height() * m_fScale );
			QRectF	rtDraw( -m_poLeftTop, siOld );
			rtDraw	= rtDraw.intersected( rect() );

			if ( !rtDraw.contains( poCenter ) )
			{
				poCet.setX( siView.width() * 0.5f );
				poCet.setY( siView.height() * 0.5f );
			}
			poCet.setX( ( poCet.x() + m_poLeftTop.x() ) / siOld.width() );
			poCet.setY( ( poCet.y() + m_poLeftTop.y() ) / siOld.height() );
			m_fScale	= fScale;
			QSizeF	siNew( m_pImage->width() * m_fScale, m_pImage->height() * m_fScale );
			m_poLeftTop.setX( m_poLeftTop.x() - ( siOld.width() - siNew.width() ) * poCet.x() );
			m_poLeftTop.setY( m_poLeftTop.y() - ( siOld.height() - siNew.height() ) * poCet.y() );
			m_poLeftTop	= limitView( m_poLeftTop, siView );
		}
	}

}

QPointF CImageDisplay::limitView( QPointF posTopLeft, QSize siView )
{
	QSizeF	siNew( m_pImage->width() * m_fScale, m_pImage->height() * m_fScale );
	if ( siNew.width() < siView.width() )
		posTopLeft.setX( ( siNew.width() - siView.width() ) * 0.5f );
	else if ( posTopLeft.x() < 0 )
		posTopLeft.setX( 0 );
	else if ( siNew.width() - posTopLeft.x() < siView.width() )
		posTopLeft.setX( siNew.width() - siView.width() );

	if ( siNew.height() < siView.height() )
		posTopLeft.setY( ( siNew.height() - siView.height() ) * 0.5f );
	else if ( posTopLeft.y() < 0 )
		posTopLeft.setY( 0 );
	else if ( siNew.height() - posTopLeft.y() < siView.height() )
		posTopLeft.setY( siNew.height() - siView.height() );

	return posTopLeft;
}

void CImageDisplay::setScale( float fScale, QPoint poCenter )
{
	if ( fScale >= 0.0f )
	{
		if ( fScale < 0.02f )
			fScale	= 0.02f;
		else if ( fScale > 20.0f )
			fScale	= 20.0f;
		if ( fScale != m_fScale )
		{
			calcScale( fScale, size(), poCenter );
		}
		m_bFitWnd	= false;
		update();
	}
	else
	{
		setFitWnd( true );
	}
}

void CImageDisplay::setOperType( EOperType eType )
{
	if ( eType != m_eOperType )
	{
		m_eOperType	= eType;
		m_operPots.clear();
		m_operPotInd	= -1;
		m_maskOperSucc	= false;
		m_pathClosed	= true;
		update();
	}
}

void CImageDisplay::setPenSize( int32_t radius )
{
	m_penRadius	= radius;
}

void CImageDisplay::paintEvent( QPaintEvent * event )
{
	if ( m_pImage && !m_pImage->isNull() )
	{
		QSizeF	siDraw( m_pImage->width() * m_fScale, m_pImage->height() * m_fScale );
		QRectF	rtDraw( -m_poLeftTop, siDraw );
		rtDraw	= rtDraw.intersected( rect() );
		QRectF	rtSour( ( m_poLeftTop.x() + rtDraw.x() ) / m_fScale, ( m_poLeftTop.y() + rtDraw.y() ) / m_fScale,
			rtDraw.width() / m_fScale, rtDraw.height() / m_fScale );
		QPainter	pnt( this );
		pnt.drawImage( rtDraw, *m_pImage, rtSour );
		if ( m_pMask )
		{
			pnt.drawImage( rtDraw, *m_pMask, rtSour );
		}
		if ( m_operPots.size() )
		{
			pnt.setPen( m_penLine );
			//pnt.setCompositionMode( QPainter::QPainter::CompositionMode_Xor );
			//pnt.drawPolygon( m_operPots.begin(), m_operPots.size() );
			QPoint	lastPt	= fromImagePos( m_pathClosed ? m_operPots.back() : m_operPots.front() ).toPoint();
			for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt )
			{
				QPoint	pos	= fromImagePos( *pt ).toPoint();
				pnt.drawLine( lastPt, pos );
				lastPt	= pos;
			}
			for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt )
			{
				QPoint	pos	= fromImagePos( *pt ).toPoint();
				QRect	rtPot( pos.x() - m_operPotRaidus, pos.y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
				pnt.fillRect( rtPot, m_bruPot );
				pnt.drawRect( rtPot );
			}
		}
	}
}

void CImageDisplay::wheelEvent( QWheelEvent *event )
{
	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta() / 8;

	if ( !numDegrees.isNull() )
	{
		QPoint	numSteps = numDegrees / 15;

		int32_t	iStep	= abs( numSteps.y() );
		float	fScale	= m_fScale;
		while ( iStep-- )
			fScale	=  numSteps.y() < 0 ? fScale / 1.1f : fScale * 1.1f;
		if ( fScale < 0.02f )
			fScale	= 0.02f;
		else if ( fScale > 10.0f )
			fScale	= 10.0f;
		setScale( fScale, event->pos() );

	}
}

void CImageDisplay::resizeEvent( QResizeEvent * event )
{
	if ( m_bFitWnd )
	{
		calcScale( -1.0f, event->size() );
	}
	else
	{
		calcScale( m_fScale, event->size() );
	}
}

void CImageDisplay::mousePressEvent( QMouseEvent * event )
{
	m_bMouse_Press	= true;
	m_poMouse_Press		= event->pos();
	m_poLeftTop_Press	= m_poLeftTop;
	if ( m_eOperType == eOperNormal )
	{
	}
	else if ( m_eOperType == eOperRect )
	{
		m_operPotInd	= -1;
		for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt )
		{
			QPoint	pos	= fromImagePos( *pt ).toPoint();
			QRect	rtPot( pos.x() - m_operPotRaidus, pos.y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
			if ( rtPot.contains( m_poMouse_Press ) )
			{
				m_operPotInd	= pt - m_operPots.begin();
				m_poOperPt_Press	= *pt;
				break;
			}
		}
		if ( m_operPotInd < 0 )
		{
			m_operPotInd	= 0;
			m_operPots.resize( 4 );
			m_poOperPt_Press	= toImagePos( m_poMouse_Press );
			for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt )
				*pt	= m_poOperPt_Press;
			m_maskOperSucc	= false;
			m_pathClosed	= true;
			update();
		}
	}
	else if ( m_eOperType == eOperPath )
	{
		m_poOperPt_Press	= toImagePos( m_poMouse_Press );
		int32_t	potInd	= -1;
		for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt )
		{
			QPoint	pos	= fromImagePos( *pt ).toPoint();
			QRect	rtPot( pos.x() - m_operPotRaidus, pos.y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
			if ( rtPot.contains( m_poMouse_Press ) )
			{
				potInd	= pt - m_operPots.begin();
				m_poOperPt_Press	= *pt;
				break;
			}
		}

		if ( m_pathClosed )
		{
			if ( potInd < 0 )
			{
				m_operPots.clear();
				m_pathClosed	= false;
				m_operPots.push_back( m_poOperPt_Press );
				m_operPots.push_back( m_poOperPt_Press );
				m_operPotInd	= 1;
				m_maskOperSucc	= false;
				update();
			}
			else
			{
				m_operPotInd	= potInd;
			}
		}
		else
		{
			if ( m_operPots.size() > 3 )
			{
				QPoint	pos	= fromImagePos( m_operPots.front() ).toPoint();
				QRect	rtPot( pos.x() - m_operPotRaidus, pos.y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
				QRect	rtPot2( event->pos().x() - m_operPotRaidus, event->pos().y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
				if ( rtPot.intersects( rtPot2 ) )
				{
					m_operPots.pop_back();
					vector<QPoint>	pts;
					for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt ) pts.push_back( pt->toPoint() );
					m_maskOperSucc	= m_inpaint->regionPath( (int32_t*)&pts.front(), pts.size() );
					checkMaskChange();
					m_pathClosed	= true;
					m_operPotInd	= -1;
					update();
					return;
				}
			}
			if ( m_operPotInd == potInd )
			{
				m_operPotInd	= m_operPots.size();
				m_operPots.push_back( m_poOperPt_Press );
			}
		}

	}
	else if ( m_eOperType == eOperPen )
	{
		if ( !m_maskOperSucc )
		{
			m_poOperPt_Press	= toImagePos( m_poMouse_Press );
			m_maskOperSucc		= m_inpaint->regionPencilBegin( m_penRadius );
			m_inpaint->regionPencilPos( m_poOperPt_Press.x(), m_poOperPt_Press.y() );
			checkMaskChange();
			update();
		}
	}
}

void CImageDisplay::checkMaskChange()
{
	int32_t	x, y, w, h;
	if ( m_inpaint->getRegionChangedRect( x, y, w, h ) )
	{
		int32_t	pitch;
		const uint8_t	*	mask	= m_inpaint->getRegionImage( pitch );
		if ( nullptr == mask ) return;
		for ( int dy = 0; dy < h; ++dy )
		{
			const uint8_t*	mskPix	= mask + ( y + dy ) * pitch + x;
			uint8_t*	imgPix	= m_pMask->scanLine( y + dy ) + x * 4;
			for ( int dx = 0; dx < w; ++dx )
			{
				imgPix[3]	= *mskPix++ == 0 ? 0 : 128;
				imgPix	+= 4;
			}
		}
		emit maskChanged();
	}
	//int32_t	pitch;
	//uint8_t	*	mask	= m_inpaint.getRegionImage( pitch );
	//for ( int dy = 0; dy < m_pMask->height(); ++dy )
	//{
	//	uint8_t*	mskPix	= mask + dy * pitch;
	//	uint8_t*	imgPix	= m_pMask->scanLine( dy );
	//	for ( int dx = 0; dx < m_pMask->width(); ++dx )
	//	{
	//		imgPix[3]	= *mskPix++ == 0 ? 0 : 128;
	//		imgPix	+= 4;
	//	}
	//}
}

void CImageDisplay::mouseReleaseEvent( QMouseEvent * event )
{
	m_bMouse_Press	= false;
	if ( m_pathClosed )
		m_operPotInd	= -1;
	if ( m_eOperType == eOperPen )
	{
		if ( m_maskOperSucc )
		{
			m_inpaint->regionPencilEnd();
			m_maskOperSucc	= false;
			emit maskChanged();
		}
	}
}

void CImageDisplay::clearImageWithPen( const QPoint& p0, const QPoint& p1 )
{
	if ( m_pImage && !m_pImage->isNull() )
	{
		QPointF	pA = QPointF( p0 + m_poLeftTop ) / m_fScale;
		QPointF	pB = QPointF( p1 + m_poLeftTop ) / m_fScale;
		QPen	pen	= QPen( QColor( 0, 0, 0 ) );
		pen.setWidth( 20 );
		QPainter	pnt( m_pImage );
		pnt.setPen( pen );
		pnt.drawLine( pA, pB );
		update();
	}
}

void CImageDisplay::mouseMoveEvent( QMouseEvent * event )
{
	if ( m_bMouse_Press )
	{
		if ( m_eOperType == eOperNormal )
		{
			QPointF	poNew	= limitView( m_poLeftTop_Press - ( event->pos() - m_poMouse_Press ), size() );
			if ( poNew != m_poLeftTop )
			{
				m_poLeftTop	= poNew;
				update();
			}
		}
		else if ( m_eOperType == eOperRect )
		{
			QPointF	poNew	= m_poOperPt_Press + toImagePos( event->pos() ) - toImagePos( m_poMouse_Press );
			if ( poNew.x() < 0 ) poNew.setX( 0 );
			if ( poNew.y() < 0 ) poNew.setY( 0 );
			if ( poNew.x() >= m_pImage->width() ) poNew.setX( m_pImage->width() - 1 );
			if ( poNew.y() >= m_pImage->height() ) poNew.setY( m_pImage->height() - 1 );

			if ( m_maskOperSucc )
				m_inpaint->regionUndo();

			qreal		l	= min( poNew.x(), m_operPots[( m_operPotInd + 2 ) % 4].x() );
			qreal		t	= min( poNew.y(), m_operPots[( m_operPotInd + 2 ) % 4].y() );
			qreal		r	= max( poNew.x(), m_operPots[( m_operPotInd + 2 ) % 4].x() );
			qreal		b	= max( poNew.y(), m_operPots[( m_operPotInd + 2 ) % 4].y() );


			m_operPots[0]	= QPointF( l, t );
			m_operPots[1]	= QPointF( r, t );
			m_operPots[2]	= QPointF( r, b );
			m_operPots[3]	= QPointF( l, b );
			m_operPotInd	= -1;
			for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt )
			{
				if ( poNew == *pt )
				{
					m_operPotInd	= pt - m_operPots.begin();
					break;
				}
			}
			m_maskOperSucc	= m_inpaint->regionRect( int32_t( l ), int32_t( t ), int32_t( r ) - int32_t( l ) + 1 , int32_t( b ) - int32_t( t ) + 1 );
			checkMaskChange();
			update();
		}
		else if ( m_eOperType == eOperPen )
		{
			if ( m_maskOperSucc )
			{
				m_poOperPt_Press	= toImagePos( event->pos() );
				m_inpaint->regionPencilPos( m_poOperPt_Press.x(), m_poOperPt_Press.y() );
				checkMaskChange();
				update();
			}
		}
	}

	if ( m_eOperType == eOperPath && m_operPotInd >= 0 )
	{
		QPointF	poNew	= m_poOperPt_Press + toImagePos( event->pos() ) - toImagePos( m_poMouse_Press );
		if ( poNew.x() < 0 ) poNew.setX( 0 );
		if ( poNew.y() < 0 ) poNew.setY( 0 );
		if ( poNew.x() >= m_pImage->width() ) poNew.setX( m_pImage->width() - 1 );
		if ( poNew.y() >= m_pImage->height() ) poNew.setY( m_pImage->height() - 1 );

		if ( m_pathClosed )
		{
			if ( m_maskOperSucc )
				m_inpaint->regionUndo();
		}
		else if ( m_operPots.size() > 3 )
		{
			QPoint	pos	= fromImagePos( m_operPots.front() ).toPoint();
			QRect	rtPot( pos.x() - m_operPotRaidus, pos.y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
			QRect	rtPot2( event->pos().x() - m_operPotRaidus, event->pos().y() - m_operPotRaidus, m_operPotRaidus * 2 + 1, m_operPotRaidus * 2 + 1 );
			if ( rtPot.intersects( rtPot2 ) )
			{
				poNew	= m_operPots.front();
			}
		}
		m_operPots[m_operPotInd]	= poNew;
		if ( m_pathClosed )
		{
			vector<QPoint>	pts;
			for ( auto pt = m_operPots.begin(); pt != m_operPots.end(); ++pt ) pts.push_back( pt->toPoint() );
			m_maskOperSucc	= m_inpaint->regionPath( (int32_t*)&pts.front(), pts.size() );
			checkMaskChange();
		}
		update();
	}


	if ( m_pImage )
	{
		static		QPoint	lastPt;
		QPointF	pF	= QPointF( event->pos() + m_poLeftTop ) / m_fScale;
		QPoint	pA( pF.x(), pF.y() );
		if ( pA != lastPt && m_pImage->rect().contains( pA ) )
		{
			lastPt	= pA;
			QColor	col( m_pImage->pixel( pA ) );

			this->setToolTip( QString( "%1, %2\n R:%3 G:%4 B:%5 A:%6" ).arg( pA.x() ).arg( pA.y() )
				.arg( col.red() ).arg( col.green() ).arg( col.blue() ).arg( col.alpha() ) );
		}
	}
}

void CImageDisplay::on_fileSelected( QImage* image, bool resetMask )
{
	if ( image && !image->isNull() )
	{
		m_pImage	= image;
		m_operPots.clear();
		m_operPotInd	= -1;

		if ( m_oldSImageize != m_pImage->size() )
		{
			m_oldSImageize	= m_pImage->size();
			resetMask	= true;
			if ( m_pMask )
			{
				delete m_pMask;
				m_pMask	= nullptr;
			}
			m_pMask		= new QImage( m_pImage->size(), QImage::Format_ARGB32 );
			if ( m_bFitWnd )
			{
				QSizeF	siImg	= m_pImage->size();
				siImg.scale( size(), Qt::KeepAspectRatio );
				m_fScale	= siImg.width() * 1.0f / m_pImage->width();
				m_poLeftTop.setX( ( siImg.width() - width() ) * 0.5f );
				m_poLeftTop.setY( ( siImg.height() - height() ) * 0.5f );
			}
			else
			{
				QSizeF	siImg( m_pImage->width() * m_fScale, m_pImage->height() * m_fScale );
				m_poLeftTop.setX( ( siImg.width() - width() ) * 0.5f );
				m_poLeftTop.setY( ( siImg.height() - height() ) * 0.5f );
			}
		}
		if ( m_pMask && resetMask ) m_pMask->fill( QColor( 255, 0, 0, 0 ) );
		emit maskChanged();
		checkMaskChange();
		update();
	}
	else
	{
		m_pImage	= nullptr;
	}
}

void CImageDisplay::undo()
{
	if ( m_pImage )
	{
		if ( m_inpaint->regionUndo() )
		{
			m_operPots.clear();
			m_operPotInd	= -1;
			checkMaskChange();
			update();
		}
	}
}

void CImageDisplay::redo()
{
	if ( m_pImage )
	{
		if ( m_inpaint->regionRedo() )
		{
			m_operPots.clear();
			m_operPotInd	= -1;
			checkMaskChange();
			update();
		}
	}
}

void CImageDisplay::startInpaint( int32_t inpType )
{
	m_operPots.clear();
	m_operPotInd	= -1;
#if 0
	int64_t	num		= 2000;
	QRect	rtInp( 50, 50, 300, 200 );
	uint32_t	tick	= timeGetTime();
	for( int i = 0; i < num; ++i )
	{
		m_inpaint.setDamagedImage( m_pImage->bits(), m_pImage->width(), m_pImage->height(), m_pImage->bytesPerLine() );
		m_inpaint.regionRect( rtInp.x(), rtInp.y(), rtInp.width(), rtInp.height() );
	//	m_inpaint.regionRect( rtInp.x(), rtInp.y() + 111, rtInp.width(), rtInp.height() );
		m_inpaint.inpaint();
	}
	tick	= timeGetTime() - tick;
	qDebug() << "inpaint width:" << rtInp.width() << ", height:" << rtInp.height() << ".  fps:" << float(num) * 1000 / tick << ", avg ms:" << float(tick) / num;
#else
	m_inpaint->inpaint( m_pImage->bits(), m_pImage->bytesPerLine(), inpType );
#endif // 1

	m_pMask->fill( QColor( 255, 0, 0, 0 ) );
	emit maskChanged();
	update();
}