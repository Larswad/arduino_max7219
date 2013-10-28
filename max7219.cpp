/*
code for max 7219 from maxim,
reduced and optimised for using more then one 7219 in a row,
______________________________________

 Code History:
 --------------

The original code was written for the Wiring board by:
 * Nicholas Zambetti and Dave Mellis /Interaction Design Institute Ivrea /Dec 2004
 * http://www.potemkin.org/uploads/Wiring/MAX7219.txt

First modification by:
 * Marcus Hannerstig/  K3, malm? h?gskola /2006
 * http://www.xlab.se | http://arduino.berlios.de

Following version is by:
 * tomek ness /FH-Potsdam / Feb 2007
 * http://design.fh-potsdam.de/

 * @acknowledgements: eric f.

This version with optimizations, nicer code layout and some experiments was added by
 * Lars Wadefalk, jul 2013

-----------------------------------

General notes:


- if you are only using one max7219, then use the function maxSingle to control
 the little guy ---maxSingle(register (1-8), collum (0-255))

- if you are using more then one max7219, and they all should work the same,
then use the function maxAll ---maxAll(register (1-8), collum (0-255))

- if you are using more than one max7219 and just want to change something
at one little guy, then use the function maxOne
---maxOne(Max you wane controll (1== the first one), register (1-8),
collum (0-255))

/* During initiation, be sure to send every part to every max7219 and then
 upload it.
For example, if you have five max7219's, you have to send the scanLimit 5 times
before you loadPin it-- other wise not every max7219 will get the data. the
function maxInUse  keeps track of this, just tell it how many max7219 you are
using.
*/
#include "max7219.h"

// define max7219 registers
const byte max7219_reg_noop					= 0x00;
const byte max7219_reg_digit0				= 0x01;
const byte max7219_reg_digit1				= 0x02;
const byte max7219_reg_digit2				= 0x03;
const byte max7219_reg_digit3				= 0x04;
const byte max7219_reg_digit4				= 0x05;
const byte max7219_reg_digit5				= 0x06;
const byte max7219_reg_digit6				= 0x07;
const byte max7219_reg_digit7				= 0x08;
const byte max7219_reg_decodeMode		= 0x09;
const byte max7219_reg_intensity		= 0x0a;
const byte max7219_reg_scanLimit		= 0x0b;
const byte max7219_reg_shutdown			= 0x0c;
const byte max7219_reg_displayTest	= 0x0f;


Max7219::Max7219(byte dataInPin, byte loadPin, byte clockPin, byte numMax)
	: m_dataInPin(dataInPin), m_loadPin(loadPin), m_clockPin(clockPin), m_numMax(numMax)
#ifdef SUPPORT_PERCENTAGE
	, m_percentMaxValue(100)
	, m_percentLastValue(0)
#endif
#ifdef SUPPORT_SCROLLING
	,m_scrollText(0), m_scrollIndex(0), m_currScrollPixRowCol(0), m_inverseScroll(false)
#endif
{
	pinMode(m_dataInPin, OUTPUT);
	pinMode(m_clockPin, OUTPUT);
	pinMode(m_loadPin, OUTPUT);

	digitalWrite(13, HIGH);

	// initiating the max 7219
	maxAll(max7219_reg_scanLimit, 0x07);
	maxAll(max7219_reg_decodeMode, 0x00);					// using an led matrix (not digits)
	maxAll(max7219_reg_shutdown, 0x01);						// not in shutdown mode
	maxAll(max7219_reg_displayTest, 0x00);				// no display test
	for(byte e = 1; e <= 8; e++)									// empty registers, turn all LEDs off
		maxAll(e, 0);

	setIntensity(15);
} // ctor


void Max7219::putByte(byte data) const
{
	byte i = 8, mask;
	while(i > 0) {
		mask = 0x01 << (i - 1);						// get bitmask
		digitalWrite(m_clockPin, LOW);		// tick
		digitalWrite(m_dataInPin, (data bitand mask) ? HIGH : LOW);
		digitalWrite(m_clockPin, HIGH);		// tock
		--i;															// move to lesser bit
	}
} // putByte


void Max7219::fill(byte pattern) const
{
	for(byte i = 1; i < 8; ++i)
		maxSingle(1, pattern);
} // fill


void Max7219::maxSingle(byte reg, byte col) const
{
	// maxSingle is the "easy"  function to use for a single max7219
	digitalWrite(m_loadPin, LOW);		// begin
	putByte(reg);										// specify register
	putByte(col);										//((data & 0x01) * 256) + data >> 1); // put data
	digitalWrite(m_loadPin, LOW);		// and load da shit
	digitalWrite(m_loadPin, HIGH);
} // maxSingle


void Max7219::maxAll(byte reg, byte col) const
{
	// initialize  all  MAX7219's in the system
	digitalWrite(m_loadPin, LOW);  // begin
	for(byte c = 1; c <= m_numMax; ++c) {
		putByte(reg);		// specify register
		putByte(col);		//((data & 0x01) * 256) + data >> 1); // put data
	}
	digitalWrite(m_loadPin, LOW);
	digitalWrite(m_loadPin, HIGH);
} // maxAll


void Max7219::setIntensity(byte intensity) const
{
	maxAll(max7219_reg_intensity, intensity bitand 0x0f);		// the first 0x0f is the value you can set (range: 0x00 to 0x0f)
}

void Max7219::maxOne(byte maxNr, byte reg, byte col) const
{
	// maxOne is for adressing different MAX7219's,
	// while having a couple of them cascaded

	digitalWrite(m_loadPin, LOW);  // begin

	for(char c = m_numMax; c > maxNr; --c) {
		putByte(0);    // means no operation
		putByte(0);    // means no operation
	}

	putByte(reg);		// specify register
	putByte(col);		//((data & 0x01) * 256) + data >> 1); // put data

	for(char c = maxNr - 1; c >= 1; --c) {
		putByte(0);    // means no operation
		putByte(0);    // means no operation
	}

	digitalWrite(m_loadPin, LOW); // and load da shit
	digitalWrite(m_loadPin, HIGH);
} // maxOne


#ifdef SUPPORT_PERCENTAGE

void Max7219::resetPercentage(unsigned int maxValue)
{
	m_percentMaxValue = maxValue;
	m_percentLastValue = 0;
	for(byte i = 1; i <= 8; ++i)
		maxSingle(i, 0);
} // resetPercentage

static byte ledBitArray[] PROGMEM = { 128, 192, 224, 240, 248, 252, 254, 255  };

void Max7219::showPercentage(unsigned int current)
{
	byte numLedsHigh = (byte)(64l * current / m_percentMaxValue);

	// no need to update if resulting in same amount of lit leds.
	if(numLedsHigh == m_percentLastValue)
		return;
	// remember it.
	m_percentLastValue = numLedsHigh;

	byte row = 1;

//	// don't process rows already lit.
//	while(numLedsHigh > 8) {
//		row++;
//		numLedsHigh -= 8;
//	}

	// now update the remaining leds on the current row.
	while(0 not_eq numLedsHigh) {
		maxSingle(row++, numLedsHigh < 8 ? pgm_read_byte_near(ledBitArray + numLedsHigh) : 255);
		if(numLedsHigh >= 8)
			numLedsHigh -= 8;
		else
			numLedsHigh = 0;
	}
} // showPercentage

#endif // SUPPORT_PERCENTAGE


#ifdef SUPPORT_SCROLLING

static byte scrollChar[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
extern byte parallax_font[];


void Max7219::resetScrollText(const byte* text, boolean inverse)
{
	m_scrollText = text;
	m_scrollIndex = 0;
	m_inverseScroll = inverse;
	m_currScrollPixRowCol = 0;
	// start out in a cleared state with right video fill pattern.
	fill(inverse ? 0xff : 0);
} // resetScrollText


void Max7219::doScrollLeft()
{
	// scroll the current char.
	word theChar = pgm_read_byte_near(m_scrollText + m_scrollIndex);
	if(theChar >= 64)
		theChar -= 64;
	word charOffset = theChar * 8;

	for(byte i = 0; i < 8; ++i) {
		scrollChar[i] <<= 1;
		boolean bitset = (pgm_read_byte_near(parallax_font + charOffset++) bitand (1 << (7 - m_currScrollPixRowCol))) not_eq 0;
		if(m_inverseScroll)
			bitset ^= true;
		scrollChar[i] or_eq bitset ? (1 << 0) : 0;
		maxSingle(i + 1, scrollChar[i]);
	}

	scrollNextPixRowCol();
} // doScrollLeft


void Max7219::doScrollUp()
{
	byte i = 0;
	for(; i < 7; ++i) {
		scrollChar[i] = scrollChar[i + 1];
		maxSingle(i + 1, scrollChar[i]);
	}
	word charOffset = getCharOffset(pgm_read_byte_near(m_scrollText + m_scrollIndex)) + m_currScrollPixRowCol;
	byte row = pgm_read_byte_near(parallax_font + charOffset);
	scrollChar[i] = m_inverseScroll ? ~row : row;
	maxSingle(i + 1, scrollChar[i]);

	scrollNextPixRowCol();
} // doScrollUp


void Max7219::setToCharacter(byte character, boolean inverse) const
{
	word charOffset = getCharOffset(character);
	for(byte i = 0; i < 8; ++i) {
		byte val = pgm_read_byte_near(parallax_font + charOffset++);
		maxSingle(i + 1, inverse ? ~val : val);
	}
} // setChar


///////////////////////////////////////////////////////////////////////////////////
// scroll helpers
///////////////////////////////////////////////////////////////////////////////////
void Max7219::scrollNextPixRowCol()
{
	m_currScrollPixRowCol++;
	// scrolled all 8 rows, then we need to calculate new char and restart bit position.
	if(8 == m_currScrollPixRowCol) {
		m_currScrollPixRowCol = 0;
		m_scrollIndex++;
		// null is a terminating char, if finding one restart scroll at first char.
		if(!pgm_read_byte_near(m_scrollText + m_scrollIndex))
			m_scrollIndex = 0;
	}
} // scrollNextPixRowCol


word Max7219::getCharOffset(byte theChar) const
{
	// scroll the current char.
	if(theChar >= 64)
		theChar -= 64;
	return 8 * (word)theChar;
} // getFontOffset


#endif // SUPPORT_SCROLLING

	//if you use just one MAX7219 it should look like this
//   maxSingle(1, 1);                       //  + - - - - - - -
//   maxSingle(2, 2);                       //  - + - - - - - -
//   maxSingle(3, 4);                       //  - - + - - - - -
//   maxSingle(4, 8);                       //  - - - + - - - -
//   maxSingle(5, 16);                      //  - - - - + - - -
//   maxSingle(6, 32);                      //  - - - - - + - -
//   maxSingle(7, 64);                      //  - - - - - - + -
//   maxSingle(8, 128);                     //  - - - - - - - +
	//if you use more than one MAX7219, it should look like this

	/*
	maxAll(1,1);                       //  + - - - - - - -
	maxAll(2,3);                       //  + + - - - - - -
	maxAll(3,7);                       //  + + + - - - - -
	maxAll(4,15);                      //  + + + + - - - -
	maxAll(5,31);                      //  + + + + + - - -
	maxAll(6,63);                      //  + + + + + + - -
	maxAll(7,127);                     //  + + + + + + + -
	maxAll(8,255);                     //  + + + + + + + +
	*/

	//

	// if you use more then one max7219 the second one should look like this
/*
	maxOne(2,1,1);                       //  + - - - - - - -
	maxOne(2,2,2);                       //  - + - - - - - -
	maxOne(2,3,4);                       //  - - + - - - - -
	maxOne(2,4,8);                       //  - - - + - - - -
	maxOne(2,5,16);                      //  - - - - + - - -
	maxOne(2,6,32);                      //  - - - - - + - -
	maxOne(2,7,64);                      //  - - - - - - + -
	maxOne(2,8,128);                     //  - - - - - - - +
*/
