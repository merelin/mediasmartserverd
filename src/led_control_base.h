/////////////////////////////////////////////////////////////////////////////
/// @file led_control_base.h
///
/// base class for LED control
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
#ifndef INCLUDED_LED_CONTROL_BASE
#define INCLUDED_LED_CONTROL_BASE

//- includes
#include <tr1/memory>

//- constants
enum {
	LED_BLUE	= 1 << 0,
	LED_RED		= 1 << 1,
};

enum LedState {
	LED_OFF		= 1 << 0,
	LED_ON		= 1 << 1,
	LED_BLINK	= 1 << 2,
};

/////////////////////////////////////////////////////////////////////////////
/// base class for LED control (if we support anything more than the 48x)
class LedControlBase {
public:
	virtual ~LedControlBase( ) { }

	virtual const char* Desc( ) const = 0;
	virtual bool Init( ) = 0;
	
	virtual void MountUsb( bool state ) = 0;
	virtual void Set( int led_type, size_t led_idx, bool state ) = 0;
	virtual void SetBrightness( int val ) = 0;
	virtual void SetSystemLed( int led_type, LedState state ) = 0;
	
	/// wrapper if someone gives us a bool
	virtual void SetSystemLed( int led_type, bool state ) {
		SetSystemLed( led_type, ( state ) ? LED_ON : LED_OFF );
	}
	
protected:
	LedControlBase( ) { }
	
private:
	// no copying
	LedControlBase( const LedControlBase& rhs );
	const LedControlBase& operator=( const LedControlBase& rhs );
};
typedef std::tr1::shared_ptr< LedControlBase > LedControlPtr;

#endif // INCLUDED_LED_CONTROL_BASE
