#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stddef.h>

/* ---- physical / format constants -------------------------------------- */

#define ADC_VREF          3.3f      /* reference voltage, volts            */
#define ADC_MAX_CODE      4095.0f   /* 12-bit codes run 0..4095            */
#define ADC_CHANNEL_COUNT 4

/* fault thresholds (volts) */
#define OVERVOLTAGE_TH    3.0f
#define UNDERVOLTAGE_TH   0.3f

/* status_flags bit masks, straight from the brief */
#define FLAG_SENSOR_FAULT 0x01u
#define FLAG_OUT_OF_RANGE 0x02u

/* cap on the number of sequence gaps we record inline; if the file is
 * truly mangled we still report the count, just not every gap. */
#define MAX_SEQ_GAPS 256

/*
 * The on-disk record is a byte-exact 16-byte block. We keep it separate
 * from ADCSample (the in-memory struct that also carries the derived
 * voltage) because fread must land on a layout that matches the file
 * precisely -- any compiler-inserted padding and every field after the
 * first misalignment decodes as garbage. Packed, sizeof == 16.
 */
typedef struct __attribute__((packed)) {
    float    timestamp;       /* seconds from start of recording        */
    uint8_t  channel_id;      /* 0..3                                   */
    uint16_t raw_value;       /* 12-bit ADC code, 0..4095               */
    int16_t  temperature;     /* tenths of a degree C (245 == 24.5)     */
    uint8_t  status_flags;    /* bit0 fault, bit1 out-of-range          */
    uint32_t sequence_number; /* monotonic; gaps mean dropped records   */
    uint8_t  reserved[2];     /* file padding, ignored                  */
} AdcRecord;                  /* 16 bytes, verified at startup          */

/*
 * Working representation. Same six meaningful fields plus the voltage we
 * compute from raw_value. Not read directly from disk, so it does not
 * need packing.
 */
typedef struct {
    float    timestamp;
    uint8_t  channel_id;
    uint16_t raw_value;
    float    voltage;         /* (raw_value / 4095.0) * Vref             */
    int16_t  temperature;     /* raw field, tenths of a degree           */
    uint8_t  status_flags;
    uint32_t sequence_number;
} ADCSample;

/* per-channel roll-up of everything the report needs */
typedef struct {
    size_t   count;
    float    mean_v;
    float    min_v;
    float    max_v;
    float    rms_v;
    float    std_v;

    /* fault tallies */
    size_t   overvoltage;     /* voltage > 3.0 V                         */
    size_t   undervoltage;    /* voltage < 0.3 V                         */
    size_t   sensor_faults;   /* status bit0                             */
    size_t   out_of_range;    /* status bit1                             */
} ChannelStats;

/* one entry per dropped stretch of sequence numbers */
typedef struct {
    uint32_t before;          /* last good seq before the gap           */
    uint32_t after;           /* first good seq after the gap           */
    uint32_t missing;         /* after - before - 1                     */
} SeqGap;

/* everything adc.c works out, handed to io.c for writing */
typedef struct {
    /* header echoes */
    uint16_t version;
    uint16_t channel_count;
    uint32_t record_count;
    uint32_t sample_rate_hz;
    size_t   records_read;    /* may be less than record_count if short */

    ChannelStats ch[ADC_CHANNEL_COUNT];

    /* integrity */
    SeqGap  gaps[MAX_SEQ_GAPS];
    size_t  gap_count;
    size_t  out_of_order;     /* seq went backwards                      */
} AnalysisResult;

/* ---- adc.c entry points ----------------------------------------------- */

/* fill s->voltage from the raw code */
void adc_convert(ADCSample *s);

/* run the full analysis over a sample buffer; fills *out. */
void adc_analyse(const ADCSample *samples, size_t n,
                 uint16_t version, uint16_t channel_count,
                 uint32_t record_count, uint32_t sample_rate_hz,
                 AnalysisResult *out);

#endif /* ADC_H */
