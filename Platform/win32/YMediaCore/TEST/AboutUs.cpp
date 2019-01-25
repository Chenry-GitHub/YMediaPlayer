#include "AboutUs.h"
#include <QDesktopServices>
AboutUs::AboutUs(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	QObject::connect(ui.pushButton, &QPushButton::clicked, this, [&] {
		QDesktopServices::openUrl(QUrl("https://blog.csdn.net/what951006"));
	});

}

AboutUs::~AboutUs()
{
}
