// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtGui>
#include <QTextEdit>
#include <QStackedWidget>

#include "vdpauwidget.h"



class MainWidget : public QStackedWidget
{
	Q_OBJECT
public:
	MainWidget(QString dataDirectory);
	
private slots:
	void nextStep();

private:
	QString getCPUModel();
	QString getGPUModel();
	QTextEdit *te;
	VdpauWidget *vw;
	QLabel *lab;
	QTimer timer;
	
	int step;
};
#endif // MAINWIDGET_H
