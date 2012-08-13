/////////////////////////////////////////////////////////////////////////////
/// @file device_monitor.cpp
///
/// Device monitoring (disk add/removal etc)
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

//- includes
#include "device_monitor.h"
#include "errno_exception.h"
#include "mediasmartserverd.h"
#include <iostream>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
extern "C" {
#include <libudev.h>
}

/////////////////////////////////////////////////////////////////////////////
/// constructor
DeviceMonitor::DeviceMonitor( )
	:	dev_context_( 0 )
	,	dev_monitor_( 0 )
{ }
	
/////////////////////////////////////////////////////////////////////////////
/// destructor
DeviceMonitor::~DeviceMonitor( ) {
	if ( dev_context_ ) udev_unref( dev_context_ );
	if ( dev_monitor_ ) udev_monitor_unref( dev_monitor_ );
}

/////////////////////////////////////////////////////////////////////////////
/// intialise
void DeviceMonitor::Init( const LedControlPtr& leds ) {
	leds_ = leds;
	
	// get udev library context
	dev_context_ = udev_new();
	if ( !dev_context_ ) throw ErrnoException( "udev_new" );
	
	// set up udev monitor
	dev_monitor_ = udev_monitor_new_from_netlink( dev_context_, "udev" );
	if ( !dev_monitor_ ) throw ErrnoException( "udev_monitor_new_from_netlink" );
	
	// only interested in scsi devices
	if ( udev_monitor_filter_add_match_subsystem_devtype( dev_monitor_, "scsi", "scsi_device" ) ) {
		throw ErrnoException( "udev_monitor_filter_add_match_subsystem_devtype" );
	}
	
	// enumerate existing devices
	enumDevices_( );
	
	// then start monitoring
	if ( udev_monitor_enable_receiving( dev_monitor_ ) ) {
		throw ErrnoException( "udev_monitor_enable_receiving" );
	}
}

/////////////////////////////////////////////////////////////////////////////
/// main looop
void DeviceMonitor::Main( ) {
	assert( dev_monitor_ );
	
	const int fd_mon = udev_monitor_get_fd( dev_monitor_ );
	const int nfds = fd_mon + 1;
	
	sigset_t sigempty;
	sigemptyset( &sigempty );
	while ( true ) {
		fd_set fds_read;
		FD_ZERO( &fds_read );
		FD_SET( fd_mon, &fds_read );
		
		// block for something interesting to happen
		int res = pselect( nfds, &fds_read, 0, 0, 0, &sigempty );
		if ( res < 0 ) {
			if ( EINTR != errno ) throw ErrnoException( "select" );
			std::cout << "Exiting on signal\n";
			return; // signalled
		}
		
		// udev monitor notification?
		if ( FD_ISSET( fd_mon, &fds_read ) ) {
			std::tr1::shared_ptr< udev_device > device( udev_monitor_receive_device( dev_monitor_ ), &udev_device_unref );
			
			const char* str = udev_device_get_action( device.get() );
			if ( !str ) {
			} else if ( 0 == strcasecmp( str, "add" ) ) {
				deviceAdded_( device.get() );
			} else if ( 0 == strcasecmp( str, "remove" ) ) {
				deviceRemove_( device.get() );
			} else {
				if ( debug ) {
					std::cout << "action: " << str << '\n';
					std::cout << ' ' << udev_device_get_syspath(device.get()) << "' (" << udev_device_get_subsystem(device.get()) << ")\n";
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// device added
void DeviceMonitor::deviceAdded_( udev_device* device ) {
	std::cout << "ADDED: '" << udev_device_get_syspath(device) << "' (" << udev_device_get_subsystem(device) << ")\n";
	deviceChanged_( device, true );
}

/////////////////////////////////////////////////////////////////////////////
/// device removed
void DeviceMonitor::deviceRemove_( udev_device* device ) {
	std::cout << "REMOVED: '" << udev_device_get_syspath(device) << "' (" << udev_device_get_subsystem(device) << ")\n";
	deviceChanged_( device, false );
}

/////////////////////////////////////////////////////////////////////////////
/// device changed
void DeviceMonitor::deviceChanged_( udev_device* device, bool state ) {
	// find the scsi_host that device is on
	udev_device* scsi_host = udev_device_get_parent_with_subsystem_devtype( device, "scsi", "scsi_host" );
	if ( !scsi_host ) return;
	
	if ( debug ) std::cout << " scsi_host: '" << udev_device_get_syspath(scsi_host) << "' (" << udev_device_get_subsystem(scsi_host) << ")\n";
	
	// ensure that scsi_host is attached to PCI (and not say USB)
	udev_device* scsi_host_parent = udev_device_get_parent( scsi_host );
	if ( !scsi_host_parent ) return;
	
	if ( debug ) std::cout << " scsi_host_parent: '" << udev_device_get_syspath(scsi_host_parent) << "' (" << udev_device_get_subsystem(scsi_host_parent) << ")\n";
	if ( 0 != strcmp("pci", udev_device_get_subsystem(scsi_host_parent)) ) return;
	
	// system number gives us the bay
	const char* sysnum = udev_device_get_sysnum( scsi_host );
	if ( !sysnum ) return;
	
	const int led_idx = atoi( sysnum );
	if ( debug || verbose > 1 ) std::cout << " sysnum: " << sysnum << '\n';
	
	// finally we can play with the appopriate LED
	if ( leds_ ) leds_->Set( LED_BLUE, led_idx, state );
}

/////////////////////////////////////////////////////////////////////////////
/// enumerate existing devices
void DeviceMonitor::enumDevices_( ) {
	assert( dev_context_ );
	
	// create udev enumeration interface
	std::tr1::shared_ptr< udev_enumerate > dev_enum( udev_enumerate_new( dev_context_ ), &udev_enumerate_unref );
	
	// only interested in scsi_device's
	udev_enumerate_add_match_property( dev_enum.get(), "DEVTYPE", "scsi_device" );
	udev_enumerate_scan_devices( dev_enum.get() ); // start
	
	//- enumerate list
	udev_list_entry* list_entry = udev_enumerate_get_list_entry( dev_enum.get() );
	for ( ; list_entry; list_entry = udev_list_entry_get_next( list_entry ) ) {
		// retrieve device
		std::tr1::shared_ptr< udev_device > device(
			udev_device_new_from_syspath(
				udev_enumerate_get_udev( dev_enum.get() ),
				udev_list_entry_get_name( list_entry )
			), &udev_device_unref
		);
		if ( !device ) continue;
		
		deviceAdded_( device.get() );
	}
}
