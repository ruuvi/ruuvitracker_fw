#include "platform.h"
#include "stm32f4xx_conf.h"
#include <stm32f4xx_i2c.h>


/* I2C Functions */
u32 platform_i2c_setup( unsigned id, u32 speed )
{
	GPIO_InitTypeDef GPIO_InitStruct;
	I2C_InitTypeDef I2C_InitStruct;
	
	// enable APB1 peripheral clock for I2C1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	// enable clock for SCL and SDA pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	I2C_DeInit(I2C1);
	
	/* setup SCL and SDA pins
	 * You can connect I2C1 to two different
	 * pairs of pins:
	 * 1. SCL on PB6 and SDA on PB7 
	 * 2. SCL on PB8 and SDA on PB9
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_9; // we are going to use PB6 and PB7
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;			// set pins to alternate function
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;		// set GPIO speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;			// set output to open drain --> the line has to be only pulled low, not driven high
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			// enable pull up resistors
	GPIO_Init(GPIOB, &GPIO_InitStruct);					// init GPIOB
	
	// Connect I2C1 pins to AF  
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);	// SCL
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1); // SDA
	
	// configure I2C1 
	I2C_StructInit(&I2C_InitStruct);
	I2C_InitStruct.I2C_ClockSpeed = speed; 		// 100kHz
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;			// I2C mode
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;	// 50% duty cycle --> standard
	I2C_InitStruct.I2C_OwnAddress1 = 0x00;			// own address, not relevant in master mode
	I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;		// disable acknowledge when reading (can be changed later on)
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
	I2C_Init(I2C1, &I2C_InitStruct);				// init I2C1
	
	// enable I2C1
	I2C_Cmd(I2C1, ENABLE);

	return speed;
}

void platform_i2c_send_start( unsigned id )
{
	// wait until I2C1 is not busy anymore
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
  
	// Send I2C1 START condition 
	I2C_GenerateSTART(I2C1, ENABLE);
	  
	// wait for I2C1 EV5 --> Master has acknowledged start condition
	while(SUCCESS != I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		;;
}

void platform_i2c_send_stop( unsigned id )
{
	// Send I2C1 STOP Condition 
	I2C_GenerateSTOP(I2C1, ENABLE);
}

int platform_i2c_send_address( unsigned id, u16 address, int direction )
{
	address<<=1; //Shift 7bit address to left by one to leave room for R/W bit
	/* wait for I2C1 EV6, check if 
	 * either Slave has acknowledged Master transmitter or
	 * Master receiver mode, depending on the transmission
	 * direction
	 */ 
	if(direction == PLATFORM_I2C_DIRECTION_TRANSMITTER){
		I2C_Send7bitAddress(I2C1, address, I2C_Direction_Transmitter);
		while(SUCCESS != I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
			;;
	}
	else if(direction == PLATFORM_I2C_DIRECTION_RECEIVER){
		I2C_Send7bitAddress(I2C1, address, I2C_Direction_Receiver);
		while(SUCCESS != I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
			;;
	}
	return 1;
}

int platform_i2c_send_byte( unsigned id, u8 data )
{
	I2C_SendData(I2C1, data);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	return 1;
}

/* This function reads one byte from the slave device 
 * and acknowledges the byte (requests another byte)
 */
uint8_t I2C_read_ack(){
	// enable acknowledge of recieved data
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	// wait until one byte has been received
	while( !I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) );
	// read data from I2C data register and return data byte
	uint8_t data = I2C_ReceiveData(I2C1);
	return data;
}

/* This function reads one byte from the slave device
 * and doesn't acknowledge the recieved data 
 */
uint8_t I2C_read_nack(){
	// disabe acknowledge of received data
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	// wait until one byte has been received
	while( !I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) );
	// read data from I2C data register and return data byte
	uint8_t data = I2C_ReceiveData(I2C1);
	return data;
}

int platform_i2c_recv_byte( unsigned id, int ack )
{
	if(ack)
		return I2C_read_ack();
	else
		return I2C_read_nack();
}
