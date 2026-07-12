#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stddef.h>

/* ADC constants. */
#define ADC_VREF          3.3f
#define ADC_MAX_CODE      4095.0f
#define ADC_CHANNEL_COUNT 4

/* Voltage limits. */
#define OVERVOLTAGE_TH    3.0f
#define UNDERVOLTAGE_TH   0.3f

/* Status flag bits. */
#define FLAG_SENSOR_FAULT 0x01u
#define FLAG_OUT_OF_RANGE 0x02u

/* Maximum number of sequence gaps to store. */
#define MAX_SEQ_GAPS 256

/* Record layout stored in the binary file. */
typedef struct __attribute__((packed))
{
    float    timestamp;        /* Time in seconds. */
    uint8_t  channel_id;       /* ADC channel. */
    uint16_t raw_value;        /* Raw ADC value. */
    int16_t  temperature;      /* Tenths of a degree C. */
    uint8_t  status_flags;     /* Status bits. */
    uint32_t sequence_number;  /* Record sequence number. */
    uint8_t  reserved[2];      /* Unused. */
} AdcRecord;

/* Sample stored in memory. */
typedef struct
{
    float    timestamp;
    uint8_t  channel_id;
    uint16_t raw_value;
    float    voltage;          /* Converted voltage. */
    int16_t  temperature;
    uint8_t  status_flags;
    uint32_t sequence_number;
} ADCSample;

/* Statistics for one channel. */
typedef struct
{
    size_t count;

    float mean_v;
    float min_v;
    float max_v;
    float rms_v;
    float std_v;

    size_t overvoltage;
    size_t undervoltage;
    size_t sensor_faults;
    size_t out_of_range;
} ChannelStats;

/* Information about a sequence gap. */
typedef struct
{
    uint32_t before;
    uint32_t after;
    uint32_t missing;
} SeqGap;

/* Results produced by the analysis. */
typedef struct
{
    uint16_t version;
    uint16_t channel_count;
    uint32_t record_count;
    uint32_t sample_rate_hz;
    size_t   records_read;

    ChannelStats ch[ADC_CHANNEL_COUNT];

    SeqGap gaps[MAX_SEQ_GAPS];
    size_t gap_count;
    size_t out_of_order;
} AnalysisResult;

/* Convert the raw ADC value into a voltage. */
void adc_convert(ADCSample *s);

/* Analyse all samples and fill the results structure. */
void adc_analyse(const ADCSample *samples, size_t n,
                 uint16_t version,
                 uint16_t channel_count,
                 uint32_t record_count,
                 uint32_t sample_rate_hz,
                 AnalysisResult *out);

#endif /* ADC_H */