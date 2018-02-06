#pragma once

#include <QWidget>
#include <QtGui>
#include "ui_InpaintGui.h"
#include "CImageDir.h"
#include "../Inpaint/Inpaint.h"


class InpaintGui : public QMainWindow
{
	Q_OBJECT

public:
	InpaintGui(QWidget *parent = Q_NULLPTR);

private:
	Ui::InpaintTestClass ui;
	CInpaint	m_inpaint;
	static void OnInpaintProgress( int32_t progress, void* cbParam );
private slots:
	void on_dirChanged( CImageDir* pImgDir, const QString& szDir );
	void on_actOpen_triggered( bool checked = false );
	void on_actSave_triggered( bool checked = false );
	void on_actEnlarge_triggered( bool checked = false );
	void on_actShrink_triggered( bool checked = false );
	void on_actOriginal_triggered( bool checked = false );
	void on_actInpaint0_triggered( bool checked = false );
	void on_actInpaint1_triggered( bool checked = false );
	void on_actInpaint2_triggered( bool checked = false );
	void on_actMove_toggled( bool checked = false );
	void on_actSelRect_toggled( bool checked = false );
	void on_actSelPath_toggled( bool checked = false );
	void on_actSelPencil_toggled( bool checked = false );
	void on_actUndo_triggered( bool checked = false );
	void on_actRedo_triggered( bool checked = false );

	void on_imageDisplay_maskChanged();

};
