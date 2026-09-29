// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/iee.h>
#include <linux/key.h>
#include <asm/haoc/haoc-bitmap.h>

struct watch_list;

static inline void iee_verify_key_type(const struct key *key)
{
	iee_verify_type(__pa(key), IEE_KEY, "key");
}

unsigned long _iee_set_key_flag_bit(unsigned long __unused, struct key *key,
				      long nr, int flag)
{
	iee_verify_key_type(key);
	switch (flag) {
	case SET_BIT_OP: {
		set_bit(nr, &key->flags);
		break;
	}
	case TEST_AND_CLEAR_BIT: {
		return test_and_clear_bit(nr, &key->flags);
	}
	case TEST_AND_SET_BIT: {
		return test_and_set_bit(nr, &key->flags);
	}
	}
	return 0;
}

void _iee_set_key_restrict_link(unsigned long __unused,
					   struct key *key,
					   struct key_restriction *restrict_link)
{
	iee_verify_key_type(key);
	key->restrict_link = restrict_link;
}

void _iee_set_key_magic(unsigned long __unused, struct key *key,
				   unsigned int magic)
{
#ifdef KEY_DEBUGGING
	iee_verify_key_type(key);
	key->magic = magic;
#endif
}

void _iee_set_key_flags(unsigned long __unused, struct key *key,
				   unsigned long flags)
{
	iee_verify_key_type(key);
	key->flags = flags;
}

void _iee_set_key_index_key(unsigned long __unused,
					   struct key *key,
					   struct keyring_index_key *index_key)
{
	iee_verify_key_type(key);
	key->index_key = *index_key;
}

void _iee_set_key_hash(unsigned long __unused, struct key *key,
				  unsigned long hash)
{
	iee_verify_key_type(key);
	key->hash = hash;
}

void _iee_set_key_len_desc(unsigned long __unused, struct key *key,
				      unsigned long len_desc)
{
	iee_verify_key_type(key);
	key->len_desc = len_desc;
}

void _iee_set_key_type(unsigned long __unused, struct key *key,
				  struct key_type *type)
{
	iee_verify_key_type(key);
	key->type = type;
}

void _iee_set_key_domain_tag(unsigned long __unused,
					struct key *key,
					struct key_tag *domain_tag)
{
	iee_verify_key_type(key);
	key->domain_tag = domain_tag;
}

void _iee_set_key_description(unsigned long __unused,
					 struct key *key, char *description)
{
	iee_verify_key_type(key);
	key->description = description;
}

void _iee_set_key_uid(unsigned long __unused, struct key *key,
				 kuid_t uid)
{
	iee_verify_key_type(key);
	key->uid = uid;
}

void _iee_set_key_gid(unsigned long __unused, struct key *key,
				 kgid_t gid)
{
	iee_verify_key_type(key);
	key->gid = gid;
}

void _iee_set_key_perm(unsigned long __unused, struct key *key,
				  key_perm_t perm)
{
	iee_verify_key_type(key);
	key->perm = perm;
}

void _iee_set_key_quotalen(unsigned long __unused, struct key *key,
				      unsigned short quotalen)
{
	iee_verify_key_type(key);
	key->quotalen = quotalen;
}

void _iee_set_key_datalen(unsigned long __unused, struct key *key,
				     unsigned short datalen)
{
	iee_verify_key_type(key);
	key->datalen = datalen;
}

void _iee_set_key_state(unsigned long __unused, struct key *key,
					   short state)
{
	iee_verify_key_type(key);
	smp_store_release(&key->state, state);
}

void _iee_set_key_user(unsigned long __unused, struct key *key,
				  struct key_user *user)
{
	iee_verify_key_type(key);
	key->user = user;
}

void _iee_set_key_security(unsigned long __unused, struct key *key,
				      void *security)
{
	iee_verify_key_type(key);
	key->security = security;
}

void _iee_set_key_expiry(unsigned long __unused, struct key *key,
				    time64_t expiry)
{
	iee_verify_key_type(key);
	key->expiry = expiry;
}

void _iee_set_key_revoked_at(unsigned long __unused,
					struct key *key, time64_t revoked_at)
{
	iee_verify_key_type(key);
	key->revoked_at = revoked_at;
}

void _iee_set_key_last_used_at(unsigned long __unused,
					  struct key *key,
					  time64_t last_used_at)
{
	iee_verify_key_type(key);
	key->last_used_at = last_used_at;
}

unsigned long _iee_set_key_usage(unsigned long __unused, struct key *key,
				   int n, int flag)
{
	iee_verify_key_type(key);
	switch (flag) {
	case REFCOUNT_INC: {
		refcount_inc(&key->usage);
		break;
	}
	case REFCOUNT_SET: {
		refcount_set(&key->usage, n);
		break;
	}
	case REFCOUNT_DEC_AND_TEST: {
		return refcount_dec_and_test(&key->usage);
	}
	case REFCOUNT_INC_NOT_ZERO: {
		return refcount_inc_not_zero(&key->usage);
	}
	}
	return 0;
}

void _iee_set_key_serial(unsigned long __unused, struct key *key,
				    key_serial_t serial)
{
	iee_verify_key_type(key);
	key->serial = serial;
}

void _iee_set_key_watchers(unsigned long __unused, struct key *key, struct watch_list *watchers)
{
#ifdef CONFIG_KEY_NOTIFICATIONS
	iee_verify_key_type(key);
	key->watchers = watchers;
#endif
}

void _iee_set_key_union(unsigned long __unused, struct key *key,
				   struct key_union *key_union)
{
	iee_verify_key_type(key);
	key->graveyard_link.next = (struct list_head *)key_union;
}

void _iee_set_key_struct(unsigned long __unused, struct key *key,
				    struct key_struct *key_struct)
{
	iee_verify_key_type(key);
	key->name_link.prev = (struct list_head *)key_struct;
}

void _iee_set_key_payload(unsigned long __unused, struct key *key,
				     union key_payload *key_payload)
{
	iee_verify_key_type(key);
	key->name_link.next = (struct list_head *)key_payload;
}
