/*
Copyright (c) 2017 stoyan shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include "blackmagic.hxx"
#include <QFile>
#include <QTime>
#include <QDialog>
#include <QMessageBox>
#include "memory.hxx"
#include "gdb-remote.hxx"

void Blackmagic::readAllRegisters(void)
{
	putPacket(GdbRemote::readRegistersRequest());
	auto x = getPacket();
	if (GdbRemote::isErrorResponse(x))
		Util::panic();
	registers = GdbRemote::readRegisters(x);
}

void Blackmagic::putPacket(const QByteArray &request)
{
char c;
	port->write(request);
	if (!port->waitForBytesWritten(1000))
		Util::panic();
	while ((c = getChar()) != '+')
		qDebug() << "!!!!!" << c;//Util::panic();
}

QByteArray Blackmagic::getPacket()
{
QByteArray packet("$");
	while (getChar() != '$')
		;
	while (1)
	{
		packet += getChar();
		if (packet[packet.length() - 1] == '#')
			break;
	}
	packet += getChar();
	packet += getChar();
	port->write("+");
	if (!port->waitForBytesWritten(1000))
		Util::panic();
	qDebug() << "received gdb packet:" << packet;
	return packet;
}

char Blackmagic::getChar()
{
char c;
	while (!port->bytesAvailable())
		port->waitForReadyRead(10);
	if (port->read(& c, 1) != 1)
		Util::panic();
	return c;
}

void Blackmagic::portReadyRead()
{
auto halt_reason = getPacket();
	qDebug() << "halt reason: " << halt_reason;
	if (!disconnect(port, SIGNAL(readyRead()), 0, 0))
		Util::panic();
	if (GdbRemote::packetData(halt_reason) == "T05"
		|| GdbRemote::packetData(halt_reason) == "T02")
		emit targetHalted(GENERIC_HALT_CONDITION);
	else
		Util::panic();
}

bool Blackmagic::reset()
{
	registers.clear();
	putPacket(GdbRemote::resetRequest());
	return true;
}

QByteArray Blackmagic::readBytes(uint32_t address, int byte_count, bool is_failure_allowed)
{
QVector<QByteArray> r;
int i;
	auto m = GdbRemote::readMemoryRequest(address, byte_count);
	for (r.clear(), i = 0; i < m.size(); i ++)
	{
		putPacket(m[i]);
		r += getPacket();
	}
	for (i = 0; i < r.size(); i ++)
		if (GdbRemote::isErrorResponse(r[i]))
			if (!is_failure_allowed)
			{
				throw MEMORY_READ_ERROR;
				Util::panic();
			}
			else
				return QByteArray();
	return GdbRemote::readMemory(r);
}

uint32_t Blackmagic::readRawUncachedRegister(uint32_t register_number)
{
	if (register_number >= registers.size())
		readAllRegisters();
	if (register_number >= registers.size())
		Util::panic();
	return registers.at(register_number);
}

bool Blackmagic::breakpointSet(uint32_t address, int length)
{
	putPacket(GdbRemote::setHardwareBreakpointRequest(address, length));
	return GdbRemote::isOkResponse(getPacket()) ? true : false;
}

bool Blackmagic::breakpointClear(uint32_t address, int length)
{
	putPacket(GdbRemote::removeHardwareBreakpointRequest(address, length));
	return GdbRemote::isOkResponse(getPacket()) ? true : false;
}

void Blackmagic::requestSingleStep(void)
{
	emit targetRunning();
	registers.clear();
	putPacket(GdbRemote::singleStepRequest());
	if (!QObject::connect(port, SIGNAL(readyRead()), this, SLOT(portReadyRead())))
		Util::panic();
	/*! \todo	WARNING - it seems that I am doing something wrong with the 'readyRead()' signal;
	 *		without the call to 'waitForReadyRead()' below, on one machine that I am testing on,
	 *		incoming data from the blackmagic probe sometimes does not cause the 'readyRead()'
	 *		signal to be emitted, which breaks the code badly; I discovered through trial and
	 *		error that the call to 'WaitForReadyRead()' function below seems to work around this issue
	 * 		on some (but not all) machines that I tested on
	 *
	 * 		the same call is also currently present in the main troll front-end code, in file 'troll.cxx',
	 * 		where waitForReadyRead() is being periodically called when the target is running - while the
	 * 		call there is in my tests sufficient to work around the issue, the duplication here makes
	 * 		the troll more responsive on some machines; tests concluded that just the call here (and without
	 * 		periodically calling waitForReadyRead()) ***DOES NOT WORK*** in all occasions, so the redundant
	 * 		call here is intended only to make the troll more responsive on some machines
	 */
	port->waitForReadyRead(1);
}

bool Blackmagic::resume(void)
{
	emit targetRunning();
	registers.clear();
	putPacket(GdbRemote::continueRequest());
	if (!QObject::connect(port, SIGNAL(readyRead()), this, SLOT(portReadyRead())))
		Util::panic();
	/*! \todo	WARNING - it seems that I am doing something wrong with the 'readyRead()' signal;
	 *		without the call to 'waitForReadyRead()' below, on one machine that I am testing on,
	 *		incoming data from the blackmagic probe sometimes does not cause the 'readyRead()'
	 *		signal to be emitted, which breaks the code badly; I discovered through trial and
	 *		error that the call to 'WaitForReadyRead()' function below seems to work around this issue
	 * 		on some (but not all) machines that I tested on
	 *
	 * 		the same call is also currently present in the main troll front-end code, in file 'troll.cxx',
	 * 		where waitForReadyRead() is being periodically called when the target is running - while the
	 * 		call there is in my tests sufficient to work around the issue, the duplication here makes
	 * 		the troll more responsive on some machines; tests concluded that just the call here (and without
	 * 		periodically calling waitForReadyRead()) ***DOES NOT WORK*** in all occasions, so the redundant
	 * 		call here is intended only to make the troll more responsive on some machines
	 */
	port->waitForReadyRead(1);
	return true;
}

bool Blackmagic::requestHalt()
{
	if (port->write("\003") == -1)
		Util::panic();
	return true;
}

bool Blackmagic::connect(void)
{
	QByteArray packet = GdbRemote::monitorRequest("swdp_scan");
	int i;
	QVector<QByteArray> r, s;

	if (!port->setDataTerminalReady(true))
		Util::panic();
	port->write("+++");
	port->waitForBytesWritten(1000);
	port->readAll();
	qDebug() << "--------------> +++";
	do
		port->waitForReadyRead(1000);
	while (port->readAll() > 0);
	
	putPacket(packet);
	do
		r.push_back(getPacket());
	while (!GdbRemote::isOkResponse(r.back()));

	for (i = 0; i < r.size() - 1; i ++)
	{
		auto s = GdbRemote::packetData(r[i]);
		if (s[0] == 'O')
		{
			qDebug() << QByteArray::fromHex(s.mid(1));
		}
	}
	putPacket(GdbRemote::attachRequest());
	if (GdbRemote::packetData(getPacket()) != "T05")
		Util::panic();
	return true;
}

QByteArray Blackmagic::memoryMap(void)
{
	putPacket(GdbRemote::memoryMapReadRequest());
	return GdbRemote::memoryMapReadData(getPacket());
}

bool Blackmagic::syncFlash(const Memory &memory_contents)
{
	QTime t;
	int i;
	QString s;
	uint32_t total;
	memory_contents.dump();
	if (memory_contents.isMemoryMatching(this))
		return true;
	auto ranges = flashAreasForRange(memory_contents.ranges[0].address, memory_contents.ranges[0].data.size());
	if (memory_contents.ranges.size() != 1 || ranges.empty())
		Util::panic();
	QDialog dialog;
	Ui::Notification mbox;
	mbox.setupUi(& dialog);
	dialog.setWindowTitle("erasing flash");
	t.start();
	for (total = i = 0; i < ranges.size(); i ++)
	{
		mbox.label->setText(QString("erasing flash at start address $%1, size $%2").arg(ranges[i].first, 0, 16).arg(ranges[i].second, 0, 16));
		dialog.show();
		QApplication::processEvents();
		
		putPacket(GdbRemote::eraseFlashMemoryRequest(ranges[i].first, ranges[i].second));
		if (!GdbRemote::isOkResponse(getPacket()))
		{
			QMessageBox::critical(0, "error erasing flash", QString("error erasing $%1 bytes of flash at address $%2").arg(ranges[i].second, 0, 16).arg(ranges[i].first, 0, 16));
			Util::panic();
		}
		total += ranges[i].second;
	}
	qDebug() << "flash erase speed" << QString("%1 bytes per second").arg((float) (total * 1000.) / t.elapsed());
	dialog.setWindowTitle("writing flash");
	mbox.label->setText(QString("writing $%1 bytes to flash at start address $%2")
	                    .arg(memory_contents.ranges[0].data.size(), 0, 16)
	                .arg(memory_contents.ranges[0].address, 0, 16));
	QApplication::processEvents();

	t.restart();
	auto r = GdbRemote::writeFlashMemoryRequest(memory_contents.ranges[0].address, memory_contents.ranges[0].data.size(), memory_contents.ranges[0].data);
	for (i = 0; i < r.size(); i ++)
	{
		putPacket(r[i]);
		if (!GdbRemote::isOkResponse(getPacket()))
		{
			QMessageBox::critical(0, "error writing flash", QString("error writing $%1 bytes of flash at address $%2").arg(memory_contents.ranges[0].data.size(), 0, 16).arg(memory_contents.ranges[0].address, 0, 16));
			Util::panic();
		}
	}
	qDebug() << "flash write speed" << QString("%1 bytes per second").arg((float) (memory_contents.ranges[0].data.size() * 1000.) / t.elapsed());
	return memory_contents.isMemoryMatching(this);
}
