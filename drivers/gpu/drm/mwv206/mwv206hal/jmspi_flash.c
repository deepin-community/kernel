/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifdef __linux__

#include "mwv206dev.h"
#include "jmspi.h"
#include "jmspi_flash.h"


#define MAC206HAL001        4
#define MAC206HAL033           0x0
#define MAC206HAL034           0x1

#define MAC206HAL003        (MAC206HAL048)
#define MAC206HAL002          (MAC206HAL003 + MAC206HAL001)

#define MAC206HAL048     (0x5E000)



#define MAC206HAL027           1

#define MAC206HAL028        0

#define MAC206HAL029            0

#define MAC206HAL030         0

#define MAC206HAL185       0x05
#define MAC206HAL189       0x01
#define MAC206HAL184       0x50
#define MAC206HAL187       0x04
#define MAC206HAL186       0x03
#define MAC206HAL188       0x06
#define MAC206HAL139              0x02

#define MAC206HAL035 60
#define MAC206HAL005     0x08
#define MAC206HAL004     0x04
#define MAC206HAL224     0x02
#define MAC206HAL225     0x01
#define MAC206HAL140 5

#define MAC206HAL070(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

static struct flash_device FUNC206HAL181[] = {
    {"UNKNOW TYPE", 0x20, 0x112233, 0x100, 4 * 1024,  512 * 1024,         0x9f},

    {"m25p40",      0xd8, 0x202013, 0x100, 64 * 1024,  512 * 1024,        0x9f},
    {"m25p80",      0xd8, 0x202014, 0x100, 64 * 1024,  1 * 1024 * 1024,   0x9f},
    {"m25p16",      0xd8, 0x202015, 0x100, 64 * 1024,  2 * 1024 * 1024,   0x9f},
    {"m25p32",      0xd8, 0x202016, 0x100, 64 * 1024,  4 * 1024 * 1024,   0x9f},
    {"m25p64",      0xd8, 0x202017, 0x100, 64 * 1024,  8 * 1024 * 1024,   0x9f},
    {"m25p128",     0xd8, 0x202018, 0x100, 256 * 1024, 16 * 1024 * 1024,  0x9f},
    {"m25pe16",     0x20, 0x208015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"m25px16",     0x20, 0x207115, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"m25px64",     0x20, 0x207117, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},

    {"m45pe80",     0xd8, 0x402014, 0x100, 64 * 1024,  1 * 1024 * 1024,   0x9f},


    {"s25fl004",    0xd8, 0x020112, 0x100, 64 * 1024,  512 * 1024,        0x9f},
    {"s25fl008",    0x20, 0x020113, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"s25fl016",    0x20, 0x020114, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"s25fl032",    0x20, 0x020115, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"s25fl064",    0x20, 0x020116, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"25l8005",     0x20, 0xC22014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"25l1605",     0x20, 0xC22015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"25l3205",     0x20, 0xC22016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"25l6405",     0x20, 0xC22017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},


    {"mt25q032",    0x20, 0x20ba16, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"mt25q64",     0x20, 0x20ba17, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"mt25q128",    0x20, 0x20ba18, 0x100, 4 * 1024,   16 * 1024 * 1024,  0x9f},
    {"mt25q256",    0x20, 0x20ba19, 0x100, 4 * 1024,   32 * 1024 * 1024,  0x9f},
    {"mt25q512",    0x20, 0x20ba20, 0x100, 4 * 1024,   64 * 1024 * 1024,  0x9f},
    {"mt25q1Gb",    0x20, 0x20ba21, 0x100, 4 * 1024,   128 * 1024 * 1024, 0x9f},
    {"mt25q2Gb",    0x20, 0x20ba22, 0x100, 4 * 1024,   256 * 1024 * 1024, 0x9f},



    {"GD25Q256C",   0x20, 0xc84019, 0x100, 4 * 1024,   32 * 1024 * 1024,  0x9f},
    {"GD25Q127C",   0x20, 0xc84018, 0x100, 4 * 1024,   16 * 1024 * 1024,  0x9f},
    {"GD25Q64C",    0x20, 0xc84017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"GD25Q32C",    0x20, 0xc84016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"GD25Q16C",    0x20, 0xc84015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"GD25Q80C",    0x20, 0xc84014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"GD25Q40C",    0x20, 0xc84013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"GD25VQ40C",   0x20, 0xc84213, 0x100, 4 * 1024,   512 * 1024,        0x9f},

    {"fm25q08a",    0x20, 0xa14014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},


    {"W25P40",      0xd8, 0xef2012, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"W25P80",      0xd8, 0xef2014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"W25P16",      0xd8, 0xef2015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"W25P32",      0xd8, 0xef2016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"W25Q40BL",    0x20, 0xef4013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"W25Q80BL",    0x20, 0xef4014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"W25Q16CL",    0x20, 0xef4015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"W25Q32",      0x20, 0xef4016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"W25q64CV",    0x20, 0xef4017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"W25Q128",     0x20, 0xef4018, 0x100, 4 * 1024,   16 * 1024 * 1024,  0x9f},
    {"W25Q256",     0x20, 0xef4019, 0x100, 4 * 1024,   32 * 1024 * 1024,  0x9f},
    {"W25Q40EW",    0x20, 0xef6013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"W25X40CL",    0x20, 0xef3013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"W25X80",      0x20, 0xef3014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"W25X16",      0x20, 0xef3015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"W25X32",      0x20, 0xef3016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"W25X64",      0x20, 0xef3017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},


    {"MX25L4005a",  0x20, 0xC22013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"MX25L8005",   0x20, 0xC22014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"MX25L1606E",  0x20, 0xC22015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"MX25L3205D",  0x20, 0xC22016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"MX25L6405D",  0x20, 0xC22017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},




    {"IS25LQ040B",  0x20, 0x9d4013, 0x100, 4 * 1024,   512 * 1024, 0x9f},
    {"IS25LP080",   0x20, 0x9d6014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"IS25LP016",   0x20, 0x9d6015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"IS25LP032",   0x20, 0x9d6016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"IS25LP064",   0x20, 0x9d6017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"IS25LP128",   0x20, 0x9d6018, 0x100, 4 * 1024,   16 * 1024 * 1024,  0x9f},
    {"IS25LP256",   0x20, 0x9d6019, 0x100, 4 * 1024,   32 * 1024 * 1024,  0x9f},
    {"IS25LP512",   0x20, 0x9d601A, 0x100, 4 * 1024,   64 * 1024 * 1024,  0x9f},


    {"en25q40",     0x20, 0x1c3013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"en25q80",     0x20, 0x1c3014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"en25q16",     0X20, 0X1c3015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"en25q32b",    0x20, 0x1c3016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"en25q64",     0x20, 0x1c3017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"en25q128",    0x20, 0x1c3018, 0x100, 4 * 1024,   16 * 1024 * 1024,  0x9f},
    {"en25qh16",    0x20, 0x1c7015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"en25qh32",    0x20, 0x1c7016, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"en25qh64",    0x20, 0x1c7017, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},
    {"en25qh128",   0x20, 0x1c7018, 0x100, 4 * 1024,   16 * 1024 * 1024,  0x9f},


    {"sst25vf040b", 0x20, 0xbf258d, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"sst25vf080b", 0x20, 0xbf258e, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"sst25vf016b", 0x20, 0xbf2541, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
    {"sst25vf032b", 0x20, 0xbf254a, 0x100, 4 * 1024,   4 * 1024 * 1024,   0x9f},
    {"sst25vf064c", 0x20, 0xbf254b, 0x100, 4 * 1024,   8 * 1024 * 1024,   0x9f},


    {"P25Q40H",     0x20, 0x856013, 0x100, 4 * 1024,   512 * 1024,        0x9f},
    {"P25Q80H",     0x20, 0x856014, 0x100, 4 * 1024,   1 * 1024 * 1024,   0x9f},
    {"P25Q16H",     0x20, 0x856015, 0x100, 4 * 1024,   2 * 1024 * 1024,   0x9f},
};

int FUNC206HAL178(JMSPI pSpiDev, int slave, struct flash_device *fdev)
{
	int result;
	unsigned int i, readid;

	uint8_t cmdContent[5] = {0};
	uint8_t rxBuffer[5] = {0};

	unsigned int sendLength = 0;
	unsigned int recvLength = 0;

	for (i = 0; i < MAC206HAL070(FUNC206HAL181); ++i) {
		cmdContent[0] = FUNC206HAL181[i].rdid_cmd;
		cmdContent[1] = 0xFF;
		cmdContent[2] = 0xFF;
		cmdContent[3] = 0xFF;

		sendLength = 0;
		recvLength = 3;

		result = FUNC206HAL172(pSpiDev,
				slave,
				cmdContent,
				4,
				NULL,
				&sendLength,
				rxBuffer,
				&recvLength,
				1,
				4,
				MAC206HAL027,
				MAC206HAL028,
				MAC206HAL029,
				MAC206HAL030
				);

		if (0 != result) {
			V206DEV005("[%s]:failed, result = %d\n", __FUNCTION__, result);
			return result;
		}

		readid = (rxBuffer[0] << 16) | (rxBuffer[1] << 8) | rxBuffer[2];

		if (1) {
			V206DEV005("[%s]:read id %#x\n", __FUNCTION__, readid);
		}

		if (readid == FUNC206HAL181[i].id) {
			memcpy(fdev, &FUNC206HAL181[i], sizeof(struct flash_device));
			return 0;
		}

	}
	return -4;
}


static int FUNC206HAL179(
		JMSPI pSpiDev,
		int slave, unsigned int Dst,
		unsigned int NByte,
		uint8_t *RecvBufPt)
{
	int result = -1;
	unsigned int recycleCount;
	uint8_t txBuffer[20] = {0};
	uint8_t rxBuffer[MAC206HAL035];

	uint8_t cmdContent[4] = {0};
	unsigned int readAddr;
	unsigned int txlen = 0;
	unsigned int rxlen = 0;

	V206DEV005("[%s]:addr %#x, len %d\n", __FUNCTION__, Dst, NByte);
	if (NByte != 1) {
		V206DEV005("just support 1 byte read!\n");
		return -1;
	}

	cmdContent[0] = MAC206HAL186;
	recycleCount = NByte;

	readAddr = Dst;
	cmdContent[1] = (readAddr & 0xFFFFFF) >> 16;
	cmdContent[2] = (readAddr & 0xFFFF) >> 8;
	cmdContent[3] = readAddr & 0xFF;
	rxlen = MAC206HAL035;
	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			4,
			txBuffer,
			&txlen,
			rxBuffer,
			&rxlen,
			4,
			MAC206HAL035 + 4,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030
			);

	if (0 != result) {
		V206DEV005("[ERROR] %s failed, result = %d\n",
				__FUNCTION__, result);
		return result;
	}

	*RecvBufPt = rxBuffer[0];
	V206DEV005("rxBuffer[0]: %#x\n", rxBuffer[0]);

	return result;

}

static uint32_t FUNC206HAL175(JMSPI pSpiDev)
{
	uint8_t     i, ch;
	uint32_t    addr;
	addr = MAC206HAL003;


	FUNC206HAL179(pSpiDev, 0, addr, 1, &ch);

	if (0xFF == ch) {
		return 0;
	}
	for (i = 0; i < MAC206HAL001; i++) {

		FUNC206HAL179(pSpiDev, 0, addr + 1, 1, &ch);
		if (0xFF == ch) {
			break;
		}
		addr++;
	}
	return addr;
}

int FUNC206HAL180(JMSPI pSpiDev,
		int slave,
		unsigned int sectorAddr,
		char erasecmd)
{
	int result, scratch;
	uint8_t cmdContent[5];
	uint8_t rxBuffer[5];
	unsigned int eraseSectorAddr;

	unsigned int sendLength = 0;
	unsigned int recvLength = 0;
	GLJ_TICK tick;



	cmdContent[0] = MAC206HAL188;
	sendLength = 0;
	recvLength = 0;

	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			1,
			NULL,
			&sendLength,
			NULL,
			&recvLength,
			0,
			1,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030);
	if (0 != result) {
		V206DEV005("%s failed at %d, result = %d\n", __FUNCTION__, __LINE__, result);
		return result;
	}


	eraseSectorAddr = sectorAddr;
	cmdContent[0] = erasecmd;
	cmdContent[1] = (eraseSectorAddr & 0xFFFFFF) >> 16;
	cmdContent[2] = (eraseSectorAddr & 0xFFFF) >> 8;
	cmdContent[3] = eraseSectorAddr & 0xFF;

	sendLength = 0;
	recvLength = 0;


	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			4,
			NULL,
			&sendLength,
			NULL,
			&recvLength,
			0,
			4,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030);
	if (0 != result) {
		V206DEV005("%s failed at %d, result = %d\n", __FUNCTION__, __LINE__, result);
		return result;
	}

	cmdContent[0] = MAC206HAL185;
	cmdContent[1] = 0xFF;

	mwv206_timed_do (tick, scratch, FUNC206LXDEV098() * MAC206HAL140) {


		memset(rxBuffer, 0, 5);

		sendLength = 0;
		recvLength = 1;



		result = FUNC206HAL172(pSpiDev,
				slave,
				cmdContent,
				2,
				NULL,
				&sendLength,
				rxBuffer,
				&recvLength,
				1,
				2,
				MAC206HAL027,
				MAC206HAL028,
				MAC206HAL029,
				MAC206HAL030);
		if (0 != result) {
			V206DEV005("%s failed at %d, result = %d\n", __FUNCTION__, __LINE__, result);
			return result;
		}
		if (MAC206HAL225 != (rxBuffer[0]&MAC206HAL225)) {
			break;
		}
	}

	if (MAC206HAL225 == (rxBuffer[0]&MAC206HAL225)) {
		V206DEV005("m25pxx_Sector_Erase time out!\n");
	}


	cmdContent[0] = MAC206HAL187;
	sendLength = 0;
	recvLength = 0;

	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			1,
			NULL,
			&sendLength,
			NULL,
			&recvLength,
			0,
			1,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030);

	if (0 != result) {
		V206DEV005("%s failed at %d, result = %d\n", __FUNCTION__, __LINE__, result);
		return result;
	}

	return 0;
}

static int FUNC206HAL182(JMSPI pSpiDev,
		int slave,
		unsigned int Dst,
		unsigned int NByte,
		const uint8_t *SndbufPt)
{
	int result = 0, scratch;

	uint8_t rxBuffer[4] = {0};
	uint8_t cmdContent[4] = {0};

	unsigned int sendLength = 0;
	unsigned int recvLength = 0;
	GLJ_TICK tick;

	if (NULL == SndbufPt) {
		V206DEV005("Sendbuf == NULL\n");
		return -1;
	}


	cmdContent[0] = MAC206HAL188;
	sendLength = 0;
	recvLength = 0;


	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			1,
			NULL,
			&sendLength,
			NULL,
			&recvLength,
			0,
			1,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030);
	if (0 != result) {
		V206DEV005("[%s]: failed for enable writing, result = %d\n",
				__FUNCTION__, result);
		return result;
	}


	cmdContent[0] = MAC206HAL139;
	cmdContent[1] = (Dst & 0xFFFFFF) >> 16;
	cmdContent[2] = (Dst & 0xFFFF) >> 8;
	cmdContent[3] = Dst & 0xFF;
	sendLength = NByte;
	recvLength = 0;


	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			4,
			(uint8_t *)SndbufPt,
			&sendLength,
			NULL,
			&recvLength,
			0,
			4 + NByte,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030);
	if (0 != result) {
		V206DEV005("mp25p10_Write failed for enable writing, result = %d\n", result);
		goto FUNC206HAL014;
	} else {
		V206DEV005("W (s[%dB],r[%dB])\n", sendLength, recvLength);
	}


	cmdContent[0] = MAC206HAL185;
	cmdContent[1] = 0xFF;
	cmdContent[2] = 0xFF;

	mwv206_timed_do (tick, scratch, FUNC206LXDEV098() * MAC206HAL140) {

		memset(rxBuffer, 0, sizeof(rxBuffer));

		sendLength = 0;
		recvLength = 1;



		result = FUNC206HAL172(pSpiDev,
				slave,
				cmdContent,
				3,
				NULL,
				&sendLength,
				rxBuffer,
				&recvLength,
				1,
				3,
				MAC206HAL027,
				MAC206HAL028,
				MAC206HAL029,
				MAC206HAL030);
		if (0 != result) {
			V206DEV005("mp25p10_Write failed for read status register, result = %d\n", result);
			goto FUNC206HAL014;
		}
		if (MAC206HAL225 != (rxBuffer[0]&MAC206HAL225)) {
			break;
		}
	}


FUNC206HAL014:

	cmdContent[0] = MAC206HAL187;
	sendLength = 0;
	recvLength = 0;


	result = FUNC206HAL172(pSpiDev,
			slave,
			cmdContent,
			1,
			NULL,
			&sendLength,
			NULL,
			&recvLength,
			0,
			1,
			MAC206HAL027,
			MAC206HAL028,
			MAC206HAL029,
			MAC206HAL030);

	if (0 != result) {
		V206DEV005("mp25p10_Write failed for enable status register write, result = %d\n", result);
		return result;
	}

	V206DEV005("W[0x%X] finish\n", Dst);

	return result;
}

static void FUNC206HAL174(JMSPI pSpiDev, struct flash_device *fdev)
{
#if 0
	int             ret, sectorsize;
	flash_info_t    flashinfo;
	ret = flash_init(&flashinfo);
	if (ret != 0) {
		return;
	}
	sectorsize = flashinfo.sector_size;
	flash_erase(&flashinfo, MAC206HAL003 / sectorsize, MAC206HAL003 / sectorsize);
#endif

	FUNC206HAL180(pSpiDev, 0, MAC206HAL003, fdev->erase_cmd);
}

static uint8_t FUNC206HAL176(uint8_t flag)
{

	if ((flag & 0xC0) != 0xC0) {
		return 6;
	} else if ((flag & 0x30) != 0x30) {
		return 4;
	} else if ((flag & 0xC) != 0xC) {
		return 2;
	}
	return 0;
}



void FUNC206HAL184(JMSPI pSpiDev)
{
	uint32_t addr = 0;
	uint8_t ch = 0, pos = 0;
	int ret;
	struct flash_device flashdev;

	ret = FUNC206HAL178(pSpiDev, 0, &flashdev);

	if (ret != 0) {
		return;
	}

	if (0xEF6013 == flashdev.id) {
		return;
	}

	addr = FUNC206HAL175(pSpiDev);

	if (0 == addr) {
		addr = MAC206HAL003;
	}



	FUNC206HAL179(pSpiDev, 0, addr, 1, &ch);

	if (0xFF != ch) {
		pos = FUNC206HAL176(ch);

		if (MAC206HAL034 == ((ch >> pos) & 0x3)) {
			return ;
		}
		pos += 2;
		if (8 <= pos) {

			addr++;


			FUNC206HAL179(pSpiDev, 0, addr, 1, &ch);
			if (addr >= MAC206HAL002) {


				FUNC206HAL174(pSpiDev, &flashdev);
				addr = MAC206HAL003;
			}
			pos = 0;
		}
	}


	ch = ch & ~(0x3 << pos);
	ch |= MAC206HAL034 << pos;


	FUNC206HAL182(pSpiDev, 0, addr, 1, &ch);
}

void FUNC206HAL183(JMSPI pSpiDev)
{
	uint32_t addr = 0;
	uint8_t ch = 0, pos = 0;
	int ret;
	struct flash_device flashdev;

	ret = FUNC206HAL178(pSpiDev, 0, &flashdev);

	if (ret != 0) {
		return;
	}

	if (0xEF6013 == flashdev.id) {
		return;
	}

	addr = FUNC206HAL175(pSpiDev);

	if (0 == addr) {
		addr = MAC206HAL003;
	}


	FUNC206HAL179(pSpiDev, 0, addr, 1, &ch);

	V206DEV005("read:addr %#x, ch %#x\n", addr, ch);

	pos = FUNC206HAL176(ch);

	ch = ch & ~(0x3 << pos);
	ch |= MAC206HAL033 << pos;


	FUNC206HAL182(pSpiDev, 0, addr, 1, &ch);
	V206DEV005("write:addr %#x, ch %#x\n", addr, ch);
}

#endif