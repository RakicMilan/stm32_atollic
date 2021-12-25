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
#ifndef NRF24
#define NRF24

#include "nrf24l01_regs.h"
#include <stdint.h>
#include <stdbool.h>

#define LOW 		GPIO_PIN_RESET
#define HIGH 		GPIO_PIN_SET

#define NRF24_ADDR_LEN 5
#define NRF24_CONFIG ((1<<EN_CRC)|(0<<CRCO))

#define NRF24_TRANSMISSON_OK 0
#define NRF24_MESSAGE_LOST   1

typedef enum
{
	NRF_SUCCESS = 0,
	NRF_ERROR,
	NRF_BUSY
}nrf24_error_t;

typedef struct
{
	uint8_t RXAddress[8];
	uint8_t TXAddress[8];
	uint8_t PayloadSize;
	uint8_t Channel;
	uint8_t RXData[32];
	uint8_t TXData[32];
}nrf24_t;

/* adjustment functions */
nrf24_error_t    NRF24Init(void);
void    NRF24SetRxAddress(nrf24_t* ctxNRF24);
void    NRF24SetTxAddress(nrf24_t* ctxNRF24);
void    NRF24Config(nrf24_t* ctxNRF24);

/* state check functions */
uint8_t NRF24GetStatus(void);
bool 	NRF24DataReady(void);
bool 	NRF24IsSending(void);
bool 	NRF24RxFifoEmpty(void);

/* core TX / RX functions */
void    NRF24Send(nrf24_t* ctxNRF24);
void    NRF24GetData(nrf24_t* ctxNRF24);

/* use in dynamic length mode */
uint8_t NRF24PayloadLength(void);

/* post transmission analysis */
uint8_t NRF24LastMessageStatus(void);
uint8_t NRF24RetransmissionCount(void);

/* Returns the payload length */
uint8_t NRF24PayloadLength(void);

/* power management */
void    NRF24PowerUpRx(void);
void    NRF24PowerUpTx(void);
void    NRF24PowerDown(void);

/* low level interface ... */
uint8_t spi_transfer(uint8_t tx);
void    NRF24TransmitBuffer(uint8_t* Data,uint8_t Len);
void    NRF24ReceiveBuffer(uint8_t* DataIn,uint8_t Len);
void    NRF24ConfigRegister(uint8_t Address, uint8_t Value);
void    NRF24ReadRegister(uint8_t Address, uint8_t* Data, uint8_t Len);
void    NRF24WriteRegister(uint8_t Address, uint8_t* Data, uint8_t Len);

#endif
