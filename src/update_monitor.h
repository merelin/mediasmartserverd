/**
 * @file     update_monitor.h
 * @author   Kai Hendrik Behrends (kai.behrends@gmail.com)
 * @date     2013-02-07
 * @version  1.0
 * 
 * Monitor program that checks for updates and notifies via system LED.
 *
 *
 * Changelog:
 *
 * 2013-02-07 - Kai Hendrik Behrends
 *  - Initialy created file. 
 *
 *
 * Copyright (c) 2013 Kai Hendrik Behrends
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#ifndef UPDATE_MONITOR_H_
#define UPDATE_MONITOR_H_

#include <pthread.h>

#include "led_control_base.h"

class UpdateMonitor
{
public:
	explicit UpdateMonitor(const LedControlPtr& leds); //Prevent implicit conversions.
	~UpdateMonitor();
	void Start();
	void Stop();
private:
	static void MonitorThreadCleanupHandler (void* /*arg*/);
	static void* MonitorThreadProc (void* /*arg*/);
	static bool GetUpdateStatus(int* update_count, int* security_update_count);
	static bool IsRebootRequired();
	
	static LedControlPtr leds_;
	static bool instance_started_;
	pthread_t monitor_thread_;
	
	/**
	 * DISALLOW_COPY_AND_ASSIGN 
	 * @see http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml#Copy_Constructors
	 */
	UpdateMonitor(const UpdateMonitor&);
	void operator=(const UpdateMonitor&);
};

#endif //UPDATE_MONITOR_H_
