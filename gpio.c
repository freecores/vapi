/* gpio.c -- test for GPIO, by using VAPI
   Copyright (C) 2001, Erez Volk, erez@opencores.org

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

#include <stdio.h>

unsigned long num_vapi_ids = 8;

int vapi_main( void )
{
  vapi_write_with_id( 0, 0x01234567 );
  if ( vapi_read() != 0x89abcdef )
    return -1;
  
  return 0;
}
