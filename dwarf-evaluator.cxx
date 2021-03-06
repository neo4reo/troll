/*
Copyright (c) 2016-2017 stoyan shopov

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
#include <QFile>
#include "dwarf-evaluator.hxx"


DwarfEvaluator::DwarfEvaluator(Sforth *sforth)
{
	this->sforth = sforth;
	QFile f(":/sforth/dwarf-evaluator.fs");
	f.open(QFile::ReadOnly);
	sforth->evaluate(f.readAll());
}

DwarfEvaluator::DwarfExpressionValue DwarfEvaluator::evaluateLocation(uint32_t cfa_value, const QString &frameBaseSforthCode, const QString &locationSforthCode)
{
	DwarfExpressionValue result;
	sforth->evaluate(QString("init-dwarf-evaluator $%1 to cfa-value").arg(cfa_value, 0, 16));
	if (!frameBaseSforthCode.isEmpty())
		sforth->evaluate(frameBaseSforthCode + " to frame-base-value ' frame-base-defined >vector frame-base-rule");
	sforth->evaluate(locationSforthCode + " expression-value-type");
	
	auto x = sforth->getResults(2);
	if (x.size() != 2)
		result.type = INVALID;
	else
		result.type = (enum DwarfExpressionType) x.at(1), result.value = x.at(0);
	return result;
}
