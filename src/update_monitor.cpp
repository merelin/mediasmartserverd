/**
 * @file     update_monitor.cpp
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
 * 2013-02-16 - Kai Hendrik Behrends
 *  - Fixed reading of update string returned by apt-check.
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

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errno_exception.h"
#include "mediasmartserverd.h"
#include "update_monitor.h"

/***********************************************************
* Public                                                   *
***********************************************************/
/**
 * Constructor (explicit).
 * Initialize the LED control interface.
 * @param leds LED control interface needed for accessing LEDs.
 * @public
 */
UpdateMonitor::UpdateMonitor(const LedControlPtr& leds)
{
	leds_ = leds;
}

/**
 * Destructor.
 * Stop the thread cleanly if not already done.
 */
UpdateMonitor::~UpdateMonitor()
{
	if(instance_started_)
	{
		Stop();
	}
}

/**
 * Starts the update monitoring thread.
 * @public
 */
void UpdateMonitor::Start()
{
	if (instance_started_)
	{
		if (verbose > 0)
		{
			std::cout << "Update monitor thread already running.\n";
		}
		return;
	}
	if (pthread_create(&monitor_thread_, NULL, MonitorThreadProc, NULL) != 0)
	{
		throw ErrnoException("pthread_create");
	}
	instance_started_ = true;
	if (verbose > 0)
	{
		std::cout << "Update monitor thread started.\n";
	}
}

/**
 * Stops the update monitoring thread.
 * @public
 */
void UpdateMonitor::Stop()
{
	if (!instance_started_)
	{
		if (verbose > 0)
		{
			std::cout << "Update monitor thread already stopped.\n";
		}
		return;
	}
	if (verbose > 0)
	{
		std::cout << "Attempting to stop update monitor thread.\n";
	}
	//Cancelling thread.
	if(pthread_cancel(monitor_thread_) != 0)
	{
		throw ErrnoException("pthread_cancel");
	}
	//Waiting for thread to join.
	if(pthread_join(monitor_thread_, NULL) != 0)
	{
		throw ErrnoException("pthread_join");
	}
	if (verbose > 0)
	{
		std::cout << "Update monitor thread stopped.\n";
	}
}

/***********************************************************
* Private                                                  *
***********************************************************/
/**
 * Cleans up after the update monitoring thread.
 * @param arg Required by definition but not used.
 * @private @static
 */
void UpdateMonitor::MonitorThreadCleanupHandler(void* /*arg*/)
{
	if (verbose > 0)
	{
		std::cout << "Cleaning up after the update monitor thread.\n";
	}
	instance_started_ = false;
	//Reset LED.
	leds_->SetSystemLed(LED_BLUE | LED_RED, false);
}

/**
 * The thread monitoring the update status and setting the system LED.
 * @param arg Required by definition but not used.
 * @private @static
 */
void* UpdateMonitor::MonitorThreadProc(void* /*arg*/)
{
	pthread_cleanup_push(MonitorThreadCleanupHandler, NULL);
	while(true)
	{
		if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) != 0)
		{
			throw ErrnoException("pthread_setcancelstate");
		}
		int update_count = -1, security_update_count = -1;
		if(!GetUpdateStatus(&update_count, &security_update_count))
		{
			break;
		}
		bool reboot_required = IsRebootRequired();
		if (verbose > 1)
		{
			std::cout << "--- Update Monitor ---\n";
			std::cout << "  Updates          : " << update_count << "\n"; 
			std::cout << "  Security Updates : " << security_update_count << "\n";
			std::cout << "  Reboot Required  : " << (reboot_required ? "YES" : "NO") << "\n";
		}
		if (reboot_required)
		{
			//Red
			leds_->SetSystemLed(LED_RED, true);
			leds_->SetSystemLed(LED_BLUE, false);
		}
		else if (security_update_count > 0)
		{
			//Purple
			leds_->SetSystemLed(LED_BLUE | LED_RED, true);
		}
		else if (update_count > 0)
		{
			//Blue
			leds_->SetSystemLed(LED_BLUE, true);
			leds_->SetSystemLed(LED_RED, false);
		}
		else //Nothing
		{
			//Off
			leds_->SetSystemLed(LED_BLUE | LED_RED, false);
		}
		if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0)
		{
			throw ErrnoException("pthread_setcancelstate");
		}
		sleep(900); //Sleep 15 minutes. //A cancellation point.
	}
	std::cout << "Update monitor thread stopped from within monitoring loop.\n";
	pthread_cleanup_pop(1); //Remove handler and execute it.
	return NULL;
}

/**
 * Checks for available system updates.
 * Uses update-notifier-common to check for updates via apt-check.
 * @param[out] update_count Returns number of avaiable updates.
 * @param[out] security_update_count Returns number of available security updates.
 * @return Returns true if information could be read.
 * @private @static
 */
bool UpdateMonitor::GetUpdateStatus(int* update_count, int* security_update_count)
{
	//For some reason stderr has the result not stdout so we redirect it to stdout.
	FILE* apt_check = popen("/usr/lib/update-notifier/apt-check 2>&1", "r");
	if (apt_check == NULL)
	{
		if (verbose > 1)
		{
			std::cout << "apt-check does not exist or can't be read.\n";
		}
		return false;
	}
	char* line = NULL;
	size_t len = 0;
	int res = getline(&line, &len, apt_check);
	pclose(apt_check);
	if (res == -1 || len < 3)
	{
		if (verbose > 1)
		{
			std::cout << "Couldn't read line. res = " << res << "; len = " << len
				<< "; line = \"" << line << "\"\n";
		}
		return false;
	}
	std::string str_line(line);
	int pos_delim = str_line.find(";");
	if (pos_delim > (int)str_line.length())
	{
		if (verbose > 1)
		{
			std::cout << "Couldn't find seperator ; in apt-check string: \""
				<< line << "\"\n";
		}
		return false;
	}
	*update_count = atoi(str_line.substr(0, pos_delim).c_str());
	*security_update_count = atoi(str_line.substr(pos_delim+1).c_str());
	return true;
}

/**
 * Checks if reboot is required.
 * @return Returns true if reboot is required.
 * @private @static
 */
bool UpdateMonitor::IsRebootRequired()
{
	std::ifstream reboot_required("/var/run/reboot-required");
	return reboot_required; //Returns whether file exists.
}

/**
 * LED control interface.
 * @private @static
 */
LedControlPtr UpdateMonitor::leds_; //Assigned in constructor.

/**
 * Helper variable keeping track of thread status.
 * @private @static
 */
bool UpdateMonitor::instance_started_ = false;
