#include <QFile>

#include "target.hxx"
#include "util.hxx"
#include "target-corefile.hxx"

TargetCorefile::TargetCorefile(const QString & rom_filename, uint32_t rom_base_address, const QString & ram_filename, uint32_t ram_base_address, const QString register_filename)
{
	this->rom_base_address = rom_base_address;
	this->ram_base_address = ram_base_address;
	QFile f;
	f.setFileName(rom_filename);
	if (f.open(QFile::ReadOnly))
		rom = f.readAll(), f.close();
	f.setFileName(ram_filename);
	if (f.open(QFile::ReadOnly))
		ram = f.readAll(), f.close();
	f.setFileName(register_filename);
	if (f.open(QFile::ReadOnly))
		register_file = f.readAll(), f.close();
}


uint32_t TargetCorefile::readWord(uint32_t address)
{
	if (rom_base_address <= address && address < rom_base_address + rom.length() - sizeof(uint32_t) + 1)
		return * (uint32_t *) (rom.data() + address - rom_base_address);
	if (ram_base_address <= address && address < ram_base_address + ram.length() - sizeof(uint32_t) + 1)
		return * (uint32_t *) (ram.data() + address - ram_base_address);
	Util::panic();
}

uint32_t TargetCorefile::readRegister(uint32_t register_number)
{
	if (register_number < register_file.length() / sizeof(uint32_t))
		return * (uint32_t *) (register_file.data() + register_number * sizeof(uint32_t));
	Util::panic();
}