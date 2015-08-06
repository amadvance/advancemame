#include "driver.h"

extern UINT8 m6850_irq_state; // referenced from machine/mpu4.c
extern int uart1_status;	  // MC6850 status
extern int uart2_status;	  // MC6850 status
extern int vid_rx;
extern int vid_acia_triggered; // flag <>0, ACIA receive IRQ
extern int norm_data_from_vid;	// data available for MPU4 from vid
extern int vid_data_from_norm;	// data available for vid from MPU4

READ8_HANDLER( uart1stat_r );
READ8_HANDLER( uart1data_r );
WRITE8_HANDLER( uart1ctrl_w );
WRITE8_HANDLER( uart1data_w );
READ8_HANDLER( uart2stat_r );
READ8_HANDLER( uart2data_r );
WRITE8_HANDLER( uart2ctrl_w );
WRITE8_HANDLER( uart2data_w );

WRITE8_HANDLER( mpu4_uart_tx_w );
WRITE8_HANDLER( mpu4_uart_ctrl_w );
READ8_HANDLER( mpu4_uart_rx_r );
READ8_HANDLER( mpu4_uart_ctrl_r );

READ16_HANDLER( vidcard_uart_ctrl_r );
WRITE16_HANDLER( vidcard_uart_ctrl_w );
READ16_HANDLER( vidcard_uart_rx_r );
WRITE16_HANDLER( vidcard_uart_tx_w );


extern void send_to_norm(int data);
extern int  read_from_norm(void);

extern void send_to_vid(int data);
