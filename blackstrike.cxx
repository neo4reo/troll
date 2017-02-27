#include <QDebug>
#include <QTime>
#include <QMessageBox>
#include "blackstrike.hxx"

#define BLACKSTIRKE_DEBUG	0

void Blackstrike::readAllRegisters()
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
int pos;
QTime t;

	t.start();
	registers.clear();
	if (port->write("swdp-scan drop gdb-attach drop\n") == -1) Util::panic();
	if (port->write(".( <<<start>>>)cr ?regs .( <<<end>>>)cr\n") == -1) Util::panic();
	do
	{
		if (port->bytesAvailable())
			s += port->readAll();
		else if (!port->waitForReadyRead(2000))
			Util::panic();
	}
	while (!s.contains("<<<end>>>"));
	if (BLACKSTIRKE_DEBUG) qDebug() << s;
	s.replace('\n', "");
	if (rx.indexIn(s) == -1)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "string recognized: " << rx.cap();
	s = rx.cap(1);
	rx.setPattern("\\s*(\\S+)");
	pos = 0;
	while ((pos = rx.indexIn(s, pos)) != -1)
	{
		bool ok;
		registers.push_back(rx.cap(1).toUInt(& ok, 16));
		if (!ok)
			Util::panic();
		if (BLACKSTIRKE_DEBUG) qDebug() << "read value" << registers.back();
		pos += rx.matchedLength();
	}
	qDebug() << "target register read took" << t.elapsed() << "milliseconds";
}

void Blackstrike::portReadyRead()
{
QString halt_reason = port->readAll();
	if (!disconnect(port, SIGNAL(readyRead()), 0, 0))
		Util::panic();
	if (halt_reason.contains("target-halted-breakpoint"))
		emit targetHalted(BREAKPOINT_HIT);
	else if (halt_reason.contains("target-halted"))
		emit targetHalted(GENERIC_HALT_CONDITION);
	else
		Util::panic();
}

QByteArray Blackstrike::interrogate(const QByteArray & query, bool *isOk)
{
	if (query.indexOf(".( <<<start>>>)") >= query.indexOf(".( <<<end>>>)"))
		Util::panic();

QByteArray s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
int l, r;
bool ok;
QTime t;

	t.start();
	if (isOk)
		* isOk = true;
	if (port->write(query + '\n') == -1) Util::panic();
	do
	{
		if (port->bytesAvailable())
			s += port->readAll();
		else if (!port->waitForReadyRead(20000))
		{
			if (isOk)
				* isOk = false;
			return "query timed out";
		}
	}
	while (!s.contains("<<<end>>>"));
	if (BLACKSTIRKE_DEBUG) qDebug() << s;
	//s.replace('\n', "");
	l = s.indexOf("<<<start>>>");
	r = s.indexOf("<<<end>>>");
	if (l >= r)
		Util::panic();
	s = s.mid(l + sizeof "<<<start>>>" - /* ignore null terminator */ 1, r - l - sizeof "<<<start>>>" + /* ignore null terminator */ 1);
	if (BLACKSTIRKE_DEBUG) qDebug() << "string recognized: " << s;
	if (BLACKSTIRKE_DEBUG) qDebug() << "target query took" << t.elapsed() << "milliseconds";
	return s;
}

bool Blackstrike::reset(void)
{
	registers.clear();
	if (!Target::interrogate(QString("target-reset .( <<<start>>>).( <<<end>>>)")).isEmpty())
		Util::panic();
	return true;
}

QByteArray Blackstrike::readBytes(uint32_t address, int byte_count, bool is_failure_allowed)
{
	/*
QString s(
" [undefined] tdump [if] : tdump ( address word-count --) 0 ?do dup t@ . 4 + loop drop ; [then] "
"$%1 $%2 .( <<<start>>>) tdump .( <<<end>>>) cr "
);
	if ((address & 3) || (byte_count & 3))
		Util::panic();
	auto x = interrogate(s.arg(address, 0, 16).arg(byte_count >> 2, 0, 16));
	qDebug() << x.length();
	*/
QTime t;
QString s(
//" [undefined] xtest [if] : xtest [ 1 16 lshift ] literal 0 do .\" ********************************\"loop ; [then] "
//" [undefined] xtest [if] : xtest [ 1 20 lshift ] literal 0 do [char] * emit loop ; [then] "
" $%1 $%2 "
" .( <<<start>>>) target-dump .( <<<end>>>) cr "
);
/*
	if ((address & 3) || (byte_count & 3))
		Util::panic();
		*/
	t.start();
	auto x = interrogate(s.arg(address, 0, 16).arg(byte_count, 0, 16).toLocal8Bit());
	qDebug() << "usb xfer speed:" << ((float) x.length() / t.elapsed()) * 1000. << "bytes/second";
	return x;
}

uint32_t Blackstrike::readWord(uint32_t address)
{
QString s;
QRegExp rx("\\s*(\\S+)");
bool ok;
uint32_t x;

	s = interrogate(QString("$%1 base @ >r hex .( <<<start>>>) t@ u. .( <<<end>>>) r> base ! cr").arg(address, 0, 16).toLocal8Bit(), & ok);
	if (!ok)
		Util::panic();

	if (rx.indexIn(s) == -1)
		Util::panic();
	x = rx.cap(1).toUInt(& ok, 16);
	if (!ok)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "read value" << x;
	return x;
}

uint32_t Blackstrike::readRawUncachedRegister(uint32_t register_number)
{
	if (register_number >= registers.size())
		readAllRegisters();
	if (register_number >= registers.size())
		Util::panic();
	return registers.at(register_number);
}

uint32_t Blackstrike::singleStep(void)
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
bool ok;
uint32_t x;
QTime t;

	t.start();

	registers.clear();
	if (port->write("base @ >r step hex .( <<<start>>>) u. .( <<<end>>>) r> base ! cr\n") == -1) Util::panic();
	do
	{
		if (port->bytesAvailable())
			s += port->readAll();
		else if (!port->waitForReadyRead(2000))
			Util::panic();
	}
	while (!s.contains("<<<end>>>"));

	s.replace('\n', "");
	if (rx.indexIn(s) == -1)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "string recognized: " << rx.cap();
	s = rx.cap(1);
	rx.setPattern("\\s*(\\S+)");
	if (rx.indexIn(s) == -1)
		Util::panic();
	x = rx.cap(1).toUInt(& ok, 16);
	if (!ok)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "target halt reason: " << x;
	qDebug() << "target single-stepping took" << t.elapsed() << "milliseconds";
	return x;
}

void Blackstrike::requestSingleStep()
{
	emit targetRunning();
	connect(port, SIGNAL(readyRead()), this, SLOT(portReadyRead()));
	registers.clear();
	if (port->write("step\n") == -1)
		Util::panic();
}

bool Blackstrike::resume()
{
	emit targetRunning();
	connect(port, SIGNAL(readyRead()), this, SLOT(portReadyRead()));
	registers.clear();
	if (port->write("target-resume\n") == -1)
		Util::panic();
	return true;
}

bool Blackstrike::requestHalt()
{
	if (port->write("\003") == -1)
		Util::panic();
	return true;
}

uint32_t Blackstrike::haltReason()
{
bool ok;
QString s;
unsigned halt_reason; 
	halt_reason = (s = interrogate(QString("?target-run-state .( <<<start>>>). .( <<<end>>>)").toLocal8Bit())).toUInt(& ok);
	if (!ok)
	{
		QMessageBox::critical(0, "cannot read target state", "cannot read target state, response is: " + s);
		Util::panic();
	}
	QMessageBox::information(0, "target halt reason", QString("target halt reason: %1").arg(halt_reason));
	return halt_reason;
}
