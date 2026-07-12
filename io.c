#include "io.h"

#include <stdlib.h>
#include <stdio.h>

/* Make sure the packed structs are the expected size. */
static int check_layouts(void)
{
    return sizeof(FileHeader) == 24 && sizeof(AdcRecord) == 16;
}

size_t io_read_file(const char *path, ADCSample **out, FileHeader *hdr)
{
    *out = NULL;

    if (!check_layouts()) {
        fprintf(stderr, "internal error: struct layout is wrong "
                        "(header=%zu record=%zu); rebuild with packing\n",
                sizeof(FileHeader), sizeof(AdcRecord));
        return 0;
    }

    FILE *f = fopen(path, "rb");

    if (!f) {
        fprintf(stderr, "error: cannot open '%s' for reading\n", path);
        return 0;
    }

    /* Read the file header. */
    if (fread(hdr, sizeof(FileHeader), 1, f) != 1)
    {
        fprintf(stderr, "error: could not read a full %zu-byte header "
                        "from '%s'\n",
                sizeof(FileHeader), path);
        fclose(f);
        return 0;
    }

    if (hdr->magic != ADC_MAGIC)
    {
        fprintf(stderr, "error: bad magic number 0x%08X in '%s' "
                        "(expected 0x%08X)\n",
                hdr->magic, path, ADC_MAGIC);
        fclose(f);
        return 0;
    }

    /* Check the header values. */
    if (hdr->version != ADC_EXPECTED_VERSION) {
        fprintf(stderr, "error: unsupported file version %u in '%s' "
                        "(expected %u)\n",
                hdr->version, path, ADC_EXPECTED_VERSION);
        fclose(f);
        return 0;
    }

    if (hdr->channel_count != ADC_EXPECTED_CHANNELS)
    {
        fprintf(stderr, "error: header declares %u channels in '%s' "
                        "(expected %u); this build assumes 4\n",
                hdr->channel_count, path, ADC_EXPECTED_CHANNELS);
        fclose(f);
        return 0;
    }

    if (hdr->sample_rate_hz != ADC_EXPECTED_RATE_HZ) {
        fprintf(stderr,
                "warning: sample rate is %u Hz in '%s' (expected %u); "
                "continuing\n",
                hdr->sample_rate_hz, path, ADC_EXPECTED_RATE_HZ);
    }

    if (hdr->record_count == 0) {
        fprintf(stderr, "error: header claims zero records\n");
        fclose(f);
        return 0;
    }

    /* Compare the expected size with the actual file size. */
    {
        long pos_after_header = ftell(f);
        long file_size = -1;

        if (fseek(f, 0, SEEK_END) == 0)
            file_size = ftell(f);

        /* Go back to the first record. */
        if (fseek(f, pos_after_header, SEEK_SET) != 0)
            file_size = -1;

        if (file_size >= 0)
        {
            long expected = (long)sizeof(FileHeader)
                          + (long)hdr->record_count * (long)sizeof(AdcRecord);

            if (file_size < expected)
            {
                fprintf(stderr,
                        "warning: file is %ld bytes but header "
                        "implies %ld (%u records); reading what is present\n",
                        file_size, expected, hdr->record_count);
            }
            else if (file_size > expected)
            {
                fprintf(stderr,
                        "warning: file is %ld bytes, %ld more than expected; "
                        "trailing data ignored\n",
                        file_size, file_size - expected);
            }
        }
    }

    /* Allocate memory for all records. */
    ADCSample *samples =
        malloc((size_t)hdr->record_count * sizeof(ADCSample));

    if (!samples)
    {
        fprintf(stderr, "error: out of memory allocating %u samples\n",
                hdr->record_count);
        fclose(f);
        return 0;
    }

    size_t got = 0;

    for (uint32_t i = 0; i < hdr->record_count; ++i)
    {
        AdcRecord raw;

        size_t r = fread(&raw, sizeof(AdcRecord), 1, f);

        if (r != 1)
        {
            /* Stop if the file ends early. */
            fprintf(stderr,
                    "warning: only %zu of %u records could be read "
                    "(file truncated?)\n",
                    got, hdr->record_count);
            break;
        }

        ADCSample *s = &samples[got++];

        s->timestamp       = raw.timestamp;
        s->channel_id      = raw.channel_id;
        s->raw_value       = raw.raw_value;
        s->temperature     = raw.temperature;
        s->status_flags    = raw.status_flags;
        s->sequence_number = raw.sequence_number;
        s->voltage         = 0.0f;
    }

    fclose(f);

    if (got == 0) {
        free(samples);
        fprintf(stderr, "error: no records could be read from '%s'\n", path);
        return 0;
    }

    *out = samples;
    return got;
}


/* ---- writing ---------------------------------------------------------- */

int io_write_results(const char *path, const AnalysisResult *r)
{
    FILE *f = fopen(path, "w");

    if (!f) {
        fprintf(stderr, "error: cannot open '%s' for writing\n", path);
        return -1;
    }

    /* Report title. */
    fprintf(f, "==========================================================\n");
    fprintf(f, "  ADC SENSOR LOG ANALYSIS\n");
    fprintf(f, "==========================================================\n\n");

    /* File information. */
    fprintf(f, "FILE HEADER\n");
    fprintf(f, "  Version         : %u\n", r->version);
    fprintf(f, "  Channel count   : %u\n", r->channel_count);
    fprintf(f, "  Record count    : %u\n", r->record_count);
    fprintf(f, "  Sample rate     : %u Hz\n", r->sample_rate_hz);
    fprintf(f, "  Records read    : %zu\n", r->records_read);

    if (r->records_read != r->record_count)
        fprintf(f,
                "  NOTE            : file was short of the declared count\n");

    fprintf(f, "\n");

    /* Statistics for each channel. */
    fprintf(f, "----------------------------------------------------------\n");
    fprintf(f, "  PER-CHANNEL STATISTICS (volts)\n");
    fprintf(f, "----------------------------------------------------------\n");

    for (uint8_t c = 0; c < ADC_CHANNEL_COUNT; ++c)
    {
        const ChannelStats *cs = &r->ch[c];

        fprintf(f, "\n  Channel %u  (%zu samples)\n", c, cs->count);

        if (cs->count == 0) {
            fprintf(f, "    (no data)\n");
            continue;
        }

        fprintf(f, "    Mean voltage   : %.6f V\n", cs->mean_v);
        fprintf(f, "    RMS voltage    : %.6f V\n", cs->rms_v);
        fprintf(f, "    Min voltage    : %.6f V\n", cs->min_v);
        fprintf(f, "    Max voltage    : %.6f V\n", cs->max_v);
        fprintf(f, "    Std deviation  : %.6f V\n", cs->std_v);
    }

    fprintf(f, "\n");

    /* Fault counts for each channel. */
    fprintf(f, "----------------------------------------------------------\n");
    fprintf(f, "  FAULT SUMMARY (per channel)\n");
    fprintf(f, "----------------------------------------------------------\n");
    fprintf(f,
            "  ch | overvolt | undervolt | sensor-fault | out-of-range\n");

    for (uint8_t c = 0; c < ADC_CHANNEL_COUNT; ++c) {
        const ChannelStats *cs = &r->ch[c];

        fprintf(f,
                "  %u  | %8zu | %9zu | %12zu | %12zu\n",
                c,
                cs->overvoltage,
                cs->undervoltage,
                cs->sensor_faults,
                cs->out_of_range);
    }

    fprintf(f, "\n");

    /* Sequence number checks. */
    fprintf(f, "----------------------------------------------------------\n");
    fprintf(f, "  SAMPLING INTEGRITY\n");
    fprintf(f, "----------------------------------------------------------\n");

    fprintf(f, "  Sequence gaps detected : %zu\n", r->gap_count);
    fprintf(f, "  Out-of-order records   : %zu\n", r->out_of_order);

    if (r->gap_count > MAX_SEQ_GAPS)
    {
        fprintf(f,
                "  Only the first %zu gaps are shown.\n",
                (size_t)MAX_SEQ_GAPS);
    }

    /* List each recorded sequence gap. */
    for (size_t i = 0; i < r->gap_count && i < MAX_SEQ_GAPS; ++i)
    {
        const SeqGap *g = &r->gaps[i];

        fprintf(f,
                "    gap %zu: seq %u -> %u  (%u missing)\n",
                i + 1,
                g->before,
                g->after,
                g->missing);
    }

    fprintf(f,
            "\n==========================================================\n");
    fprintf(f, "  END OF REPORT\n");
    fprintf(f,
            "==========================================================\n");

    fclose(f);
    return 0;
}
