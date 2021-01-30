/***************************************************************************
* 
* File Name:    xmodem.c
* 
* Description:  
*   (+) USART_USE is the uart device of STM32F4
*   (+) UART_WAIT_1S should be modified to adapt(depends on code execution frequency)
*   (+) Start xmodem recieve using upgradeFW()
*   (+) XM_SOH: xmodem(128bytes) (no use, it can be modified to adapt)
*   (+) XM_STX: 1k-xmodem(1024bytes)
*   (+) Print CCCC to serial terminal
*   (+) Serial terminal transfer file with (1k-xmodem) 
*
* Revision History:
* 1-24, 2019	Chi Bao	File created
* 
***************************************************************************/
#include "includes.h"

#define DEFINE_DATE          (__DATE__)               // string
#define DEFINE_TIME          (__TIME__)               // string
#define DEFINE_FUNCTION      (__FUNCTION__)           // string
#define DEFINE_LINE          (__LINE__)               // int
#define DEFINE_STDC          (__STDC__)               // int



void delay_ForRecvKey(int number)
{
    long uli;
    
    for(uli = 0; uli < (long)(20 * number); uli++)
    {
        __asm(" nop");	
    }	
}

uchar checkRxFlag(void)
{
	if(USART_USE->SR&(1<<5))
    {
		return 1;
	}

	return 0;
}

uchar xfer_buffer[1024+10];

char cGlorecv;
uint uiGlocount;

long xfer_wait_byte(long timeOut)
{
	uchar ucFlag;
    ulong time = timeOut;

	ucFlag = 0;

    while (time--)
    {
		ucFlag = checkRxFlag();
        if(ucFlag)
        {                             
            cGlorecv = USART_USE->DR;
           return 1;
        }                               
    }
   
    return 0;
}

void xfer_send_byte(char theChar)
{
    uartSendChar(theChar);
}

int xfer_recv_byte(const long timeOut)
{
    int result;

    if (xfer_wait_byte(timeOut))
    {
        result = cGlorecv;
    }
    else
    {
        result = -1;
    }
    
    return result;
}

void canTrans(void)
{
    int i = 0;
    
    for (i = 0; i < 9; i++)
    {
        xfer_send_byte(XM_NAK);
        delay_us(100);
    }
}

uint xmodemSum(uchar *ptr, int count)
{
    int crc = 0;
    char i = 0;
    
    crc = 0;
    
    while (--count >= 0)
    {
        crc = crc ^ *ptr++ << 8;
        
        for (i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
            {
                crc = crc << 1 ^ 0x1021;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }
    
    return (crc & 0xFFFF);
}

void xmodem_retry(void)
{
    while (xfer_recv_byte(UART_WAIT_1S) != -1)
    {
        ;
    }
    
    xfer_send_byte(XM_NAK);
    ucNAK_Flg = NAK_MARK;
}

char xfer_recv_block(uint blocksize)
{
    uint    totalCount;
    uint    count;
    int recvChar;
    
    count = 0;
    totalCount = blocksize + 4;
    
    while (count < totalCount)
    {
        recvChar = xfer_recv_byte(UART_WAIT_1S);
        
        if (recvChar != -1)
        {
            xfer_buffer[count++] = recvChar;
        }
        else
        {
            break;
        }
    }
    
    return (count == totalCount) ? 1 : 0;
}

char xmodemCrc16checksum(uint blocksize)
{
    uint remotecrc = 0;
    uint crc = 0;
    
    remotecrc = 0x00ff & xfer_buffer[blocksize + 2];
    remotecrc = remotecrc << 8;
    remotecrc  |= 0x00ff & xfer_buffer[blocksize + 3];
    
    
    crc = xmodemSum(&xfer_buffer[2], (blocksize));
    
    if (crc == remotecrc)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

char xmodem_check_block(uint blockNo, uint blocksize)
{
    uchar ucLocals1;
    uint uiLocal2;
    
    ucLocals1 =  ~xfer_buffer[1];
    ucLocals1 &= 0x00FF;
    
    uiLocal2 = blockNo;
    uiLocal2 &= 0x00FF;
    
    if ((uiLocal2 == xfer_buffer[0]) &&
        (uiLocal2 == ucLocals1) &&
        (xmodemCrc16checksum(blocksize)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

unsigned long xfer_bufferToUlong(unsigned int uiFormalOffset)
{
    unsigned long ulLocal;
    ulLocal = 0;
    
    ulLocal = xfer_buffer[uiFormalOffset + 2];
    ulLocal = (ulLocal << 8) | xfer_buffer[uiFormalOffset + 3];
    ulLocal = (ulLocal << 8) | xfer_buffer[uiFormalOffset + 4];
    ulLocal = (ulLocal << 8) | xfer_buffer[uiFormalOffset + 5];
    
    return ulLocal;
}

uchar ucNAK_Flg;
char xmodem_receive(void)
{
    ulong retris;
    uchar result;
    ulong offset;
    uint blocksize;
	//ulong ulFPGA_Offset;
    int recvChar;
    uint blockNo;
    

    result = FALSE;
    retris = RETRIES;
    recvChar = 0;
    offset = 0;
    blocksize = 0;
    //ulFPGA_Offset = 0;

    while (retris--)
    {
        if (!xfer_wait_byte(UART_WAIT_1S))
        {
            xfer_send_byte(XM_C);
        }
        else
        {
            if (cGlorecv == 27)//press ESC to exit.
            {
                return 0;
            }
            else
            {
                if (cGlorecv != 13)
                {
                    break;
                }
            }
        }
        
        if (retris == 0)
        {    
            return FALSE;
        }
    }
    
    retris = RETRIES;
    blockNo = 1;
    
    while (retris)
    {
        if( (blockNo == 1)&& (ucNAK_Flg != NAK_MARK) )
        {
            recvChar = cGlorecv;
        }
        else
        {
            recvChar = xfer_recv_byte(UART_WAIT_1S);
            
            if (recvChar == -1)
            {
                retris--;
                continue;
            }
        }
        
        switch (recvChar)
        {
        	case XM_SOH:
                canTrans();
                retris = 0;
                break;
                
            case XM_STX:
            
                blocksize = BLOCK_SIZE;
                
                if (xfer_recv_block(blocksize))
                {
                    if (xmodem_check_block(blockNo, blocksize))
                    {
                        if (blockNo == 1)
                        {
                            offset = 0;

                            //first package
                            
                            
                            offset += blocksize;
                              
                        }
                        else 
                        {
                            
                            
                    
                    
                    
                            offset += blocksize;
                        }
                      
                        xfer_send_byte(XM_ACK);
                        blockNo++;
                        retris = RETRIES;
                    }
                    else
                    {
                        canTrans();
                        retris = 0;
                    }
                }
                else
                {
                    retris--;
                }
                
                break;
                
            case XM_CAN:
                xfer_send_byte(XM_ACK);
                retris = 0;
                result = FALSE;
                break;
                
            case XM_EOT:
                xfer_send_byte(XM_ACK);                
				ucNAK_Flg = 0;
                result = TRUE;
                retris = 0;
                break;
                
            default:
                xmodem_retry();
                retris--;
                
                if (retris == 0)
                {
                    canTrans();
                    
                    result = FALSE;
                }
                
                break;
        }
    }
    ucNAK_Flg = 0;				
	
	return result; 
}


uchar updateFirmware(void)
{
	char result;
	result = xmodem_receive();
	
    if( (result == FALSE) || (result == FALSE_CHECKERR) || (result == FALSE_NAK_OVERTIME) 
			|| (result == FALSE_OVERTIME) ||(result == FALSE_CANCEL) || (result == FALSE_EOL) ) 
	{
	    xfer_send_byte(XM_CAN);
        xfer_send_byte(XM_CAN);
        xfer_send_byte(XM_CAN);
        xfer_send_byte(XM_CAN);
        xfer_send_byte(XM_CAN);
        xfer_send_byte(XM_CAN);    
        xfer_send_byte(XM_CAN);
        xfer_send_byte(XM_CAN);
        printf("\r\nXMODEM CANCEL!\r\n");
		return result;
	}
	else
	{
        //xmodem recieve ok


		return TRUE;	   
   }	
}

void upgradeFW(void)
{
	uchar result = 0;
	char cComndBuf[12];	

	memset( cComndBuf, 0, sizeof(cComndBuf) );

    USART_USE->CR1 &= ~(USART_CR1_RXNEIE);    	//disable uart not empty interrupt    	

	memset(uiSectorFlg,0,sizeof(uiSectorFlg));
	result = updateFirmware();

	if(result == TRUE)
    {
		//TODO:
        
	}
	else
	{
        //TODO:
        
	}
	
    USART_USE->CR1 |= USART_CR1_RXNEIE;    	//enable uart not empty interrupt
}

