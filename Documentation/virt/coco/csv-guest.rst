.. SPDX-License-Identifier: GPL-2.0

===================================================================
CSV Guest API Documentation
===================================================================

1. General description
======================

The CSV guest driver exposes IOCTL interfaces via the /dev/csv-guest misc
device to allow userspace to get certain CSV guest-specific details.

2. API description
==================

In this section, for each supported IOCTL, the following information is
provided along with a generic description.

:Input parameters: Parameters passed to the IOCTL and related details.
:Output: Details about output data and return value (with details about
         the non common error values).

2.1 CSV_CMD_GET_REPORT
-----------------------

:Input parameters: struct csv_report_req
:Output: Upon successful execution, CSV_REPORT data is copied to
         csv_report_req.report_data and return 0. Return -EINVAL for invalid
         operands, -EIO on VMMCALL failure or standard error number on other
         common failures.

The CSV_CMD_GET_REPORT IOCTL can be used by the attestation software to get
the CSV_REPORT from the CSV module using VMMCALL[KVM_HC_VM_ATTESTATION].
