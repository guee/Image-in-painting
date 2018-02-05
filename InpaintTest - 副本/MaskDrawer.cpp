#include "MaskDrawer.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#include <QtGui>

CMaskDrawer::CMaskDrawer()
{
	m_curPencil	= nullptr;
	m_currOper	= 0;
	m_unitedChanged	= false;
	m_sobelSum		= 0;
}


CMaskDrawer::~CMaskDrawer()
{
}

bool CMaskDrawer::resetSize( int32_t width, int32_t height )
{
	if ( width > 0 && height > 0 )
	{
		if ( m_maskImage.width != width || m_maskImage.height != height )
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
		}
		else
		{
			resetMask();
		}
	}
	else
	{
		m_maskImage.clear();
		m_maskImage_exp.clear();
	}
	m_inpRect	= GRect();
	m_rtUniteds.clear();
	m_rtAbsolutes.clear();
	m_watermark.clear();
	m_changedRect.clear();
	m_picChips.clear();
	m_currOper	= 0;
	m_curPencil	= nullptr;
	m_unitedChanged	= false;

	return true;
}

void CMaskDrawer::resetMask()
{
	if ( getUnited() )
	{
		for ( int32_t y = m_inpRect.top(); y <= m_inpRect.bottom(); ++y )
		{
			memset( m_maskImage.pixel( m_inpRect.left(), y ), 0, m_inpRect.width() );
		}
		m_inpRect	= GRect();
		m_rtUniteds.clear();
		m_rtAbsolutes.clear();
	}
}

bool CMaskDrawer::fillRect( int32_t x, int32_t y, int32_t width, int32_t height )
{
	if ( !m_maskImage.isVaild() ) return false;
	if ( m_picChips.empty() ) resetMask();
	GRect	rtFill( x, y, width, height );
	rtFill	= m_maskImage.rtImage.intersected( rtFill );
	if ( rtFill.isEmpty() ) return false;
	if ( m_curPencil ) pencilEnd();

	SChip*	chip	= new SChip;
	chip->eType		= eRect;
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
	if ( m_picChips.empty() ) resetMask();
	if ( m_curPencil ) pencilEnd();
	m_curPencil		= new SChip;
	m_curPencil->eType	= ePencil;
	m_curPencil->radius	= radius;
	return true;
}

bool CMaskDrawer::pencilPos( int32_t x, int32_t y )
{
	if ( nullptr == m_curPencil ) return false;

	GRect	rtPen	= drawLine( m_curPencil->pots.empty() ? GPoint( x, y ) : m_curPencil->pots.back(), GPoint( x, y ), m_curPencil->radius );
	m_changedRect.push_back( rtPen );
	if ( m_curPencil->rect.isValid() )
		m_curPencil->rect	|= rtPen;
	else
		m_curPencil->rect	= rtPen;
	m_curPencil->pots.push_back( GPoint( x, y ) );
	return true;
}

bool CMaskDrawer::pencilEnd()
{
	if ( nullptr == m_curPencil ) return false;
	if ( m_curPencil->pots.size() )
	{
		if ( m_currOper < m_picChips.size() )
		{
			m_picChips.erase( m_picChips.begin() + m_currOper, m_picChips.end() );
		}
		m_picChips.push_back( m_curPencil );
		m_curPencil	= nullptr;
		if ( !redo() )
		{
			delete m_picChips.back();
			m_picChips.pop_back();
			return false;
		}
	}
	else
	{
		delete m_curPencil;
		m_curPencil	= nullptr;
	}
	return true;
}

bool CMaskDrawer::pathClosed( const int32_t pots[], int32_t potCount )
{
	if ( !m_maskImage.isVaild() ) return false;
	if ( m_picChips.empty() ) resetMask();
	if ( potCount < 3 || nullptr == pots ) return false;
	if ( m_curPencil ) pencilEnd();

	SChip*	chip	= new SChip;
	chip->eType		= ePath;
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
	chip->rect	= m_maskImage.rtImage.intersect( GRect( l - 1, t - 1, r - l + 2, b - t + 2 ) );

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
	if ( m_curPencil ) pencilEnd();
	if ( m_currOper == m_picChips.size() ) return false;
	SChip*	chip	= m_picChips[m_currOper];
	if ( redoOper( chip ) )
	{
		++m_currOper;
		return true;
	}
	return false;
}

bool CMaskDrawer::undo()
{
	if ( m_curPencil ) pencilEnd();
	if ( m_currOper == 0 ) return false;
	SChip*	chip	= m_picChips[m_currOper - 1];

	if ( undoOper( chip ) )
	{
		--m_currOper;
		return true;
	}
	return false;
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

inline int32_t	CMaskDrawer::otsuHistogram( int32_t hisCount, uint32_t* hisTab )
{
	uint32_t	sumLeft		= 0;
	uint32_t	countLeft	= 0;
	uint32_t	sumRight	= 0;
	uint32_t	countRight	= 0;
	double_t	maxVariance		= -1;
	int32_t		thresholdVal	= -1;

	for ( int32_t i = 0; i < hisCount; ++i )
	{
		sumRight	+= hisTab[i] * i;
		countRight	+= hisTab[i];
	}

	for ( int32_t i = 0; i < hisCount; ++i )
	{
		sumLeft		+= hisTab[i] * i;
		countLeft	+= hisTab[i];
		sumRight	-= hisTab[i] * i;
		countRight	-= hisTab[i];
		if ( sumLeft == 0 ) continue;
		else if ( sumRight == 0 ) break;
		double	avgLeft		= (double)sumLeft / countLeft;
		double	avgRight	= (double)sumRight / countRight;
		double	variance	= countLeft * countRight * ( avgLeft - avgRight ) * ( avgLeft - avgRight );
		if ( variance > maxVariance )
		{
			maxVariance	= variance;
			thresholdVal	= i;
		}
	}
	return thresholdVal;
}

inline bool	CMaskDrawer::otsuOnlyNumber( int32_t hisCount, uint32_t* numTab, vector<int32_t>& lines )
{
	uint32_t	countTot	= 0;
	uint32_t	countTot2	= 0;
	uint32_t	countLeft	= 0;
	uint32_t	countRight	= 0;
	int32_t		threshold	= -1;
	double		maxVariance		= -1;
	if ( hisCount < 3 ) return false;
	for ( int32_t i = 0; i < hisCount; ++i )
	{
		countTot	+= numTab[i];
	}
	if ( countTot < 3 ) return false;

	lines.clear();
	int32_t		thStart	= 0;
	countRight	= countTot;
	while ( thStart < hisCount - 2 )
	{
		maxVariance	= -1;
		threshold	= -1;
		for ( int32_t i = thStart; i < hisCount - 1; ++i )
		{
			countLeft	+= numTab[i];
			countRight	-= numTab[i];
			if ( countLeft == 0 ) continue;
			else if ( countRight == 0 ) break;
			double	avgLeft		= (double)countLeft / ( i - thStart + 1 );
			double	avgRight	= (double)countRight / ( hisCount - i - 1 );
			double	variance	= ( i - thStart + 1 ) * ( hisCount - i - 1 ) * ( avgLeft - avgRight ) * ( avgLeft - avgRight );
			if ( variance > maxVariance )
			{
				maxVariance	= variance;
				threshold	= i;
				countTot2	= countRight;
			}
		}
		if ( threshold >= 0 )
		{
			lines.push_back( threshold );
			thStart	= threshold + 1;
			countLeft	= 0;
			countRight	= countTot2;
		}
		else
			break;
	}

	thStart	= hisCount - 1;
	countLeft	= countTot;
	countRight	= 0;
	while ( thStart > 1 )
	{
		maxVariance	= -1;
		threshold	= -1;
		for ( int32_t i = thStart; i > 0; --i )
		{
			countLeft	-= numTab[i];
			countRight	+= numTab[i];
			if ( countRight == 0 ) continue;
			else if ( countLeft == 0 ) break;
			double	avgLeft		= (double)countLeft / ( i );
			double	avgRight	= (double)countRight / ( thStart - i + 1 );
			double	variance	= ( i ) * ( thStart - i + 1 ) * ( avgLeft - avgRight ) * ( avgLeft - avgRight );
			if ( variance > maxVariance )
			{
				maxVariance	= variance;
				threshold	= i;
				countTot2	= countLeft;
			}
		}
		if ( threshold >= 0 )
		{
			lines.push_back( threshold );
			thStart	= threshold - 1;
			countLeft	= countTot2;
			countRight	= 0;
		}
		else
			break;
	}
	if ( lines.size() )
	{
		sort( lines.begin(), lines.end(),
			[]( const int32_t& p1, const int32_t& p2 ) { return p1 < p2; } );
		return true;

	}
	return false;
}

template<class mapIterator>
inline mapIterator CMaskDrawer::otsuHistogram( mapIterator begin, mapIterator end )
{
	uint32_t	sumLeft		= 0;
	uint32_t	countLeft	= 0;
	uint32_t	sumRight	= 0;
	uint32_t	countRight	= 0;
	double_t	maxVariance		= -1;
	mapIterator	thresholdIter	= begin;
	for ( auto w = begin; w != end; ++w )
	{
		sumRight	+= w->first * w->second;
		countRight	+= w->second;
	}
	for ( auto w = begin; w != end; ++w )
	{
		sumLeft		+= w->first * w->second;
		countLeft	+= w->second;
		sumRight	-= w->first * w->second;
		countRight	-= w->second;
		if ( sumLeft == 0 ) continue;
		else if ( sumRight == 0 ) break;
		double	avgLeft		= (double)sumLeft / countLeft;
		double	avgRight	= (double)sumRight / countRight;
		double	variance	= countLeft * countRight * ( avgLeft - avgRight ) * ( avgLeft - avgRight );
		if ( variance > maxVariance )
		{
			maxVariance	= variance;
			thresholdIter	= w;
		}
	}
	return thresholdIter;
}


bool CMaskDrawer::watermarkDetection( uint8_t * imgBuf, int32_t pitch )
{
	if ( !m_maskImage.isVaild() ) return false;
	if ( !m_watermark.isVaild() )
	{
		if ( !m_watermark.create( nullptr, m_maskImage.width, m_maskImage.height ) )
			return false;
		m_wmarkChips.clear();
		m_sobelSum		= 0;
		m_maxWeight		= 0;
	}

	const		uint8_t	thresholdVal	= 32;
	uint32_t	sobelSum		= 0;
	uint32_t	sobelOverlay	= 0;
	for ( int32_t y = 1; y < m_watermark.height - 1; ++y )
	{
		uint8_t*	imgPix	= imgBuf + y * pitch + 4;
		uint8_t*	imgPixT	= imgPix - pitch;
		uint8_t*	imgPixB	= imgPix + pitch;
		SDetectPixel*	detPix	= m_watermark.pixel( 1, y );
		for ( int32_t x = 1; x < m_watermark.width - 1; ++x )
		{
			//不对图像进行灰度化处理，而直接分别对 RGB 使用 sobel 算子计算梯度，可以得到更准确的边缘信息。
			int32_t		sx	= ( ( imgPixT[4] + imgPix[4] * 2 + imgPixB[4] ) - ( imgPixT[-4] + imgPix[-4] * 2 + imgPixB[-4] ) );
			int32_t		sy	= ( ( imgPixT[-4] + imgPixT[0] * 2 + imgPixT[4] ) - ( imgPixB[-4] + imgPixB[0] * 2 + imgPixB[4] ) );
			uint32_t	sobelB	= uint32_t( sqrt( sx * sx + sy * sy ) );
			sx	= ( ( imgPixT[5] + imgPix[5] * 2 + imgPixB[5] ) - ( imgPixT[-3] + imgPix[-3] * 2 + imgPixB[-3] ) );
			sy	= ( ( imgPixT[-3] + imgPixT[1] * 2 + imgPixT[5] ) - ( imgPixB[-3] + imgPixB[1] * 2 + imgPixB[5] ) );
			uint32_t	sobelG	= uint32_t( sqrt( sx * sx + sy * sy ) );
			sx	= ( ( imgPixT[6] + imgPix[6] * 2 + imgPixB[6] ) - ( imgPixT[-2] + imgPix[-2] * 2 + imgPixB[-2] ) );
			sy	= ( ( imgPixT[-2] + imgPixT[2] * 2 + imgPixT[6] ) - ( imgPixB[-2] + imgPixB[2] * 2 + imgPixB[6] ) );
			uint32_t	sobelR	= uint32_t( sqrt( sx * sx + sy * sy ) );
			//直接使用阀值来判断是否是有效的边缘像素，而不使用非极大值抑制等方法得到单像素宽的边缘线。
			//因为在不同的画面上，水印图像的单像素宽边缘的可能不能重叠，但使用阀值得到的较宽的边缘线，就一般能够重叠。
			if ( sobelB > 150 || sobelG > 150 || sobelR > 150 || ( sobelB + sobelG + sobelR ) > 300 )
			{
				detPix->sobel1	= 1;
				if ( !detPix->mark )
				{
					++sobelSum;
					if ( detPix->sobel2 )
					{
						++sobelOverlay;
					}
				}
			}
			else
			{
				detPix->sobel1	= 0;
			}
			++detPix;

			imgPix	+= 4;
			imgPixT	+= 4;
			imgPixB	+= 4;
		}
	}
	if ( sobelSum && ( sobelSum + m_sobelSum ) / 5 > sobelOverlay )
	{
		m_sobelSum	=  sobelSum;
		if ( m_maxWeight == 255 )
		{
			for ( int32_t y = 0; y < m_watermark.height; ++y )
			{
				SDetectPixel*	detPix	= m_watermark.pixel( 0, y );
				for ( int32_t x = 0; x < m_watermark.width; ++x )
				{
					detPix->weight	/= 2;
					++detPix;
				}
			}
			m_maxWeight	/= 2;
		}
		uint32_t	hisWeight[256]	= { 0 };
		for ( int32_t y = 0; y < m_watermark.height; ++y )
		{
			SDetectPixel*	detPix	= m_watermark.pixel( 0, y );
			for ( int32_t x = 0; x < m_watermark.width; ++x )
			{
				detPix->weight	+= detPix->sobel1 * detPix->sobel2;
				detPix->sobel2	= detPix->sobel1;
				if ( detPix->weight )
				{
					++hisWeight[detPix->weight];
					if ( m_maxWeight < detPix->weight ) m_maxWeight = detPix->weight;
				}
				++detPix;
			}
		}
		int32_t	threshold	= otsuHistogram( 256, hisWeight );
		if ( threshold >= 0 )
		{
			//if ( threshold * 5 / 2 >= m_maxWeight )
			//{
			//	int32_t	threshold2	= otsuHistogram( threshold + 1, hisWeight );
			//	qDebug() << "m_maxWeight=" << m_maxWeight << ",  threshold=" << threshold << ",  threshold2=" << threshold2;
			//	if ( threshold2 > 1 && threshold2 * 4 >= threshold ) threshold	= threshold2;
			//}
			uint32_t*	hisSplitH	= new uint32_t[m_watermark.height];
			uint32_t*	hisSplitW	= new uint32_t[m_watermark.width];
			memset( hisSplitH, 0, sizeof( uint32_t )* m_watermark.height );
			for ( int32_t y = 1; y < m_watermark.height; ++y )
			{
				SDetectPixel*	detPix	= m_watermark.pixel( 0, y );
				uint16_t		linePixCount	= 0;
				for ( int32_t x = 0; x < m_watermark.width; ++x )
				{
					detPix->mark2	= 0;
					if ( detPix->weight > threshold )
					{
						detPix->mark	= 1;
						++linePixCount;
					}
					else
					{
						detPix->mark	= 0;
					}
					detPix->integral	= linePixCount + detPix[-m_watermark.width].integral;
					++detPix;
				}
				hisSplitH[y]	= linePixCount;
			}
			vector<GRect>	watermarks;
			vector<int32_t> linesH;
			vector<int32_t> linesW;
			if ( otsuOnlyNumber( m_watermark.height, hisSplitH, linesH ) )
			{
				int32_t		lastY	= linesH[0];
				for ( int32_t i = 1; i < linesH.size(); ++i )
				{
					int32_t	hl	= linesH[i] - lastY + 1;
					if ( hl <= 3 ) continue;
					memset( hisSplitW, 0, sizeof( uint32_t )* m_watermark.width );
					SDetectPixel*	detPix	= m_watermark.pixel( 1, linesH[i] );
					SDetectPixel*	detPixT	= m_watermark.pixel( 1, lastY );
					for ( int32_t dx = 1; dx < m_watermark.width; ++dx )
					{
						hisSplitW[dx]	= detPix->integral - detPix[-1].integral - detPixT->integral + detPixT[-1].integral;
						++detPix;
						++detPixT;
					}
					if ( otsuOnlyNumber( m_watermark.width, hisSplitW, linesW ) )
					{
						int32_t	lastX	= linesW[0];
						for ( int32_t j = 1; j < linesW.size(); ++j )
						{
							int32_t	wl	= linesW[j] - lastX + 1;
							if ( wl <= 3 ) continue;
							detPix	= m_watermark.pixel( linesW[j], linesH[i] );
							detPixT	= m_watermark.pixel( linesW[j], lastY );
							int32_t	pixCount	= detPix->integral - detPix[-wl].integral - detPixT->integral + detPixT[-wl].integral;
							if ( pixCount > ( hl + wl ) * 3 )
								watermarks.push_back( GRect( lastX, lastY, wl, hl ) );
							else if ( pixCount > ( hl + wl ) )
							{
								for ( int32_t dy = lastY; dy <= linesH[i] + 1; ++dy )
								{
									hisSplitH[dy]	= detPixT->integral - detPixT[-wl].integral - detPixT[-m_watermark.width].integral + detPixT[-m_watermark.width - wl].integral;
									detPixT	+= m_watermark.width;
								}
								vector<int32_t> linesHM;
								if ( otsuOnlyNumber( hl + 1, hisSplitH + lastY, linesHM ) )
								{
									if ( linesHM.size() > 2 ) linesHM.erase( linesHM.begin() + 1, linesHM.end() - 1 );
									for ( int32_t k = 0; k < linesHM.size(); ++k ) linesHM[k] += lastY;
									linesHM.push_back( linesH[i] );
									int32_t		lastY2	= lastY;
									for ( int32_t k = 0; k < linesHM.size(); ++k )
									{
										detPix	= m_watermark.pixel( linesW[j], linesHM[k] );
										detPixT	= m_watermark.pixel( linesW[j], lastY2 );
										int32_t	hlm	= linesHM[k] - lastY2 + 1;
										if ( hlm <= 3 ) continue;
										pixCount	= detPix->integral - detPix[-wl].integral - detPixT->integral + detPixT[-wl].integral;
										if ( pixCount >( hlm + wl ) * 2 )
											watermarks.push_back( GRect( lastX, lastY2, wl, hlm ) );

										lastY2	= linesHM[k];
									}
								}
							}
							lastX	= linesW[j];
						}
					}
					lastY	= linesH[i];
				}
			}
			delete[]hisSplitH;
			delete[]hisSplitW;
			//删除之前的水印检测结果。
			while( m_wmarkChips.size() )
			{
				if ( m_wmarkChips.back() < m_currOper )
				{
					undoOper( m_picChips[ m_wmarkChips.back()] );
					m_picChips.erase( m_picChips.begin() + m_wmarkChips.back() );
					--m_currOper;
				}
				m_wmarkChips.pop_back();
			}
			//重新添加水印检测结果。
			while ( watermarks.size() )
			{
				vector<GPoint>	edgePots;
				vector<GRect>	uniteds;
				uniteds.push_back( watermarks.back().adjusted( -4, -4, 4, 4 ) );
				watermarks.pop_back();
				for ( int32_t i = 0; i < watermarks.size(); ++i )
				{
					for ( int32_t j = 0; j < uniteds.size(); ++j )
					{
						if ( uniteds[j].intersects( watermarks[i] ) )
						{
							uniteds.push_back( watermarks[i].adjusted( -4, -4, 4, 4 ) );
							watermarks.erase( watermarks.begin() + i );
							--i;
							break;
						}
					}
				}
				int32_t	l, t, r, b, mx, my;
				l = t = INT_MAX;
				r = b = INT_MIN;
				for ( auto rt = uniteds.begin(); rt != uniteds.end(); ++rt )
				{
					*rt	= m_watermark.rtImage.intersect( *rt );
					for ( int32_t y = rt->top(); y < rt->bottom(); ++y )
					{
						SDetectPixel*	detPix	= m_watermark.pixel( rt->left(), y );
						for ( int32_t x = rt->left(); x < rt->right(); ++x )
						{
							if ( detPix->mark && detPix->mark2 == 0 )
							{
								detPix->mark2	= 1;
								edgePots.push_back( GPoint( x, y ) );
							}
							++detPix;
						}
					}
					if ( rt->left() < l ) l = rt->left();
					if ( rt->top() < t ) t = rt->top();
					if ( rt->right() > r ) r = rt->right();
					if ( rt->bottom() > b ) b = rt->bottom();
				}
				mx = ( r + l ) / 2;
				my = ( t + b ) / 2;
				if ( convexClosure( edgePots ) )
				{
					for ( auto pt = edgePots.begin(); pt != edgePots.end(); ++pt )
					{
						pt->setX( pt->x() < mx ? pt->x() - 6 : pt->x() + 6 );
						pt->setY( pt->y() < my ? pt->y() - 6 : pt->y() + 6 );
					}
					if ( pathClosed( (int32_t*)&edgePots.front(), edgePots.size() ) )
					{
						m_wmarkChips.push_back( m_currOper - 1 );
					}
				}
			}
		}
	}
	return !m_wmarkChips.empty();
}

bool CMaskDrawer::convexClosure( vector<GPoint>& pots )
{
	if ( pots.size() < 3 ) return false;
	vector<GPoint> outPots;
	sort( pots.begin(), pots.end(),
		[]( const GPoint& p1, const GPoint& p2 ) {
		return ( p1.x() < p2.x() ) || ( p1.x() == p2.x() && p1.y() < p2.y() );
	} );
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
	pots	= outPots;
	return true;
};


GRect CMaskDrawer::drawLine( const GPoint& poStart, const GPoint& poEnd, float radius )
{
	if ( radius == 0.0f )
	{
		int32_t	stepCount	= max( abs( poStart.x() - poEnd.x() ), abs( poStart.y() - poEnd.y() ) );
		float	dx			= ( poEnd.x() - poStart.x() ) / stepCount;
		float	dy			= ( poEnd.y() - poStart.y() ) / stepCount;
		float	x			= poStart.x();
		float	y			= poStart.y();
		for ( int32_t i = 0; i <= stepCount; ++i )
		{
			if ( x >= 0 && x < m_maskImage.width && y >= 0 && y < m_maskImage.height )
			{
				SMaskPixel*	pix	= m_maskImage.pixel( x, y );
				pix->curDraw	= 1;
			}
			x	+= dx;
			y	+= dy;
		}
	}
	else
	{
		float	angle		= atan2( poEnd.y() - poStart.y(), poEnd.x() - poStart.x() );
		int32_t	stepCount	= int32_t( ( radius * 2.0 ) + 0.9999999 );
		float	da			= M_PI / stepCount;

		vector<GPoint>	pots;
		angle	+= M_PI * 0.5;
	
		for ( int32_t i = 0; i <= stepCount; ++i )
		{
			pots.push_back( GPoint( cos( angle ) * radius + 0.5 + poStart.x(), sin( angle ) * radius + 0.5 + poStart.y() ) );
			angle	+= da;
		}
		angle		= atan2( poEnd.y() - poStart.y(), poEnd.x() - poStart.x() ) - M_PI * 0.5;
		for ( int32_t i = 0; i <= stepCount; ++i )
		{
			pots.push_back( GPoint( cos( angle ) * radius + 0.5 + poEnd.x(), sin( angle ) * radius + 0.5 + poEnd.y() ) );
			angle	+= da;
		}
		fillPath( pots );
	}
	GRect	rtChg( min( poStart.x(), poEnd.x() ) - radius, min( poStart.y(), poEnd.y() ) - radius,
		abs( poEnd.x() - poStart.x() ) + ( radius * 2 + 2 ), abs( poEnd.y() - poStart.y() ) + ( radius * 2 + 2 ) );
	return m_maskImage.rtImage.intersected( rtChg );
}

void CMaskDrawer::fillPath( const vector<GPoint>& pots )
{
	vector<map<int32_t, int32_t>>	allLines;

	allLines.resize( m_maskImage.height );
	GPoint	poLast	= pots.back();
	int32_t	dyLast	= 0;
	for ( auto pt = pots.end() - 1; pt > pots.begin(); --pt )
	{
		dyLast	= ( pt->y() - (pt-1)->y() );
		dyLast	= dyLast < 0 ? -1 : ( dyLast > 0 ? 1 : 0 );
		if ( dyLast != 0 ) break;
	}
	if ( dyLast == 0 ) return;
	for ( auto pt = pots.begin(); pt != pots.end(); ++pt )
	{
		int32_t	stepCount	= abs( pt->y() - poLast.y() );
		if ( stepCount )
		{
			int32_t	dy			= ( pt->y() - poLast.y() ) / stepCount;
			float	dx			= float( pt->x() - poLast.x() ) / stepCount;
			int32_t	y			= poLast.y();
			float	x			= poLast.x();
			int32_t	i			= 0;
			if ( dyLast * dy == 1 )
			{
				i	= 1;
				y	+= dy;
				x	+= dx;
			}
			for ( ; i <= stepCount; ++i )
			{
				if ( y >= 0 && y < m_maskImage.height )
					++( allLines[y][x] );
				y	+= dy;
				x	+= dx;
			}
			dyLast	= dy;
		}
		poLast	= *pt;
	}
	int32_t		y	= 0;
	for ( auto li = allLines.begin(); y < allLines.size(); ++li, ++y )
	{
		auto	liSt	= li->begin();
		while( liSt != li->end() )
		{
			auto	liEn	= liSt;
			if ( --liSt->second == 0 ) ++liEn;
			if ( liEn == li->end() ) break;
			int32_t	stax	= max( 0, liSt->first );
			int32_t	endx	= min( liEn->first, ( m_maskImage.width - 1 ) );
			SMaskPixel*	pix	= m_maskImage.pixel( stax, y );
			for ( int32_t x = stax; x <= endx; ++x )
			{
				pix->curDraw	= 1;
				++pix;
			}
			liSt	= liEn;
			if ( --liEn->second == 0 )	++liSt;
		}
	}
}

bool CMaskDrawer::getUnited()
{
	if ( m_unitedChanged )
	{
		m_unitedChanged	= false;
		m_rtUniteds.clear();
		m_rtAbsolutes.clear();
		m_inpRect	= GRect();
		bool	isAdjoin	= false;
		vector<bool>	isUniteds;
		for ( int32_t i = 0; i < m_currOper; ++i )
		{
			m_rtUniteds.push_back( m_picChips[i]->rect );
			isUniteds.push_back( m_picChips[i]->eType == eRect ? false : true );
		}

		do
		{
			isAdjoin	= false;
			for ( int32_t i = 0; i < m_rtUniteds.size() - 1; ++i )
			{
				GRect	rtUnited	=	m_rtUniteds[i].adjusted( -1, -1, 1, 1 );
				for ( int32_t j = i + 1; j < m_rtUniteds.size(); ++j )
				{
					if ( rtUnited.intersects( m_rtUniteds[j] ) )
					{
						rtUnited	|= m_rtUniteds[j].adjusted( -1, -1, 1, 1 );
						m_rtUniteds.erase( m_rtUniteds.begin() + j );
						isUniteds.erase( isUniteds.begin() + j );
						isUniteds[i]	= true;
						--j;
						isAdjoin	= true;
					}
				}
				m_rtUniteds[i]	= rtUnited.adjusted( 1, 1, -1, -1 );
			}
		} while ( isAdjoin );

		m_inpRect	= m_rtUniteds[0];
		for ( int32_t i = 0; i < m_rtUniteds.size(); ++i )
		{
			m_inpRect	|= m_rtUniteds[i];
			if ( isUniteds[i] == false )
			{
				m_rtAbsolutes.push_back( m_rtUniteds[i] );
				m_rtUniteds.erase( m_rtUniteds.begin() + i );
				isUniteds.erase( isUniteds.begin() + i );
				--i;
			}
		}
	}
	return m_inpRect.isValid();
}

bool CMaskDrawer::redoOper( SChip* chip )
{
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
	else if ( chip->eType == ePencil )
	{
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
	}
	m_unitedChanged	= true;
	return true;
}

bool CMaskDrawer::undoOper( SChip* chip )
{
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
	else if ( chip->eType == ePath )
	{
		if ( chip->rect.isNull() ) return true;
		fillPath( chip->pots );
		for ( int32_t y = chip->rect.top(); y <= chip->rect.bottom(); ++y )
		{
			SMaskPixel*	pix	= m_maskImage.pixel( chip->rect.left(), y );
			for ( int32_t x = chip->rect.left(); x <= chip->rect.right(); ++x )
			{
				if ( pix->curDraw )
				{
					--( pix->drawCount );
					pix->curDraw	= 0;
				}
				++pix;
			}
		}
		m_changedRect.push_back( chip->rect );
	}
	else if ( chip->eType == ePencil )
	{
		for ( auto pt = chip->pots.begin(); pt < chip->pots.end() - 1; ++pt )
		{
			drawLine( *pt, *( pt + 1 ), chip->radius );
		}
		for ( int32_t y = chip->rect.top(); y <= chip->rect.bottom(); ++y )
		{
			SMaskPixel*	pix	= m_maskImage.pixel( chip->rect.left(), y );
			for ( int32_t x = chip->rect.left(); x <= chip->rect.right(); ++x )
			{
				if ( pix->curDraw )
				{
					--( pix->drawCount );
					pix->curDraw	= 0;
				}
				++pix;
			}
		}
		m_changedRect.push_back( chip->rect );
	}
	m_unitedChanged	= true;
	return true;
}
