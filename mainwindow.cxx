#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include "libtroll.hxx"

#include <QByteArray>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	QFile debug_file("blackmagic");
	if (!debug_file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(0, "error opening target executable", QString("error opening file ") + debug_file.fileName());
		exit(1);
	}
	debug_file.seek(debug_aranges_offset);
	QByteArray debug_aranges = debug_file.read(debug_aranges_len);
	
	DwarfData dwdata(debug_aranges.data(), debug_aranges.length());
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata.compilation_unit_count()));
}

MainWindow::~MainWindow()
{
	delete ui;
}
