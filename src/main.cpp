// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <QtGui>
#include "mainwidget.h"

int main(int argc, char **argv)
{
   QApplication app(argc, argv);

   QString directoryString("");
   if(argc > 1) {
      directoryString.append(argv[1]);
   } else {
      directoryString.append("./");
   }

   if(!directoryString.endsWith("/")) {
      directoryString.append("/");
   }

   MainWidget *mw = new MainWidget(directoryString);

   mw->show();

   return app.exec();
}
