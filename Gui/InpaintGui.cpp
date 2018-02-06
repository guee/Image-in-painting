#include "InpaintGui.h"

InpaintGui::InpaintGui(QWidget *parent)
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

void InpaintGui::on_dirChanged( CImageDir* pImgDir, const QString& szDir )
{
}

void InpaintGui::on_actInpaint0_triggered( bool checked )
{
	ui.widgetImage->startInpaint( 0 );
}

void InpaintGui::on_actInpaint1_triggered( bool checked )
{
	ui.widgetImage->startInpaint( 1 );
}

void InpaintGui::on_actInpaint2_triggered( bool checked )
{
	ui.widgetImage->startInpaint( 2 );
}

void InpaintGui::on_actOpen_triggered( bool checked )
{
	QString	szDir = QFileDialog::getExistingDirectory( this, "Dir", ui.widgetDir->dir() );
	if ( !szDir.isEmpty() )
	{
		ui.widgetDir->setDir( szDir, QSize( 100, 100 ) );
	}
}

void InpaintGui::on_actSave_triggered( bool checked )
{
	if ( ui.widgetImage->image() == nullptr || ui.widgetImage->image()->isNull() ) return;
	QString	szFile	= QFileDialog::getSaveFileName( this, "Save", ui.widgetDir->dir(), "Image (*.png *.jpg *.bmp)" );
	if ( !szFile.isEmpty() )
	{
		ui.widgetImage->image()->save( szFile, "png" );
	}

}

void InpaintGui::on_actEnlarge_triggered( bool checked )
{
	float	oldScale	= ui.widgetImage->scale();
	ui.widgetImage->setScale( oldScale * 1.5f );
}

void InpaintGui::on_actShrink_triggered( bool checked )
{
	float	oldScale	= ui.widgetImage->scale();
	ui.widgetImage->setScale( oldScale / 1.5f );
}

void InpaintGui::on_actOriginal_triggered( bool checked )
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

void InpaintGui::on_actMove_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperNormal );
}

void InpaintGui::on_actSelRect_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperRect );
}

void InpaintGui::on_actSelPath_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperPath );
}

void InpaintGui::on_actSelPencil_toggled( bool checked )
{
	if ( checked ) ui.widgetImage->setOperType( CImageDisplay::eOperPen );
}

void InpaintGui::on_actUndo_triggered( bool checked )
{
	ui.widgetImage->undo();
}

void InpaintGui::on_actRedo_triggered( bool checked )
{
	ui.widgetImage->redo();
}

void InpaintGui::on_imageDisplay_maskChanged()
{
	int32_t		operCur	= 0;
	int32_t		operNum	= m_inpaint.regionOperNum( operCur );
	ui.actRedo->setDisabled( operCur == operNum );
	ui.actUndo->setDisabled( operCur == 0 );

}

void InpaintGui::OnInpaintProgress( int32_t progress, void* cbParam )
{
	InpaintGui*	thisPtr	= (InpaintGui*)cbParam;
	QPaintEvent*	event	= new QPaintEvent( thisPtr->ui.widgetImage->rect() );
	QCoreApplication::postEvent( thisPtr->ui.widgetImage, event );
}