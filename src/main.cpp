#include <QApplication>
#include <QDesktopWidget>
#include <QSurfaceFormat>
#include <iostream>

#include "glwidget.h"
#include "mainwindow.h"

using namespace std;

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	QCoreApplication::setApplicationName("Qt GLGame");
	QCoreApplication::setOrganizationName("WIZUT");
	QCoreApplication::setApplicationVersion(QT_VERSION_STR);

	// creates object for MainWindow class
	MainWindow mainWindow;
	mainWindow.resize(mainWindow.sizeHint()); // size of the main window (defined in GLWidget)
	mainWindow.show(); // shows main window
	mainWindow.showFullScreen(); // main window will be maximized

	return app.exec();
}