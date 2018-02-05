#pragma once

#include <QWidget>
#include <QtGui>
#include "ui_CImageDisplay.h"
#include "Inpaint.h"

class CImageDisplay : public QWidget
{
	Q_OBJECT

public:
	CImageDisplay(QWidget *parent = Q_NULLPTR);
	~CImageDisplay();

	void setInpaint( CInpaint* inpaint ) { m_inpaint = inpaint; }

	float setFitWnd( bool bFitWnd );
	void setScale( float fScale, QPoint poCenter = QPoint( -1, -1 ) );
	float scale() { return m_fScale; }
	bool fitWnd() { return m_bFitWnd; }

	enum EOperType
	{
		eOperNormal,
		eOperRect,
		eOperPath,
		eOperPen,
	};
	void setOperType( EOperType eType );
	void setPenSize( int32_t radius );

	QImage*		image() { return m_pImage; }
	void undo();
	void redo();
	void startInpaint( bool isBest );
private:
	Ui::CImageDisplay ui;
	CInpaint*	m_inpaint;
	EOperType	m_eOperType;
	int32_t		m_penRadius;
	QVector<QPointF>	m_operPots;
	int32_t		m_operPotRaidus;
	int32_t		m_operPotInd;
	QPen		m_penLine;
	QBrush		m_bruPot;
	bool		m_maskOperSucc;
	bool		m_pathClosed;

	QImage*		m_pMask;
	QImage*		m_pImage;
	float		m_fScale;
	bool		m_bFitWnd;
	QPointF		m_poLeftTop;

	QPointF		m_poLeftTop_Press;
	QPoint		m_poMouse_Press;
	QPointF		m_poOperPt_Press;
	bool		m_bMouse_Press;
	QSize		m_oldSImageize;

	void paintEvent( QPaintEvent *event );
	void wheelEvent( QWheelEvent *event );
	void resizeEvent( QResizeEvent *event );
	void mousePressEvent( QMouseEvent *event );
	void mouseReleaseEvent( QMouseEvent *event );
	void mouseMoveEvent( QMouseEvent *event );
	void calcScale( float fScale, QSize siView, QPoint poCenter = QPoint( -1, -1 ) );
	QPointF limitView( QPointF posTopLeft, QSize siView );


	inline QPointF toImagePos( const QPointF& pot )
	{
		return ( pot + m_poLeftTop ) / m_fScale;
	}
	inline QPointF fromImagePos( const QPointF& pot )
	{
		return pot * m_fScale - m_poLeftTop;
	}

	void checkMaskChange();

	void clearImageWithPen( const QPoint& p0, const QPoint& p1 );
signals:
	void maskChanged();
public slots:
	void on_fileSelected( QImage* image, bool resetMask );
};
