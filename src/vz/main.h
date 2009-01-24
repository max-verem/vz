/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2009 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1.tv, 2009.

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

ChangeLog:
	2009-01-24:
		*all func and variable defenition for XXXservers moved here
*/

#ifndef MAIN_H
#define MAIN_H

extern int f_exit;
extern void* scene;	// scene loaded
extern void* config;
int CMD_screenshot(char* filename,char** error_log);
int CMD_loadscene(char* filename,char** error_log);

#endif /* MAIN_H */
