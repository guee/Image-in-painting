#pragma once

#include <QWidget>
#include <QThread>
#include <QStandardItemModel>
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include "ui_CImageDir.h"
#include "../Inpaint/Inpaint.h"

class CDirThread;
class CImageDir : public QWidget
{
	Q_OBJECT

public:
	CImageDir(QWidget *parent = Q_NULLPTR);
	~CImageDir();

	void setInpaint( CInpaint* inpaint ) { m_inpaint = inpaint; }
	void setDir( const QString& szDir, QSize siIcon );
	const QString& dir() { return m_szDir; }
private:
	Ui::CImageDir ui;
	friend		CDirThread;
	bool		m_detWatermark;
	CInpaint*	m_inpaint;
	QIcon		m_icoBase;
	CDirThread*	m_pThread;
	QSize		m_siIcon;
	QSize		m_siGrid;
	QImage*		m_pImage;
	QString		m_szDir;
	void resizeEvent( QResizeEvent *event );
private slots:
	void on_fileListDone( bool thumbDone );
	void on_fileThumbIcon( int iIndex, QIcon* pIcon );
	void on_listView_clicked( const QModelIndex &index );
signals:
	void fileSelected( QImage* image, bool resetMask );
};

class CDirThread : public QThread
{
	Q_OBJECT
public:
	CDirThread( const QString& szDir, QSize siIcon, CImageDir* parent );
	~CDirThread();
	void stop();
	QStringList	m_szFiles;
private:
	QString		m_szDir;
	QSize		m_siIcon;
	bool		m_bExitThread;
	CImageDir*	m_parentDlg;
	virtual void run();
signals:
	void fileListDone( bool );
	void fileThumbIcon( int, QIcon* );
};