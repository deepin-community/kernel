//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#ifndef __LIST_H
#define __LIST_H

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((long) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, sample, member) \
    (void *)((char *)(ptr) - ((char *)&(sample)->member - (char *)(sample)))


struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};

static inline void list_init_head(struct list_head *item)
{
    item->prev = item;
    item->next = item;
}

#define INIT_LIST_HEAD list_init_head

static inline void list_add(struct list_head *item, struct list_head *list)
{
    item->prev = list;
    item->next = list->next;
    list->next->prev = item;
    list->next = item;
}

static inline void list_add_tail(struct list_head *item, struct list_head *list)
{
    item->next = list;
    item->prev = list->prev;
    list->prev->next = item;
    list->prev = item;
}

static inline void list_replace(struct list_head *from, struct list_head *to)
{
    to->prev = from->prev;
    to->next = from->next;
    from->next->prev = to;
    from->prev->next = to;
}

static inline void list_del(struct list_head *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

static inline void list_del_init(struct list_head *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
    item->next = item;
    item->prev = item;
}

static inline int list_empty(struct list_head *item)
{
    return (item->next == item);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(struct list_head *list,
				struct list_head *head)
{
	return list->next == head;
}


static inline void __list_splice(struct list_head *list,
				 struct list_head *prev,
				 struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void list_splice_init(struct list_head *list,
				    struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head, head->next);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void list_splice_tail_init(struct list_head *list,
					 struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head->prev, head);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member) \
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)


#define list_for_each_entry(pos, head, member)                  \
   for (pos = container_of((head)->next, pos, member);          \
        &pos->member != (head);                                 \
        pos = container_of(pos->member.next, pos, member))

#define list_for_each_entry_safe(pos, storage, head, member)    \
   for (pos = container_of((head)->next, pos, member),          \
        storage = container_of(pos->member.next, pos, member);  \
        &pos->member != (head);                                 \
        pos = storage, storage = container_of(storage->member.next, storage, member))

#define list_for_each_entry_safe_rev(pos, storage, head, member)\
   for (pos = container_of((head)->prev, pos, member),          \
        storage = container_of(pos->member.prev, pos, member);  \
        &pos->member != (head);                                 \
        pos = storage, storage = container_of(storage->member.prev, storage, member))

#define list_for_each_entry_from(pos, start, head, member)      \
   for (pos = container_of((start), pos, member);               \
        &pos->member != (head);                                 \
        pos = container_of(pos->member.next, pos, member))

#define list_for_each_entry_from_rev(pos, start, head, member)  \
   for (pos = container_of((start), pos, member);               \
        &pos->member != (head);                                 \
        pos = container_of(pos->member.prev, pos, member))

#endif

