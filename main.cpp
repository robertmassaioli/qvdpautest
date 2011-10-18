// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <QtGui>

#include "mainwidget.h"



int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	
	MainWidget *mw = new MainWidget();

	mw->show();

	return app.exec();
}
