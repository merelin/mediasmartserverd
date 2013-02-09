/////////////////////////////////////////////////////////////////////////////
/// @file led_acerh341.h
///
/// LED control for Acer Aspire easyStore H341
///
/// -------------------------------------------------------------------------
///
/// Copyright (c) 2009-2010 Chris Byrne
/// 
/// This software is provided 'as-is', without any express or implied
/// warranty. In no event will the authors be held liable for any damages
/// arising from the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose,
/// including commercial applications, and to alter it and redistribute it
/// freely, subject to the following restrictions:
/// 
/// 1. The origin of this software must not be misrepresented; you must not
/// claim that you wrote the original software. If you use this software
/// in a product, an acknowledgment in the product documentation would be
/// appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, and must not
/// be misrepresented as being the original software.
/// 
/// 3. This notice may not be removed or altered from any source
/// distribution.
///
/////////////////////////////////////////////////////////////////////////////

/// Changelog - 2012-02-04 - Kai Hendrik Behrends
///
/// Copy of src/led_acerh340.h created and modified for the Acer Aspire easyStore H341 according to the post
/// by sparkvolt on http:mediasmartserver.net as seen here:
/// http://www.mediasmartserver.net/forums/viewtopic.php?f=23&t=7720&sid=5f7634569b817c79e9fff21012e74f46&start=90#p94053


#ifndef INCLUDED_LED_ACERH341
#define INCLUDED_LED_ACERH341

//- includes
#include "led_control_sch5127_base.h"
#include "mediasmartserverd.h"


/////////////////////////////////////////////////////////////////////////////
/// LED control for Acer Aspire easyStore H341
class LedAcerH341 : public LedControlSCH5127Base {
public:
	/// constructor
	LedAcerH341( ) { }
	
	/// destructor
	virtual ~LedAcerH341( ) { }
	
	/////////////////////////////////////////////////////////////////////////
	const char* Desc( ) const { return "Acer Aspire easyStore H341";		}
	
	/////////////////////////////////////////////////////////////////////////
	/// attempt to initialise device
	virtual bool Init( ) {
		// initialise SCH5127
		if ( !LedControlSCH5127Base::Init() ) return false;
		
		// set up io permissions to other ports we may use
		if ( ioperm(io_sch5127_regs_ + REG_HWM_INDEX, 1, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_HWM_DATA,  1, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_GP1,       4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_GP2,       4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_GP3,       4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_GP4,       4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_GP5,       4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_sch5127_regs_ + REG_GP6,       4, 1) ) throw ErrnoException("ioperm");
		
		//
		if ( ioperm(io_lpc_gpiobase_ + GPO_BLINK,	4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_lpc_gpiobase_ + GP_IO_SEL,	4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_lpc_gpiobase_ + GP_IO_SEL2,	4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_lpc_gpiobase_ + GP_LVL,		4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(io_lpc_gpiobase_ + GP_LVL2,		4, 1) ) throw ErrnoException("ioperm");
		
		enableLeds_( );
		
		return true;
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// set system LED (off, on, or blink)
	virtual void SetSystemLed( int led_type, LedState state ) {
		const bool on_off_state = ( LED_ON == state );
		if ( led_type & LED_BLUE ) setGpLpcLvl_( OUT_SYSTEM_BLUE, !on_off_state );
		if ( led_type & LED_RED  ) setGpLpcLvl_( OUT_SYSTEM_RED,  !on_off_state );
		
		const bool blink_state  = ( LED_BLINK == state );
		int val = 0;
		if ( led_type & LED_BLUE ) val |= 1 << OUT_SYSTEM_BLUE;
		if ( led_type & LED_RED  ) val |= 1 << OUT_SYSTEM_RED;
		if ( val ) doBits_( val, io_lpc_gpiobase_ + GPO_BLINK, blink_state );
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// (un)mount USB device
	virtual void MountUsb( bool state ) {
		setGpLpcLvl_( OUT_USB_DEVICE, state );
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// set brightness level
	/// @param val Brightness level from 0 to 9
	virtual void SetBrightness( int val ) {
		static const unsigned char LED_BRIGHTNESS[] = {
			0x00, 0xbe, 0xc3, 0xcb, 0xd3, 0xdb, 0xe3, 0xeb, 0xf3, 0xff
		};
		val = std::max( 0, std::min<int>( val, sizeof(LED_BRIGHTNESS) / sizeof(LED_BRIGHTNESS[0]) - 1 ) );
		
		outb( HWM_PWM3_DUTY_CYCLE, io_sch5127_regs_ + REG_HWM_INDEX );
		outb( LED_BRIGHTNESS[val], io_sch5127_regs_ + REG_HWM_DATA  );
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// control leds
	/// @param led_type LED type to turn on/off LED_BLUE, LED_RED, LED_BLUE | LED_RED
	/// @param led_idx Which LED to turn on/off (0 -> 3)
	/// @param state Whether we are turning LED on (true) or off (false)
	virtual void Set( int led_type, size_t led_idx, bool state ) {
		if ( led_idx >= MAX_HDD_LEDS ) return;
		
		if ( led_type & LED_BLUE ) setGpRegsLvl_( ioLedBlue_(led_idx), state );
		if ( led_type & LED_RED  ) setGpRegsLvl_( ioLedRed_(led_idx),  state );
	}
	
protected:
	/////////////////////////////////////////////////////////////////////////
	/// mappings for LEDs /// changed for h341
	enum {
		OUT_BLUE0       = 0x4b,
        OUT_BLUE1       = 0x4c,
        OUT_BLUE2       = 0x52,
        OUT_BLUE3       = 0x50,

        OUT_RED0        = 0x59,
        OUT_RED1        = 0x58,
        OUT_RED2        = 0x4e,
        OUT_RED3        = 0x51,

        //-
        OUT_USB_DEVICE	= 0x06, ///< bit 6

        OUT_USB_LED     = 0x12,
        OUT_POWER       = 0x1b,
        OUT_SYSTEM_RED  = 0x18,
        OUT_SYSTEM_BLUE = 0x0A,
	};
	
	static const size_t MAX_HDD_LEDS = 4;
	
	
	/////////////////////////////////////////////////////////////////////////
	/// is this an expected PCI device and vnedor id?
	virtual bool chkPciDeviceVendorId_( unsigned int did_vid ) const {
		/// changed for h341
		return ( 0x29168086 == did_vid );
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// blue LED mappings
	int ioLedBlue_( size_t led_idx ) const {
		const int IO_LEDS_BLUE[] = { OUT_BLUE0, OUT_BLUE1, OUT_BLUE2, OUT_BLUE3 };
		assert( led_idx < sizeof(IO_LEDS_BLUE) / sizeof(IO_LEDS_BLUE[0]) );
		return IO_LEDS_BLUE[ led_idx ];
	}
	/////////////////////////////////////////////////////////////////////////
	/// red LED mappings
	int ioLedRed_( size_t led_idx ) const {
		const int IO_LEDS_RED[] = { OUT_RED0, OUT_RED1, OUT_RED2, OUT_RED3 };
		assert( led_idx < sizeof(IO_LEDS_RED) / sizeof(IO_LEDS_RED[0]) );
		return IO_LEDS_RED[ led_idx ];
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// enable LEDs
	void enableLeds_( ) {
		// work out which bits we need
		int bits1 = 0, bits2 = 0;
		setBit32_( OUT_USB_DEVICE,	bits1, bits2 );
		setBit32_( OUT_USB_LED,		bits1, bits2 );
		setBit32_( OUT_POWER,		bits1, bits2 );
		setBit32_( OUT_SYSTEM_BLUE,	bits1, bits2 );
		setBit32_( OUT_SYSTEM_RED,	bits1, bits2 );
		
		setGpioSelInput_( bits1, bits2 );
		
		//TODO: SCH5127 bits
	}
};

#endif // INCLUDED_LED_ACERH341
