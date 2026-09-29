/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_KEY_H
#define _LINUX_IEE_KEY_H

#include <linux/key.h>
#include <asm/haoc/haoc-def.h>

extern unsigned long long iee_rw_gate(int flag, ...);

static void __maybe_unused iee_set_key_union(struct key *key, struct key_union *key_union)
{
	if (!haoc_enabled) {
		key->graveyard_link.next = (struct list_head *)key_union;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_UNION, key, key_union);
}

static void __maybe_unused iee_set_key_struct(struct key *key, struct key_struct *key_struct)
{
	if (!haoc_enabled) {
		key->name_link.prev = (struct list_head *)key_struct;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_STRUCT, key, key_struct);
}

static void __maybe_unused iee_set_key_payload(struct key *key, union key_payload *key_payload)
{
	if (!haoc_enabled) {
		key->name_link.next = (struct list_head *)key_payload;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_PAYLOAD, key, key_payload);
}

extern bool iee_set_key_usage(struct key *key, int n, int flag);

static void __maybe_unused iee_set_key_serial(struct key *key, key_serial_t serial)
{
	if (!haoc_enabled) {
		key->serial = serial;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_SERIAL, key, serial);
}

#ifdef CONFIG_KEY_NOTIFICATIONS
static void __maybe_unused iee_set_key_watchers(struct key *key, struct watch_list *watchers)
{
	if (!haoc_enabled) {
		key->watchers = watchers;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_WATCHERS, key, watchers);
}
#endif

static void __maybe_unused iee_set_key_user(struct key *key, struct key_user *user)
{
	if (!haoc_enabled) {
		key->user = user;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_USERS, key, user);
}

static void __maybe_unused iee_set_key_security(struct key *key, void *security)
{
	if (!haoc_enabled) {
		key->security = security;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_SECURITY, key, security);
}

static void __maybe_unused iee_set_key_expiry(struct key *key, time64_t expiry)
{
	if (!haoc_enabled) {
		key->expiry = expiry;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_EXPIRY, key, expiry);
}

static void __maybe_unused iee_set_key_revoked_at(struct key *key, time64_t revoked_at)
{
	if (!haoc_enabled) {
		key->revoked_at = revoked_at;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_REVOKED_AT, key, revoked_at);
}

static void __maybe_unused iee_set_key_last_used_at(struct key *key, time64_t last_used_at)
{
	if (!haoc_enabled) {
		key->last_used_at = last_used_at;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_LAST_USED_AT, key, last_used_at);
}

static void __maybe_unused iee_set_key_uid(struct key *key, kuid_t uid)
{
	if (!haoc_enabled) {
		key->uid = uid;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_UID, key, uid);
}

static void __maybe_unused iee_set_key_gid(struct key *key, kgid_t gid)
{
	if (!haoc_enabled) {
		key->gid = gid;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_GID, key, gid);
}

static void __maybe_unused iee_set_key_perm(struct key *key, key_perm_t perm)
{
	if (!haoc_enabled) {
		key->perm = perm;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_PERM, key, perm);
}

static void __maybe_unused iee_set_key_quotalen(struct key *key, unsigned short quotalen)
{
	if (!haoc_enabled) {
		key->quotalen = quotalen;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_QUOTALEN, key, quotalen);
}

static void __maybe_unused iee_set_key_datalen(struct key *key, unsigned short datalen)
{
	if (!haoc_enabled) {
		key->datalen = datalen;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_DATALEN, key, datalen);
}

static void __maybe_unused iee_set_key_state(struct key *key, short state)
{
	if (!haoc_enabled) {
		smp_store_release(&key->state, state);
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_STATE, key, state);
}

#ifdef KEY_DEBUGGING
static void __maybe_unused iee_set_key_magic(struct key *key, unsigned int magic)
{
	if (!haoc_enabled) {
		key->magic = magic;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_MAGIC, key, magic);
}
#endif

static void __maybe_unused iee_set_key_flags(struct key *key, unsigned long flags)
{
	if (!haoc_enabled) {
		key->flags = flags;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_FLAGS, key, flags);
}

static void __maybe_unused iee_set_key_index_key(struct key *key,
			struct keyring_index_key *index_key)
{
	if (!haoc_enabled) {
		key->index_key = *index_key;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_INDEX_KEY, key, index_key);
}

static void __maybe_unused iee_set_key_hash(struct key *key, unsigned long hash)
{
	if (!haoc_enabled) {
		key->hash = hash;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_HASH, key, hash);
}

static void __maybe_unused iee_set_key_len_desc(struct key *key, unsigned long len_desc)
{
	if (!haoc_enabled) {
		key->len_desc = len_desc;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_LEN_DESC, key, len_desc);
}

static void __maybe_unused iee_set_key_type(struct key *key, struct key_type *type)
{
	if (!haoc_enabled) {
		key->type = type;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_TYPE, key, type);
}

static void __maybe_unused iee_set_key_domain_tag(struct key *key, struct key_tag *domain_tag)
{
	if (!haoc_enabled) {
		key->domain_tag = domain_tag;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_TAG, key, domain_tag);
}

static void __maybe_unused iee_set_key_description(struct key *key, char *description)
{
	if (!haoc_enabled) {
		key->description = description;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_DESCRIPTION, key, description);
}

static void __maybe_unused iee_set_key_restrict_link(struct key *key,
			struct key_restriction *restrict_link)
{
	if (!haoc_enabled) {
		key->restrict_link = restrict_link;
		return;
	}

	iee_rw_gate(IEE_OP_SET_KEY_RESTRICT_LINK, key, restrict_link);
}

static bool __maybe_unused iee_set_key_flag_bit(struct key *key, long nr, int flag)
{
	bool ret;

	if (!haoc_enabled) {
		switch (flag) {
		case SET_BIT_OP:
			set_bit(nr, &key->flags);
			break;
		case TEST_AND_CLEAR_BIT:
			return test_and_clear_bit(nr, &key->flags);
		case TEST_AND_SET_BIT:
			return test_and_set_bit(nr, &key->flags);
		}
		return false;
	}

	ret = iee_rw_gate(IEE_OP_SET_KEY_FLAG_BIT, key, nr, flag);
	return ret;
}

#endif
