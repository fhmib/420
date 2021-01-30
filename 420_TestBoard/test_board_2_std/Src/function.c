#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"
#include "function.h"
#include "main.h"
#include "usart.h"
#include "tim.h"
#include "spi.h"
#include "xmodem.h"
#include "flash_if.h"

uint8_t fw_buf[64 * 1024];
uint8_t rBuf[TRANS_MAX_LENGTH];
uint8_t rLen;
uint8_t txBuf[TRANS_MAX_LENGTH];
uint32_t err_code = 0;

#define FW_BLOCK_MAX_SIZE 128
#define TO_STR(s) #s
#if 0
#define GET_RESULT()  do {\
                        if (ret >= 0) {\
                          if (rBuf[rLen - 2] != 0) {\
                            PRINT(CMD_FAILED, rBuf[rLen - 2]);\
                          } else {\
                            PRINT(CMD_SUCCESS);\
                          }\
                        } else {\
                          PRINT(CMD_FAILED, ret);\
                        }\
                      } while(0)
#endif

uint32_t block_size = FW_BLOCK_MAX_SIZE;


int8_t cmd_get_sn(uint8_t argc, char **argv)
{
  int8_t ret;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_SN, NULL, 0, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  rBuf[CMD_SEQ_MSG_DATA + 10] = 0;
  PRINT("Serial Number: %s\r\n", (char*)&rBuf[CMD_SEQ_MSG_DATA]);

  return ret;
}

int8_t cmd_get_date(uint8_t argc, char **argv)
{
  int8_t ret;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_MDATE, NULL, 0, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  rBuf[CMD_SEQ_MSG_DATA + 9] = 0;
  PRINT("Menufacture Date: %s\r\n", (char*)&rBuf[CMD_SEQ_MSG_DATA]);

  return ret;
}

int8_t cmd_get_pn(uint8_t argc, char **argv)
{
  int8_t ret;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_PN, NULL, 0, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  rBuf[CMD_SEQ_MSG_DATA + 17] = 0;
  PRINT("Part Number: %s\r\n", (char*)&rBuf[CMD_SEQ_MSG_DATA]);

  return ret;
}

int8_t cmd_get_vendor(uint8_t argc, char **argv)
{
  int8_t ret;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_VENDOR, NULL, 0, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  rBuf[CMD_SEQ_MSG_DATA + 17] = 0;
  PRINT("Vendor: %s\r\n", (char*)&rBuf[CMD_SEQ_MSG_DATA]);

  return ret;
}

int8_t cmd_version(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t *p;
  uint32_t i = 5000;

  if (argc == 2 && !strcasecmp(argv[1], "repeat")) {
    while (i--) {
      ret = process_command(CMD_QUERY_VERSION, txBuf, 0, rBuf, &rLen);
      if (ret) {
        PRINT("i = %u\r\n", i);
        return ret;
      }
    }
  } else if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_VERSION, txBuf, 0, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  //PRINT_HEX("Received", rBuf, rLen);

  p = rBuf + CMD_SEQ_MSG_DATA;
  p[8] = 0;
  PRINT("Firmware Version: %s\r\n", (char*)p);

  return ret;
}

int8_t cmd_temp(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t *p;
  int16_t temp;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_TEMP, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  p = rBuf + CMD_SEQ_MSG_DATA;
  temp = (int16_t)switch_endian_16(*(uint16_t*)p);
  PRINT("Temperature: %.1lfC\r\n", (double)temp/10);

  return ret;
}

int8_t cmd_IL(uint8_t argc, char **argv)
{
  int8_t ret;
  int16_t val;
  double val_f;

  if (argc == 2) {
    txBuf[0] = strtoul(argv[1], NULL, 0);
  } else {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_IL, txBuf, 1, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  PRINT("Insertion Loss %u: ", rBuf[CMD_SEQ_MSG_DATA]);
  val = (int16_t)Buffer_To_BE16(&rBuf[CMD_SEQ_MSG_DATA + 1]);
  val_f = (double)val / 100;
  PRINT("%.2lf\r\n", val_f);

  return ret;
}

int8_t cmd_tosa(uint8_t argc, char **argv)
{
  if (argc == 2 && !strcasecmp(argv[1], "thr")) {
    get_tosa_thr(argc, argv);
  } else if (argc == 2 && !strcasecmp(argv[1], "enable")) {
    set_tosa_status(0);
  } else if (argc == 2 && !strcasecmp(argv[1], "disable")) {
    set_tosa_status(1);
  } else if (argc == 2 && !strcasecmp(argv[1], "status")) {
    get_tosa_status();
  } else {
    cmd_help2(argv[0]);
    return 0;
  }
  
  return 0;
}

int8_t get_tosa_thr(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t *p;
  int16_t temp;

  ret = process_command(CMD_QUERY_TOSA_THR, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  p = rBuf + CMD_SEQ_MSG_DATA;
  temp = (int16_t)switch_endian_16(*(uint16_t*)p);
  PRINT("Power High Max Threshold: %.2lfdBm\r\n", (double)temp/100);
  temp = (int16_t)switch_endian_16(*(uint16_t*)(p + 2));
  PRINT("Power High Min Threshold: %.2lfdBm\r\n", (double)temp/100);
  temp = (int16_t)switch_endian_16(*(uint16_t*)(p + 4));
  PRINT("Power Low Max Threshold: %.2lfdBm\r\n", (double)temp/100);
  temp = (int16_t)switch_endian_16(*(uint16_t*)(p + 6));
  PRINT("Power Low Min Threshold: %.2lfdBm\r\n", (double)temp/100);

  return ret;
}

int8_t set_tosa_status(uint8_t status)
{
  int8_t ret;
  
  txBuf[0] = status;
  ret = process_command(CMD_SET_TOSA_STATUS, txBuf, 1, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  return ret;
}

int8_t get_tosa_status()
{
  int8_t ret;
  
  ret = process_command(CMD_GET_TOSA_STATUS, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  if (rBuf[CMD_SEQ_MSG_DATA] == 0) {
    PRINT("Tosa has been enabled\r\n");
  } else if (rBuf[CMD_SEQ_MSG_DATA] == 1) {
    PRINT("Tosa is disabled\r\n");
  } else {
    PRINT("Unknown tosa status = %#X\r\n", rBuf[CMD_SEQ_MSG_DATA]);
  }

  return ret;
}

int8_t cmd_rx_pd_cali(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t *p;
  int16_t val;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_RX_PD_CALI, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  p = rBuf + CMD_SEQ_MSG_DATA;
  val = (int16_t)switch_endian_16(*(uint16_t*)p);
  PRINT("RX PD Power: %.2lfdBm\r\n", (double)val/100);

  return ret;
}

int8_t cmd_tap_pd_cali(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t *p;
  int16_t val;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_TAP_PD_CALI, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  p = rBuf + CMD_SEQ_MSG_DATA;
  val = (int16_t)switch_endian_16(*(uint16_t*)p);
  PRINT("TAP PD Power: %.2lfdBm\r\n", (double)val/100);

  return ret;
}

int8_t cmd_voltage(uint8_t argc, char **argv)
{
  if (argc == 2 && !strcasecmp(argv[1], "get")) {
    return get_voltage();
//  } else if (argc == 3 && !strcasecmp(argv[1], "get") && !strcasecmp(argv[2], "thr")) {
//    return get_voltage_threshold();
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t get_voltage()
{
  int8_t ret;
  uint8_t *p = rBuf + CMD_SEQ_MSG_DATA;
  int16_t i, value;
  double voltage;

  ret = process_command(CMD_QUERY_VOLTAGE, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  PRINT("Valid voltage count : %u\r\n", *p);
  p += 1;
  for (i = 0; i < 6; ++i) {
    value = (int16_t)Buffer_To_BE16(p);
    voltage = (double)value / 10;
    PRINT("Voltage target Value : %.1lfV\r\n", voltage);
    value = (int16_t)Buffer_To_BE16(p + 2);
    voltage = (double)value / 10;
    PRINT("Voltage Current Value : %.1lfV\r\n", voltage);
    p += 4;
  }

  return ret;
}

int8_t cmd_voa(uint8_t argc, char **argv)
{
  if (argc == 4 && !strcasecmp(argv[1], "set")) {
    return set_voa(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "get")) {
    return get_voa(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t set_voa(uint8_t argc, char **argv)
{
  double d_val;

  txBuf[0] = strtoul(argv[2], NULL, 10);
  d_val = atof(argv[3]);
  BE16_To_Buffer((uint16_t)(int16_t)(d_val * 10), txBuf + 1);
  return process_command(CMD_SET_VOA, txBuf, 3, rBuf, &rLen);
}

int8_t get_voa(uint8_t argc, char **argv)
{
  int8_t ret;
  double d_val;

  txBuf[0] = strtoul(argv[2], NULL, 10);
  ret = process_command(CMD_GET_VOA, txBuf, 1, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  d_val = (double)Buffer_To_BE16(rBuf + CMD_SEQ_MSG_DATA + 1) / 10;
  PRINT("CHN = %u\r\n, Atten = %.1lf\r\n", rBuf[CMD_SEQ_MSG_DATA], d_val);

  return 0;
}

int8_t cmd_alarm(uint8_t argc, char **argv)
{
  int8_t ret;
  uint32_t exp;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_ALARM, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  exp = Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA);
  PRINT("Alarm: %#X\r\n", exp);

  return ret;
}

int8_t cmd_device_status(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t status;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_STATUS, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  status = rBuf[CMD_SEQ_MSG_DATA];
  if (status == 0) {
    PRINT("Device is idle\r\n");
  } else if (status == 1) {
    PRINT("Device is busy\r\n");
  } else {
    PRINT("Unknown device status code\r\n");
  }

  return ret;
}

int8_t cmd_history_alarm(uint8_t argc, char **argv)
{
  int8_t ret, i;
  uint32_t seq, exp;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_ALARM_HISTORY, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 10; i++) {
    seq = Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA + i * 8);
    exp = Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA + i * 8 + 4);
    PRINT("Seq : %u, Alarm: %#X\r\n", seq, exp);
  }

  return ret;
}

int8_t cmd_upgrade(uint8_t argc, char **argv)
{
  if (argc == 2 && !strcasecmp(argv[1], "init")) {
    return upgrade_init();
  } else if (argc == 3 && !strcasecmp(argv[1], "file") && !strcasecmp(argv[2], "xmodem")) {
    return upgrade_file_xmodem();
  } else if (argc == 2 && !strcasecmp(argv[1], "run")) {
    return upgrade_install();
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t upgrade_init()
{
  block_size = FW_BLOCK_MAX_SIZE;
  PRINT("Initialize Upgrade\r\n");
  txBuf[0] = 1;
  return process_command(CMD_UPGRADE_MODE, txBuf, 1, rBuf, &rLen);
}

int8_t upgrade_file(uint8_t verify)
{
  int8_t ret;
  uint32_t fw_crc;
  uint16_t seq = 1;
  uint32_t fw_length, every_size = FW_BLOCK_MAX_SIZE, send_size = 0;
  uint8_t retry = 0;

  HAL_Delay(1);
  __HAL_UART_DISABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
  CLEAR_BIT(TERMINAL_UART.Instance->SR, USART_SR_RXNE);
  __HAL_UART_FLUSH_DRREGISTER(&TERMINAL_UART);
  uart1_irq_sel = 0;
  PRINT2("Download image...\r\n");

  if (HAL_UART_Receive(&TERMINAL_UART, fw_buf, 256, 1000 * 60) != HAL_OK) {
    PRINT2("UART Timeout\r\n");
    __HAL_UART_ENABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
    uart1_irq_sel = 1;
    return 1;
  }
  fw_length = (fw_buf[FW_HEAD_FW_LENGTH] << 24) | (fw_buf[FW_HEAD_FW_LENGTH + 1] << 16) |\
           (fw_buf[FW_HEAD_FW_LENGTH + 2] << 8) | (fw_buf[FW_HEAD_FW_LENGTH + 3] << 0);
  fw_crc = (fw_buf[FW_HEAD_CRC] << 24) | (fw_buf[FW_HEAD_CRC + 1] << 16) |\
           (fw_buf[FW_HEAD_CRC + 2] << 8) | (fw_buf[FW_HEAD_CRC + 3] << 0);
  HAL_UART_Receive(&TERMINAL_UART, fw_buf + 256, fw_length, 1000 * 10);
  PRINT2("Download success, Length = %u, crc = %#X\r\n", fw_length, fw_crc);
  if (verify) {
    if (Cal_CRC32(&fw_buf[256], fw_length) == fw_crc) {
      PRINT2("CRC32 success\r\n");
    } else {
      PRINT2("CRC32 failed\r\n");
      __HAL_UART_ENABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
      uart1_irq_sel = 1;
      return 2;
    }
  } else {
    PRINT2("Skip CRC32\r\n");
  }

  PRINT2("Sending image...\r\n");
  while (send_size < fw_length + FW_FILE_HEADER_LENGTH) {
    *(uint16_t*)(&txBuf[0]) = switch_endian_16(seq);
    /*
    every_size = send_size + block_size > fw_length + FW_FILE_HEADER_LENGTH ?\
                 fw_length + FW_FILE_HEADER_LENGTH - send_size : block_size ;
    *(uint32_t*)(&txBuf[4]) = switch_endian(every_size);
    */
    //PRINT2("send_size = %u\r\n", send_size);
    if (send_size + every_size > fw_length + FW_FILE_HEADER_LENGTH) {
      memset(&txBuf[2], 0, every_size);
      memcpy(&txBuf[2], &fw_buf[send_size], fw_length + FW_FILE_HEADER_LENGTH - send_size);
    } else {
      memcpy(&txBuf[2], &fw_buf[send_size], every_size);
    }
    ret = process_command(CMD_UPGRADE_DATA, txBuf, 2 + every_size, rBuf, &rLen);
    if (ret) {
      if (retry < 3) {
        PRINT2("Retry, seq = %u\r\n", seq);
        retry++;
        continue;
      }
      break;
    }
    seq++;
    send_size += every_size;
  }

  if (send_size >= fw_length + FW_FILE_HEADER_LENGTH)
    PRINT2("Send fw success\r\n");
  else
    PRINT2("Send fw failed\r\n");

  __HAL_UART_ENABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
  uart1_irq_sel = 1;

  return ret;
}

int8_t upgrade_file_xmodem(void)
{
  uint8_t ret;
  uint32_t fw_length, every_size = FW_BLOCK_MAX_SIZE, send_size = 0;
  uint32_t fw_crc;
  uint16_t seq = 1;
  uint8_t retry = 0;

  PRINT("Erasing falsh");
  HAL_Delay(10);
  FLASH_If_Erase(FLASH_SECTOR_19);
  PRINT(".");
  HAL_Delay(10);
  FLASH_If_Erase(FLASH_SECTOR_20);
  PRINT(".");
  HAL_Delay(10);
  FLASH_If_Erase(FLASH_SECTOR_21);
  PRINT(".");
  HAL_Delay(10);
  FLASH_If_Erase(FLASH_SECTOR_22);
  PRINT(".");
  HAL_Delay(10);
  FLASH_If_Erase(FLASH_SECTOR_23);
  PRINT("Done.\r\n");
  PRINT("Download image...\r\n");
  ret = xmodem_receive();
  if (ret) {
    PRINT("xmodem failed, ret = %u\r\n", ret);
    return 0;
  }
  PRINT("\r\nSending image...\r\n");
  memcpy(fw_buf, (uint8_t*)IMAGE_ADDRESS, FW_FILE_HEADER_LENGTH);
  fw_length = (fw_buf[FW_HEAD_FW_LENGTH] << 24) | (fw_buf[FW_HEAD_FW_LENGTH + 1] << 16) |\
           (fw_buf[FW_HEAD_FW_LENGTH + 2] << 8) | (fw_buf[FW_HEAD_FW_LENGTH + 3] << 0);
  fw_crc = (fw_buf[FW_HEAD_CRC] << 24) | (fw_buf[FW_HEAD_CRC + 1] << 16) |\
           (fw_buf[FW_HEAD_CRC + 2] << 8) | (fw_buf[FW_HEAD_CRC + 3] << 0);
  PRINT("Download success, Size = %u(%#X), crc = %u(%#X)\r\n", fw_length, fw_length, fw_crc, fw_crc);
  while (send_size < fw_length + FW_FILE_HEADER_LENGTH) {
    *(uint16_t*)(&txBuf[0]) = switch_endian_16(seq);

    if (send_size + every_size > fw_length + FW_FILE_HEADER_LENGTH) {
      memset(&txBuf[2], 0, every_size);
      memcpy(&txBuf[2], (uint8_t*)(IMAGE_ADDRESS + send_size), fw_length + FW_FILE_HEADER_LENGTH - send_size);
    } else {
      memcpy(&txBuf[2], (uint8_t*)(IMAGE_ADDRESS + send_size), every_size);
    }
    ret = process_command(CMD_UPGRADE_DATA, txBuf, 2 + every_size, rBuf, &rLen);
    if (ret) {
      if (retry < 3) {
        PRINT2("Retry, seq = %u\r\n", seq);
        retry++;
        continue;
      }
      break;
    }
    seq++;
    send_size += every_size;
  }

  return 0;
}

int8_t upgrade_install()
{
  return process_command(CMD_UPGRADE_RUN, txBuf, 0, rBuf, &rLen);
}

int8_t cmd_reset(uint8_t argc, char **argv)
{
  int8_t ret = 0;

  if (argc == 2 && !strcasecmp(argv[1], "hard")) {
    HAL_GPIO_WritePin(HARD_RESET_GPIO_Port, HARD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(HARD_RESET_GPIO_Port, HARD_RESET_Pin, GPIO_PIN_SET);
  } else if (argc == 2 && !strcasecmp(argv[1], "soft")) {
    ret = process_command(CMD_SOFTRESET, txBuf, 0, rBuf, &rLen);
    if (ret) {
      return ret;
    }
  } else {
    cmd_help2(argv[0]);
    return 0;
  }

  return ret;
}

int8_t cmd_time(uint8_t argc, char **argv)
{
  if (argc == 8 && !strcasecmp(argv[1], "set")) {
    return set_log_time(argc - 2, argv + 2);
  } else if (argc == 2 && !strcasecmp(argv[1], "get")) {
    return get_log_time();
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t set_log_time(uint8_t argc, char **argv)
{
  txBuf[0] = strtoul(argv[0], NULL, 10) - 2000;
  txBuf[1] = strtoul(argv[1], NULL, 10);
  txBuf[2] = strtoul(argv[2], NULL, 10);
  txBuf[3] = strtoul(argv[3], NULL, 10);
  txBuf[4] = strtoul(argv[4], NULL, 10);
  txBuf[5] = strtoul(argv[5], NULL, 10);

  return process_command(CMD_SET_LOG_TIME, txBuf, 6, rBuf, &rLen);
}

int8_t get_log_time(void)
{
  int8_t ret;
  uint8_t *p = rBuf + CMD_SEQ_MSG_DATA;

  ret = process_command(CMD_QUERY_LOG_TIME, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  PRINT("Received date %u-%u-%u %u:%u:%u\r\n", *p + 2000, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5));
  
  return ret;
}

int8_t cmd_performance(uint8_t argc, char **argv)
{
  int8_t ret, i;
  uint8_t *p = rBuf + CMD_SEQ_MSG_DATA;
  uint32_t u_val32;
  int32_t val32;
  double d_val;

  if (argc == 2 && !strcasecmp(argv[1], "all")) {
    for (i = 0; i <= 0xA; ++i) {
      txBuf[i] = i;
    }
    ret = process_command(CMD_QUERY_PERFORMANCE, txBuf, 0x12 + 1, rBuf, &rLen);
  } else {
    for (i = 0; i < argc - 1; ++i) {
      txBuf[i] = (uint8_t)strtoul(argv[i + 1], NULL, 0);
    }
    ret = process_command(CMD_QUERY_PERFORMANCE, txBuf, argc - 1, rBuf, &rLen);
  }
  if (ret) {
    return ret;
  }

  while (i--) {
    switch (*p) {
      case 0:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        PRINT("LD_Currnet : %d(%#X)mA\r\n", val32, val32);
        break;
      case 1:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        d_val = (double)val32 / 10;
        PRINT("LD_Temperature : %.1lf(%#X)C\r\n", d_val, val32);
        break;
      case 2:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        PRINT("TEC_Current : %d(%#X)mA\r\n", val32, val32);
        break;
      case 3:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        PRINT("TEC_Voltage : %d(%#X)mV\r\n", val32, val32);
        break;
      case 4:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        PRINT("LD_BACKLIGHT_VOL : %d(%#X)mV\r\n", val32, val32);
        break;
      case 5:
        u_val32 = Buffer_To_BE32(p + 1);
        PRINT("LD_LOCK_STATE : %d(%#X)\r\n", u_val32, u_val32);
        break;
      case 6:
        u_val32 = Buffer_To_BE32(p + 1);
        PRINT("TEC_LOCK_STATE : %d(%#X)\r\n", u_val32, u_val32);
        break;
      case 7:
        u_val32 = Buffer_To_BE32(p + 1);
        PRINT("LD_ON_OFF_STATE : %d(%#X)\r\n", u_val32, u_val32);
        break;
      case 8:
        u_val32 = Buffer_To_BE32(p + 1);
        PRINT("LD_Modulation_Mode : %d(%#X)\r\n", u_val32, u_val32);
        break;
      case 9:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        d_val = (double)val32 / 100;
        PRINT("TAP PD Power : %.2lf(%#X)dBm\r\n", d_val, val32);
        break;
      case 0xA:
        val32 = (int32_t)Buffer_To_BE32(p + 1);
        d_val = (double)val32 / 100;
        PRINT("Rev Pd Power : %.2lf(%#X)dBm\r\n", d_val, val32);
        break;
      default:
        PRINT("Unknown performance id %u\r\n", *p);
        break;
    }
    p += 5;
  }

  return ret;
}

int8_t cmd_threshold(uint8_t argc, char **argv)
{
  if (argc == 5 && !strcasecmp(argv[1], "set")) {
    return set_threshold(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "get")) {
    return get_threshold(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  
  return 0;
}
  
int8_t set_threshold(uint8_t argc, char **argv)
{
  int32_t val32_low, val32_high;
  int8_t ret;
  
  sscanf(argv[3], "%d", &val32_low);
  sscanf(argv[4], "%d", &val32_high);

  txBuf[0] = (uint8_t)strtoul(argv[2], NULL, 0);
  BE32_To_Buffer(val32_low, txBuf + 1);
  BE32_To_Buffer(val32_high, txBuf + 5);
  ret = process_command(CMD_SET_THRESHOLD, txBuf, 9, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  return ret;
}
  
int8_t get_threshold(uint8_t argc, char **argv)
{
  int32_t val32_low, val32_high;
  int8_t ret, i;

  if (!strcasecmp(argv[2], "all")) {
    for (i = 0; i <= 0xA; ++i) {
      txBuf[0] = i;
      ret = process_command(CMD_QUERY_THRESHOLD, txBuf, 1, rBuf, &rLen);
      if (ret) {
        return ret;
      }
      
      val32_low = (int32_t)Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA + 1);
      val32_high = (int32_t)Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA + 5);
      PRINT("%u(%#X): %d(%#X) %d(%#X)\r\n", rBuf[CMD_SEQ_MSG_DATA], rBuf[CMD_SEQ_MSG_DATA], val32_low, val32_low, val32_high, val32_high);
    }
  } else {
    txBuf[0] = (uint8_t)strtoul(argv[2], NULL, 0);
    ret = process_command(CMD_QUERY_THRESHOLD, txBuf, 1, rBuf, &rLen);
    if (ret) {
      return ret;
    }
    
    val32_low = (int32_t)Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA + 1);
    val32_high = (int32_t)Buffer_To_BE32(rBuf + CMD_SEQ_MSG_DATA + 5);
    PRINT("%u(%#X): %d(%#X) %d(%#X)\r\n", rBuf[CMD_SEQ_MSG_DATA], rBuf[CMD_SEQ_MSG_DATA], val32_low, val32_low, val32_high, val32_high);
  }
  return ret;
}

int8_t cmd_log(uint8_t argc, char **argv)
{
  if (argc == 3 && !strcasecmp(argv[1], "size")) {
    return log_size(argc, argv);
  } else if (argc == 5 && !strcasecmp(argv[1], "get")) {
    return log_content(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t log_size(uint8_t argc, char **argv)
{
  int8_t ret;
  uint32_t size;
  uint8_t *p;

  txBuf[0] = (uint8_t)strtoul(argv[2], NULL, 0);
  ret = process_command(CMD_QUERY_LOG_SIZE, txBuf, 1, rBuf, &rLen);
  if (ret) {
    return ret;
  }

  p = rBuf + CMD_SEQ_MSG_DATA + 1;
  size = Buffer_To_BE32(p);
  PRINT("type = %u, size = %u\r\n", rBuf[CMD_SEQ_MSG_DATA], size);

  return ret;
}

int8_t log_content(uint8_t argc, char **argv)
{
  int8_t ret;
  uint32_t offset, offset_returned;
  uint32_t size, cplt_size;
  
  txBuf[0] = (uint8_t)strtoul(argv[2], NULL, 0);
  offset = strtoul(argv[3], NULL, 0);
  size = strtoul(argv[4], NULL, 0);
  if (size > 200) {
    for (cplt_size = 0; cplt_size < size; ) {
      if (cplt_size + 200 > size) {
        txBuf[1] = size - cplt_size;    
        BE32_To_Buffer(offset + cplt_size, txBuf + 2);
        cplt_size += txBuf[1];
      } else {
        txBuf[1] = 200;    
        BE32_To_Buffer(offset + cplt_size, txBuf + 2);
        cplt_size += txBuf[1];
      }
      ret = process_command(CMD_GET_LOG_CONTENT, txBuf, 6, rBuf, &rLen);
      if (ret) {
        return ret;
      }

      offset_returned = Buffer_To_BE32(&rBuf[CMD_SEQ_MSG_DATA + 2]);
      PRINT("type = %u, offset = %u, length = %u\r\n", rBuf[CMD_SEQ_MSG_DATA], offset_returned, rBuf[CMD_SEQ_MSG_DATA + 1]);
      if (rBuf[CMD_SEQ_MSG_DATA] == 0) {
        PRINT_HEX("LOG", &rBuf[CMD_SEQ_MSG_DATA + 6], rBuf[CMD_SEQ_MSG_DATA + 1]);
      } else {
        PRINT_CHAR("LOG", &rBuf[CMD_SEQ_MSG_DATA + 6], rBuf[CMD_SEQ_MSG_DATA + 1]);
      }
    }
  } else {
    txBuf[1] = (uint8_t)size;    
    BE32_To_Buffer(offset, txBuf + 2);
    ret = process_command(CMD_GET_LOG_CONTENT, txBuf, 6, rBuf, &rLen);
    if (ret) {
      return ret;
    }

    offset_returned = Buffer_To_BE32(&rBuf[CMD_SEQ_MSG_DATA + 2]);
    PRINT("type = %u, offset = %u, length = %u\r\n", rBuf[CMD_SEQ_MSG_DATA], offset_returned, rBuf[CMD_SEQ_MSG_DATA + 1]);
    if (rBuf[CMD_SEQ_MSG_DATA] == 0) {
      PRINT_HEX("LOG", &rBuf[CMD_SEQ_MSG_DATA + 6], rBuf[CMD_SEQ_MSG_DATA + 1]);
    } else {
      PRINT_CHAR("LOG", &rBuf[CMD_SEQ_MSG_DATA + 6], rBuf[CMD_SEQ_MSG_DATA + 1]);
    }
  }

  return ret;
}

int8_t cmd_led(uint8_t argc, char **argv)
{
  if (argc == 5 && !strcasecmp(argv[1], "set")) {
    return set_led(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "get")) {
    return get_led(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}
  
int8_t set_led(uint8_t argc, char **argv)
{  
  txBuf[0] = strtoul(argv[2], NULL, 0);
  txBuf[1] = strtoul(argv[3], NULL, 0);
  txBuf[2] = strtoul(argv[4], NULL, 0);
  
  return process_command(CMD_SET_LED, txBuf, 3, rBuf, &rLen);
}
  
int8_t get_led(uint8_t argc, char **argv)
{
  int8_t ret;

  txBuf[0] = strtoul(argv[2], NULL, 0);
  ret = process_command(CMD_GET_LED, txBuf, 1, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  PRINT("CHN = %u, COLOR = %u, MODE = %u\r\n", rBuf[CMD_SEQ_MSG_DATA], rBuf[CMD_SEQ_MSG_DATA + 1], rBuf[CMD_SEQ_MSG_DATA + 2]);
  
  return 0;
}
  
int8_t cmd_tube(uint8_t argc, char **argv)
{
  if (argc == 3 && !strcasecmp(argv[1], "set")) {
    return set_tube(argc, argv);
  } else if (argc == 2 && !strcasecmp(argv[1], "get")) {
    return get_tube(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}
  
int8_t set_tube(uint8_t argc, char **argv)
{
  uint16_t value;

  value = strtoul(argv[2], NULL, 0);
  BE16_To_Buffer(value, txBuf);
  
  return process_command(CMD_SET_TUBE, txBuf, 2, rBuf, &rLen);
}
  
int8_t get_tube(uint8_t argc, char **argv)
{
  int8_t ret;

  ret = process_command(CMD_GET_TUBE, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  PRINT("Value = %u(=%#X)\r\n", Buffer_To_BE16(rBuf + CMD_SEQ_MSG_DATA), Buffer_To_BE16(rBuf + CMD_SEQ_MSG_DATA));
  
  return 0;
}

int8_t cmd_led_flash(uint8_t argc, char **argv)
{
  if (argc == 4 && !strcasecmp(argv[1], "set")) {
    return set_led_flash(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "get")) {
    return get_led_flash(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}
  
int8_t set_led_flash(uint8_t argc, char **argv)
{
  uint16_t value;

  txBuf[0] = strtoul(argv[2], NULL, 0);
  value = strtoul(argv[3], NULL, 0);
  BE16_To_Buffer(value, txBuf + 1);
  
  return process_command(CMD_SET_LED_FLASH, txBuf, 3, rBuf, &rLen);
}
  
int8_t get_led_flash(uint8_t argc, char **argv)
{
  int8_t ret;

  txBuf[0] = strtoul(argv[2], NULL, 0);
  ret = process_command(CMD_GET_LED_FLASH, txBuf, 1, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  PRINT("Mode = %u, Value = %u(=%#X)\r\n", rBuf[CMD_SEQ_MSG_DATA], Buffer_To_BE16(rBuf + CMD_SEQ_MSG_DATA + 1), Buffer_To_BE16(rBuf + CMD_SEQ_MSG_DATA + 1));
  
  return 0;
}

int8_t cmd_power_mode(uint8_t argc, char **argv)
{
  if (argc == 3 && !strcasecmp(argv[1], "set")) {
    return set_power_mode(argc, argv);
  } else if (argc == 2 && !strcasecmp(argv[1], "get")) {
    return get_power_mode(argc, argv);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t set_power_mode(uint8_t argc, char **argv)
{
  txBuf[0] = strtoul(argv[2], NULL, 0);
  
  return process_command(CMD_SET_POWER_MODE, txBuf, 1, rBuf, &rLen);
}

int8_t get_power_mode(uint8_t argc, char **argv)
{
  int8_t ret;

  ret = process_command(CMD_GET_POWER_MODE, txBuf, 0, rBuf, &rLen);
  if (ret) {
    return ret;
  }
  
  PRINT("Value = %u(=%#X)\r\n", rBuf[CMD_SEQ_MSG_DATA], rBuf[CMD_SEQ_MSG_DATA]);
  
  return 0;
}



int8_t cmd_for_dbg(uint8_t argc, char **argv)
{
  uint32_t i;
  uint32_t buf_len;
  uint8_t *p_rcv_buf = fw_buf + 10 * 1024;

  if (argc < 2) {
    cmd_help2(argv[0]);
    return 0;
  }

  sprintf((char*)fw_buf, "OnetCli\r");
  buf_len = strlen((char*)fw_buf);
  if (print_trans_data) {
    PRINT_HEX("tx data", fw_buf, buf_len);
  }
  if (HAL_UART_Transmit(&COMMUNICATION_UART, fw_buf, buf_len, 0xFF) != HAL_OK) {
    EPT("Transmit failed 1\r\n");
    return 100;
  }
  HAL_Delay(20);

  fw_buf[0] = 0;
  for (i = 0; i < argc - 1; ++i) {
    if (i != 0) {
      strcat((char*)fw_buf, " ");
    }
    strcat((char*)fw_buf, argv[i + 1]);
  }
  strcat((char*)fw_buf, "\r");
  buf_len = strlen((char*)fw_buf);
  if (print_trans_data) {
    PRINT_HEX("tx data", fw_buf, buf_len);
  }
  if (HAL_UART_Transmit(&COMMUNICATION_UART, fw_buf, buf_len, 0xFF) != HAL_OK) {
    EPT("Transmit failed 2\r\n");
    return 100;
  }

  i = 0;
  while (HAL_UART_Receive(&COMMUNICATION_UART, p_rcv_buf + i, 1, 200) == HAL_OK) {
    ++i;
    if (i > 1 && p_rcv_buf[i - 1] == '>' && (p_rcv_buf[i - 2] == '\n' || p_rcv_buf[i - 2] == '\r')) {
      break;
    }
  }
  p_rcv_buf[i] = 0;
  HAL_Delay(20);

  if (print_trans_data) {
    PRINT_HEX("rx data", p_rcv_buf, i);
  }
#if 0
  while (i) {
    PRINT("%#X ", p_rcv_buf[strlen((char*)p_rcv_buf) - i]);
    --i;
  }
  PRINT("\r\n");
#endif

  PRINT("%s\r\n", (char*)p_rcv_buf);

  sprintf((char*)fw_buf, "msh_switch_hex\r");
  buf_len = strlen((char*)fw_buf);
  if (print_trans_data) {
    PRINT_HEX("tx data", fw_buf, buf_len);
  }
  if (HAL_UART_Transmit(&COMMUNICATION_UART, fw_buf, buf_len, 0xFF) != HAL_OK) {
    EPT("Transmit failed 3\r\n");
    return 100;
  }

  return 0;
}

int8_t cmd_for_debug(uint8_t argc, char **argv)
{
  if (argc == 2 && !strcasecmp(argv[1], "monitor")) {
    return debug_monitor(argc, argv);
  } else if (argc == 4 && !strcasecmp(argv[1], "pin")) {
    return debug_pin(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "print_hex")) {
    return debug_print_hex(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "timer")) {
    return debug_set_tim(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "set_freq")) {
    return debug_set_freq(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "spi")) {
    return debug_spi(argc, argv);
  } else if (argc == 2 && !strcasecmp(argv[1], "send_hex")) {
    return debug_send_hex(argc, argv);
  } else if (argc == 3 && !strcasecmp(argv[1], "send_hex") && !strcasecmp(argv[2], "no_check")) {
    return debug_send_hex(argc, argv);
  } else {
    cmd_help2(argv[0]);
    return 0;
  }
}

int8_t debug_monitor(uint8_t argc, char **argv)
{
  GPIO_PinState state;

  state = HAL_GPIO_ReadPin(IN_ALARM_GPIO_Port, IN_ALARM_Pin);
  PRINT("Alarm signal is %s\r\n", state == GPIO_PIN_SET ? "high" : "low");
  state = HAL_GPIO_ReadPin(Ready_GPIO_Port, Ready_Pin);
  PRINT("Ready signal is %s\r\n", state == GPIO_PIN_SET ? "high" : "low");
  state = HAL_GPIO_ReadPin(BUTTON_N_GPIO_Port, BUTTON_N_Pin);
  PRINT("Button signal is %s\r\n", state == GPIO_PIN_SET ? "high" : "low");
  state = HAL_GPIO_ReadPin(MOD_ABS_GPIO_Port, MOD_ABS_Pin);
  PRINT("MOD_ABS signal is %s\r\n", state == GPIO_PIN_SET ? "high" : "low");

  return 0;
}

int8_t debug_print_hex(uint8_t argc, char **argv)
{
  if (!strcasecmp(argv[2], "on")) {
    print_trans_data = 1;
  } else if (!strcasecmp(argv[2], "off")) {
    print_trans_data = 0;
  } else {
    cmd_help2(argv[0]);
    return 0;
  }
  
  return 0;
}

int8_t debug_pin(uint8_t argc, char **argv)
{
  uint32_t val, type = 0xFF;
  GPIO_TypeDef *port = NULL;
  uint16_t pin = 0;

  if (!strcasecmp(argv[2], "hard_reset")) {
    port = HARD_RESET_GPIO_Port;
    pin = HARD_RESET_Pin;
  } else {
    cmd_help2(argv[0]);
    return 0;
  }

  val = strtoul(argv[3], NULL, 10);
  if (type == 0xFF) {
    if (val) {
      HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    }
  }

  return 0;
}

int8_t debug_set_tim(uint8_t argc, char **argv)
{
  if (!strcasecmp(argv[2], "on")) {
    HAL_TIM_Base_Start_IT(&htim3);
  } else if (!strcasecmp(argv[2], "off")) {
    HAL_TIM_Base_Stop_IT(&htim3);
  } else {
    cmd_help2(argv[0]);
    return 0;
  }
  
  return 0;
}

int8_t debug_set_freq(uint8_t argc, char **argv)
{
  uint32_t value = strtoul(argv[2], NULL, 10);
  if (value == 0) {
    return 1;
  }
  tim_counter_max = value;
  return 0;
}

int8_t debug_spi(uint8_t argc, char **argv)
{
  uint8_t chan;
  uint8_t txbuf[2], rxbuf[2], chan_rb;
  HAL_StatusTypeDef hal_status;
  uint16_t val;
  
  chan = (uint8_t)strtoul(argv[2], NULL, 10);
  if (chan > 15) {
    PRINT("Channel number invalid\r\n");
    return 1;
  }
  txbuf[0] = (0x1 << 4) | (0x0 << 3) | (chan >> 1);
  txbuf[1] = chan << 7;
  PRINT("txbuf: %#X, %#X\r\n", txbuf[0], txbuf[1]);

  HAL_GPIO_WritePin(SPI5_CS_GPIO_Port, SPI5_CS_Pin, GPIO_PIN_RESET);
  hal_status = HAL_SPI_TransmitReceive(&hspi5, txbuf, rxbuf, 2, 50);
  HAL_GPIO_WritePin(SPI5_CS_GPIO_Port, SPI5_CS_Pin, GPIO_PIN_SET);
  //osDelay(1);
  PRINT("rxbuf: %#X, %#X\r\n", rxbuf[0], rxbuf[1]);

  HAL_GPIO_WritePin(SPI5_CS_GPIO_Port, SPI5_CS_Pin, GPIO_PIN_RESET);
  hal_status = HAL_SPI_TransmitReceive(&hspi5, txbuf, rxbuf, 2, 50);
  HAL_GPIO_WritePin(SPI5_CS_GPIO_Port, SPI5_CS_Pin, GPIO_PIN_SET);
  //osDelay(1);
  PRINT("rxbuf: %#X, %#X\r\n", rxbuf[0], rxbuf[1]);

  HAL_GPIO_WritePin(SPI5_CS_GPIO_Port, SPI5_CS_Pin, GPIO_PIN_RESET);
  hal_status = HAL_SPI_TransmitReceive(&hspi5, txbuf, rxbuf, 2, 50);
  HAL_GPIO_WritePin(SPI5_CS_GPIO_Port, SPI5_CS_Pin, GPIO_PIN_SET);
  PRINT("rxbuf: %#X, %#X\r\n", rxbuf[0], rxbuf[1]);

  if (hal_status != HAL_OK) {
    PRINT("Read ADC7953_SPI5 failed, ErrorCode = %#X\r\n", hspi5.ErrorCode);
    return 2;
  }

  chan_rb = rxbuf[0] >> 4;
  if (chan_rb != chan) {
    PRINT("Read ADC7953_SPI5 failed, rChanIdx != chanIdx\r\n");
    return 3;
  } else {
    val = ((rxbuf[0] & 0xf) << 8) | rxbuf[1];
    PRINT("Value = %u\r\n", val);
  }

  return 0;
}

int8_t debug_send_hex(uint8_t argc, char **argv)
{
  char *p;
  uint8_t *p_data = fw_buf + 1024 * 10;
  uint32_t i;
  uint8_t rcv_crc;
  uint8_t rcv_len, err_code;

  HAL_Delay(1);
  __HAL_UART_DISABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
  CLEAR_BIT(TERMINAL_UART.Instance->SR, USART_SR_RXNE);
  __HAL_UART_FLUSH_DRREGISTER(&TERMINAL_UART);
  uart1_irq_sel = 0;
  PRINT("Put data in 15 seconds...\r\n");

  memset(fw_buf, 0, 1024 * 10);
  if (HAL_UART_Receive(&TERMINAL_UART, fw_buf, 1024 * 10, 1000 * 15) != HAL_TIMEOUT) {
    PRINT("Failed\r\n");
    __HAL_UART_ENABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
    uart1_irq_sel = 1;
    return 1;
  }

  __HAL_UART_ENABLE_IT(&TERMINAL_UART, UART_IT_RXNE);
  uart1_irq_sel = 1;

  if (strlen((char*)fw_buf) == 0) {
    PRINT("No data received\r\n");
  } else {
    for (p = strtok((char*)fw_buf, " \t\r\n,"), i = 0; p != NULL; ++i, p = strtok(NULL, " \t\r\n,")) {
      p_data[i] = (uint8_t)strtoul(p, NULL, 0);
    }
    if (argc == 3 && !strcasecmp(argv[2], "no_check")) {
      rcv_crc = Cal_Check((uint8_t*)&p_data[1], i - 1);
      p_data[i] = rcv_crc;
      i += 1;
    }
    PRINT_HEX("tx_buf", p_data, i);
    if (HAL_UART_Transmit(&COMMUNICATION_UART, p_data, i, 0xFF) != HAL_OK) {
      EPT("Transmit failed\r\n");
      return 100;
    }
    rBuf[0] = 0;
    while (rBuf[0] != TRANS_START_BYTE) {
      if (HAL_UART_Receive(&COMMUNICATION_UART, rBuf, 1, 950) != HAL_OK) {
        EPT("Receive failed : Received timeout 1\r\n");
        return 101;
      }
    }
    if (HAL_UART_Receive(&COMMUNICATION_UART, rBuf + 1, 1, 0xFF) != HAL_OK) {
      EPT("Receive failed : Received timeout 2\r\n");
      return 102;
    }
    rcv_len = rBuf[1];
    if (HAL_UART_Receive(&COMMUNICATION_UART, rBuf + 2, rcv_len - 1, 0xFF) != HAL_OK) {
      EPT("Receive failed : Received timeout 3\r\n");
      PRINT_HEX("Received messages", rBuf, 2);
      return 103;
    }
    PRINT_HEX("rx_buf", rBuf, rcv_len + 1);

    rcv_crc = Cal_Check(&rBuf[1], rcv_len - 1);
    if (rcv_crc != rBuf[1 + rcv_len - 1]) {
      EPT("Checksum failed\r\n");
      return 104;
    }

    err_code = rBuf[1 + rcv_len - 2];

    PRINT("Returned status from module is %d (= %#X)\r\n", err_code, err_code);
    if (err_code != 0) {
      return 105;
    }
  }

  return 0;
}



int8_t process_command(uint16_t cmd, uint8_t *pdata, uint8_t len, uint8_t *rx_buf, uint8_t *rx_len)
{
  uint8_t cmd_len = 0;
  uint32_t print_len;
  uint8_t rcv_crc;
  uint8_t rcv_len, err_code;
  uint32_t tim2_counter_value;

  CLEAR_BIT(COMMUNICATION_UART.Instance->SR, USART_SR_RXNE);
  __HAL_UART_FLUSH_DRREGISTER(&COMMUNICATION_UART);

  communication_buf[cmd_len++] = TRANS_START_BYTE;
  communication_buf[cmd_len++] = 1 + 2 + len + 1; // Length + command + data + chk
  communication_buf[cmd_len++] = (uint8_t)(cmd >> 8);
  communication_buf[cmd_len++] = (uint8_t)cmd;
  if (len) {
    memcpy(&communication_buf[cmd_len], pdata, len);
    cmd_len += len;
  }
  communication_buf[cmd_len++] = Cal_Check((uint8_t*)&communication_buf[1], 1 + 2 + len);

  //PRINT("tx crc32 = %#X\r\n", rcv_crc);
  if (print_trans_data && switch_endian_16(*(uint16_t*)&communication_buf[CMD_SEQ_MSG_ID]) != CMD_FOR_DEBUG) {
    if (cmd_len > 0xFF) print_len = 0x100;
    else print_len = cmd_len;
#if 0
    if (switch_endian(*(uint32_t*)&communication_buf[CMD_SEQ_MSG_ID]) != CMD_UPGRADE_DATA) {
      //PRINT_HEX("tx_buf", communication_buf, print_len);
    }
#endif
    PRINT_HEX("tx_buf", communication_buf, print_len);
  }

  HAL_TIM_Base_Init(&htim2);
  tim2_counter = 0;
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
  HAL_TIM_Base_Start_IT(&htim2);
  if (HAL_UART_Transmit(&COMMUNICATION_UART, communication_buf, cmd_len, 0xFF) != HAL_OK) {
    EPT("Transmit failed\r\n");
    return 100;
  }

  rx_buf[0] = 0;
  while (rx_buf[0] != TRANS_START_BYTE) {
    if (HAL_UART_Receive(&COMMUNICATION_UART, rx_buf, 1, 950) != HAL_OK) {
      EPT("Receive failed : Received timeout 1\r\n");
      return 101;
    }
  }
  if (HAL_UART_Receive(&COMMUNICATION_UART, rx_buf + 1, 1, 0xFF) != HAL_OK) {
    EPT("Receive failed : Received timeout 2\r\n");
    return 102;
  }
  rcv_len = rx_buf[1];
  if (HAL_UART_Receive(&COMMUNICATION_UART, rx_buf + 2, rcv_len - 1, 0xFF) != HAL_OK) {
    EPT("Receive failed : Received timeout 3\r\n");
    PRINT_HEX("Received messages", rx_buf, 2);
    return 103;
  }
  HAL_TIM_Base_Stop_IT(&htim2);
  tim2_counter_value = __HAL_TIM_GET_COUNTER(&htim2);
  if (print_trans_data && switch_endian_16(*(uint16_t*)&communication_buf[CMD_SEQ_MSG_ID]) != CMD_FOR_DEBUG) {
    print_len = rcv_len + 1;
#if 0
    if (switch_endian(*(uint32_t*)&rx_buf[CMD_SEQ_MSG_ID]) != CMD_UPGRADE_DATA && 
      switch_endian(*(uint32_t*)&rx_buf[CMD_SEQ_MSG_ID]) != CMD_QUERY_LOG) {
      //PRINT_HEX("rx_buf", rx_buf, print_len);
    }
#endif
    PRINT_HEX("rx_buf", rx_buf, print_len);
  }

  rcv_crc = Cal_Check(&rx_buf[1], rcv_len - 1);
  if (rcv_crc != rx_buf[1 + rcv_len - 1]) {
    EPT("Check failed\r\n");
    return 104;
  }

  err_code = rx_buf[1 + rcv_len - 2];

  PRINT("Time is %ums\r\n", tim2_counter * 1500 + tim2_counter_value);
  PRINT("Returned status from module is %d (= %#X)\r\n", err_code, err_code);
  if (err_code != 0) {
    return 105;
  }
  *rx_len = rcv_len + 1;

  return 0;
}

uint8_t Cal_Check(uint8_t *pdata, uint32_t len)
{
  uint32_t i;
  uint8_t chk = 0;

  for (i = 0; i < len; ++i) {
    chk ^= pdata[i];
  }
  
  return (chk + 1);
}

/**
  * @brief  Update CRC16 for input byte
  * @param  crc_in input value 
  * @param  input byte
  * @retval None
  */
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
  uint32_t crc = crc_in;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  }
  
  while(!(in & 0x10000));

  return crc & 0xffffu;
}

/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = p_data+size;

  while(p_data < dataEnd)
    crc = UpdateCRC16(crc, *p_data++);
 
  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc&0xffffu;
}

uint32_t Cal_CRC32(uint8_t* packet, uint32_t length)
{
  static uint32_t CRC32_TABLE[] = {
    /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
  };

  uint32_t CRC32 = 0xFFFFFFFF;
  for (uint32_t i = 0; i < length; i++) {
    CRC32 = (CRC32_TABLE[((CRC32) ^ (packet[i])) & 0xff] ^ ((CRC32) >> 8));
  }

  return ~CRC32;
}

void PRINT_HEX(char *head, uint8_t *pdata, uint32_t len)
{
  uint32_t i;
  
  PRINT("************* PRINT HEX *************\r\n");
  PRINT("%s:\r\n", head);
  for (i = 0; i < len; ++i) {
    if (i % 0x10 == 0) {
      HAL_Delay(5);
      PRINT("%08X : ", i / 0x10);
    }
    PRINT("0x%02X%s", pdata[i], (i + 1) % 0x10 == 0 ? "\r\n" : i == len - 1 ? "\r\n" : " ");
  }
  PRINT("************* PRINT END *************\r\n");
}

void PRINT_CHAR(char *head, uint8_t *pdata, uint32_t len)
{
  uint32_t i;
  
  PRINT("************* PRINT CHAR *************\r\n");
  PRINT("%s:\r\n", head);
  for (i = 0; i < len; ++i) {
    if (i % 0x40 == 0) {
      HAL_Delay(5);
      PRINT("%08X : ", i / 0x40);
    }
    PRINT("%c%s", pdata[i] == '\n' ? 'N' : pdata[i] == '\r' ? 'R' : pdata[i],\
          (i + 1) % 0x40 == 0 ? "\r\n" : i == len - 1 ? "\r\n" : "");
  }
  PRINT("************* PRINT CHAR *************\r\n");
}

uint32_t switch_endian(uint32_t i)
{
  return (((i>>24)&0xff) | ((i>>8)&0xff00) | ((i<<8)&0xff0000) | ((i<<24)&0xff000000));
}

uint16_t switch_endian_16(uint16_t i)
{
  return (((i>>8)&0xff) | ((i<<8)&0xff00));
}

void BE32_To_Buffer(uint32_t data, uint8_t *pbuf)
{
  pbuf[0] = (uint8_t)(data >> 24);
  pbuf[1] = (uint8_t)(data >> 16);
  pbuf[2] = (uint8_t)(data >> 8);
  pbuf[3] = (uint8_t)(data);
}

uint32_t Buffer_To_BE32(uint8_t *pbuf)
{
  return (pbuf[0] << 24) | (pbuf[1] << 16) | (pbuf[2] << 8) | (pbuf[3]);
}

void BE16_To_Buffer(uint16_t data, uint8_t *pbuf)
{
  pbuf[0] = (uint8_t)(data >> 8);
  pbuf[1] = (uint8_t)(data);
}

uint16_t Buffer_To_BE16(uint8_t *pbuf)
{
  return (pbuf[0] << 8) | (pbuf[1]);
}
