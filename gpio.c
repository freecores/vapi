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

#define ALL_ONES 0xFFFFFFFFLU
#define ALL_ZEROS 0x00000000LU


/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FILE__, __LINE__))

static void fail (char *file, int line)
{
  printf( "Test failed in %s:%i\n", file, line );
  exit( 1 );
}


static void test_registers( void )
{
}

static void test_simple_io( void )
{
  unsigned i;
  unsigned long oe;

  for ( i = 1, oe = 1; i < 31; ++ i, oe = (oe << 1) | 1 ) {
    ASSERT( vapi_read() == oe );
    vapi_write( ~oe );
    ASSERT( vapi_read() == 0 );
    vapi_write( oe );
  }
}


static void test_interrupts( void )
{
  unsigned i;

  ASSERT( vapi_read() == 0x80000000 );
  for ( i = 0; i < 31; ++ i ) {
    vapi_write( 1LU << i );
    ASSERT( vapi_read() == ((i % 2) ? 0x80000000 : 0) );
  }
}

static void test_external_clock( void )
{
  unsigned i, j, edge;
  unsigned long junk = 0xdf662b5c;

  for ( edge = 0; edge < 2; ++ edge ) {
    ASSERT( vapi_read() == 0x80000000 );
    vapi_write_with_id( 2, !edge );
    for ( i = 0; i < 31; ++ i ) {
      for ( j = 0; j < 4; ++ j, junk = junk * 0xf7b1dfbd + 0x5bf9f28b )
	vapi_write( junk );
    
      vapi_write( 1LU << i );
      vapi_write_with_id( 2, edge );
    
      ASSERT( vapi_read() == ((i % 2) ? 0x80000000 : 0) );
    
      for ( j = 0; j < 4; ++ j, junk = junk * 0xf7b1dfbd + 0x5bf9f28b )
	vapi_write( junk );
      vapi_write_with_id( 2, !edge );
    }
  }
}


static void endshake( void )
{
  ASSERT( vapi_read() == 0x12340000 );
  vapi_write( 0x00005678 );
  ASSERT( vapi_read() == 0xDeadDead );
}

int vapi_main( void )
{
  test_registers();
  test_simple_io();
  test_interrupts();
  test_external_clock();
  endshake();
  
  return 0;
}
