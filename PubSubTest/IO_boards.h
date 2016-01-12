#ifndef __IO_BOARDS_H__
#define __IO_BOARDS_H__

#include <Arduino.h>

namespace IO {

	const bool INVERT = true;

	class Modules { // Класс для работы со входными/выходными модулями
	public:
		Modules(); // Конструктор по умолчанию (на данный момент практической пользы не имеет)
		Modules( // Инициализирующий конструктор
			uint8_t nOutModules, // Количество выходных модулей
			uint8_t nInModules, // Количество входных модулей
			uint8_t outStrobePin, // Номер пина, к которому подключен сигнал защелкивания выходных модулей
			uint8_t outDataPin, // Номер пина, к которому подключен сигнал данных выходных модулей
			uint8_t inStrobePin, // Номер пина, к которому подключен сигнал защелкивания входных модулей
			uint8_t inDataPin, // Номер пина, к которому подключен сигнал данных входных модулей
			uint8_t clockPin // Номер пина, к которому подключен сигнал тактирования модулей
			);
		~Modules();

		void
			refresh(), // Синхронизация входных/выходных буферов и модулей
			setOutVal( // Выставляет в выходном буфере необходимое состояние определенного пина 
				//        выходного модуля
				uint8_t module, // Номер выходного модуля (нумерация от контроллера начинаеся с нуля)
				uint8_t pin, // Номер пина выходного модуля
				bool value // Состояние выставляемое на выходе модуля
				);

		bool
			getOutVal( // Возвращает из выходного буфера состояние пина выходного модуля
				uint8_t module, // Номер выходного модуля (нумерация от контроллера начинаеся с нуля)
				uint8_t pin // Номер пина выходного модуля
				),
			getInVal( // Возвращает из входно буфера состояние пина входного модуля
				uint8_t module, // Номер входного модуля (нумерация от контроллера начинаеся с нуля)
				uint8_t pin // Номер пина входного модуля
				);

	protected:
		uint8_t
			_nOutModules, // Колличество выходных модулей
			_nInModules, // Количество входных модулей
			_nOverlapModules, // Количество модулей с которыми можно работать параллельно

			_outStrobeBit, // Битовая маска пина защелкивания выходных модулей для порта AVR
			_outDataBit, // Битовая маска пина данных выходных модулей для порта AVR
			_inStrobeBit, // Битовая маска пина защелкивания входных модулей для порта AVR
			_inDataBit, // Битовая маска пина данных входных модулей для порта AVR
			_clockBit, // Битовая маска пина тактирования модулей для порта AVR

			*_outputs, // Указатель на выходной буфер
			*_inputs; // Указатель на входной буфер

		volatile uint8_t // Указатели на регистры портов AVR необходимые для работы соответствующими пинами
			*_outStrobeOutReg, // Выходной регистр порта, содержащего пин защелкивания выходных модулей
			*_outDataOutReg, // Выходной регистр порта, содержащего пин данных выходных модулей
			*_inStrobeOutReg, // Выходной регистр порта, содержащего пин защелкивания входных модулей
			*_inDataInReg, // Входной регистр порта, содержащего пин данных входных модулей
			*_clockOutReg; // Выходной регистр порта, содержащего пин тактирования модулей
	};

	class IIn {
	public:
		virtual bool getState() = 0;
		virtual bool isChanged() = 0;
	};

	class IOut {
	public:
		virtual void setState(bool state) = 0;
		virtual bool getState() = 0;
	};

	class IExt {
	public:
		IExt(const uint8_t module, const uint8_t pin, const bool inverted);
		static Modules& _modules;

	protected:
		const uint8_t _module;
		const uint8_t _pin;
		const bool _inverted;
	};

	class ExtIn : public IIn, public IExt {
	public:
		ExtIn(const uint8_t module, const uint8_t pin, const bool inverted = false);
		bool getState();
		bool isChanged();

	protected:
		bool _state;
	};

	class ExtOut : public IOut, public IExt {
	public:
		ExtOut(const uint8_t module, const uint8_t pin, const bool inverted = false);
		void setState(bool state);
		bool getState();
	};

	class DebouncedIn : public IIn {
	public:
		DebouncedIn(IIn& in, const unsigned long delay);
		bool getState();
		bool isChanged();

	protected:
		IIn& _in;
		const unsigned long _delay;
		unsigned long _lastChangeTime;
		bool _state;
		bool _lastState;
	};

}

#endif