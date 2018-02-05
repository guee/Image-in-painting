#include "InpaintTest.h"

InpaintTest::InpaintTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	QActionGroup*	grp	= new QActionGroup( this );
	grp->addAction( ui.actMove );
	grp->addAction( ui.actSelRect );
	grp->addAction( ui.actSelPath );
	grp->addAction( ui.actSelPencil );
	grp->setExclusive( true );
	ui.actMove->setChecked( true );
	ui.widgetDir->setInpaint( &m_inpaint );
	ui.widgetImage->setInpaint( &m_inpaint );
	connect( ui.widgetDir, SIGNAL( dirChanged( CImageDir*, const QString& ) ), this, SLOT( on_dirChanged( CImageDir*, const QString& ) ) );
	connect( ui.widgetDir, SIGNAL( fileSelected( QImage*, bool ) ), ui.widgetImage, SLOT( on_fileSelected( QImage*, bool ) ) );
	connect( ui.widgetImage, SIGNAL( maskChanged() ), this, SLOT( on_imageDisplay_maskChanged() ) );
	QString		szImgDir	= QStandardPaths::writableLocation( QStandardPaths::PicturesLocation );
	on_imageDisplay_maskChanged();
	ui.widgetDir->setDir( szImgDir, QSize( 100, 100 ) );
}

void InpaintTest::on_dirChanged( CImageDir* pImgDir, const QString& szDir )
{
}

void InpaintTest::on_actInpaint_triggered( bool checked )
{
	ui.widgetImage->startInpaint( false );
}

void InpaintTest::on_actInpaint2_triggered( bool checked )
{
	ui.widgetImage->startInpaint( true );
}

void InpaintTest::on_actOpen_triggered( bool checked )
{
	QString	szDir = QFileDialog::getExistingDirectory( this, "Dir", ui.widgetDir->dir() );
	if ( !szDir.isEmpty() )
	{
		ui.widgetDir->setDir( szDir, QSize( 100, 100 ) );
	}
}

void InpaintTest::on_actSave_triggered( bool checked )
{
	if ( ui.widgetImage->image() == nullptr || ui.widgetImage->image()->isNull() ) return;
	QString	szFile	= QFileDialog::getSaveFileName( this, "Save", ui.widgetDir->dir(), "Image (*.png *.jpg *.bmp)" );
	if ( !szFile.isEmpty() )
	{
		ui.widgetImage->image()->save( szFile, "png" );
	}

}

void InpaintTest::on_actEnlarge_triggered( bool checked )
{
	float	oldScale	= ui.widgetImage->scale();
	ui.widgetImage->setScale( oldScale * 1.5f );
}

void InpaintTest::on_actShrink_triggered( bool checked )
{
	float	oldScale	= ui.widgetImage->scale();
	ui.widgetImage->setScale( oldScale / 1.5f );
}

void InpaintTest::on_actOriginal_triggered( bool checked )
{
	if ( ui.widgetImage->fitWnd() )
	{
		ui.widgetImage->setScale( 1.0f );
	}
	else
	{
		ui.widgetImage->setFitWnd( true );
	}
}

void InpaintTest::on_actMove_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperNormal );
}

void InpaintTest::on_actSelRect_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperRect );
}

void InpaintTest::on_actSelPath_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperPath );
}

void InpaintTest::on_actSelPencil_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperPen );
}

void InpaintTest::on_actUndo_triggered( bool checked )
{
	ui.widgetImage->undo();
}

void InpaintTest::on_actRedo_triggered( bool checked )
{
	ui.widgetImage->redo();
}

void InpaintTest::on_imageDisplay_maskChanged()
{
	int32_t		operCur	= 0;
	int32_t		operNum	= m_inpaint.regionOperNum( operCur );
	ui.actRedo->setDisabled( operCur == operNum );
	ui.actUndo->setDisabled( operCur == 0 );

}

void InpaintTest::OnInpaintProgress( int32_t progress, void* cbParam )
{
	InpaintTest*	thisPtr	= (InpaintTest*)cbParam;
	QPaintEvent*	event	= new QPaintEvent( thisPtr->ui.widgetImage->rect() );
	QCoreApplication::postEvent( thisPtr->ui.widgetImage, event );
}