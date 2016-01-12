#include <Arduino.h>

#include "IO_boards.h"

using namespace IO;

Modules::Modules() : // Конструктор по умолчанию (на данный момент не имеет практической пользы)
	_nOutModules(0),
	_nInModules(0),
	_nOverlapModules(0),
	_outStrobeBit(0),
	_outDataBit(0),
	_inStrobeBit(0),
	_inDataBit(0),
	_clockBit(0),
	_outputs(0),
	_inputs(0),
	_outStrobeOutReg(0),
	_outDataOutReg(0),
	_inStrobeOutReg(0),
	_inDataInReg(0),
	_clockOutReg(0)
{
	// empty
}

Modules::Modules( // Инициализирующий конструктор
	uint8_t nOutModules, // Количество выходных модулей
	uint8_t nInModules, // Количество входных модулей
	uint8_t outStrobePin, // Номер пина, к которому подключен сигнал защелкивания выходных модулей
	uint8_t outDataPin, // Номер пина, к которому подключен сигнал данных выходных модулей
	uint8_t inStrobePin, // Номер пина, к которому подключен сигнал защелкивания входных модулей
	uint8_t inDataPin, // Номер пина, к которому подключен сигнал данных входных модулей
	uint8_t clockPin // Номер пина, к которому подключен сигнал тактирования модулей
) :
	_nOutModules(nOutModules),
	_nInModules(nInModules)
{
	// Преобразование номеров Arduino пинов в указатели на регистры и битовые маски
	// соответствующих пинов AVR
	_outStrobeBit = digitalPinToBitMask(outStrobePin);
	uint8_t port = digitalPinToPort(outStrobePin);
	volatile uint8_t * outStrobeModeReg = portModeRegister(port);
	_outStrobeOutReg = portOutputRegister(port);

	_outDataBit = digitalPinToBitMask(outDataPin);
	port = digitalPinToPort(outDataPin);
	volatile uint8_t * outDataModeReg = portModeRegister(port);
	_outDataOutReg = portOutputRegister(port);

	_inStrobeBit = digitalPinToBitMask(inStrobePin);
	port = digitalPinToPort(inStrobePin);
	volatile uint8_t * inStrobeModeReg = portModeRegister(port);
	_inStrobeOutReg = portOutputRegister(port);

	_inDataBit = digitalPinToBitMask(inDataPin);
	port = digitalPinToPort(inDataPin);
	volatile uint8_t * inDataModeReg = portModeRegister(port);
	_inDataInReg = portInputRegister(port);
	volatile uint8_t * inDataOutReg = portOutputRegister(port);

	_clockBit = digitalPinToBitMask(clockPin);
	port = digitalPinToPort(clockPin);
	volatile uint8_t * clockModeReg = portModeRegister(port);
	_clockOutReg = portOutputRegister(port);

	// Начальная конфигурация необходимых входов/выходов AVR
	uint8_t oldSREG = SREG;
	cli();

	*outStrobeModeReg |= _outStrobeBit;
	*_outStrobeOutReg |= _outStrobeBit;

	*outDataModeReg |= _outDataBit;
	*_outDataOutReg &= ~_outDataBit;

	*inStrobeModeReg |= _inStrobeBit;
	*_inStrobeOutReg |= _inStrobeBit;

	*inDataModeReg &= ~_inDataBit;
	*inDataOutReg |= _inDataBit;

	*clockModeReg |= _clockBit;
	*_clockOutReg &= ~_clockBit;

	SREG = oldSREG;

	// Вычисление колличества входных/выходных модулей, скоторыми можно работать одновременно
	if (_nOutModules < _nInModules) {
		_nOverlapModules = _nOutModules;
	}
	else {
		_nOverlapModules = _nInModules;
	}

	// Выделение и инициализация памяти под хранение состояния входных/выходных модулей
	_outputs = new uint8_t[_nOutModules];
	memset(_outputs, 0, _nOutModules);
	_inputs = new uint8_t[_nInModules];
}

Modules::~Modules() {
	delete[] _outputs;
	delete[] _inputs;
}

void Modules::refresh () { // Синхронизация входных/выходных буферов и модулей
	uint8_t 
		outBuf, // Буфер хранения записываемого в выходные модули байта
		inBuf, // Буфер хранения считываемого из входных модулей байта
		i = 0; // Счетчик циклов

	uint8_t oldSREG = SREG;
	cli();
	// Защелкивание входов на сдвиговый регистр
	*_inStrobeOutReg &= ~_inStrobeBit;
	*_inStrobeOutReg |= _inStrobeBit;

	// Цикл чтения/записи байт которые мы можем обработать параллельно
	while (i < _nOverlapModules) {
		outBuf = _outputs[i];
		inBuf = 0;
		// Чтение и запись байта
		for (uint8_t j = 0; j < 8; j++) {
			// Запись бита в выходной модуль
			if (outBuf & 0x80) {
				*_outDataOutReg |= _outDataBit;
			}
			else {
				*_outDataOutReg &= ~_outDataBit;
			}
			outBuf = (outBuf << 1);
			// Чтение бита из входных модулей
			inBuf = (inBuf << 1);
			if ((*_inDataInReg) & _inDataBit) {
				inBuf |= 1;
			}
			// Тактирование
			*_clockOutReg |= _clockBit;
			*_clockOutReg &= ~_clockBit;
		}
		_inputs[i] = inBuf;
		i++;
	}

	// Добиваем остатки по выходам, если таковые имеются
	while (i < _nOutModules) {
		outBuf = _outputs[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (outBuf & 0x80) {
				*_outDataOutReg |= _outDataBit;
			}
			else {
				*_outDataOutReg &= ~_outDataBit;
			}
			*_clockOutReg |= _clockBit;
			*_clockOutReg &= ~_clockBit;
			outBuf = (outBuf << 1);
		}
		i++;
	}

	// Защелкиваем сдвиговые регистры на выхода
	*_outStrobeOutReg |= _outStrobeBit;
	*_outStrobeOutReg &= ~_outStrobeBit;

	// Добиваем остатки по входам если таковые имеются
	while (i < _nInModules) {
		inBuf = 0;
		for (uint8_t j = 0; j < 8; j++) {
			inBuf = (inBuf << 1);
			if ((*_inDataInReg) & _inDataBit) {
				inBuf |= 1;
			}
			*_clockOutReg |= _clockBit;
			*_clockOutReg &= ~_clockBit;
		}
		_inputs[i] = inBuf;
		i++;
	}
	SREG = oldSREG;
}

void Modules::setOutVal( // Выставляет в выходном буфере необходимое состояние определенного
					  // пина выходного модуля
	uint8_t module, // Номер выходного модуля (нумерация от контроллера начинаеся с нуля)
	uint8_t pin, // Номер пина выходного модуля
	bool value // Состояние выставляемое на выходе модуля
) {
	if (module < _nOutModules && pin < 8) {
		module = _nOutModules - module - 1;
		if (value) {
			_outputs[module] |= (1 << pin);
		}
		else {
			_outputs[module] &= ~(1 << pin);
		}
	}	
}

bool Modules::getOutVal( // Возвращает из выходного буфера состояние пина выходного модуля
	uint8_t module, // Номер выходного модуля (нумерация от контроллера начинаеся с нуля)
	uint8_t pin // Номер пина выходного модуля
) {
	if (module < _nOutModules && pin < 8) {
		module = _nOutModules - module - 1;
		return (_outputs[module] & (1 << pin));
	}
	return false;
}

bool Modules::getInVal( // Возвращает из входно буфера состояние пина входного модуля
	uint8_t module, // Номер входного модуля (нумерация от контроллера начинаеся с нуля)
	uint8_t pin // Номер пина входного модуля
) {
	if (module < _nInModules && pin < 8)
		if (_inputs[module] & (1 << pin))
			return true;
	return false;
}


IExt::IExt(const uint8_t module, const uint8_t pin, const bool inverted) :
	_module(module), _pin(pin), _inverted(inverted) {
	// empty
}


ExtIn::ExtIn(const uint8_t module, const uint8_t pin, const bool inverted) :
	IExt(module, pin, inverted), _state(false) {
	// empty
}

bool ExtIn::getState() {
	_state = _modules.getInVal(_module, _pin);
	return (_inverted ? !_state : _state);
}

bool ExtIn::isChanged() {
	return (_state != _modules.getInVal(_module, _pin));
}


ExtOut::ExtOut(const uint8_t module, const uint8_t pin, const bool inverted) :
	IExt(module, pin, inverted) {
	_inverted ? _modules.setOutVal(_module, _pin, true) : _modules.setOutVal(_module, _pin, false);
}

void ExtOut::setState(bool state) {
	_inverted ? _modules.setOutVal(_module, _pin, !state) : _modules.setOutVal(_module, _pin, state);
}

bool ExtOut::getState() {
	return (_inverted ? !_modules.getOutVal(_module, _pin) : _modules.getOutVal(_module, _pin));
}

DebouncedIn::DebouncedIn(IIn& in, const unsigned long delay) :
	_in(in), _delay(delay) {
	_lastChangeTime = millis();
	_state = _in.getState();
	_lastState = _state;
}

bool DebouncedIn::getState() {
	return _state;
}

bool DebouncedIn::isChanged() {
	bool currentState = _in.getState();

	if (_lastState != currentState) {
		_lastChangeTime = millis();
	}

	_lastState = currentState;

	if ((millis() - _lastChangeTime) > _delay) {
		if (currentState != _state) {
			_state = currentState;
			return true;
		}
	}
	
	return false;
}