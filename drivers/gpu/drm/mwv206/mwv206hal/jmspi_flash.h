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
#ifndef _MWV206FLASH_H_
#define _MWV206FLASH_H_
#ifdef __cplusplus
extern "C" {
#endif

struct flash_device {
	char *flashname;
	char erase_cmd;
	unsigned int id;
	unsigned int pagesize;
	unsigned long sectorsize;
	unsigned long size_in_bytes;
	unsigned char rdid_cmd;
};


int mwv206flashDetect(int dev, int spi, int slave);


int mwv206flashDetectEx(int dev, int spi, int slave, int *sectorsize, int *size_in_byte);


int mwv206flashDetectExEx(int dev, int spi, int slave, struct flash_device *device);


int mwv206flashGetName(int dev, int spi, int slave, char *flashname);



int mwv206flashEraseAll(int dev, int spi, int slave);


int mwv206flashEraseSector(int dev,
		int spi,
		int slave,
		unsigned int sectoraddr,
		unsigned int sectornum);


int mwv206flashWrite(int dev,
		int spi,
		int slave,
		unsigned int Dst,
		unsigned int NByte,
		const unsigned char *SndbufPt);


int mwv206flashRead(int dev,
		int spi,
		int slave,
		unsigned int Dst,
		unsigned int NByte,
		unsigned char *RcvBufPt);


int mwv206flashDebug(int en);



int mwv206startupDataGen(int dev, char *filename,
		int srcbit, int mode, int color);


int mwv206startupEn(int dev, int en);



int mwv206startupState(int dev, int *state);



int jm7200vbiosVersion(int dev, int flashaddr);

#ifdef __cplusplus
}
#endif

#endif