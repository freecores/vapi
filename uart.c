/* uart.c -- test for uart, by using VAPI
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


#include "vapi.h"

unsigned long num_vapi_ids = 1;  
  
/* UART messages */
#define TX_CMD      0xff000000
#define TX_CMD0     0x00000000
#define TX_CHAR     0x000000ff
#define TX_NBITS    0x00000300
#define TX_STOPLEN  0x00000400
#define TX_PARITY   0x00000800
#define TX_EVENP    0x00001000
#define TX_STICK    0x00002000
#define TX_BREAK    0x00004000

/* Send speed */
#define TX_CMD1     0x01000000
#define TX_SPEED    0x0000ffff

/* Send receive LCR reg */
#define TX_CMD2     0x02000000

/* Send skew */
#define TX_CMD3     0x03000000

/* Set break */
#define TX_CMD4     0x04000000
#define TX_CMD4_BREAK 0x00010000
#define TX_CMD4_DELAY 0x0000ffff

/* Check FIFO */
#define TX_CMD5     0x05000000
#define TX_CMD5_FIFOFULL 0x0000001

#define RX_CMD0     0x00000000
#define RX_PARERR   0x00010000
#define RX_FRAERR   0x00020000

/* fails if x is false */
#define ASSERT(x) ((x)?1:fail (__FUNCTION__, __LINE__))
#define MARK() printf ("Passed line %i\n", __LINE__)
#define VAPI_READ(x) {unsigned long r = vapi_read (); printf ("expected 0x%08x, read 0x%08x\n", r, (x)); ASSERT(r == (x));}

#ifndef __LINE__
#define __LINE__  0
#endif

void fail (char *func, int line)
{
#ifndef __FUNCTION__
#define __FUNCTION__ "?"
#endif
  printf ("Test failed in %s:%i\n", func, line);
  exit (1);
}
char str[5000];

/* current comm. control bits */
int control, control_rx;

void recv_char (char c)
{
  unsigned long tmp = vapi_read ();
  printf ("expected %08x, read %08x\n", control_rx | c, tmp);
  /* test if something is wrong */
  ASSERT ((tmp & 0xffffff00) == control_rx);
  tmp &= 0xff;
  if (tmp) printf ("'%c'\n", (char)tmp);
  else printf ("\\0\n");
  printf ("expected %02x, read %02x\n", c, tmp);
  ASSERT (c == (char)tmp);
}

void send_char (char c)
{
  vapi_write (c | (control & 0xffffff00));
}

char *read_string (char *s)
{
  char *t = s;
  unsigned long tmp = 1;
  while (tmp) {
    tmp = vapi_read ();
    printf ("%08x, %08x\n", tmp, control_rx);
    /* test if something is wrong */
    ASSERT ((tmp & 0xffffff00) == control_rx);
    tmp &= 0xff;
    if (tmp) printf ("'%c'\n", (char)tmp);
    else printf ("\\0\n");
    *(t++) = (char)tmp;
  }
  return s;
}


void compare_string (char *s) {
  while (*s) {
    unsigned long tmp = vapi_read ();
    /* test if something is wrong */
    ASSERT (tmp == (control_rx | *s));
    tmp &= 0xff;
    if (tmp) printf ("'%c'\n", (char)tmp);
    else printf ("\\0\n");
    s++;
  }
}

void send_string (char *s)
{
  while (*s)
    vapi_write (*(s++) | (control & 0xffffff00));
}

void init_8n1 ()
{
  vapi_write(TX_CMD1 | 2); /* Set tx/rx speed */
  control = control_rx = TX_CMD0 | TX_NBITS;
  vapi_write(TX_CMD2 | control_rx); /* Set rx mode */
}

void test_registers ()
{
  /* This test is performed just by cpu, if it is not stopped, we have an error.  */
  printf ("Testing registers... ");
  MARK();
}

void send_recv_test ()
{
  printf ("send_recv_test\n");
  MARK();
  read_string (str);
  MARK();
  printf ("OK\nread: %s\n",str);
  ASSERT (strcmp (str, "send_test_is_running") == 0);
  send_string ("recv");
  MARK();
  printf ("OK\n");
}

void break_test ()
{
  printf ("break_test\n");
  /* receive a break */
  VAPI_READ (TX_BREAK);
  MARK();
  vapi_write (control | '*');
  MARK();
  VAPI_READ (control | '!');
  MARK();
  
  /* send a break */
  vapi_write (TX_CMD4 | TX_CMD4_BREAK | (0) & TX_CMD4_DELAY);
  vapi_write (control | 'b');
  MARK();
  VAPI_READ (control | '#');
  MARK();
  vapi_write (TX_CMD4 | (0) & TX_CMD4_DELAY);
  vapi_write (control | '$');
  MARK();
  
  /* Receive a breaked string "ns<brk>*", only "ns" should be received.  */
  compare_string ("ns");
  VAPI_READ (TX_BREAK);
  send_string ("?");
  MARK();

  /* Send a break */
  VAPI_READ (control | '#');
  vapi_write(TX_CMD4 | TX_CMD4_BREAK | (5 & TX_CMD4_DELAY));
  vapi_write (control | 0);
  MARK();
#if 0
  /* Wait four chars */
  send_string ("234");
  MARK();
#endif

  /* FIFO should be empty */
  vapi_write(TX_CMD5);
  send_string ("5");
  /* FIFO should be nonempty and */
  vapi_write(TX_CMD5 | TX_CMD5_FIFOFULL); 
  MARK();
  /* it should contain '?' */
  VAPI_READ ('?' | control);
  /* Reset break signal*/
  vapi_write(TX_CMD4 | (0) & TX_CMD4_DELAY);
  MARK();
  vapi_write ('!' | control);
  printf ("OK\n");
}

/* Tries to send data in different modes in both directions */

/* Utility function, that tests current configuration */
void test_mode (int nbits)
{
  unsigned mask = (1 << nbits) - 1;
  recv_char ('U' & mask); //0x55
#if DETAILED
  recv_char ('U' & mask); //0x55
  send_char ('U');        //0x55  
#endif
  send_char ('U');        //0x55
  recv_char ('a' & mask); //0x61
#if DETAILED
  recv_char ('a' & mask); //0x61
  send_char ('a');        //0x61
#endif
  send_char ('a');        //0x61
}

void different_modes_test ()
{
  int speed, length, parity;
  printf ("different modes test\n");
  /* Init */
  /* Test different speeds */
  for (speed = 1; speed < 5; speed++) {
    vapi_write(TX_CMD1 | speed); /* Set tx/rx speed */
    control_rx = control = 0x03 << 8;    /* 8N1 */
    vapi_write(TX_CMD2 | control_rx); /* Set rx mode */
    test_mode (8);
    MARK();
  }
  MARK();
  
  vapi_write(TX_CMD1 | 1); /* Set tx/rx speed */
  MARK();
  
  /* Test all parity modes with different char lengths */
  for (parity = 0; parity < 8; parity++)
    for (length = 0; length < 4; length++) {
      control_rx = control = (length | (0 << 2) | (parity << 3)) << 8;
      vapi_write(TX_CMD2 | control_rx);
      test_mode (5 + length);
      MARK();
    }
  MARK();
  
  /* Test configuration, if we have >1 stop bits */
  for (length = 0; length < 4; length++) {
    control_rx = control = (length | (1 << 2) | (0 << 3)) << 8;
    vapi_write(TX_CMD2 | control_rx);
    test_mode (5 + length);
    MARK();
  }
  MARK();
  
  /* Restore normal mode */
  recv_char ('T'); /* Wait for acknowledge */
  vapi_write(TX_CMD1 | 2); /* Set tx/rx speed */
  control_rx = control = 0x03 << 8;    /* 8N1 @ 2*/
  vapi_write(TX_CMD2 | control_rx); /* Set rx mode */
  send_char ('x'); /* Send a character. It is possible that this char is received unproperly */
  recv_char ('T'); /* Wait for acknowledge before ending the test */
  MARK();
  printf ("OK\n");
}

/* Test various FIFO levels, break and framing error interrupt, etc */

void interrupt_test ()
{
  int i;
  vapi_write(TX_CMD1 | 6); /* Set tx/rx speed */
  printf ("interrupt_test\n");
  /* start interrupt test */
  recv_char ('I'); /* Test trigger level 1 */
  send_char ('0');
  
  recv_char ('I'); /* Test trigger level 4 */
  send_char ('1');
  send_char ('2');
  send_char ('3');
  send_char ('4');
  
  recv_char ('I'); /* Test trigger level 8 */
  send_char ('5');
  send_char ('6');
  send_char ('7');
  send_char ('8');
  send_char ('9');
  
  recv_char ('I'); /* Test trigger level 14 */
  send_char ('a');
  send_char ('b');
  send_char ('c');
  send_char ('d');
  send_char ('e');
  send_char ('f');
  send_char ('g');

  recv_char ('I'); /* Test OE */
  send_char ('h');
  send_char ('i');
  send_char ('j');
  send_char ('*'); /* should not be put in the fifo */

  recv_char ('I'); /* test break interrupt */
  /* send a break */
  vapi_write (TX_CMD4 | TX_CMD4_BREAK | (0) & TX_CMD4_DELAY);
  vapi_write (control | 'b');
  MARK();
  recv_char ('B'); /* Release break */
  MARK();
  vapi_write (TX_CMD4 | (0) & TX_CMD4_DELAY);
  vapi_write (control | '$');
  MARK();

  /* TODO: Check for parity error */
  /* TODO: Check for frame error */
  
  /* Check for timeout */
  recv_char ('I');
  send_char ('T'); /* Send char -> timeout should occur */
  recv_char ('T'); /* Wait for acknowledge before changing configuration */
  MARK ();
  
  /* Restore previous mode */
  vapi_write(TX_CMD1 | 2); /* Set tx/rx speed */
  control = control_rx = TX_CMD0 | TX_NBITS;
  vapi_write(TX_CMD2 | control_rx); /* Set rx mode */
  recv_char ('T'); /* Wait for acknowledge before ending the test */
  printf ("OK\n");
}

/* Test if all control bits are set correctly.  Lot of this was already tested
   elsewhere and tests are not duplicated.  */

void control_register_test ()
{
  printf ("control_register_test\n");
  recv_char ('*');
  send_char ('*');
  MARK ();
  recv_char ('!');
  send_char ('!');
  MARK ();
  
  recv_char ('1');
  recv_char ('*');
  send_char ('*');
  printf ("OK\n");
}

void line_error_test ()
{
  vapi_write(TX_CMD2 | control_rx); /* Set rx mode */
  recv_char ('c');
  vapi_write(TX_CMD1 | 3); /* Set incorrect tx/rx speed */
  send_char ('a');
  vapi_write(TX_CMD1 | 2); /* Set correct tx/rx speed */
  send_char ('b');
  MARK ();
  
#if COMPLETE
  recv_char ('*');
  control = TX_CMD0 | TX_NBITS;
  control_rx = TX_CMD0 | TX_NBITS | TX_STOPLEN;
  vapi_write(TX_CMD2 | control_rx); /* Set rx mode */
  recv_char ('*');
  MARK ();
#endif
}

int vapi_main ()
{
  
  /* Test section area */
  test_registers ();
  init_8n1 ();
  send_recv_test ();
  break_test();
  different_modes_test ();
  interrupt_test ();
  control_register_test ();
  line_error_test ();
  /* End of test section area */
  
  send_char ('@');
  return 0;
}
