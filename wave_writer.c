/*
 * Base documents:
 * https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 * http://en.wikipedia.org/wiki/WAV
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wave_writer.h"

#define FOUR_CC(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | ((d)<<0))
#define FORMAT 1
#define FMT_LEN 16
#define DEFAULT_RIFF_LEN (4 + 8 + FMT_LEN + 8)

struct wave_writer {
    int format;
    int num_channels;
    int sample_rate;
    int sample_bits;
    int num_samples;
    FILE *fp;
};

static int
get_data_len(const wave_writer *ww) {
    int l = ww->num_samples * ww->num_channels * ww->sample_bits / 8;
    assert(0 <= l);
    if (l % 2 != 0) {
        l++;
    }
    return l;
}

static int
write_byte(FILE *fp, unsigned char in)
{
    if (1 > fwrite(&in, 1, 1, fp)) {
        return 0;
    }
    return 1;
}

static int
write_int32_b(FILE *fp, int in)
{
    unsigned char a, b, c, d;

    a = (in>>24) & 0xFF;
    b = (in>>16) & 0xFF;
    c = (in>> 8) & 0xFF;
    d = (in>> 0) & 0xFF;

    if (!write_byte(fp, a)) return 0;
    if (!write_byte(fp, b)) return 0;
    if (!write_byte(fp, c)) return 0;
    if (!write_byte(fp, d)) return 0;

    return 1;
}

static int
write_int32_l(FILE *fp, int in)
{
    unsigned char a, b, c, d;

    a = (in>> 0) & 0xFF;
    b = (in>> 8) & 0xFF;
    c = (in>>16) & 0xFF;
    d = (in>>24) & 0xFF;

    if (!write_byte(fp, a)) return 0;
    if (!write_byte(fp, b)) return 0;
    if (!write_byte(fp, c)) return 0;
    if (!write_byte(fp, d)) return 0;

    return 1;
}

static int
write_int16_l(FILE *fp, int in)
{
    unsigned char a, b;

    a = (in>> 0) & 0xFF;
    b = (in>> 8) & 0xFF;

    if (!write_byte(fp, a)) return 0;
    if (!write_byte(fp, b)) return 0;

    return 1;
}

static int
write_riff_chunk(struct wave_writer *ww)
{
    if (!write_int32_b(ww->fp, FOUR_CC('R','I','F','F'))) {
        return 0;
    }

    if (!write_int32_l(ww->fp, DEFAULT_RIFF_LEN + get_data_len(ww))) {
        return 0;
    }

    if (!write_int32_b(ww->fp, FOUR_CC('W','A','V','E'))) {
        return 0;
    }

    if (!write_int32_b(ww->fp, FOUR_CC('f','m','t',' '))) {
        return 0;
    }

    if (!write_int32_l(ww->fp, FMT_LEN)) {
        return 0;
    }

    if (!write_int16_l(ww->fp, ww->format)) {
        return 0;
    }

    if (!write_int16_l(ww->fp, ww->num_channels)) {
        return 0;
    }

    if (!write_int32_l(ww->fp, ww->sample_rate)) {
        return 0;
    }

    if (!write_int32_l(ww->fp, ww->sample_rate * ww->num_channels * ww->sample_bits / 8)) {
        return 0;
    }

    if (!write_int16_l(ww->fp, ww->num_channels * ww->sample_bits / 8)) {
        return 0;
    }

    if (!write_int16_l(ww->fp, ww->sample_bits)) {
        return 0;
    }

    if (!write_int32_b(ww->fp, FOUR_CC('d','a','t','a'))) {
        return 0;
    }

    if (!write_int32_l(ww->fp, get_data_len(ww))) {
        return 0;
    }

    return 1;
}

static int
is_acceptable_num_channels(const wave_writer_format *format) {
    return 1 <= format->num_channels 
             && format->num_channels <= 8;
}

static int
is_acceptable_sample_bits(const wave_writer_format *format) {
    return format->sample_bits == 8
        || format->sample_bits == 16
        || format->sample_bits == 24;
}

static int
is_acceptable_sample_rate(const wave_writer_format *format) {
    return format->sample_rate == 8000
        || format->sample_rate == 11025
        || format->sample_rate == 16000
        || format->sample_rate == 22050
        || format->sample_rate == 24000
        || format->sample_rate == 32000
        || format->sample_rate == 44100
        || format->sample_rate == 48000;
}

struct wave_writer *
wave_writer_open(const char *filename, const wave_writer_format *format, wave_writer_error *error)
{
    struct wave_writer *ww = NULL;

    assert(filename != NULL);
    assert(format != NULL);
    assert(error != NULL);

    if (!is_acceptable_num_channels(format) || !is_acceptable_sample_bits(format) || !is_acceptable_sample_rate(format)) {
        *error = WW_BAD_FORMAT;
        goto format_error;
    }

    ww = (struct wave_writer *)calloc(1, sizeof(struct wave_writer));
    if (!ww) {
        *error = WW_ALLOC_ERROR;
        goto alloc_error;
    }

    ww->fp = fopen(filename, "wb");
    if (!ww->fp) {
        *error = WW_OPEN_ERROR;
        goto open_error;
    }

    ww->format = FORMAT;
    ww->num_channels = format->num_channels;
    ww->sample_rate = format->sample_rate;
    ww->sample_bits = format->sample_bits;
    ww->num_samples = 0;

    if (!write_riff_chunk(ww)) {
        *error = WW_IO_ERROR;
        goto writing_error;
    }

    return ww;

writing_error:
    fclose(ww->fp);
open_error:
    free(ww);
alloc_error:
format_error:

    return NULL;
}

int
wave_writer_close(struct wave_writer *ww, wave_writer_error *error)
{
    int result = 1;

    if (ww) {
        int l = ww->num_samples * ww->num_channels * ww->sample_bits / 8;
        assert(0 <= l);
        if (l % 2 != 0) {
            write_byte(ww->fp, 0);
        }

        if (fseek(ww->fp, 0, SEEK_SET) != 0) {
            *error = WW_IO_ERROR;
            result = 0;
            goto release;
        }

        if (!write_riff_chunk(ww)) {
            *error = WW_IO_ERROR;
            result = 0;
            goto release;
        }

release:
        fclose(ww->fp);
        free(ww);
    }

    return result;
}

int
wave_writer_get_format(struct wave_writer *ww)
{
    assert(ww != NULL);

    return ww->format;
}

int
wave_writer_get_num_channels(struct wave_writer *ww)
{
    assert(ww != NULL);

    return ww->num_channels;
}

int
wave_writer_get_sample_rate(struct wave_writer *ww)
{
    assert(ww != NULL);

    return ww->sample_rate;
}

int
wave_writer_get_sample_bits(struct wave_writer *ww)
{
    assert(ww != NULL);

    return ww->sample_bits;
}

int
wave_writer_get_num_samples(struct wave_writer *ww)
{
    assert(ww != NULL);

    return ww->num_samples;
}

int
wave_writer_put_samples(struct wave_writer *ww, int n, void *buf)
{
    int ret;

    assert(ww != NULL);
    assert(buf != NULL);

    ret = fwrite(buf, ww->num_channels * ww->sample_bits / 8, n, ww->fp);
    if (ret < n) {
        return -1;
    }

    ww->num_samples += ret;

    return ret;
}

