#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "adc.h"

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s <adc_sensor_log.bin>\n", argv0);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *infile = argv[1];

    /* ---- read the binary file ---- */
    FileHeader hdr;
    ADCSample *samples = NULL;
    size_t n = io_read_file(infile, &samples, &hdr);
    if (n == 0) {
        return 1;
    }

    /* ---- convert raw codes to volts ---- */
    for (size_t i = 0; i < n; ++i)
        adc_convert(&samples[i]);

    /* ---- run the analysis ---- */
    AnalysisResult result;
    adc_analyse(samples, n, hdr.version, hdr.channel_count,
                hdr.record_count, hdr.sample_rate_hz, &result);

    /* ---- write the report ---- */
    io_write_results("results.txt", &result);

    printf("Processed %zu records from '%s'.\n", n, infile);
    printf("  results.txt written\n");

    /* ---- tidy up ---- */
    free(samples);
    return 0;
}
