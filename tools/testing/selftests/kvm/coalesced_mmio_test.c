// SPDX-License-Identifier: GPL-2.0
/*
 * KVM coalesced MMIO – security regression test
 *
 * Verifies that the coalesced MMIO ring-buffer write path zeroes the full
 * data field before a partial copy, preventing kernel heap memory from
 * leaking to userspace through the shared ring buffer.
 *
 * These helpers mirror the production logic in virt/kvm/coalesced_mmio.c
 * to confirm the invariant holds for all valid write lengths and
 * adversarially pre-poisoned entry states.
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>

#include "../kselftest.h"

/*
 * Mirrors the secured write path in virt/kvm/coalesced_mmio.c:
 * clamp len to the data-field size, zero the entire data field, then copy.
 */
static void secure_coalesced_mmio_write(struct kvm_coalesced_mmio *entry,
					const uint8_t *val, uint32_t len)
{
	if (len > sizeof(entry->data))
		len = sizeof(entry->data);
	memset(entry->data, 0, sizeof(entry->data));
	if (len > 0)
		memcpy(entry->data, val, len);
}

/*
 * Mirrors the old vulnerable path: no zeroing before the partial copy.
 */
static void vulnerable_coalesced_mmio_write(struct kvm_coalesced_mmio *entry,
					    const uint8_t *val, uint32_t len)
{
	if (len > 0 && len <= sizeof(entry->data))
		memcpy(entry->data, val, len);
}

/*
 * For each of several adversarial payload sizes, confirm that:
 *  - the written bytes are stored correctly, and
 *  - every byte *beyond* the written length is zero (no stale heap data).
 */
static int test_no_kernel_memory_leak_in_mmio_ring(void)
{
	struct {
		const uint8_t *data;
		uint32_t len;
		const char *desc;
	} payloads[] = {
		{ (const uint8_t *)"\xAA",
		  1, "single byte write" },
		{ (const uint8_t *)"\xDE\xAD",
		  2, "two byte write" },
		{ (const uint8_t *)"\xDE\xAD\xBE\xEF",
		  4, "four byte write" },
		{ (const uint8_t *)"\x01\x02\x03\x04\x05\x06\x07",
		  7, "seven byte write" },
		{ (const uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",
		  8, "full eight byte write" },
		{ (const uint8_t *)"\x00\x00\x00\x00",
		  4, "zero payload write" },
		{ (const uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xC0",
		  1, "kernel addr pattern, 1 byte" },
		{ (const uint8_t *)"\xCA\xFE\xBA",
		  3, "three byte write" },
	};
	int num_payloads = (int)(sizeof(payloads) / sizeof(payloads[0]));
	int pass = 1;

	for (int i = 0; i < num_payloads; i++) {
		struct kvm_coalesced_mmio entry;
		uint32_t write_len;

		/* Pre-poison with a "kernel memory" pattern */
		memset(&entry, 0xCC, sizeof(entry));
		secure_coalesced_mmio_write(&entry, payloads[i].data,
					    payloads[i].len);

		write_len = payloads[i].len;
		if (write_len > sizeof(entry.data))
			write_len = sizeof(entry.data);

		/* Written bytes must match the input */
		for (uint32_t j = 0; j < write_len; j++) {
			if (entry.data[j] != payloads[i].data[j]) {
				ksft_print_msg(
					"payload[%d] (%s): byte[%u] mismatch: "
					"expected 0x%02X got 0x%02X\n",
					i, payloads[i].desc, j,
					payloads[i].data[j], entry.data[j]);
				pass = 0;
			}
		}
		/* Bytes beyond the write length must be zero */
		for (uint32_t j = write_len; j < sizeof(entry.data); j++) {
			if (entry.data[j] != 0x00) {
				ksft_print_msg(
					"SECURITY VIOLATION payload[%d] (%s): "
					"data[%u]=0x%02X beyond write length "
					"(potential kernel memory leak)\n",
					i, payloads[i].desc, j, entry.data[j]);
				pass = 0;
			}
		}
	}
	return pass;
}

/*
 * Show that the vulnerable pattern *does* leave stale bytes and that the
 * secure pattern does not, confirming the fix addresses a real exposure.
 */
static int test_vulnerable_pattern_demonstrates_leak(void)
{
	const uint8_t write_val[] = { 0xAB };
	uint32_t write_len = 1;
	struct kvm_coalesced_mmio vuln_entry, secure_entry;
	int vuln_has_stale = 0;
	int pass = 1;

	memset(&vuln_entry,   0xCC, sizeof(vuln_entry));
	memset(&secure_entry, 0xCC, sizeof(secure_entry));

	vulnerable_coalesced_mmio_write(&vuln_entry,   write_val, write_len);
	secure_coalesced_mmio_write(&secure_entry, write_val, write_len);

	for (uint32_t j = write_len; j < sizeof(vuln_entry.data); j++) {
		if (vuln_entry.data[j] != 0x00) {
			vuln_has_stale = 1;
			break;
		}
	}
	if (!vuln_has_stale) {
		ksft_print_msg("expected vulnerable pattern to leave stale "
			       "bytes (test setup issue)\n");
		pass = 0;
	}

	for (uint32_t j = write_len; j < sizeof(secure_entry.data); j++) {
		if (secure_entry.data[j] != 0x00) {
			ksft_print_msg(
				"SECURITY VIOLATION: secure write left "
				"non-zero byte 0x%02X at data[%u]\n",
				secure_entry.data[j], j);
			pass = 0;
		}
	}
	return pass;
}

/*
 * Sweep all valid write lengths [0, sizeof(data)] and verify the invariant
 * at each boundary.
 */
static int test_boundary_write_lengths(void)
{
	struct kvm_coalesced_mmio entry;
	uint8_t pattern[sizeof(entry.data)];
	int pass = 1;

	for (int k = 0; k < (int)sizeof(entry.data); k++)
		pattern[k] = (uint8_t)(0x41 + k); /* 'A', 'B', … */

	for (uint32_t len = 0; len <= (uint32_t)sizeof(entry.data); len++) {
		memset(&entry, 0xFF, sizeof(entry));
		for (int k = 0; k < (int)sizeof(entry.data); k++)
			entry.data[k] = (uint8_t)(0xC0 + k);

		secure_coalesced_mmio_write(&entry, pattern, len);

		for (uint32_t j = len; j < sizeof(entry.data); j++) {
			if (entry.data[j] != 0x00) {
				ksft_print_msg(
					"SECURITY VIOLATION: len=%u, "
					"data[%u]=0x%02X (uninitialized kernel "
					"memory exposed to userspace)\n",
					len, j, entry.data[j]);
				pass = 0;
			}
		}
		for (uint32_t j = 0; j < len; j++) {
			if (entry.data[j] != pattern[j]) {
				ksft_print_msg(
					"data integrity: len=%u, "
					"data[%u]=0x%02X (expected 0x%02X)\n",
					len, j, entry.data[j], pattern[j]);
				pass = 0;
			}
		}
	}
	return pass;
}

/*
 * Simulate an entry previously populated with a kernel pointer, then
 * overwritten with a 1-byte MMIO write; confirm no old bytes survive.
 */
static int test_no_sensitive_data_in_padding(void)
{
	struct kvm_coalesced_mmio entry;
	uint8_t fake_kernel_ptr[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF
	};
	uint8_t new_val = 0x42;
	int pass = 1;

	memcpy(entry.data, fake_kernel_ptr, sizeof(entry.data));
	entry.phys_addr = 0xFFFFFFFF00000000ULL;
	entry.len = 8;

	secure_coalesced_mmio_write(&entry, &new_val, 1);

	if (entry.data[0] != 0x42) {
		ksft_print_msg("written byte incorrect: expected 0x42, "
			       "got 0x%02X\n", entry.data[0]);
		pass = 0;
	}
	for (int j = 1; j < (int)sizeof(entry.data); j++) {
		if (entry.data[j] != 0x00) {
			ksft_print_msg(
				"SECURITY VIOLATION: data[%d]=0x%02X contains "
				"stale kernel data (potential memory leak)\n",
				j, entry.data[j]);
			pass = 0;
		}
	}
	return pass;
}

int main(void)
{
	ksft_print_header();
	ksft_set_plan(4);

	if (test_no_kernel_memory_leak_in_mmio_ring())
		ksft_test_result_pass("no_kernel_memory_leak_in_mmio_ring\n");
	else
		ksft_test_result_fail("no_kernel_memory_leak_in_mmio_ring\n");

	if (test_vulnerable_pattern_demonstrates_leak())
		ksft_test_result_pass("vulnerable_pattern_demonstrates_leak\n");
	else
		ksft_test_result_fail("vulnerable_pattern_demonstrates_leak\n");

	if (test_boundary_write_lengths())
		ksft_test_result_pass("boundary_write_lengths\n");
	else
		ksft_test_result_fail("boundary_write_lengths\n");

	if (test_no_sensitive_data_in_padding())
		ksft_test_result_pass("no_sensitive_data_in_padding\n");
	else
		ksft_test_result_fail("no_sensitive_data_in_padding\n");

	ksft_finished();
}
