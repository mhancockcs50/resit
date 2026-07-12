#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "adc.h"

/* File header stored at the start of the binary file. */
typedef struct __attribute__((packed))
{
    uint32_t magic;           /* File identifier. */
    uint16_t version;        /* File format version. */
    uint16_t channel_count; /* Number of ADC channels. */
    uint32_t record_count;    /* Number of records in the file. */
    uint32_t sample_rate_hz; /* Sampling rate in Hz. */
    uint8_t reserved[8];     /* Reserved for future use. */
} FileHeader;

#define ADC_MAGIC 0xADC1BEEFu

/* Expected values in the file header. */
#define ADC_EXPECTED_VERSION  1u
#define ADC_EXPECTED_CHANNELS 4u
#define ADC_EXPECTED_RATE_HZ  1000u

/* Read the binary file into an array of ADCSample. */
size_t io_read_file(const char *path, ADCSample **out, FileHeader *hdr);

/* Write the analysis results to a report file. */
int io_write_results(const char *path, const AnalysisResult *r);

#endif /* IO_H */