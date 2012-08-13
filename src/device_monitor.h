/////////////////////////////////////////////////////////////////////////////
/// @file device_monitor.h
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
#ifndef INCLUDED_DEVICE_MONITOR
#define INCLUDED_DEVICE_MONITOR

//- includes
#include "led_control_base.h"

#include <string>
#include <map>

//- forwards
struct udev;
struct udev_device;
struct udev_monitor;

/////////////////////////////////////////////////////////////////////////////
/// device monitor
class DeviceMonitor {
public:
	DeviceMonitor( );
	~DeviceMonitor( );
	
	void Init( const LedControlPtr& leds );
	void Main( );

        int numDisks()  {  return num_disks_;  }
        std::string &statsFile( int disk_idx )  {  return stats_files_[disk_idx];  }
        int ledIndex( int disk_idx )  {  return leds_idx_[disk_idx];  }
	
protected:
	void deviceAdded_( udev_device* device );
	void deviceRemove_( udev_device* device );
	void deviceChanged_( udev_device* device, bool state );
	void enumDevices_();
	int scsiHostIndex_( udev_device* device );
	bool acceptDevice_( udev_device* device );
	
	udev*			dev_context_;	///< udev library context
	udev_monitor*	dev_monitor_;	///< udev monitor context
	
	LedControlPtr	leds_;			///< led control interface

        int num_disks_;
        std::string stats_files_[10];  // each disk's stat file
        bool led_enabled_[10];  // does a particular led have a disk in the bay?
        int leds_idx_[10];      // maps disk index to led index
};

#endif // INCLUDED_DEVICE_MONITOR
