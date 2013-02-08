/////////////////////////////////////////////////////////////////////////////
/// @file mediasmartserverd.cpp
///
/// Daemon for controlling the LEDs on the HP MediaSmart Server EX48X
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
/// Added system vendor and product name detection to get_led_interface().
/// Neccessary for adding H341 without breaking HPEX485.
/// Disabled SystemLed.

//- includes
#include "errno_exception.h"
#include "device_monitor.h"
#include "led_acerh340.h"
#include "led_acerh341.h"
#include "led_hpex485.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <stdio.h>

#include <getopt.h>
#include <libudev.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

using std::cout;

//- globals
int debug = 0;		///< show debug messages
int verbose = 0;	///< how much debugging we spew out
bool activity = 0;	///< do we make the lights blink?



/////////////////////////////////////////////////////////////////////////////
/// our signal handler
static void sig_handler( int ) { }

/////////////////////////////////////////////////////////////////////////////
/// register signal handlers
void init_signals( ) {
	struct sigaction sa;
	sa.sa_flags = 0;
	sa.sa_handler = &sig_handler;
	sigemptyset( &sa.sa_mask );
	if ( -1 == sigaction(SIGINT,  &sa, 0) ) throw ErrnoException( "sigaction(SIGINT)"  );
	if ( -1 == sigaction(SIGTERM, &sa, 0) ) throw ErrnoException( "sigaction(SIGTERM)" );
}

/////////////////////////////////////////////////////////////////////////////
/// drop root priviledges
void drop_priviledges( ) {
	const passwd* pw = getpwnam( "nobody" );
	if ( !pw ) return; // :(
	
	setgid( pw->pw_gid );
	setuid( pw->pw_uid );
}

/////////////////////////////////////////////////////////////////////////////
// get attribute from udev device specified by subsytem an device name
const char *GetUdevDeviceAttribute(const char *subsystem, const char *sysname, const char *sysattr) {
	struct udev *udev;
	struct udev_device *device;
	const char *attributeValue;

	udev = udev_new();
	if (!udev) throw ErrnoException("udev_new");

	device = udev_device_new_from_subsystem_sysname(udev, subsystem, sysname);
	attributeValue = strdup(udev_device_get_sysattr_value(device, sysattr));
	//free
	udev_device_unref(device);
	udev_unref(udev);
	
	return attributeValue;
}

/////////////////////////////////////////////////////////////////////////////
/// attempt to get an LED control interface
LedControlPtr get_led_interface( ) {
	LedControlPtr control;
	
	const char *systemVendor = GetUdevDeviceAttribute("dmi", "id", "sys_vendor");
	const char *productName = GetUdevDeviceAttribute("dmi", "id", "product_name");
	
	if(verbose > 0) cout << "--- SystemVendor: \"" << systemVendor
		<< "\" - ProductName: \"" << productName << "\" ---\n";
	
	if (strcmp(systemVendor, "Acer") == 0) {
		if(verbose > 0) cout << "Recognized SystemVendor: \"Acer\"\n";
		if (strcmp(productName, "Aspire easyStore H340") == 0) {
			// H340
			if(verbose > 0) cout << "Recognized ProductName: \"Aspire easyStore H340\"\n";
			control.reset( new LedAcerH340 );
			if ( control->Init( ) ) return control;
		}
		if (strcmp(productName, "Aspire easyStore H341") == 0) {
			// H341
			if(verbose > 0) cout << "Recognized ProductName: \"Aspire easyStore H341\"\n";
			control.reset( new LedAcerH341 );
			if ( control->Init( ) ) return control;
		}
	}
	else { // HP // If you have the system vendor and product name please add.
		// HP48X
		if(verbose > 0) cout << "Unrecognized SystemVendor: \"" 
			<< systemVendor << "\"\nAssuming HP\n";
		control.reset( new LedHpEx48X );
		if ( control->Init( ) ) return control;
	}
	//free
	free((char*)systemVendor);
	free((char*)productName);
	return LedControlPtr( );
}

/////////////////////////////////////////////////////////////////////////////
/// show command line help
int show_help( ) {
	cout << "Usage: mediasmartserverd [OPTION]...\n"
		<< "     --brightness=X    Set LED brightness (1 to 10)\n"
		<< " -D, --daemon          Detach and run in the background\n"
		<< " -a, --activity        Use the bay lights as disk activity lights\n"
		<< "     --debug           Print debug messages\n"
		<< "     --help            Print help text\n"
		<< " -v, --verbose         verbose (use twice to be more verbose)\n" 
		<< " -V, --version         Show version number\n" 
	;
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
/// show version
int show_version( ) {
	cout << "mediasmartserverd 0.0.1 compiled on " __DATE__ " " __TIME__ "\n";
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
/// run a light show
int run_light_show( const LedControlPtr& leds, int light_show ) {
	
	int light_leds = 0;
	size_t show_mode = 0;
	if ( 1 == light_show ) {
		// holiday lights
		srand( time(0) );
	} else {
		show_mode = (light_show - 2) % 4 + 1;
		switch ( (light_show - 2) / 4 ) {
		default:
		case 0: light_leds = LED_BLUE; break;
		case 1: light_leds = LED_RED;  break;
		case 2: light_leds = LED_BLUE | LED_RED; break;
		}
	}
	
	
	size_t state = 0;
	
	sigset_t sigempty;
	sigemptyset( &sigempty );
	while ( true ) {
		
		switch ( show_mode ) {
		case 0: // holiday lights
		{
			for ( size_t i = 0; i < 4; ++i ) {
				switch ( rand() % 4 ) {
				default:
				case 0: light_leds = 0; break;
				case 1: light_leds = LED_BLUE; break;
				case 2: light_leds = LED_RED;  break;
				case 3: light_leds = LED_BLUE | LED_RED; break;
				}
				
				leds->Set(  light_leds, i, true );
				leds->Set( ~light_leds, i, false );
			}
			break;
		}
		case 1: // descending chasers
		{
			for ( size_t i = 0; i < 4; ++i ) leds->Set( light_leds, i, (i == (3 - state)) );
			if ( ++state >= 4 ) state = 0;
			break;
		}
		case 2: // ascending chasers
		{
			for ( size_t i = 0; i < 4; ++i ) leds->Set( light_leds, i, (i == state) );
			if ( ++state >= 4 ) state = 0;
			break;
		}
		case 3: // knight rider
		{
			const size_t sel = ( state < 3 ) ? state : 6 - state;
			for ( size_t i = 0; i < 4; ++i ) leds->Set( light_leds, i, (i == sel) );
			if ( ++state >= 6 ) state = 0;
			break;
		}
		case 4: // pulsing
		{
			for ( size_t i = 0; i < 4; ++i ) leds->Set( light_leds, i, true );
			const size_t sel = 1 + ( ( state < 9 ) ? state : 16 - state );
			leds->SetBrightness( sel );
			if ( ++state >= 16 ) state = 0;
			break;
		}
		default:
			cout << "Unsupported light show\n";
			return 1;
		}
		
		
		// wait a bit
		struct timespec timeout = { 0, 200000000 };
		int res = pselect( 0, 0, 0, 0, &timeout, &sigempty );
		if ( res < 0 ) {
			if ( EINTR != errno ) throw ErrnoException( "select" );
			cout << "Exiting on signal\n";
			break; // signalled
		}
	}
	
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
/// main entry point
int main( int argc, char* argv[] ) try {
	int brightness = -1;
	int light_show = 0;
	int mount_usb = -1;
	bool run_as_daemon = false;
	bool xmas = false;
	
	// long command line arguments
	const struct option long_opts[] = {
		{ "brightness", required_argument,	0, 'b' },
		{ "daemon",		no_argument,		0, 'D' },
		{ "activity",		no_argument,		0, 'a' },
		{ "debug",		no_argument,		0, 'd' },
		{ "help",		no_argument,		0, 'h' },
		{ "light-show",	required_argument,	0, 'S' },
		{ "usb",		required_argument,	0, 'U' },
		{ "verbose",	no_argument,		0, 'v' },
		{ "version",	no_argument,		0, 'V' },
		{ "xmas",		no_argument,		0, 'X' },
		{ 0, 0, 0, 0 },
	};
	
	// pass command line arguments
	while ( true ) {
		const int c = getopt_long( argc, argv, "DvaV", long_opts, 0 );
		if ( -1 == c ) break;
		
		switch ( c ) {
		case 'b': // brightness
			if ( optarg ) brightness = atoi( optarg );
			break;
		case 'd': // debug
			++debug;
			break;
		case 'D': // run as a daemon (background)
			run_as_daemon = true;
			break;
		case 'a': // run as a daemon (background)
			activity = true;
			break;
		case 'h': // help!
			return show_help( );
		case 'S': // light-show
			if ( optarg ) light_show = atoi( optarg );
			break;
		case 'U': // mount/unmount USB device
			if ( optarg ) mount_usb = atoi( optarg );
			break;
		case 'v': // verbose, more verbose, even more verbose
			++verbose;
			break;
		case 'V': // our version
			return show_version( );
		case 'X': // light all the LEDs up like a xmas tree
			xmas = true;
			break;
		case '?': // no idea
			cout << "Try `" << argv[0] << " --help' for more information.\n";
			return 1;
		default:
			cout << "+++ '" << (char)c << "'\n";
		}
	}
	
	
	// register signal handlers
	init_signals( );
	
	// find led control interface
	LedControlPtr leds = get_led_interface( );
	if ( !leds ) throw std::runtime_error( "Failed to find an LED control interface" );
	
	// drop root priviledges
	drop_priviledges( );
	
	// mount USB?
	if ( mount_usb >= 0 ) {
		if ( debug || verbose > 0 ) cout << ( (mount_usb) ? "M" : "Unm" ) << "ounting USB device\n";
		leds->MountUsb( !!mount_usb );
	}
	
	// run as a daemon?
	if ( run_as_daemon && daemon( 0, 0 ) ) throw ErrnoException( "daemon" );
	
	cout << "Found: " << leds->Desc( ) << '\n';
	
	// disable annoying blinking guy // changed to disabling completely
	leds->SetSystemLed( LED_RED, false );
	leds->SetSystemLed( LED_BLUE, false );
	
	//
	if ( brightness >= 0 ) leds->SetBrightness( brightness );
	
	// clear out LEDs
	leds->Set( LED_BLUE | LED_RED, 0, xmas );
	leds->Set( LED_BLUE | LED_RED, 1, xmas );
	leds->Set( LED_BLUE | LED_RED, 2, xmas );
	leds->Set( LED_BLUE | LED_RED, 3, xmas );
	if ( xmas ) return 0;
	
	if ( light_show > 0 ) return run_light_show( leds, light_show );
	
	// initialise device monitor
	DeviceMonitor device_monitor;
	device_monitor.Init( leds );
	
	// begin monitoring
	device_monitor.Main( );
	
	// re-enable annoying blinking // leaving it disabled
	//leds->SetSystemLed( LED_BLUE, LED_BLINK );
    for( int i = 0; i < device_monitor.numDisks(); ++i )
    {
        int led_idx = device_monitor.ledIndex(i);
        leds->Set( LED_RED, led_idx, false );
        leds->Set( LED_BLUE, led_idx, false );
    }
	
	return 0;
	
} catch ( std::exception& e ) {
	std::cerr << e.what() << '\n';
	if ( 0 != getuid() ) cout << "Try running as root\n";
	
	return 1;
}
