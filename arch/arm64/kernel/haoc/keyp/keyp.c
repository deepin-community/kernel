// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/iee.h>
#include <linux/key.h>
#include <asm/haoc/haoc-bitmap.h>

struct watch_list;

static inline void iee_verify_key_type(const struct key *key)
{
	iee_verify_type((unsigned long)key, IEE_KEY, "key");
}

static inline struct key *key_write_ptr(struct key *key)
{
	iee_verify_key_type(key);
	return haoc_enabled ? __ptr_to_iee(key) : key;
}

unsigned long __iee_code _iee_set_key_flag_bit(unsigned long __unused, struct key *key,
				      long nr, int flag)
{
	key = key_write_ptr(key);
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

void __iee_code _iee_set_key_restrict_link(unsigned long __unused,
					   struct key *key,
					   struct key_restriction *restrict_link)
{
	key = key_write_ptr(key);
	key->restrict_link = restrict_link;
}

void __iee_code _iee_set_key_magic(unsigned long __unused, struct key *key,
				   unsigned int magic)
{
	key = key_write_ptr(key);
#ifdef KEY_DEBUGGING
	key->magic = magic;
#endif
}

void __iee_code _iee_set_key_flags(unsigned long __unused, struct key *key,
				   unsigned long flags)
{
	key = key_write_ptr(key);
	key->flags = flags;
}

void __iee_code _iee_set_key_index_key(unsigned long __unused,
					   struct key *key,
					   struct keyring_index_key *index_key)
{
	key = key_write_ptr(key);
	key->index_key = *index_key;
}

void __iee_code _iee_set_key_hash(unsigned long __unused, struct key *key,
				  unsigned long hash)
{
	key = key_write_ptr(key);
	key->hash = hash;
}

void __iee_code _iee_set_key_len_desc(unsigned long __unused, struct key *key,
				      unsigned long len_desc)
{
	key = key_write_ptr(key);
	key->len_desc = len_desc;
}

void __iee_code _iee_set_key_type(unsigned long __unused, struct key *key,
				  struct key_type *type)
{
	key = key_write_ptr(key);
	key->type = type;
}

void __iee_code _iee_set_key_domain_tag(unsigned long __unused,
					struct key *key,
					struct key_tag *domain_tag)
{
	key = key_write_ptr(key);
	key->domain_tag = domain_tag;
}

void __iee_code _iee_set_key_description(unsigned long __unused,
					 struct key *key, char *description)
{
	key = key_write_ptr(key);
	key->description = description;
}

void __iee_code _iee_set_key_uid(unsigned long __unused, struct key *key,
				 kuid_t uid)
{
	key = key_write_ptr(key);
	key->uid = uid;
}

void __iee_code _iee_set_key_gid(unsigned long __unused, struct key *key,
				 kgid_t gid)
{
	key = key_write_ptr(key);
	key->gid = gid;
}

void __iee_code _iee_set_key_perm(unsigned long __unused, struct key *key,
				  key_perm_t perm)
{
	key = key_write_ptr(key);
	key->perm = perm;
}

void __iee_code _iee_set_key_quotalen(unsigned long __unused, struct key *key,
				      unsigned short quotalen)
{
	key = key_write_ptr(key);
	key->quotalen = quotalen;
}

void __iee_code _iee_set_key_datalen(unsigned long __unused, struct key *key,
					     unsigned short datalen)
{
	key = key_write_ptr(key);
	key->datalen = datalen;
}

void __iee_code _iee_set_key_state(unsigned long __unused, struct key *key,
				   short state)
{
	key = key_write_ptr(key);
	smp_store_release(&key->state, state);
}

void __iee_code _iee_set_key_user(unsigned long __unused, struct key *key,
				  struct key_user *user)
{
	key = key_write_ptr(key);
	key->user = user;
}

void __iee_code _iee_set_key_security(unsigned long __unused, struct key *key,
				      void *security)
{
	key = key_write_ptr(key);
	key->security = security;
}

void __iee_code _iee_set_key_expiry(unsigned long __unused, struct key *key,
				    time64_t expiry)
{
	key = key_write_ptr(key);
	key->expiry = expiry;
}

void __iee_code _iee_set_key_revoked_at(unsigned long __unused,
					struct key *key, time64_t revoked_at)
{
	key = key_write_ptr(key);
	key->revoked_at = revoked_at;
}

void __iee_code _iee_set_key_last_used_at(unsigned long __unused,
					  struct key *key,
					  time64_t last_used_at)
{
	key = key_write_ptr(key);
	key->last_used_at = last_used_at;
}

unsigned long __iee_code _iee_set_key_usage(unsigned long __unused, struct key *key,
				   int n, int flag)
{
	key = key_write_ptr(key);
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

void __iee_code _iee_set_key_serial(unsigned long __unused, struct key *key,
				    key_serial_t serial)
{
	key = key_write_ptr(key);
	key->serial = serial;
}

void __iee_code _iee_set_key_watchers(unsigned long __unused, struct key *key,
					struct watch_list *watchers)
{
	key = key_write_ptr(key);
#ifdef CONFIG_KEY_NOTIFICATIONS
	key->watchers = watchers;
#endif
}

void __iee_code _iee_set_key_union(unsigned long __unused, struct key *key,
				   struct key_union *key_union)
{
	key = key_write_ptr(key);
	key->graveyard_link.next = (struct list_head *)key_union;
}

void __iee_code _iee_set_key_struct(unsigned long __unused, struct key *key,
				    struct key_struct *key_struct)
{
	key = key_write_ptr(key);
	key->name_link.prev = (struct list_head *)key_struct;
}

void __iee_code _iee_set_key_payload(unsigned long __unused, struct key *key,
					     union key_payload *key_payload)
{
	key = key_write_ptr(key);
	key->name_link.next = (struct list_head *)key_payload;
}
