#include "contiki.h"
#include "sys/ctimer.h"
#include "hal_lcd.h"

#include "msp430.h"
#include "font.h"

#define SPIOUT  P10OUT
#define SPIDIR  P10DIR
#define SPISEL  P10SEL

#define _SCLK	BIT0					// SPI clock
#define _SDATA	BIT4					// SPI data (sent to display)
#define _SCS	BIT3					// SPI chip select

#define MLCD_WR 0x01					// MLCD write line command
#define MLCD_CM 0x04					// MLCD clear memory command
#define MLCD_SM 0x00					// MLCD static mode command
#define MLCD_VCOM 0x02					// MLCD VCOM bit

extern void doubleWideAsm(unsigned char c, unsigned char* buff);

volatile unsigned char VCOM;			// current state of VCOM (0x04 or 0x00)

static unsigned char LineBuff[LCD_ROW/8];		// line buffer


static inline void Enable_SCS()
{
  SPIOUT |= _SCS;
}

static inline void Disable_SCS()
{
  SPIOUT &= ~_SCS;
}

void printSharp(const char* text, unsigned char line, unsigned char options);

// send one byte over SPI, does not handle SCS
// input: value		byte to be sent
void SPIWriteByte(unsigned char value)
{
    while(!(UCA3IFG & UCTXIFG));  						// wait for transfer to complete
    UCA3TXBUF = value;							// store byte
    while(!(UCA3STAT & UCBUSY));
}

// transfer line buffer to display using SPI
// input: line	position where line buffer is rendered
void SPIWriteLine(unsigned char line, unsigned char* LineBuff)
{
    SPIWriteByte(line + 1);								// send line address

    unsigned char j = 0;
    while(j < (LCD_ROW/8))								// write pixels / 8 bytes
    {
      SPIWriteByte(LineBuff[j++]);							// store low byte
    }
    
    SPIWriteByte(0);									// send 16 bit to latch buffers and end transfer        									// SCS low, finished talking to display
}

// write a string to display, truncated after 12 characters
// input: text		0-terminated string
//        line		vertical position of text, 0-95
//        options	can be combined using OR
//					DISP_INVERT	inverted text
//					DISP_WIDE double-width text (except for SPACE)
//					DISP_HIGH double-height text
void halLcdPrintXY(char text[], int col, int line, unsigned char options)
{
	Enable_SCS();								// SCS high, ready talking to display

	SPIWriteByte(MLCD_WR | VCOM);						// send command to write line(s)
	// c = char
	// b = bitmap
	// i = text index
	// j = line buffer index
	// k = char line
	unsigned char c, b, i, j, k;

	// rendering happens line-by-line because this display can only be written by line
	k = 0;
	while(k < 8 && line < LCD_COL)						// loop for 8 character lines while within display
	{
		i = 0;
		j = 0;
		while(j < (LCD_ROW/8) && (c = text[i]) != 0)	// while we did not reach end of line or string
		{
			if(c < ' ' || c > 0x7f)						// invalid characters are replace with SPACE
			{
				c = ' ';
			}

			c = c - 32;									// convert character to index in font table
			b = font8x8[(c*8)+k];						// retrieve byte defining one line of character

			if(!(options & INVERT_TEXT))				// invert bits if DISP_INVERT is _NOT_ selected
			{											// pixels are LOW active
				b = ~b;
			}

			if((options & WIDE_TEXT) && (c != 0) && (j + 1 < LCD_ROW/8))		// double width rendering if DISP_WIDE and character is not SPACE
			{
				doubleWideAsm(b, &LineBuff[j]);			// implemented in assembly for efficiency/space reasons
				j += 2;									// we've written two bytes to buffer
			}
			else										// else regular rendering
			{
				LineBuff[j] = b;						// store pixels in line buffer
				j++;									// we've written one byte to buffer
			}

			i++;										// next character
		}

		while(j < (LCD_ROW/8))							// pad line for empty characters
		{
			LineBuff[j] = 0xff;
			j++;
		}

		SPIWriteLine(line++, LineBuff);							// write line buffer

		if(options & HIGH_TEXT && line < LCD_COL)	// repeat line if DISP_HIGH is selected
		{
			SPIWriteLine(line++, LineBuff);
		}

		k++;											// next pixel line
	}

        SPIWriteByte(0);        
        Disable_SCS();
}

static void SPIInit()
{
    UCA3CTL1 = UCSWRST;

    UCA3CTL0 = UCMODE0 + UCMST + UCSYNC + UCCKPH; // master, 3-pin SPI mode, LSB
    UCA3CTL1 |= UCSSEL__SMCLK; // SMCLK for now
    UCA3BR0 = 16; // 16MHZ / 16 = 1Mhz
    UCA3BR1 = 0;

    //Configure ports.
    SPIDIR |= _SCLK | _SDATA | _SCS;
    SPISEL |= _SCLK | _SDATA;
    SPIOUT &= ~(_SCLK | _SDATA | _SCS);

    UCA3CTL1 &= ~UCSWRST;
}

static struct ctimer timer;

static void vcomswitch(void* ptr)
{
  VCOM ^= MLCD_VCOM;
  
  // put display into low-power static mode
  Enable_SCS();									// SCS high, ready talking to display
  SPIWriteByte(MLCD_SM | VCOM);					                // send static mode command
  SPIWriteByte(0);								// send command trailer
  Disable_SCS();								// SCS lo, finished talking to display
}

void halLcdClearScreen()
{
  // initialize display
  Enable_SCS();										// SCS high, ready talking to display
  SPIWriteByte(MLCD_CM | VCOM);						// send clear display memory command
  SPIWriteByte(0);									// send command trailer
  Disable_SCS();
}

void halLcdBackLightInit()
{
}

void halLcdSetBackLight(unsigned char n)
{
}

void halLcdInit()
{
  SPIInit();
  
  // wait spi stable
  __delay_cycles(100);
  
  ctimer_set(&timer, CLOCK_SECOND, vcomswitch, NULL);
  
  // enable disply
  P8DIR |= BIT1; // p8.1 is display
}

void halLcdActive()
{
    P8OUT |= BIT1; // set 1 to active display
}