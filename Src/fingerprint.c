#include "fingerprint.h"
#include "string.h"

char *msgFinger = "FINGERPRINT_TIMEOUT\r\n";
uint32_t fingerPassword = 0;
uint32_t fingerAddress = 0xFFFFFFFF;

uint8_t FINGERPRINT_VerifyPassword() {
	uint8_t packet[] = { FINGERPRINT_VERIFYPASSWORD, (fingerPassword >> 24),
			(fingerPassword >> 16), (fingerPassword >> 8), fingerPassword };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 7,
			packet);

	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);

	if ((status == HAL_OK) && (finger_RxBuf[6] == FINGERPRINT_ACKPACKET)
			&& (finger_RxBuf[9] == FINGERPRINT_OK)) {
		return HAL_OK;
	}

#ifdef FINGERPRINT_DEBUG
	if (status == HAL_TIMEOUT) {
		HAL_UART_Transmit_IT(&huart2, (uint8_t*) msgFinger, 20);
	}
#endif
	return HAL_ERROR;
}

uint8_t FINGERPRINT_GetImage() {
	uint8_t packet[] = { FINGERPRINT_GETIMAGE };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 3,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	return finger_RxBuf[9];
}
uint8_t FINGERPRINT_GetChar(uint8_t slot) {
	uint8_t packet[] = { FINGERPRINT_GETCHAR, slot };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 5,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	return finger_RxBuf[9];
}
uint8_t FINGERPRINT_Search() {
	fingerID = 0xFFFF;
	matchScore = 0xFFFF;
	// high speed search of slot #1 starting at page 0x0000 and page #0x00A3
	uint8_t packet[] = { FINGERPRINT_LOWSPEEDSEARCH, 0x01, 0x00, 0x00, 0x00,
			0xA3 };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 8,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 16,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	fingerID = finger_RxBuf[10];
	fingerID <<= 8;
	fingerID |= finger_RxBuf[11];
	matchScore = finger_RxBuf[12];
	matchScore <<= 8;
	matchScore |= finger_RxBuf[13];
	return finger_RxBuf[9];
}

uint8_t FINGERPRINT_CreateModel() {
	uint8_t packet[] = { FINGERPRINT_REGMODEL };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 3,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	return finger_RxBuf[9];
}

uint8_t FINGERPRINT_StoreModel(uint16_t id) {
	//read a fingerprint template from flash into Char Buffer 1
	uint8_t packet[] = { FINGERPRINT_STORE, 0x01, id >> 8, id & 0xFF };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 6,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	return finger_RxBuf[9];
}
uint8_t FINGERPRINT_DeleteModel(uint16_t id) {
	uint8_t packet[] = { FINGERPRINT_DELETE, id >> 8, id & 0xFF, 0x00, 0x01 };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 7,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	return finger_RxBuf[9];
}
uint8_t FINGERPRINT_EmptyDatabase() {
	uint8_t packet[] = { FINGERPRINT_EMPTY };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 3,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 12,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	return finger_RxBuf[9];
}
uint8_t FINGERPRINT_GetTemplateCount() {
	 templateCount = 0xFFFF;
	  // get number of templates in memory
	uint8_t packet[] = { FINGERPRINT_TEMPLATECOUNT };
	FINGERPRINT_WritePacket(fingerAddress, FINGERPRINT_COMMANDPACKET, 3,
			packet);
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, finger_RxBuf, 14,
	FINGERPRINT_DEFAULTTIMEOUT);
	if ((status != HAL_OK) && (finger_RxBuf[6] != FINGERPRINT_ACKPACKET)) {
		return -1;
	}
	templateCount = finger_RxBuf[10];
	templateCount <<= 8;
	templateCount |= finger_RxBuf[11];
	return finger_RxBuf[9];
}
void FINGERPRINT_WritePacket(uint32_t addr, uint8_t packettype, uint16_t len,
		uint8_t *packet) {
	uint8_t tx_buffer[30];
	tx_buffer[0] = (uint8_t) (FINGERPRINT_STARTCODE >> 8);
	tx_buffer[1] = (uint8_t) FINGERPRINT_STARTCODE;
	tx_buffer[2] = (uint8_t) (addr >> 24);
	tx_buffer[3] = (uint8_t) (addr >> 16);
	tx_buffer[4] = (uint8_t) (addr >> 8);
	tx_buffer[5] = (uint8_t) addr;
	tx_buffer[6] = (uint8_t) packettype;
	tx_buffer[7] = (uint8_t) (len >> 8);
	tx_buffer[8] = (uint8_t) len;
	uint16_t sum = (len >> 8) + (len & 0xFF) + packettype;
	uint8_t i = 0;
	for (i = 0; i < len - 2; i++) {
		tx_buffer[9 + i] = (uint8_t) packet[i];
		sum += packet[i];
	}
	tx_buffer[9 + i] = (uint8_t) (sum >> 8);
	tx_buffer[10 + i] = (uint8_t) sum;
	HAL_UART_Transmit(&huart1, tx_buffer, 11 + i, FINGERPRINT_DEFAULTTIMEOUT);
}
