#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include "adc.h"
#include "stats.h"

void adc_convert(ADCSample *s)
{
    /* Convert the raw ADC value to a voltage. */
    s->voltage = ((float)s->raw_value / ADC_MAX_CODE) * ADC_VREF;
}

/* ---- helpers ---------------------------------------------------------- */

static void ch_reset(ChannelStats *cs)
{
    cs->count = 0;
    cs->mean_v = cs->min_v = cs->max_v = cs->rms_v = cs->std_v = 0.0f;

    cs->overvoltage = 0;
    cs->undervoltage = 0;
    cs->sensor_faults = 0;
    cs->out_of_range = 0;
}

/* Collect the voltages for one channel. */
static size_t gather_channel(const ADCSample *s, size_t n,
                             uint8_t ch, double **out)
{
    const ADCSample *end = s + n;

    size_t cnt = 0;

    for (const ADCSample *p = s; p != end; ++p)
        if (p->channel_id == ch)
            ++cnt;

    double *buf = malloc(cnt ? cnt * sizeof(double) : 1);

    if (!buf) {
        *out = NULL;
        return 0;
    }

    double *w = buf;

    for (const ADCSample *p = s; p != end; ++p)
        if (p->channel_id == ch)
            *w++ = (double)p->voltage;

    *out = buf;
    return cnt;
}

/* Check for missing or out-of-order records. */
static void check_integrity(const ADCSample *s, size_t n,
                            AnalysisResult *out)
{
    out->gap_count = 0;
    out->out_of_order = 0;

    if (n < 2)
        return;

    const ADCSample *end = s + n;
    const ADCSample *prev = s;

    for (const ADCSample *cur = s + 1; cur != end; ++cur, ++prev)
    {
        uint32_t p = prev->sequence_number;
        uint32_t c = cur->sequence_number;

        if (c < p)
        {
            out->out_of_order++;
        }
        else if (c - p > 1u)
        {
            if (out->gap_count < MAX_SEQ_GAPS)
            {
                SeqGap *g = &out->gaps[out->gap_count];

                g->before = p;
                g->after = c;
                g->missing = c - p - 1u;
            }

            out->gap_count++;
        }
    }
}

/* ---- main analysis ---------------------------------------------------- */

void adc_analyse(const ADCSample *samples, size_t n,
                 uint16_t version,
                 uint16_t channel_count,
                 uint32_t record_count,
                 uint32_t sample_rate_hz,
                 AnalysisResult *out)
{
    out->version = version;
    out->channel_count = channel_count;
    out->record_count = record_count;
    out->sample_rate_hz = sample_rate_hz;
    out->records_read = n;

    size_t active_channels = channel_count < ADC_CHANNEL_COUNT
        ? channel_count
        : ADC_CHANNEL_COUNT;

    for (uint8_t c = 0; c < ADC_CHANNEL_COUNT; ++c)
        ch_reset(&out->ch[c]);

    /* Count samples and faults. */
    const ADCSample *send = samples + n;

    for (const ADCSample *s = samples; s != send; ++s)
    {
        if (s->channel_id >= ADC_CHANNEL_COUNT)
            continue;

        ChannelStats *cs = &out->ch[s->channel_id];

        cs->count++;

        if (s->voltage > OVERVOLTAGE_TH)
            cs->overvoltage++;

        if (s->voltage < UNDERVOLTAGE_TH)
            cs->undervoltage++;

        if (s->status_flags & FLAG_SENSOR_FAULT)
            cs->sensor_faults++;

        if (s->status_flags & FLAG_OUT_OF_RANGE)
            cs->out_of_range++;
    }

    /* Calculate statistics for each channel. */
    for (uint8_t c = 0; c < ADC_CHANNEL_COUNT; ++c)
    {
        ChannelStats *cs = &out->ch[c];

        if (cs->count == 0)
            continue;

        double *v = NULL;
        size_t cnt = gather_channel(samples, n, c, &v);

        if (!v)
            continue;

        cs->mean_v = (float)stats_mean(v, cnt);
        cs->min_v = (float)stats_min(v, cnt);
        cs->max_v = (float)stats_max(v, cnt);
        cs->std_v = (float)stats_stddev(v, cnt);
        cs->rms_v = (float)stats_rms(v, cnt);

        free(v);
    }

    /* Check the sequence numbers. */
    check_integrity(samples, n, out);
}