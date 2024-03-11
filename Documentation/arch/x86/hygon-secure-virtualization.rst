.. SPDX-License-Identifier: GPL-2.0

===========================
HYGON Secure Virtualization
===========================

China Secure Virtualization (CSV) is a key virtualization feature on Hygon
processors.

The 1st generation of CSV (CSV for short) is a secure virtualization technology
to provide memory encryption for the virtual machine (VM), each VM's memory is
encrypted by its unique encryption key which is managed by secure processor.

The 2nd generation of CSV (CSV2 for short) provides security enhancement to CSV
by encrypting not only the VM's memory but also the vCPU's registers of the VM.

The 3rd generation of CSV (CSV3 for short) is a more advanced secure
virtualization technology, it integrates secure processor, memory encryption and
memory isolation to provide the ability to protect guest's private data. The CSV3
guest's context like CPU registers, control block and nested page table is accessed
only by the guest itself and the secure processor. Neither other guests nor the
host can tamper with the guest's context.

The secure processor is a separate processor inside Hygon hardware. The firmware
running inside the secure processor performs activities in a secure way, such as
OVMF encryption, VM launch, secure memory management and nested page table
management etc. For more information, please see CSV spec and CSV3 spec from Hygon.

A CSV guest is running in the memory that is encrypted with a dedicated encrypt
key which is set by the secure processor. And CSV guest's memory encrypt key is
unique from the others. A low latency crypto engine resides on Hygon hardware
to minimize the negative effect on memory bandwidth. In CSV guest, a guest private
page will be automatically decrypted when read from memory and encrypted when
written to memory.

CSV3 provides an enhancement technology named memory isolation to improve the
security. A dedicated memory isolation hardware is built in Hygon hardware. Only
the secure processor has privilege to configure the isolation hardware. The VMM
allocates CMA memory and transfers them to secure processor. The secure processor
maps the memory to secure nested page table and manages them as guest's private
memory. Any memory access (read or write) to CSV3 guest's private memory outside
the guest will be blocked by isolation hardware.

A CSV3 guest may declare some memory regions as shared to share data with the
host. When a page is set as shared, read/write on the page will bypass the
isolation hardware and the guest's shared memory can be accessed by the host. A
method named CSV3 secure call command is designed and CSV3 guest sends the secure
call command to the secure processor to change private memory to shared memory.
In the method, 2 dedicated pages are reserved at early stage of the guest. Any
read/write on the dedicated pages will trigger nested page fault. When NPF
happens, the host helps to issue an external command to the secure processor but
cannot tamper with the data in the guest's private memory. Then the secure
processor checks the fault address and handles the command if the address is
exactly the dedicated pages.

Support for CSV can be determined through the CPUID instruction. The CPUID
function 0x8000001f reports information to CSV::

	0x8000001f[eax]:
		Bit[1]	  indicates support for CSV
		Bit[3]	  indicates support for CSV2
		Bit[30]	  indicates support for CSV3

If CSV is support, MSR 0xc0010131 can be used to determine if CSV is active::

	0xc0010131:
		Bit[0]	  0 = CSV is not active
			  1 = CSV is active
		Bit[1]	  0 = CSV2 is not active
			  1 = CSV2 is active
		Bit[30]	  0 = CSV3 is not active
			  1 = CSV3 is active

All CSV/CSV2's configurations must be enabled in CSV3. Linux can activate CSV3 by
default (CONFIG_HYGON_CSV=y, CONFIG_CMA=y). CSV3 guest's memory is managed by
CMA (Contiguous Memory Allocation). User must specify CSV3 total secure memory on
the linux kernel command line with csv_mem_size or csv_mem_percentage::

	csv_mem_size=nn[MG]
		[KNL,CSV]
		Reserve specified CSV3 memory size in CMA. CSV3's memory will be
		allocated from these CMAs.
		For instance, csv_mem_size=40G, 40G memory is reserved for CSV3.

	csv_mem_percentage=nn
		[KNL,CSV]
		Reserve specified memory size which is prorated according to the
		whole system memory size. CSV3 guest's memory will be allocated
		from these CMAs.
		For instance, csv_mem_percentage=60, means 60% system memory is
		reserved for CSV3.
		The maximum percentage is 80. And the default percentage is 0.

Limitations
The reserved CSV3 memory within CMA cannot be used by kernel or any application that
may pin memory using long term gup during the application's life time.
For instance, if the whole system memory is 64G and 32G is reserved for CSV3 with
kernel command line csv_mem_percentage=50, only 32G memory is available for CSV/CSV2.
As a result, user will fail to run a CSV/CSV2 guest with memory size which exceeds
32G.
