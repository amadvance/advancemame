/**********************************************************************

    Motorola 6850 ACIA interface and emulation

    This function is a simple emulation of a pair of MC6850
    Asynchronous Communications Interface Adapters.

**********************************************************************/

#include "driver.h"
#include "6850acia.h"

UINT8 m6850_irq_state; // referenced from machine/mpu4.c
void update_mpu68_interrupts(void); // referenced from machine/mpu4.c

#define LOG_SERIAL	  // log serial communication
#define  UART_LOG //enable UART data logging
#define LOG_CTRL	  // log serial communication

static int  get_normal_uart_status(void);	// retrieve status of uart on MPU4 board
int uart1_status;	  // MC6850 status
int uart2_status;	  // MC6850 status
int vid_data_from_norm;	// data available for vid from MPU4
int vid_normdata;			// data
int norm_data_from_vid;	// data available for MPU4 from vid
int norm_viddata;			// data
int vid_acia_triggered; // flag <>0, ACIA receive IRQ
int vid_uart_status;
int norm_uart_status;
int aciadata;
int ctrl;

int vid_rx;

void send_to_vid(int data)
{
  vid_data_from_norm = 1;		// set flag, data from MPU4 board available
  vid_normdata       = data;	// store data

  vid_uart_status |= 0x83; //TX full, RX empty, IRQ

  vid_acia_triggered = 1;		// set flag, acia IRQ triggered

  m6850_irq_state = 1;
  update_mpu68_interrupts();
  //cpunum_set_input_line(1, 2, HOLD_LINE );    // trigger IRQ

  #ifdef LOG_SERIAL
  logerror("svid  %02X  (%c)\n",data, data );
  #endif
}

///////////////////////////////////////////////////////////////////////////

int read_from_norm(void)
{
  int data = vid_normdata;

  vid_data_from_norm = 0;	  // clr flag,

  #ifdef LOG_SERIAL
  logerror("rnorm:%02X  (%c)\n",data, data );
  #endif

  return data;
}

///////////////////////////////////////////////////////////////////////////

int  read_from_vid(void)
{
  int data = norm_viddata;

  norm_data_from_vid = 0;	  // clr flag

  #ifdef LOG_SERIAL
  logerror("rvid:  %02X(%c)\n",data, data );
  #endif

  return data;
}

///////////////////////////////////////////////////////////////////////////

void send_to_norm(int data)
{
  norm_data_from_vid = 1;		// set flag, data from adder available
  norm_viddata       = data;	// store data

  norm_uart_status |= 0x83; //TX full, RX empty, IRQ

  #ifdef LOG_SERIAL
  logerror("snorm %02X(%c)\n",data, data );
  #endif
}

///////////////////////////////////////////////////////////////////////////

int get_vid_uart_status(void)
{

//  if ( vid_data_from_norm ) vid_uart_status |= 0x01;  // receive  buffer full
//  if ( !norm_data_from_vid ) vid_uart_status |= 0x02; // transmit buffer empty

  #ifdef LOG_SERIAL
 logerror("Vid control status(%02X)\n",vid_uart_status);
  #endif

  return vid_uart_status;
}

///////////////////////////////////////////////////////////////////////////

static int get_normal_uart_status(void)
{

//  if ( norm_data_from_vid  ) norm_uart_status |= 0x01; // receive  buffer full
//  if ( !vid_data_from_norm) norm_uart_status |= 0x02;  // transmit buffer empty

 #ifdef LOG_SERIAL
 logerror("MPU4 control status(%02X)\n",norm_uart_status);
  #endif
  return norm_uart_status;
}

///////////////////////////////////////////////////////////////////////////
// serial port ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( uart1stat_r )
{
  return uart1_status;
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( uart1data_r )
{
{
if (norm_uart_status & 0x01)
{
//  int data = read_from_vid();
}
uart1_status &= 0x7e; // Clear the IRQ and the RDR Full

  return 0x06; //for now
}
}
///////////////////////////////////////////////////////////////////////////

WRITE8_HANDLER( uart1ctrl_w )
{
      switch (data & 0x03)
      {
      case 0x00:
         logerror("#1 Uart clock: x1\n");
         break;
      case 0x01:
         logerror("#1 Uart clock: x16\n");
         break;
      case 0x02:
         logerror("#1 Uart clock: x32\n");
         break;
      case 0x03:
	     logerror("Resetting #1 Uart\n");
	     uart1_status = 0x02;
         break;
      }
      switch (data & 0x1c)
      {
      case 0x00:
         logerror("#1 Uart - Bits :7 Parity: Even Stop Bits: 2\n");
         break;
      case 0x04:
         logerror("#1 Uart - Bits :7 Parity: Odd Stop Bits: 2\n");
         break;
      case 0x08:
         logerror("#1 Uart - Bits :7 Parity: Even Stop Bits: 1\n");
         break;
      case 0x0c:
         logerror("#1 Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      case 0x10:
         logerror("#1 Uart - Bits :8 Parity: None Stop Bits: 2\n");
         break;
      case 0x14:
         logerror("#1 Uart - Bits :8 Parity: None Stop Bits: 1\n");
         break;
      case 0x18:
         logerror("#1 Uart - Bits :8 Parity: Even Stop Bits: 1\n");
         break;
      case 0x1c:
         logerror("#1 Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      }
      switch (data & 0x60)
      {
      case 0x00:
         logerror("#1 Uart - /RTS=0, TX IRQ=0\n");
         break;
      case 0x20:
         logerror("#1 Uart - /RTS=0, TX IRQ=1\n");
         break;
      case 0x40:
         logerror("#1 Uart - /RTS=1, TX IRQ=0\n");
         break;
      case 0x60:
         logerror("#1 Uart - /RTS=0, TX break level, TX IRQ=0\n");
         break;
      }
      if (data & 0x80)
      {
         logerror("#1 Uart - RX IRQ=1\n");
      }
      else
      {
         logerror("#1 Uart - RX IRQ=0\n");
      }
#ifdef LOG_SERIAL
logerror("#1 Uart control reg (%02X)\n",data);
#endif

   }




///////////////////////////////////////////////////////////////////////////

WRITE8_HANDLER( uart1data_w )
{
uart1_status &= 0x7d;// Clear the IRQ and the TDR Empty
  #ifdef UART_LOG
  logerror("uart1:%c\n", data);
  #endif
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( uart2stat_r )
{
  return uart2_status;
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( uart2data_r )
{
if (norm_uart_status & 0x01)
	{
    //int data = read_from_vid();
	}
uart2_status &= 0x7e; // Clear the IRQ and the RDR Full
    return 0x06;
}

///////////////////////////////////////////////////////////////////////////

WRITE8_HANDLER( uart2ctrl_w )
{
      switch (data & 0x03)
      {
      case 0x00:
         logerror("#2 Uart clock: x1\n");
         break;
      case 0x01:
         logerror("#2 Uart clock: x16\n");
         break;
      case 0x02:
         logerror("#2 Uart clock: x32\n");
         break;
      case 0x03:
	     logerror("Resetting #2 Uart\n");
	     uart2_status = 0x02;
         break;
      }

      switch (data & 0x1c)
      {
      case 0x00:
         logerror("#2 Uart - Bits :7 Parity: Even Stop Bits: 2\n");
         break;
      case 0x04:
         logerror("#2 Uart - Bits :7 Parity: Odd Stop Bits: 2\n");
         break;
      case 0x08:
         logerror("#2 Uart - Bits :7 Parity: Even Stop Bits: 1\n");
         break;
      case 0x0c:
         logerror("#2 Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      case 0x10:
         logerror("#2 Uart - Bits :8 Parity: None Stop Bits: 2\n");
         break;
      case 0x14:
         logerror("#2 Uart - Bits :8 Parity: None Stop Bits: 1\n");
         break;
      case 0x18:
         logerror("#2 Uart - Bits :8 Parity: Even Stop Bits: 1\n");
         break;
      case 0x1c:
         logerror("#2 Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      }

      switch (data & 0x60)
      {
      case 0x00:
         logerror("#2 Uart - /RTS=0, TX IRQ=0\n");
         break;
      case 0x20:
         logerror("#2 Uart - /RTS=0, TX IRQ=1\n");
         break;
      case 0x40:
         logerror("#2 Uart - /RTS=1, TX IRQ=0\n");
         break;
      case 0x60:
         logerror("#2 Uart - /RTS=0, TX break level, TX IRQ=0\n");
         break;
      }

      if (data & 0x80)
      {
         logerror("#2 Uart - RX IRQ=1\n");
      }
      else
      {
         logerror("#2 Uart - RX IRQ=0\n");
      }

#ifdef LOG_SERIAL
logerror("#2 Uart control reg (%02X)\n",data);
#endif
}

///////////////////////////////////////////////////////////////////////////

WRITE8_HANDLER( uart2data_w )
{
uart2_status = uart2_status & 0x7d;// Clear the IRQ and the TDR Empty
#ifdef UART_LOG
logerror("uart2:%c\n", data);
#endif
}

///////////////////////////////////////////////////////////////////////////

WRITE8_HANDLER( mpu4_uart_tx_w )
{
norm_uart_status = norm_uart_status & 0xfd;// Clear the IRQ and the TDR Empty
#ifdef LOG_SERIAL
logerror("transmitting to VidCard (%02X)\n",data);
#endif
  send_to_vid(data);
}

///////////////////////////////////////////////////////////////////////////

WRITE8_HANDLER( mpu4_uart_ctrl_w )
{

int ctrl;

ctrl = data;

if ((ctrl & 0x03) == 0x03)
   {
      logerror("Resetting norm Uart\n");
      norm_uart_status = 0x02;
   }
   else
   {
         switch (ctrl & 0x03)
      {
      case 0x00:
         logerror("norm Uart clock: x1\n");
         break;
      case 0x01:
         logerror("norm Uart clock: x16\n");
         break;
      case 0x02:
         logerror("norm Uart clock: x32\n");
         break;
      }

      switch (ctrl & 0x1c)
      {
      case 0x00:
         logerror("norm Uart - Bits :7 Parity: Even Stop Bits: 2\n");
         break;
      case 0x04:
         logerror("norm Uart - Bits :7 Parity: Odd Stop Bits: 2\n");
         break;
      case 0x08:
         logerror("norm Uart - Bits :7 Parity: Even Stop Bits: 1\n");
         break;
      case 0x0c:
         logerror("norm Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      case 0x10:
         logerror("norm Uart - Bits :8 Parity: None Stop Bits: 2\n");
         break;
      case 0x14:
         logerror("norm Uart - Bits :8 Parity: None Stop Bits: 1\n");
         break;
      case 0x18:
         logerror("norm Uart - Bits :8 Parity: Even Stop Bits: 1\n");
         break;
      case 0x1c:
         logerror("norm Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      }

      switch (ctrl & 0x60)
      {
      case 0x00:
         logerror("norm Uart - /RTS=0, TX IRQ=0\n");
         break;
      case 0x20:
         logerror("norm Uart - /RTS=0, TX IRQ=1\n");
         break;
      case 0x40:
         logerror("norm Uart - /RTS=1, TX IRQ=0\n");
         break;
      case 0x60:
         logerror("norm Uart - /RTS=0, TX break level, TX IRQ=0\n");
         break;
      }

      if (ctrl & 0x80)
      {
         logerror("norm Uart - RX IRQ=1\n");
      }
      else
      {
         logerror("norm Uart - RX IRQ=0\n");
      }
   }


#ifdef LOG_SERIAL
logerror("MPU4 control reg (%02X)\n",data);
#endif
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( mpu4_uart_rx_r )
{
if (norm_uart_status & 0x01)
{
  aciadata = read_from_vid();
}
norm_uart_status = norm_uart_status & 0x7e; // Clear the IRQ and the RDR Full

#ifdef LOG_SERIAL
logerror("Received from VidCard (%02X)\n",aciadata);
#endif
  return aciadata;
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( mpu4_uart_ctrl_r )
{
  return get_normal_uart_status();
}

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////

READ16_HANDLER( vidcard_uart_ctrl_r )
{
  return get_vid_uart_status();
}

///////////////////////////////////////////////////////////////////////////

WRITE16_HANDLER( vidcard_uart_ctrl_w )
{
//  vid_data_from_norm = 0; // data available for adder from sc2
//  vid_normdata       = 0; // data
//  norm_data_from_vid = 0; // data available for sc2 from adder
//  norm_viddata         = 0;   // data

  if (ACCESSING_MSB)
  {
  ctrl = ((data >> 8) & 0xff);
  }
  if (ACCESSING_LSB)
  {
  ctrl = (data & 0xff);
  }

     switch (ctrl & 0x03)
      {
      case 0x00:
         logerror("Vid Uart clock: x1\n");
         break;
      case 0x01:
         logerror("Vid Uart clock: x16\n");
         break;
      case 0x02:
         logerror("Vid Uart clock: x32\n");
         break;
      case 0x03:
         logerror("Resetting Vid Uart\n");
         vid_uart_status = 0x02;
         break;
      }

      switch (ctrl & 0x1c)
      {
      case 0x00:
         logerror("Vid Uart - Bits :7 Parity: Even Stop Bits: 2\n");
         break;
      case 0x04:
         logerror("Vid Uart - Bits :7 Parity: Odd Stop Bits: 2\n");
         break;
      case 0x08:
         logerror("Vid Uart - Bits :7 Parity: Even Stop Bits: 1\n");
         break;
      case 0x0c:
         logerror("Vid Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      case 0x10:
         logerror("Vid Uart - Bits :8 Parity: None Stop Bits: 2\n");
         break;
      case 0x14:
         logerror("Vid Uart - Bits :8 Parity: None Stop Bits: 1\n");
         break;
      case 0x18:
         logerror("Vid Uart - Bits :8 Parity: Even Stop Bits: 1\n");
         break;
      case 0x1c:
         logerror("Vid Uart - Bits :7 Parity: Odd Stop Bits: 1\n");
         break;
      }

      switch (ctrl & 0x60)
      {
      case 0x00:
         logerror("Vid Uart - /RTS=0, TX IRQ=0\n");
         break;
      case 0x20:
         logerror("Vid Uart - /RTS=0, TX IRQ=1\n");
         break;
      case 0x40:
         logerror("Vid Uart - /RTS=1, TX IRQ=0\n");// In this situation, we need to trigger our RTS function
         break;
      case 0x60:
         logerror("Vid Uart - /RTS=0, TX break level, TX IRQ=0\n");
         break;
      }

      if (ctrl & 0x80)
      {
         logerror("Vid Uart - RX IRQ=1\n");
      }
      else
      {
         logerror("Vid Uart - RX IRQ=0\n");
      }
   //}

  #ifdef LOG_CTRL
  logerror("VID uart ctrl:%02X\n", ctrl);
  #endif
}

///////////////////////////////////////////////////////////////////////////

READ16_HANDLER( vidcard_uart_rx_r )
{
if (vid_uart_status & 0x01)
{
  aciadata = read_from_norm();
}
vid_uart_status = vid_uart_status & 0x7e; // Clear the IRQ and the RDR Full
//  return aciadata;
  return (aciadata << 8) | 0x00ff;

#ifdef LOG_SERIAL
  logerror("Received from NORM (%02X)\n",aciadata);
#endif

}

///////////////////////////////////////////////////////////////////////////

WRITE16_HANDLER( vidcard_uart_tx_w )
{
vid_uart_status = vid_uart_status & 0xfd;//0x7d;// Clear the IRQ and the TDR Empty

  if (ACCESSING_MSB)
  send_to_norm((data >> 8) & 0xff);
  if (ACCESSING_LSB)
  send_to_norm(data & 0xff); //Fill TDR

}

///////////////////////////////////////////////////////////////////////////
