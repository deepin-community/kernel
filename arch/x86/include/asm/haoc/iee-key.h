/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_KEY_H
#define _LINUX_IEE_KEY_H

#include <linux/key.h>
#include <asm/haoc/haoc-def.h>

extern unsigned long long iee_rw_gate(int flag, ...);

static void __maybe_unused iee_set_key_union(struct key *key, struct key_union *key_union)
{
	iee_rw_gate(IEE_OP_SET_KEY_UNION, key, key_union);
}

static void __maybe_unused iee_set_key_struct(struct key *key, struct key_struct *key_struct)
{
	iee_rw_gate(IEE_OP_SET_KEY_STRUCT, key, key_struct);
}

static void __maybe_unused iee_set_key_payload(struct key *key, union key_payload *key_payload)
{
	iee_rw_gate(IEE_OP_SET_KEY_PAYLOAD, key, key_payload);
}

extern bool iee_set_key_usage(struct key *key, int n, int flag);

static void __maybe_unused iee_set_key_serial(struct key *key, key_serial_t serial)
{
	iee_rw_gate(IEE_OP_SET_KEY_SERIAL, key, serial);
}

#ifdef CONFIG_KEY_NOTIFICATIONS
static void __maybe_unused iee_set_key_watchers(struct key *key, struct watch_list *watchers)
{
	iee_rw_gate(IEE_OP_SET_KEY_WATCHERS, key, watchers);
}
#endif

static void __maybe_unused iee_set_key_user(struct key *key, struct key_user *user)
{
	iee_rw_gate(IEE_OP_SET_KEY_USERS, key, user);
}

static void __maybe_unused iee_set_key_security(struct key *key, void *security)
{
	iee_rw_gate(IEE_OP_SET_KEY_SECURITY, key, security);
}

static void __maybe_unused iee_set_key_expiry(struct key *key, time64_t expiry)
{
	iee_rw_gate(IEE_OP_SET_KEY_EXPIRY, key, expiry);
}

static void __maybe_unused iee_set_key_revoked_at(struct key *key, time64_t revoked_at)
{
	iee_rw_gate(IEE_OP_SET_KEY_REVOKED_AT, key, revoked_at);
}

static void __maybe_unused iee_set_key_last_used_at(struct key *key, time64_t last_used_at)
{
	iee_rw_gate(IEE_OP_SET_KEY_LAST_USED_AT, key, last_used_at);
}

static void __maybe_unused iee_set_key_uid(struct key *key, kuid_t uid)
{
	iee_rw_gate(IEE_OP_SET_KEY_UID, key, uid);
}

static void __maybe_unused iee_set_key_gid(struct key *key, kgid_t gid)
{
	iee_rw_gate(IEE_OP_SET_KEY_GID, key, gid);
}

static void __maybe_unused iee_set_key_perm(struct key *key, key_perm_t perm)
{
	iee_rw_gate(IEE_OP_SET_KEY_PERM, key, perm);
}

static void __maybe_unused iee_set_key_quotalen(struct key *key, unsigned short quotalen)
{
	iee_rw_gate(IEE_OP_SET_KEY_QUOTALEN, key, quotalen);
}

static void __maybe_unused iee_set_key_datalen(struct key *key, unsigned short datalen)
{
	iee_rw_gate(IEE_OP_SET_KEY_DATALEN, key, datalen);
}

static void __maybe_unused iee_set_key_state(struct key *key, short state)
{
	iee_rw_gate(IEE_OP_SET_KEY_STATE, key, state);
}

#ifdef KEY_DEBUGGING
static void __maybe_unused iee_set_key_magic(struct key *key, unsigned int magic)
{
	iee_rw_gate(IEE_OP_SET_KEY_MAGIC, key, magic);
}
#endif

static void __maybe_unused iee_set_key_flags(struct key *key, unsigned long flags)
{
	iee_rw_gate(IEE_OP_SET_KEY_FLAGS, key, flags);
}

static void __maybe_unused iee_set_key_index_key(struct key *key,
			struct keyring_index_key *index_key)
{
	iee_rw_gate(IEE_OP_SET_KEY_INDEX_KEY, key, index_key);
}

static void __maybe_unused iee_set_key_hash(struct key *key, unsigned long hash)
{
	iee_rw_gate(IEE_OP_SET_KEY_HASH, key, hash);
}

static void __maybe_unused iee_set_key_len_desc(struct key *key, unsigned long len_desc)
{
	iee_rw_gate(IEE_OP_SET_KEY_LEN_DESC, key, len_desc);
}

static void __maybe_unused iee_set_key_type(struct key *key, struct key_type *type)
{
	iee_rw_gate(IEE_OP_SET_KEY_TYPE, key, type);
}

static void __maybe_unused iee_set_key_domain_tag(struct key *key, struct key_tag *domain_tag)
{
	iee_rw_gate(IEE_OP_SET_KEY_TAG, key, domain_tag);
}

static void __maybe_unused iee_set_key_description(struct key *key, char *description)
{
	iee_rw_gate(IEE_OP_SET_KEY_DESCRIPTION, key, description);
}

static void __maybe_unused iee_set_key_restrict_link(struct key *key,
			struct key_restriction *restrict_link)
{
	iee_rw_gate(IEE_OP_SET_KEY_RESTRICT_LINK, key, restrict_link);
}

static bool __maybe_unused iee_set_key_flag_bit(struct key *key, long nr, int flag)
{
	bool ret;

	ret = iee_rw_gate(IEE_OP_SET_KEY_FLAG_BIT, key, nr, flag);
	return ret;
}

#endif
