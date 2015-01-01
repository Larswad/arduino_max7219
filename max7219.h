#ifndef MAX_7219_H
#define MAX_7219_H

#include <Arduino.h>

#ifdef UNDER_QT
// Reason for defining this is only that Qt Creator should understand the define when browsing the source files
// It won't be defined like this when compiling under arduino tools.
#define PROGMEM
#endif

#define SUPPORT_SCROLLING
#define SUPPORT_PERCENTAGE
//#undef SUPPORT_SCROLLING
//#undef SUPPORT_PERCENTAGE

#ifdef SUPPORT_SCROLLING
extern const byte* scrollFont();
extern byte scrollFontData(const word offset);
#endif

class Max7219
{
public:
	explicit Max7219(byte dataInPin, byte loadPin, byte clockPin, byte numMax = 1);
	~Max7219() {}
	void maxSingle(byte reg, byte col) const;
	void maxOne(byte maxNr, byte reg, byte col) const;
	void maxAll(byte reg, byte col) const;
	void setIntensity(byte intensity) const; // range 0..15
	void fill(byte pattern = 0xff) const;
	void clear()
	{
		fill(0);
	}

#ifdef SUPPORT_PERCENTAGE
	void resetPercentage(unsigned int maxValue);
	void showPercentage(unsigned int current);
#endif

#ifdef SUPPORT_SCROLLING
#define PARALLAX_FONT_SIZE 512

	void resetScrollText(const byte* text, boolean inverse = false);
	void doScrollLeft();
	void doScrollUp();
	void setToCharacter(byte character, boolean inverse = false) const;
#endif

private:
	void putByte(byte data) const;
	byte m_dataInPin;
	byte m_loadPin;
	byte m_clockPin;
	// specifies how many MAX7219's you'll use
	byte m_numMax;
#ifdef SUPPORT_PERCENTAGE
	unsigned int m_percentMaxValue;
	byte m_percentLastValue;
#endif

#ifdef SUPPORT_SCROLLING
	word charOffset(byte theChar) const;
	void scrollNextPixRowCol();

	const byte* m_scrollText;
	word m_scrollIndex;
	byte m_currScrollPixRowCol;
	boolean m_inverseScroll;
#endif
};

#endif
