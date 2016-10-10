#include <QtWidgets/QApplication>
#include "screenwidget.h"
#include <QDesktopWidget>





int main(int argc, char *argv[])
{

	QApplication a(argc, argv);

	ScreenWidget w;
    w.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint | Qt::ToolTip);



	w.resize(QApplication::desktop()->size());
	w.showFullScreen();

	w.show();
	w.grabDesktop();

	return a.exec();
}
