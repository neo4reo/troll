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
#ifndef GDBREMOTE_HXX
#define GDBREMOTE_HXX

#include <QByteArray>
#include <QString>
#include <QVector>
#include "util.hxx"

class GdbRemote
{
private:
	static QByteArray escape(QByteArray & data) { return data.replace('}', "}]").replace('#', "}\003").replace('$', "}\004"); }
	static QByteArray unescape(QByteArray & data) { int i; for (i = 0; i < data.length(); i ++) if (data[i] == '}') data.remove(i, 1), data[i] = data[i] ^ ' '; return data; }
	static int checksum(const QByteArray & data) { int i, x; for (i = x = 0; i < data.length(); x += data[i ++]); return x & 0xff; }
	static QByteArray makePacket(QByteArray data) { return QByteArray("$") + data + "#" + QString("%1").arg(checksum(data), 2, 16, QChar('0')).toLocal8Bit(); }
	static QByteArray strip(const QByteArray & packet) { return packet.mid(1, packet.length() - 4); }
public:
	static bool isValidPacket(const QByteArray & packet)
	{
		bool ok; int l; char c;
		return ((l = packet.length()) < 4 || packet[0] != '$' || packet[l - 3] != '#' || (c = packet.mid(l - 2).toInt(& ok, 16), ! ok)
			|| c != checksum(packet.mid(1, l - 4))) ? false : true;
	}
	static int errorCode(const QByteArray & reply) { if (!isValidPacket(reply) || reply.length() != 7 || reply[1] != 'E') return -1; return (reply.mid(2, 2).toInt(0, 16)); }
	static bool ok(const QByteArray & data) { return data.operator ==(makePacket("OK")); }
	static QByteArray readRegistersRequest(void) { return makePacket("g"); }
	static QVector<uint32_t> readRegisters(const QByteArray & reply)
	{
		QVector<uint32_t> registers;
		if (!isValidPacket(reply)) Util::panic();
		int i;
		auto x = strip(reply);
		if (x.length() & 7) Util::panic();
		for (i = 0; i < x.length() >> 3; i ++)
			registers.push_back(x.mid(i << 3, 8).toInt(0, 16));
		return registers;
	}
	static QVector<QByteArray> readMemoryRequest(uint32_t address, uint32_t length, int chunk_size)
	{
		QVector<QByteArray> packets;
		while (length)
		{
			int x = Util::min(length, (unsigned) chunk_size);
			packets.push_back(makePacket(QString("m%1,%2").arg(address, 0, 16).arg(x, 0, 16).toLocal8Bit()));
			length -= x, address += x;
		}
		return packets;
	}
	static QByteArray readMemory(const QVector<QByteArray> & reply)
	{
		QByteArray data;
		int i;
		for (i = 0; i < reply.size(); i ++)
		{
			if (reply[i][0] == 'E')
				return QByteArray();
			data += QByteArray::fromHex(reply[i]);
		}
		return data;
	}
	static QVector<QByteArray> writeFlashMemoryRequest(uint32_t address, uint32_t length, int chunk_size, const QByteArray & data)
	{
		QVector<QByteArray> packets;
		int i = 0;
		while (length)
		{
			int x = Util::min(length, (unsigned) chunk_size);
			packets.push_back(makePacket(QString("vFlashWrite:%1,%2:").arg(address, 0, 16).arg(x, 0, 16).toLocal8Bit() + data.mid(i, x)));
			length -= x, address += x, i += x;
		}
		return packets;
	}
	static QByteArray eraseFlashMemoryRequest(uint32_t address, uint32_t length) { return makePacket(QString("vFlashErase:%1,%2").arg(address, 0, 16).arg(length, 0, 16).toLocal8Bit()); }
	
};

#endif // GDBREMOTE_HXX
