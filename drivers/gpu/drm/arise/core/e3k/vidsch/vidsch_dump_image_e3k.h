#ifndef VIDSCH_DUMP_IMAGE_E3K_H
#define VIDSCH_DUMP_IMAGE_E3K_H
typedef struct _HangDumpFileHeader
{
    unsigned int        nHeaderVersion;
    unsigned int        nDeviceID;
    unsigned int        nSliceMask;
    unsigned int        nHangDumpFileHeaderSizeInByte;
    unsigned int        reserved0[4];

    unsigned long long  nAdapterMemorySizeInByte;
    unsigned long long  nPCIEMemorySizeInByte;          // default 3G

    unsigned long long  nRecordNums;                    // the DMA/Context/Ring Buffer numbers which DUMP file include.

                                                        // DMA
    unsigned long long  nDmaOffset;
    unsigned long long  nSingleDmaSizeInByte;           // nRecordNums*nSingleDmaSizeInByte = 0x40000

                                                        // Context
    unsigned long long  nContextOffset;
    unsigned long long  nSingleContextSizeInByte;       // nRecordNums*nSingleContextSizeInByte = 0x160000

                                                        // Ring Buffer
    unsigned long long  nRingBufferOffset;
    unsigned long long  nSingleRingBufferSizeInByte;    // nRecordNums*nSingleRingBufferSizeInByte

    unsigned long long  nTransferBufferOffsetInFBFile;  // useless, remove it
    unsigned long long  nGartTableL3Offset;
    unsigned long long  nGartTableL2Offset;
    unsigned long long  dummyPageEntry;

    unsigned long long  nBlBufferOffset;
    unsigned long long  reserved1[0x20];
} HangDumpFileHeader;
#endif
