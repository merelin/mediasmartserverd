/////////////////////////////////////////////////////////////////////////////
/// @file led_hpex485.h
///
/// base class for LED control over systems using the SCH5127 chipset
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
#ifndef INCLUDED_LED_CONTROL_SCH5127_BASE
#define INCLUDED_LED_CONTROL_SCH5127_BASE

//- includes
#include "led_control_base.h"
#include "mediasmartserverd.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sys/io.h>

/////////////////////////////////////////////////////////////////////////////
/// base class for LED control over systems using the SCH5127 chipset
class LedControlSCH5127Base : public LedControlBase {
public:
	/// constructor
	LedControlSCH5127Base( )
		:	io_lpc_gpiobase_( 0 )
		,	io_sch5127_regs_( 0 )
	{ }
	
	/// destructor
	virtual ~LedControlSCH5127Base( ) { }
	
	/////////////////////////////////////////////////////////////////////////
	/// attempt to initialise device
	virtual bool Init( ) {
		if ( !initPciLpc_( )  ) return false;
		if ( !initSch5127_( ) ) return false;
		
		disableWatchDog_( );
		
		return true;
	}
	
protected:
	/////////////////////////////////////////////////////////////////////////
	/// IHR9 General Purpose I/O Registers
	enum {
		GPIO_USE_SEL		= 0x00,	///< GPIO Use Select
		GP_IO_SEL			= 0x04,	///< GPIO Input/Output Select
		//					= 0x08, ///< reserved
		GP_LVL				= 0x0C,	///< GPIO Level for Input or Output
		//					= 0x10, ///< reserved
		//					= 0x14, ///< reserved
		GPO_BLINK			= 0x18,	///< GPIO Blink Enable
		// GP_SER_BLINK		= 0x1C,	///< GP Serial Blink [31:0]
		// GP_SB_CMDSTS		= 0x20,	///< GP Serial Blink Command Status [31:0]
		// GP_SB_DATA		= 0x24,	///< GP Serial BLink Data [31:0]
		//					= 0x28, ///< reserved
		// GPI_INV			= 0x2C,	///< GPIO Signal Invert
		GPIO_USE_SEL2		= 0x30, ///< GPIO Use Select 2 [60:32]
		GP_IO_SEL2			= 0x34,	///< GPIO Input/Output Select 2 [60:32]
		GP_LVL2				= 0x38,	///< GPIO Level for INput or Output 2 [60:32]
		//					= 0x3C,	///< reserved
	};
	
	/// SCH5127 Runtime Registers
	enum {
		REG_GP1				= 0x4B,	///< General Purpose I/O Data Register 1
		REG_GP2				= 0x4C,	///< General Purpose I/O Data Register 2
		REG_GP3				= 0x4D,	///< General Purpose I/O Data Register 3
		REG_GP4				= 0x4E,	///< General Purpose I/O Data Register 4
		REG_GP5				= 0x4F,	///< General Purpose I/O Data Register 5
		REG_GP6				= 0x50,	///< General Purpose I/O Data Register 6
		
		REG_WDT_TIME_OUT	= 0x65,	///< Watch-dog Timeout
		REG_WDT_VAL			= 0x66,	///< Watch-dog Timer Time-out Value
		REG_WDT_CFG			= 0x67,	///< Watch-dog timer Configuration
		REG_WDT_CTRL		= 0x68,	///< Watch-dog timer Control
		
		REG_HWM_INDEX		= 0x70,	///< HWM Index Register (SCH5127 runtime register)
		REG_HWM_DATA		= 0x71,	///< HWM Data Register (SCH5127 runtime register)
	};
	
	/// SCH5127 Hardware monitoring register set
	enum {
		HWM_PWM3_DUTY_CYCLE	= 0x32,	///< PWM3 Current Duty Cycle
	};
	
	/////////////////////////////////////////////////////////////////////////
	/// is this an expected PCI device and vnedor id?
	virtual bool chkPciDeviceVendorId_( unsigned int did_vid ) const = 0;
	
	
	/////////////////////////////////////////////////////////////////////////
	/// initialise LPC Interface controller via PCI
	bool initPciLpc_( ) {
		// The LPC bridge function of the ICH9 resides in PCI Device 31:Function 0
		enum {
			CONF_VENDOR_ID	= 0x8000F800,   ///< Vendor Identification (enable, bus 0, device 31, function 0, register 0x00)
			CONF_GPIOBASE	= 0x8000F848,   ///< GPIO Base address     (enable, bus 0, device 31, function 0, register 0x48)
		};
		
		// PCI configuration space
		const unsigned int PCI_CONFIG_ADDRESS	= 0x0CF8;
		const unsigned int PCI_CONFIG_DATA		= 0x0CFC;
		
		//
		if ( ioperm(PCI_CONFIG_DATA,    4, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(PCI_CONFIG_ADDRESS, 4, 1) ) throw ErrnoException("ioperm");
		
		// retrieve vendor and device identification
		outl( CONF_VENDOR_ID, PCI_CONFIG_ADDRESS );
		const unsigned int did_vid = inl( PCI_CONFIG_DATA );
		if ( !chkPciDeviceVendorId_(did_vid) ) return false;
		
		// retrieve GPIO Base Address
		outl( CONF_GPIOBASE, PCI_CONFIG_ADDRESS );
		io_lpc_gpiobase_ = inl( PCI_CONFIG_DATA );
		
		// sanity check the address
		// (only bits 15:6 provide an address while the rest are reserved as always being zero)
		if ( 0x1 != (io_lpc_gpiobase_ & 0xFFFF007F) ) {
			if ( debug || verbose > 0 ) std::cerr << Desc() << ": Expected 0x1 but got " << (io_lpc_gpiobase_ & 0xFFFF007F) << '\n';
			return false;
		}
		io_lpc_gpiobase_ &= ~0x1; // remove hardwired 1 which indicates I/O space
		
		// finished with these ports
		ioperm( PCI_CONFIG_DATA,    4, 0 );
		ioperm( PCI_CONFIG_ADDRESS, 4, 0 );
		
		return true;
	}	
	
	/////////////////////////////////////////////////////////////////////////
	/// initialise SuperI/O for SCH5127 and retrieve runtime registers offset
	bool initSch5127_( ) {
		enum {
			IDX_LDN			= 0x07,	///< Logical Device Number
			IDX_ID			= 0x20,	///< device identification
			IDX_BASE_MSB	= 0x60,	///< base address MSB register
			IDX_BASE_LSB	= 0x61,	///< base address LSB register
			IDX_ENTER		= 0x55,	///< enter configuration mode
			IDX_EXIT		= 0xaa,	///< exit configuration mode
		};
		
		// try LPC SIO @ 0x2e
		unsigned int sio_addr = 0x2e;
		unsigned int sio_data = sio_addr + 1;
		if ( ioperm(sio_addr, 1, 1) ) throw ErrnoException("ioperm");
		if ( ioperm(sio_data, 1, 1) ) throw ErrnoException("ioperm");
		
		// enter configuration mode
		outb( IDX_ENTER, sio_addr );
		
		// retrieve identification
		outb( IDX_ID, sio_addr );
		const unsigned int device_id = inb( sio_data );
		if ( debug ) std::cout << Desc() << ": Device 0x" << std::hex << device_id << std::dec << "\n";
		
		// 
		{
			outb( 0x26, sio_addr );
			const unsigned int in = inb( sio_data );
			if ( 0x4e == in ) {
				outb( IDX_EXIT, sio_addr );
				
				// finished with these ports
				ioperm( sio_addr, 1, 0 );
				ioperm( sio_data, 1, 0 );
				
				// and switch to these if we are told to
				if ( debug ) std::cout << Desc() << ": Using 0x4e\n";
				sio_addr = 0x4e;
				sio_data = sio_addr + 1;
				
				if ( ioperm(sio_addr, 1, 1) ) throw ErrnoException("ioperm");
		        if ( ioperm(sio_data, 1, 1) ) throw ErrnoException("ioperm");
				
				outb( IDX_ENTER, sio_addr );
			}
		}
		
		// select logical device 0x0a (base address?)
		outb( IDX_LDN, sio_addr );
		outb( 0x0a, sio_data );
		
		// get base address of runtime registers
		outb( IDX_BASE_MSB, sio_addr );
		const unsigned int index_msb = inb( sio_data );
		outb( IDX_BASE_LSB, sio_addr );
		const unsigned int index_lsb = inb( sio_data );
		
		io_sch5127_regs_ = index_msb << 8 | index_lsb;
		
		// exit configuration
		outb( IDX_EXIT, sio_addr );
		
		// finished with SuperI/O ports
		ioperm(sio_data, 1, 0);
		ioperm(sio_addr, 1, 0);
		
		return true;
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// disable watchdog timer
	void disableWatchDog_( ) {
		// watchdog registers to zero out
		const int WDT_REGS[] = { REG_WDT_VAL, REG_WDT_TIME_OUT, REG_WDT_CFG, REG_WDT_CTRL };
		const size_t WDT_REGS_CNT = sizeof(WDT_REGS) / sizeof(WDT_REGS[0]);
		
		// determine the I/O range of those registers
		const int reg_min = *std::min_element( WDT_REGS, WDT_REGS + WDT_REGS_CNT );
		const int reg_max = *std::max_element( WDT_REGS, WDT_REGS + WDT_REGS_CNT );
		const int reg_cnt = reg_max - reg_min + 1;
		
		// get access to the entire range
		if ( ioperm(io_sch5127_regs_ + reg_min, reg_cnt, 1) ) throw ErrnoException("ioperm");
		
		// zero them out
		for ( size_t i = 0; i < WDT_REGS_CNT; ++i ) {
			outb( 0, io_sch5127_regs_ + WDT_REGS[i] );
		}
		
		// done
		ioperm(io_sch5127_regs_ + reg_min, reg_cnt, 0);
	}
	
	/////////////////////////////////////////////////////////////////////////
	static void setBit32_( int bit, int& bits1, int& bits2 ) {
		int& bits = (bit < 32) ? bits1 : bits2;
		bits |= 1 << bit;
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// set/clear bit state
	void doBits_( unsigned int bits, unsigned int port, bool state ) {
		const unsigned int val = inl( port );
		const unsigned int new_val = ( state )
			?	val | bits
			:	val & ~bits
		;
		if ( val != new_val ) outl( new_val, port );
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// set/clear appropriate GPIO level via io_lpc_gpiobase_
	void setGpLpcLvl_( int bit, bool state ) {
		doBits_(
			(1 << (bit % 32)),
			io_lpc_gpiobase_ + ((bit < 32) ? GP_LVL : GP_LVL2),
			state
		);
	}
	
	/////////////////////////////////////////////////////////////////////////
	/// set/clear appropriate GPIO level via io_sch5127_regs_ runtime regs
	void setGpRegsLvl_( int bit, bool state ) {
		const int reg = ((bit >> 4) & 0xF) - 1;
		assert( reg >= 0 );
		
		doBits_(
			(1 << (bit & 0xF)),
			io_sch5127_regs_ + REG_GP1 + reg,
			state
		);
	}

	/////////////////////////////////////////////////////////////////////////
	/// select specified I/Os as inputs
	void setGpioSelInput_( int bits1, int bits2 ) {
		// Use Select (0 = native function, 1 = GPIO)
		{
			const unsigned int gpio_use_sel  = io_lpc_gpiobase_ + GPIO_USE_SEL;
			const unsigned int gpio_use_sel2 = io_lpc_gpiobase_ + GPIO_USE_SEL2;
			if ( ioperm(gpio_use_sel,  4, 1) ) throw ErrnoException("ioperm");
			if ( ioperm(gpio_use_sel2, 4, 1) ) throw ErrnoException("ioperm");
			
			outl( inl(gpio_use_sel)  | bits1, gpio_use_sel  );
			outl( inl(gpio_use_sel2) | bits2, gpio_use_sel2 );
			
			ioperm( gpio_use_sel, 4, 0 );
			ioperm( gpio_use_sel2, 4, 0 );
		}
		// Input/Output select (0 = Output, 1 = Input)
		{
			const unsigned int gp_io_sel  = io_lpc_gpiobase_ + GP_IO_SEL;
			const unsigned int gp_io_sel2 = io_lpc_gpiobase_ + GP_IO_SEL2;
			if ( ioperm(gp_io_sel,  4, 1) ) throw ErrnoException("ioperm");
			if ( ioperm(gp_io_sel2, 4, 1) ) throw ErrnoException("ioperm");
			
			outl( inl(gp_io_sel)  & ~bits1, gp_io_sel  );
			outl( inl(gp_io_sel2) & ~bits2, gp_io_sel2 );
			
			ioperm( gp_io_sel, 4, 0 );
			ioperm( gp_io_sel2, 4, 0 );
		}
	}
	
	unsigned int io_lpc_gpiobase_;	///< I/O offset to LPC GPIO on the IHR9
	unsigned int io_sch5127_regs_;	///< I/O offset to SCH5127 runtime registers
};

#endif // INCLUDED_LED_CONTROL_SCH5127_BASE
