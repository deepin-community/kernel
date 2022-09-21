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
#ifndef _MWV206ADDRLIST_H_
#define _MWV206ADDRLIST_H_

#include "mwv206dev.h"
#include "mwv206kdebug.h"
#include "gljos.h"
#include "asm/string.h"

typedef unsigned long LISTTYPE;

typedef struct mwv206_list_s {
	LISTTYPE *list;
	int  ecount;
} mwv206_addrlist_t;


static inline mwv206_addrlist_t *mwv206_addrlist_create(int ecount)
{
	mwv206_addrlist_t *plist;
	int size;

	plist = FUNC206LXDEV010(sizeof(mwv206_addrlist_t));
	if (plist == 0) {
		V206KDEBUG002("[ERROR] %s malloc error.\n", __FUNCTION__);
		return NULL;
	}

	plist->ecount = ecount;
	size = ecount * sizeof(LISTTYPE);
	plist->list = FUNC206LXDEV010(size);
	if (plist->list == 0) {
		V206KDEBUG002("[ERROR] %s malloc list-elements error.\n", __FUNCTION__);
		return NULL;
	}

	memset(plist->list, 0, size);
	V206DEV005("[INFO] %s successfully.\n", __FUNCTION__);
	return plist;
}


static inline void mwv206_addrlist_destroy(mwv206_addrlist_t *plist)
{
	if (plist) {
		if (plist->list) {
			FUNC206LXDEV120(plist->list);
		}
		FUNC206LXDEV120(plist);
	}
	V206DEV005("[INFO] %s successfully.\n", __FUNCTION__);
	return;
}


static inline int mwv206_addrlist_insert(mwv206_addrlist_t *plist, LISTTYPE *element,
		int checkrepeat, int engrow)
{
	int i, idx;
	int ecount, esize, size;
	LISTTYPE *list;
	LISTTYPE zero;

	if (plist == NULL) {
		return 0;
	}

	ecount = plist->ecount;
	esize  = sizeof(LISTTYPE);
	list = plist->list;

	V206DEV005("[INFO] %s: %d.\n", __FUNCTION__, __LINE__);
	memset(&zero, 0, esize);
	idx = -1;
	if (checkrepeat) {
		for (i = 0; i < ecount; i++) {

			V206DEV005("i = %d\n", i);
			if ((idx == -1) && (memcmp(list, &zero, esize) == 0)) {
				idx = i;
				V206DEV005("idx = %d\n", idx);
			}

			if (memcmp(list, element, esize) == 0) {
				V206DEV005("[INFO] has been save to list(%p)[%d] before.\n", list, i);
				return i;
			}
			list++;
		}
	} else {
		for (i = 0; i < ecount; i++) {
			if (memcmp(list, &zero, esize) == 0) {
				idx = i;
				break;
			}
			list++;
		}
	}

	if (idx != -1) {
		memcpy(plist->list + idx, element, esize);
		V206DEV005("[INFO] save to list(%p)[%d] successfully.\n", plist, idx);
		return idx;
	}

	if (engrow == 0) {
		V206KDEBUG002("%s: no free space.", __FUNCTION__);
		return -1;
	}

	idx = ecount;


	{
		LISTTYPE *ptemp;
		size = ecount * esize;
		ptemp = FUNC206LXDEV010(size * 2);
		if (ptemp == 0) {
			V206KDEBUG002("[ERROR] %s grow list-elements error.\n", __FUNCTION__);
			return -2;
		}
		memset(ptemp, 0, size * 2);
		memcpy(ptemp, plist->list, size);
		FUNC206LXDEV120(plist->list);
		plist->list = ptemp;
		memcpy(plist->list + ecount, element, esize);
		plist->ecount = ecount * 2;
	}
	V206DEV005("[INFO] save to list(%p)[%d] successfully, ecount is grow to be %d.\n",
			plist, ecount, ecount * 2);
	return idx;
}


static inline int mwv206_addrlist_find(mwv206_addrlist_t *plist, LISTTYPE *element)
{
	int i;
	LISTTYPE *list;

	if (plist == NULL) {
		return -1;
	}

	list = plist->list;

	for (i = 0; i < plist->ecount; i++) {
		if (memcmp(list, element, sizeof(LISTTYPE)) == 0) {
			V206DEV005("[INFO] %s: find done, list[%d] .\n", __FUNCTION__, i);
			return i;
		}
		list++;
	}
	return -1;
}


static inline int mwv206_addrlist_delete(mwv206_addrlist_t *plist, LISTTYPE *element)
{
	int i;
	int ecount, esize;
	LISTTYPE *list;

	if (plist == NULL) {
		return 0;
	}

	ecount = plist->ecount;
	esize  = sizeof(LISTTYPE);
	list = plist->list;

	for (i = 0; i < ecount; i++) {
		if (memcmp(list, element, esize) == 0) {
			memset(list, 0, esize);
			V206DEV005("[INFO] %s: delete from list(%p)[%d] successfully.\n", __FUNCTION__, plist, i);
			return 0;
		}
		list++;
	}

	V206DEV005("[ERROR] %s: delete from list(%p) failure.\n", __FUNCTION__, plist);
	return -1;
}


static inline int mwv206_addrlist_isempty(mwv206_addrlist_t *plist)
{
	int i;
	int ecount, esize;
	LISTTYPE *list;
	LISTTYPE zero;

	if (plist == NULL) {
		return 0;
	}

	ecount = plist->ecount;
	esize  = sizeof(LISTTYPE);
	list = plist->list;

	memset(&zero, 0, esize);
	for (i = 0; i < ecount; i++) {
		if (memcmp(list, &zero, esize) != 0) {
			return 0;
		}
		list++;
	}

	return 1;
}
#endif