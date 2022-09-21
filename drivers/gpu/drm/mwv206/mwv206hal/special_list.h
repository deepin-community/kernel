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
#ifndef _SPECIAL_LIST_H_
#define _SPECIAL_LIST_H_


typedef struct TAG_SPECIAL_LIST_ENTRY {
	struct TAG_SPECIAL_LIST_ENTRY *Plink;
	struct TAG_SPECIAL_LIST_ENTRY *Nlink;
} SPECIAL_LIST_ENTRY, *P_SPECIAL_LIST_ENTRY, SPECIAL_LIST_HEAD, *P_SPECIAL_LIST_HEAD;



typedef struct TAG_SPECIAL_SINGLE_LIST_ENTRY {
	struct TAG_SPECIAL_SINGLE_LIST_ENTRY *Next;
} SPECIAL_SINGLE_LIST_ENTRY, *P_SPECIAL_SINGLE_LIST_ENTRY;


#define InitializeListHead(ListHead) (\
		(ListHead)->Plink = (ListHead)->Nlink = (ListHead))


#define IsListEmpty(ListHead) \
	((ListHead)->Plink == (ListHead))


#define RemoveHeadList(ListHead) \
	RemoveEntryList((ListHead)->Plink)


#define RemoveTailList(ListHead) \
	RemoveEntryList((ListHead)->Nlink)


#define RemoveEntryList(Entry) {\
	P_SPECIAL_LIST_ENTRY _EX_Nlink;\
	P_SPECIAL_LIST_ENTRY _EX_Plink;\
	_EX_Plink = (Entry)->Plink;\
	_EX_Nlink = (Entry)->Nlink;\
	_EX_Nlink->Plink = _EX_Plink;\
	_EX_Plink->Nlink = _EX_Nlink;\
}


#define InsertTailList(ListHead, Entry) {\
	P_SPECIAL_LIST_ENTRY _EX_Nlink;\
	P_SPECIAL_LIST_ENTRY _EX_ListHead;\
	_EX_ListHead = (ListHead);\
	_EX_Nlink = _EX_ListHead->Nlink;\
	(Entry)->Plink = _EX_ListHead;\
	(Entry)->Nlink = _EX_Nlink;\
	_EX_Nlink->Plink = (Entry);\
	_EX_ListHead->Nlink = (Entry);\
}


#define InsertAfterEntry(Entry, Entry1) {\
	(Entry1)->Plink = (Entry)->Plink;\
	(Entry)->Plink = (Entry1);  \
	(Entry1)->Plink->Nlink = (Entry1);\
	(Entry1)->Nlink = (Entry);  \
}


#define InsertBeforEntry(Entry, Entry1) {\
	(Entry1)->Plink = (Entry);\
	(Entry1)->Nlink = (Entry)->Nlink;\
	(Entry)->Nlink = (Entry1);\
	(Entry1)->Nlink->Plink = (Entry1);\
}


#define ReplaceListEntry(OldEntry, NewEntry) {\
	(OldEntry)->Nlink->Plink = (NewEntry);\
	(OldEntry)->Plink->Nlink = (NewEntry);\
	(NewEntry)->Nlink = (OldEntry)->Nlink;\
	(NewEntry)->Plink = (OldEntry)->Plink;\
}


#define InsertHeadList(ListHead, Entry) {\
	P_SPECIAL_LIST_ENTRY _EX_Plink;\
	P_SPECIAL_LIST_ENTRY _EX_ListHead;\
	_EX_ListHead = (ListHead);\
	_EX_Plink = _EX_ListHead->Plink;\
	(Entry)->Plink = _EX_Plink;\
	(Entry)->Nlink = _EX_ListHead;\
	_EX_Plink->Nlink = (Entry);\
	_EX_ListHead->Plink = (Entry);\
}


#define NextEntry(entry)    ((entry)->Plink)


#define PrvEntry(entry)     ((entry)->Nlink)


#define InitializeSingleListHead(ListHead) do { \
	(ListHead)->Next = NULL;\
} while (0)


#define PopEntryList(ListHead) \
{\
	P_SPECIAL_SINGLE_LIST_ENTRY FirstEntry;\
	P_SPECIAL_SINGLE_LIST_ENTRY SecondEntry;\
	FirstEntry = (ListHead)->Next;\
	SecondEntry = FirstEntry->Next;\
	if (SecondEntry != NULL) {     \
		(ListHead)->Next = SecondEntry;\
	}                             \
}

#define PushEntryList(ListHead, Entry) do { \
	(Entry)->Next = (ListHead)->Next; \
	(ListHead)->Next = (Entry);\
} while (0)


#define RemoveEntry(ListHead, Entry) \
{\
	P_SPECIAL_SINGLE_LIST_ENTRY PreEntry;\
	PreEntry = (ListHead)->Next;\
	while (NULL != PreEntry) {\
		if ((Entry) == PreEntry->Next) {\
			PreEntry->Next = (Entry)->Next;\
			break;\
		} \
		PreEntry = PreEntry->Next;\
	} \
}


#define STRUCT_OFFSET(struct_pointer, element) \
	(unsigned long)(&(((struct_pointer)0)->element))
#endif