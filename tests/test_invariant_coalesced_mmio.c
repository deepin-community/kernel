#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>

/*
 * Simulates the vulnerable pattern: only 'len' bytes are written,
 * but the full data field may contain uninitialized/stale memory.
 *
 * The SECURE version must zero-initialize the entire data field before
 * copying, ensuring no kernel memory leakage.
 */
static void secure_coalesced_mmio_write(struct kvm_coalesced_mmio *entry,
                                         const uint8_t *val, uint32_t len)
{
    /* Security invariant: zero the entire data field before partial write */
    memset(entry->data, 0, sizeof(entry->data));
    if (len > 0 && len <= sizeof(entry->data)) {
        memcpy(entry->data, val, len);
    }
}

/*
 * Simulates the VULNERABLE pattern for comparison:
 * only copies 'len' bytes, leaving remainder uninitialized.
 */
static void vulnerable_coalesced_mmio_write(struct kvm_coalesced_mmio *entry,
                                             const uint8_t *val, uint32_t len)
{
    /* BUG: does NOT zero the data field first */
    if (len > 0 && len <= sizeof(entry->data)) {
        memcpy(entry->data, val, len);
    }
}

START_TEST(test_no_kernel_memory_leak_in_mmio_ring)
{
    /*
     * Invariant: After writing 'len' bytes to the coalesced MMIO ring entry,
     * ALL bytes of the data field beyond 'len' MUST be zero (not contain
     * stale/uninitialized memory that could leak kernel data to userspace).
     */

    /* Adversarial payloads: various write lengths smaller than data field */
    struct {
        const uint8_t *data;
        uint32_t len;
        const char *desc;
    } payloads[] = {
        /* Write 1 byte - 7 bytes must be zeroed */
        { (const uint8_t *)"\xAA", 1, "single byte write" },
        /* Write 2 bytes - 6 bytes must be zeroed */
        { (const uint8_t *)"\xDE\xAD", 2, "two byte write" },
        /* Write 4 bytes (common MMIO width) - 4 bytes must be zeroed */
        { (const uint8_t *)"\xDE\xAD\xBE\xEF", 4, "four byte write" },
        /* Write 7 bytes - 1 byte must be zeroed */
        { (const uint8_t *)"\x01\x02\x03\x04\x05\x06\x07", 7, "seven byte write" },
        /* Write full 8 bytes - no leakage possible but must still be correct */
        { (const uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8, "full eight byte write" },
        /* Write with all-zero payload */
        { (const uint8_t *)"\x00\x00\x00\x00", 4, "zero payload write" },
        /* Write with kernel-pointer-like pattern */
        { (const uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xC0", 1, "kernel addr pattern, 1 byte" },
        /* Write with 3 bytes (odd size) */
        { (const uint8_t *)"\xCA\xFE\xBA", 3, "three byte write" },
    };

    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        struct kvm_coalesced_mmio entry;

        /* Pre-poison the entry with "kernel memory" pattern to simulate
         * stale heap data that could be leaked */
        memset(&entry, 0xCC, sizeof(entry));

        /* Apply the secure write */
        secure_coalesced_mmio_write(&entry, payloads[i].data, payloads[i].len);

        uint32_t write_len = payloads[i].len;
        if (write_len > sizeof(entry.data))
            write_len = sizeof(entry.data);

        /* Verify written bytes match the input */
        for (uint32_t j = 0; j < write_len; j++) {
            ck_assert_msg(entry.data[j] == payloads[i].data[j],
                "Payload[%d] (%s): byte[%u] written incorrectly: "
                "expected 0x%02X, got 0x%02X",
                i, payloads[i].desc, j,
                payloads[i].data[j], entry.data[j]);
        }

        /* SECURITY INVARIANT: bytes beyond 'len' MUST be zero,
         * not contain stale/uninitialized kernel memory */
        for (uint32_t j = write_len; j < sizeof(entry.data); j++) {
            ck_assert_msg(entry.data[j] == 0x00,
                "SECURITY VIOLATION - Payload[%d] (%s): "
                "data[%u] beyond write length contains non-zero byte 0x%02X "
                "(potential kernel memory leak to userspace)",
                i, payloads[i].desc, j, entry.data[j]);
        }
    }
}
END_TEST

START_TEST(test_vulnerable_pattern_demonstrates_leak)
{
    /*
     * This test demonstrates that the VULNERABLE pattern DOES leak data,
     * confirming our secure version is actually fixing something real.
     * We verify the vulnerable pattern leaves stale bytes, while the
     * secure pattern does not.
     */
    const uint8_t write_val[] = { 0xAB };
    uint32_t write_len = 1;

    struct kvm_coalesced_mmio vuln_entry;
    struct kvm_coalesced_mmio secure_entry;

    /* Poison both entries with "kernel memory" */
    memset(&vuln_entry, 0xCC, sizeof(vuln_entry));
    memset(&secure_entry, 0xCC, sizeof(secure_entry));

    /* Apply vulnerable write */
    vulnerable_coalesced_mmio_write(&vuln_entry, write_val, write_len);

    /* Apply secure write */
    secure_coalesced_mmio_write(&secure_entry, write_val, write_len);

    /* Vulnerable entry SHOULD have stale bytes (0xCC) beyond write_len */
    int vuln_has_stale = 0;
    for (uint32_t j = write_len; j < sizeof(vuln_entry.data); j++) {
        if (vuln_entry.data[j] != 0x00) {
            vuln_has_stale = 1;
            break;
        }
    }
    /* Confirm the vulnerability exists in the vulnerable pattern */
    ck_assert_msg(vuln_has_stale == 1,
        "Expected vulnerable pattern to leave stale bytes (test setup issue)");

    /* Secure entry MUST NOT have stale bytes beyond write_len */
    for (uint32_t j = write_len; j < sizeof(secure_entry.data); j++) {
        ck_assert_msg(secure_entry.data[j] == 0x00,
            "SECURITY VIOLATION: secure write left non-zero byte 0x%02X "
            "at data[%u] - potential kernel memory leak",
            secure_entry.data[j], j);
    }
}
END_TEST

START_TEST(test_boundary_write_lengths)
{
    /*
     * Invariant: For all valid write lengths (0 to sizeof(entry.data)),
     * the secure write must never leave uninitialized bytes in the data field.
     */
    struct kvm_coalesced_mmio entry;
    uint8_t pattern[sizeof(entry.data)];

    for (int k = 0; k < (int)sizeof(entry.data); k++) {
        pattern[k] = (uint8_t)(0x41 + k); /* 'A', 'B', 'C', ... */
    }

    for (uint32_t len = 0; len <= sizeof(entry.data); len++) {
        /* Poison with adversarial "kernel memory" patterns */
        memset(&entry, 0xFF, sizeof(entry));
        /* Also sprinkle in kernel-pointer-like values */
        for (int k = 0; k < (int)sizeof(entry.data); k++) {
            entry.data[k] = (uint8_t)(0xC0 + k);
        }

        secure_coalesced_mmio_write(&entry, pattern, len);

        /* All bytes beyond 'len' must be zero */
        for (uint32_t j = len; j < sizeof(entry.data); j++) {
            ck_assert_msg(entry.data[j] == 0x00,
                "SECURITY VIOLATION: len=%u, data[%u]=0x%02X (should be 0x00) "
                "- uninitialized kernel memory exposed to userspace",
                len, j, entry.data[j]);
        }

        /* Written bytes must match pattern */
        for (uint32_t j = 0; j < len; j++) {
            ck_assert_msg(entry.data[j] == pattern[j],
                "Data integrity failure: len=%u, data[%u]=0x%02X (expected 0x%02X)",
                len, j, entry.data[j], pattern[j]);
        }
    }
}
END_TEST

START_TEST(test_no_sensitive_data_in_padding)
{
    /*
     * Invariant: The data field must not inadvertently carry sensitive data.
     * The data field specifically must be clean after a secure write operation
     * regardless of prior entry state.
     */
    struct kvm_coalesced_mmio entry;

    /* Simulate entry previously used with sensitive kernel data */
    uint8_t fake_kernel_ptr[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF  /* kernel addr */
    };
    memcpy(entry.data, fake_kernel_ptr, sizeof(entry.data));
    entry.phys_addr = 0xFFFFFFFF00000000ULL;
    entry.len = 8;

    /* Now perform a small write (simulating 1-byte MMIO write) */
    uint8_t new_val = 0x42;
    secure_coalesced_mmio_write(&entry, &new_val, 1);

    /* The written byte must be correct */
    ck_assert_msg(entry.data[0] == 0x42,
        "Written byte incorrect: expected 0x42, got 0x%02X", entry.data[0]);

    /* All remaining bytes must be zeroed - no kernel pointer leakage */
    for (int j = 1; j < (int)sizeof(entry.data); j++) {
        ck_assert_msg(entry.data[j] == 0x00,
            "SECURITY VIOLATION: data[%d]=0x%02X contains stale kernel data "
            "(was 0x%02X before write) - kernel memory leak to userspace",
            j, entry.data[j], fake_kernel_ptr[j]);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_no_kernel_memory_leak_in_mmio_ring);
    tcase_add_test(tc_core, test_vulnerable_pattern_demonstrates_leak);
    tcase_add_test(tc_core, test_boundary_write_lengths);
    tcase_add_test(tc_core, test_no_sensitive_data_in_padding);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
