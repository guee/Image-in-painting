#include "CImageDir.h"

CImageDir::CImageDir(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.listView->setViewMode( QListView::IconMode );
	QStandardItemModel*	pModel	= new QStandardItemModel( ui.listView );
	ui.listView->setModel( pModel );
	ui.listView->setIconSize( QSize( 100, 100 ) );
	ui.listView->setGridSize( QSize( 115, 125 ) );
	ui.listView->setMovement( QListView::Static );
	m_pThread	= nullptr;
	m_pImage	= nullptr;

	QPixmap		pixIcon( 100, 100 );
	QPainter	pntIcon( &pixIcon );
	pntIcon.fillRect( pixIcon.rect(), QColor( 128, 128, 128 ) );
	m_icoBase	= QIcon( pixIcon );
}

CImageDir::~CImageDir()
{
	if ( m_pThread )
	{
		m_pThread->stop();
		delete m_pThread;
		m_pThread	= nullptr;
	}
	if ( m_pImage ) delete m_pImage;
}

void CImageDir::setDir( const QString & szDir, QSize siIcon )
{
	if ( m_pThread )
	{
		m_pThread->stop();
		delete m_pThread;
		m_pThread	= nullptr;
	}
	m_szDir			= QDir::fromNativeSeparators( szDir );
	if ( !m_szDir.endsWith( "/" ) ) m_szDir += "/";
	m_siIcon	= siIcon;
	m_siGrid	= QSize( siIcon.width() + 15, siIcon.height() + 25 );
	ui.listView->setIconSize( m_siIcon );
	ui.listView->setGridSize( m_siGrid );
	QStandardItemModel*	pModel	= (QStandardItemModel*)ui.listView->model();
	pModel->clear();

	m_detWatermark	= true;
	m_inpaint->regionReset( 0, 0 );
	m_pThread	= new CDirThread( m_szDir, m_siIcon, this );
}

void CImageDir::resizeEvent( QResizeEvent *event )
{
	ui.listView->setGridSize( m_siGrid );
}

void CImageDir::on_fileListDone( bool thumbDone )
{
	if ( m_pThread == nullptr ) return;
	QStandardItemModel*	pModel	= (QStandardItemModel*)ui.listView->model();
	if ( false == thumbDone )
	{
		for ( int32_t i = 0; i < m_pThread->m_szFiles.size(); ++i )
		{
			QStandardItem*	item	= new QStandardItem( m_icoBase, m_pThread->m_szFiles[i] );
			item->setData( m_szDir + m_pThread->m_szFiles[i] );
			pModel->appendRow( item );
		}
	}
	else if ( m_detWatermark )
	{

	}

}

void CImageDir::on_fileThumbIcon( int iIndex, QIcon* pIcon )
{
	if ( m_pThread == nullptr ) return;
	if ( pIcon )
	{
		QStandardItemModel*	pModel	= (QStandardItemModel*)ui.listView->model();
		QStandardItem*		pItem	= pModel->item( iIndex );
		if ( pItem )
		{
			pItem->setIcon( *pIcon );
			ui.listView->setGridSize( m_siGrid );
		}
		delete pIcon;
	}
}

void CImageDir::on_listView_clicked( const QModelIndex &index )
{
	if ( index.row() >= 0 )
	{
		QStandardItemModel*	pModel	= (QStandardItemModel*)ui.listView->model();
		QImage*	image	= new QImage( pModel->item( index.row() )->data().toString() );
		if ( image )
		{
			if ( !image->isNull() )
			{
				if ( m_pImage )
					*m_pImage	= image->convertToFormat( QImage::Format_ARGB32 );
				else
					m_pImage	= new QImage( image->convertToFormat( QImage::Format_ARGB32 ) );
			}
			delete image;
		}
		if ( m_pImage )
		{
			if ( !m_detWatermark )
			{
				m_inpaint->regionReset( m_pImage->width(), m_pImage->height() );
			}
		}
		emit fileSelected( m_pImage, !m_detWatermark );
	}
}

CDirThread::CDirThread( const QString & szDir, QSize siIcon, CImageDir * parent )
	:QThread(parent)
{
	m_bExitThread	= false;
	m_siIcon		= siIcon;
	m_szDir			= szDir;
	m_parentDlg		= parent;
	QWidget::connect( this, SIGNAL( fileListDone( bool ) ), parent, SLOT( on_fileListDone( bool ) ) );
	QWidget::connect( this, SIGNAL( fileThumbIcon( int, QIcon* ) ), parent, SLOT( on_fileThumbIcon( int, QIcon* ) ) );
	start();
}

CDirThread::~CDirThread()
{
	m_bExitThread	= true;
}

void CDirThread::run()
{
	QDir	dir( m_szDir );
	QStringList filters;
	filters << "*.bmp" << "*.jpg" << "*.png";
	dir.setNameFilters( filters );
	dir.setFilter( QDir::Files | QDir::NoDotAndDotDot );
	dir.setSorting( QDir::Name | QDir::IgnoreCase );
	m_szFiles	= dir.entryList();
	emit fileListDone( false );

	QSize		sizeOld(0,0);
	for ( int32_t i = 0; i < m_szFiles.size(); ++i )
	{
		if ( m_bExitThread ) break;
		QPixmap		pixIcon( m_siIcon );
		pixIcon.fill( QColor( 0, 0, 0, 0 ) );
		QPainter	pnt( &pixIcon );
		QImage		imgFile( m_szDir + m_szFiles[i] );
		if ( imgFile.isNull() ) continue;
		if ( m_parentDlg->m_detWatermark )
		{
			if ( sizeOld.width() == 0 && sizeOld.height() == 0 )
			{
				sizeOld	= imgFile.size();
				m_parentDlg->m_inpaint->regionReset( sizeOld.width(), sizeOld.height() );
			}

			if ( sizeOld.width() == imgFile.width() && sizeOld.height() == imgFile.height() )
			{
				imgFile	= imgFile.convertToFormat( QImage::Format_ARGB32 );
				m_parentDlg->m_inpaint->regionDetection( imgFile.bits(), imgFile.bytesPerLine() );
			}
			else
			{
				m_parentDlg->m_detWatermark	= false;
			}

		}

		if ( m_bExitThread ) break;
		QSize		siPix	= imgFile.size();
		siPix.scale( m_siIcon, Qt::KeepAspectRatio );
		QRect		rtDraw( ( m_siIcon.width() - siPix.width() ) / 2, ( m_siIcon.height() - siPix.height() ) / 2, siPix.width(), siPix.height() );
		pnt.drawImage( rtDraw, imgFile );
		if ( m_bExitThread ) break;

		QIcon*	pIcon	= new QIcon( pixIcon );
		if ( pIcon->isNull() )
		{
			delete pIcon;
		}
		else
		{
			emit fileThumbIcon( i , pIcon );
		}
	}
	emit fileListDone( true );
}

void CDirThread::stop()
{
	m_bExitThread	= true;
	wait();
}
