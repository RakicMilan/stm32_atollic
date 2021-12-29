/*
* ----------------------------------------------------------------------------
* â€œTHE COFFEEWARE LICENSEâ€� (Revision 1):
* <ihsan@kehribar.me> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a coffee in return.
* -----------------------------------------------------------------------------
* This library is based on this library: 
*   https://github.com/aaronds/arduino-nrf24l01
* Which is based on this library: 
*   http://www.tinkerer.eu/AVRLib/nRF24L01
* -----------------------------------------------------------------------------
*/
#include <stdint.h>
#include "nrf24.h"
#include "gpio.h"
#include "main.h"


uint8_t NRF24PayloadSize;

/* software spi routine */
uint8_t spi_transfer(uint8_t tx)
{
	uint8_t rx_data = 0;

#ifdef USE_SPI1
	SPI1->DR = tx;
	while(!(SPI1->SR & SPI_SR_TXE)){;}
	while(!(SPI1->SR & SPI_SR_RXNE)){;}
	while((SPI1->SR & SPI_SR_BSY)){;}
	rx_data = SPI1->DR;
#else
	SPI2->DR = tx;
	while(!(SPI2->SR & SPI_SR_TXE)){;}
	while(!(SPI2->SR & SPI_SR_RXNE)){;}
	while((SPI2->SR & SPI_SR_BSY)){;}
	rx_data = SPI2->DR;
#endif

	return rx_data;
}

void NRF24ChipEnable(GPIO_PinState state)
{
	HAL_GPIO_WritePin(NRF24_CE_GPIO_Port, NRF24_CE_Pin, state);
}
/* ------------------------------------------------------------------------- */
void NRF24ChipSelect(GPIO_PinState state)
{
	HAL_GPIO_WritePin(NRF24_CS_GPIO_Port, NRF24_CS_Pin, state);
}

/* init the hardware pins */
nrf24_error_t NRF24Init(void)
{
	NRF24ChipEnable(HIGH);
	HAL_Delay(500);
	NRF24ChipEnable(LOW);
	NRF24ChipSelect(HIGH);

	return NRF_SUCCESS;
}

/* configure the module */
void NRF24Config(nrf24_t* ctxNRF24)
{
	/* Use static payload length ... */
	NRF24PayloadSize = ctxNRF24->PayloadSize;

	// Set RF channel
	NRF24ConfigRegister(RF_CH,ctxNRF24->Channel);

	// Set length of incoming payload 
	NRF24ConfigRegister(RX_PW_P0, 0x00); // Auto-ACK pipe ...
	NRF24ConfigRegister(RX_PW_P1, ctxNRF24->PayloadSize); // Data payload pipe
	NRF24ConfigRegister(RX_PW_P2, 0x00); // Pipe not used 
	NRF24ConfigRegister(RX_PW_P3, 0x00); // Pipe not used 
	NRF24ConfigRegister(RX_PW_P4, 0x00); // Pipe not used 
	NRF24ConfigRegister(RX_PW_P5, 0x00); // Pipe not used 

	// 1 Mbps, TX gain: 0dbm
	NRF24ConfigRegister(RF_SETUP, (0<<RF_DR)|((0x03)<<RF_PWR));

	// CRC enable, 1 byte CRC length
	NRF24ConfigRegister(CONFIG,NRF24_CONFIG);

	// Auto Acknowledgment
	NRF24ConfigRegister(EN_AA,(1<<ENAA_P0)|(1<<ENAA_P1)|(0<<ENAA_P2)|(0<<ENAA_P3)|(0<<ENAA_P4)|(0<<ENAA_P5));

	// Enable RX addresses
	NRF24ConfigRegister(EN_RXADDR,(1<<ERX_P0)|(1<<ERX_P1)|(0<<ERX_P2)|(0<<ERX_P3)|(0<<ERX_P4)|(0<<ERX_P5));

	// Auto retransmit delay: 1000 us and Up to 15 retransmit trials
	NRF24ConfigRegister(SETUP_RETR,(0x04<<ARD)|(0x0F<<ARC));

	// Dynamic length configurations: No dynamic length
	NRF24ConfigRegister(DYNPD,(0<<DPL_P0)|(0<<DPL_P1)|(0<<DPL_P2)|(0<<DPL_P3)|(0<<DPL_P4)|(0<<DPL_P5));

	// Start listening
	NRF24PowerUpRx();
}

/* Set the RX address */
void NRF24SetRxAddress(nrf24_t* ctxNRF24)
{
	NRF24ChipEnable(LOW);
	NRF24WriteRegister(RX_ADDR_P1, ctxNRF24->RXAddress, NRF24_ADDR_LEN);
	NRF24ChipEnable(HIGH);
}

/* Returns the payload length */
uint8_t NRF24TxPayloadLength(void)
{
	return NRF24PayloadSize;
}

/* Set the TX address */
void NRF24SetTxAddress(nrf24_t* ctxNRF24)
{
	/* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
	NRF24WriteRegister(RX_ADDR_P0,ctxNRF24->TXAddress,NRF24_ADDR_LEN);
	NRF24WriteRegister(TX_ADDR,ctxNRF24->TXAddress,NRF24_ADDR_LEN);
}

/* Checks if data is available for reading */
/* Returns 1 if data is ready ... */
bool NRF24DataReady(void) 
{
	// See note in getData() function - just checking RX_DR isn't good enough
	uint8_t status = NRF24GetStatus();

	// We can short circuit on RX_DR, but if it's not set, we still need
	// to check the FIFO for any pending packets
	if ( status & (1 << RX_DR) ) 
	{
		return true;
	}

	return !NRF24RxFifoEmpty();
}

/* Checks if receive FIFO is empty or not */
bool NRF24RxFifoEmpty(void)
{
	uint8_t fifoStatus;

	NRF24ReadRegister(FIFO_STATUS,&fifoStatus,1);
	
	return (fifoStatus & (1 << RX_EMPTY) ? true : false);
}

/* Returns the length of data waiting in the RX fifo */
uint8_t NRF24RxPayloadLength(void)
{
	uint8_t status;
	NRF24ChipSelect(LOW);
	spi_transfer(R_RX_PL_WID);
	status = spi_transfer(0x00);
	NRF24ChipSelect(HIGH);
	return status;
}

/* Reads payload bytes into data array */
void NRF24GetData(nrf24_t* ctxNRF24)
{
	/* Pull down chip select */
	NRF24ChipSelect(LOW);                               

	/* Send cmd to read rx payload */
	spi_transfer( R_RX_PAYLOAD );
	
	/* Read payload */
	NRF24ReceiveBuffer(ctxNRF24->RXData, ctxNRF24->PayloadSize);
	
	/* Pull up chip select */
	NRF24ChipSelect(HIGH);

	/* Reset status register */
	NRF24ConfigRegister(STATUS,(1<<RX_DR));   
}

/* Returns the number of retransmissions occured for the last message */
uint8_t NRF24RetransmissionCount(void)
{
	uint8_t rv;
	NRF24ReadRegister(OBSERVE_TX,&rv,1);
	rv = rv & 0x0F;
	return rv;
}

// Sends a data package to the default address. Be sure to send the correct
// amount of bytes as configured as payload on the receiver.
void NRF24Send(nrf24_t* ctxNRF24)
{    
	/* Go to Standby-I first */
	NRF24ChipEnable(LOW);
	 
	/* Set to transmitter mode , Power up if needed */
	NRF24PowerUpTx();

	/* Do we really need to flush TX fifo each time ? */
	#if 1
		/* Pull down chip select */
		NRF24ChipSelect(LOW);           

		/* Write cmd to flush transmit FIFO */
		spi_transfer(FLUSH_TX);     

		/* Pull up chip select */
		NRF24ChipSelect(HIGH);                    
	#endif 

	/* Pull down chip select */
	NRF24ChipSelect(LOW);

	/* Write cmd to write payload */
	spi_transfer(W_TX_PAYLOAD);

	/* Write payload */
	NRF24TransmitBuffer(ctxNRF24->TXData,ctxNRF24->PayloadSize);

	/* Pull up chip select */
	NRF24ChipSelect(HIGH);

	/* Start the transmission */
	NRF24ChipEnable(HIGH);    
}

bool NRF24IsSending(void)
{
	uint8_t status;

	/* read the current status */
	status = NRF24GetStatus();
							
	/* if sending successful (TX_DS) or max retries exceded (MAX_RT). */
	if((status & ((1 << TX_DS)  | (1 << MAX_RT))))
	{        
		return false; /* false */
	}

	return true; /* true */
}

uint8_t NRF24GetStatus(void)
{
	uint8_t rv;
	NRF24ChipSelect(LOW);
	rv = spi_transfer(NOP);
	NRF24ChipSelect(HIGH);
	return rv;
}

uint8_t NRF24LastMessageStatus(void)
{
	uint8_t rv;

	rv = NRF24GetStatus();

	/* Transmission went OK */
	if((rv & ((1 << TX_DS))))
	{
		return NRF24_TRANSMISSON_OK;
	}
	/* Maximum retransmission count is reached */
	/* Last message probably went missing ... */
	else if((rv & ((1 << MAX_RT))))
	{
		return NRF24_MESSAGE_LOST;
	}  
	/* Probably still sending ... */
	else
	{
		return 0xFF;
	}
}

void NRF24PowerUpRx(void)
{     
	NRF24ChipSelect(LOW);
	spi_transfer(FLUSH_RX);
	NRF24ChipSelect(HIGH);

	NRF24ConfigRegister(STATUS,(1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT)); 

	NRF24ChipEnable(LOW);    
	NRF24ConfigRegister(CONFIG,NRF24_CONFIG|((1<<PWR_UP)|(1<<PRIM_RX)));    
	NRF24ChipEnable(HIGH);
}

void NRF24PowerUpTx(void)
{
	NRF24ConfigRegister(STATUS,(1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT)); 
	NRF24ConfigRegister(CONFIG,NRF24_CONFIG|((1<<PWR_UP)|(0<<PRIM_RX)));
}

void NRF24PowerDown(void)
{
	NRF24ChipEnable(LOW);
	NRF24ConfigRegister(CONFIG,NRF24_CONFIG);
}

/* send and receive multiple bytes over SPI */
void NRF24ReceiveBuffer(uint8_t* DataIn, uint8_t Len)
{
	uint8_t i;

	for(i = 0; i < Len; i++)
	{
		DataIn[i] = spi_transfer(0xff);
	}
}

/* send multiple bytes over SPI */
void NRF24TransmitBuffer(uint8_t* Data,uint8_t Len)
{
	uint8_t i;

	for(i = 0;i < Len;i++)
	{
		spi_transfer(Data[i]);
	}
}

/* Clocks only one byte into the given NRF24 register */
void NRF24ConfigRegister(uint8_t Address, uint8_t Value)
{
	NRF24ChipSelect(LOW);
	spi_transfer(W_REGISTER | (REGISTER_MASK & Address));
	spi_transfer(Value);
	NRF24ChipSelect(HIGH);
}

/* Read single register from NRF24 */
void NRF24ReadRegister(uint8_t Address, uint8_t* Data, uint8_t Len)
{
	NRF24ChipSelect(LOW);
	spi_transfer(R_REGISTER | (REGISTER_MASK & Address));
	NRF24ReceiveBuffer(Data,Len);
	NRF24ChipSelect(HIGH);
}

/* Write to a single register of nrf24 */
void NRF24WriteRegister(uint8_t Address, uint8_t* Data, uint8_t Len) 
{
	NRF24ChipSelect(LOW);
	spi_transfer(W_REGISTER | (REGISTER_MASK & Address));
	NRF24TransmitBuffer(Data,Len);
	NRF24ChipSelect(HIGH);
}
