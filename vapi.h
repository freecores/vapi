/* vapi.h - Verification API Interface
   Copyright (C) 2001, Marko Mlinar, markom@opencores.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Writes an unsigned long to server */
void vapi_write(unsigned long data) {

/* Reads an unsigned long from server */
unsigned long vapi_read();

/* Polls the port if anything is available and if do_read is set something is read from port. */
int vapi_waiting ();

/* Main function, actually the testcase, defined by VAPI user.  */
extern void vapi_main();
