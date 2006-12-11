/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*----------------------------------------------------------------------------
  
	HIGH PRECISION TIMERS 

----------------------------------------------------------------------------*/

#pragma comment(lib, "winmm")

struct hr_sleep_data
{
	HANDLE event;
	MMRESULT timer;
};

static void hr_sleep_init(struct hr_sleep_data* sleep_data)
{
	timeBeginPeriod(1);
	sleep_data->event = CreateEvent(NULL, FALSE,FALSE,NULL);
	sleep_data->timer = timeSetEvent
	(
		1,
		0,
		(LPTIMECALLBACK)sleep_data->event,
		NULL,
		TIME_PERIODIC | TIME_CALLBACK_EVENT_PULSE
	);
};

static void hr_sleep_destroy(struct hr_sleep_data* sleep_data)
{
	timeEndPeriod(1);
	timeKillEvent(sleep_data->timer);
	CloseHandle(sleep_data->event);
};

static void hr_sleep(struct hr_sleep_data* sleep_data, unsigned long delay_miliseconds)
{
	long a = timeGetTime();

	while( (timeGetTime() - a) < delay_miliseconds )
		WaitForSingleObject(sleep_data->event, INFINITE);
};
