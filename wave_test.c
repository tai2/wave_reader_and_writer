#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wave_reader.h"
#include "wave_writer.h"

#define BUFFER_SIZE 4096

void test_read_write(const char *filename) {
    wave_reader *wr;
    wave_reader *wr2;
    wave_reader_error rerror;
    wave_writer *ww;
    wave_writer_error werror;
    wave_writer_format format;
    char copied[256];

    wr = wave_reader_open(filename, &rerror);
    if (wr) {
        snprintf(copied, sizeof(copied), "%s.copy", filename);
        format.num_channels = wave_reader_get_num_channels(wr);
        format.sample_rate = wave_reader_get_sample_rate(wr);
        format.sample_bits = wave_reader_get_sample_bits(wr);
        ww = wave_writer_open(copied, &format, &werror);
        if (ww) {
            unsigned char *buf;
            int n;

            printf("filename=%s format=%d num_channels=%d sample_rate=%d sample_bits=%d num_samples=%d\n",
                    filename,
                    wave_reader_get_format(wr),
                    wave_reader_get_num_channels(wr),
                    wave_reader_get_sample_rate(wr),
                    wave_reader_get_sample_bits(wr),
                    wave_reader_get_num_samples(wr));
            buf = (unsigned char *)malloc(BUFFER_SIZE * format.num_channels * format.sample_bits / 8);

            while (0 < (n = wave_reader_get_samples(wr, BUFFER_SIZE, buf))) {
                wave_writer_put_samples(ww, n, buf);
            }

            assert(wave_reader_get_format(wr) == wave_writer_get_format(ww));
            assert(wave_reader_get_num_channels(wr) == wave_writer_get_num_channels(ww));
            assert(wave_reader_get_sample_rate(wr) == wave_writer_get_sample_rate(ww));
            assert(wave_reader_get_sample_bits(wr) == wave_writer_get_sample_bits(ww));
            assert(wave_reader_get_num_samples(wr) == wave_writer_get_num_samples(ww));
            wave_writer_close(ww, &werror);
            free(buf);

            wr2 = wave_reader_open(copied, &rerror);
            if (wr2) {
                assert(wave_reader_get_format(wr) == wave_reader_get_format(wr2));
                assert(wave_reader_get_num_channels(wr) == wave_reader_get_num_channels(wr2));
                assert(wave_reader_get_sample_rate(wr) == wave_reader_get_sample_rate(wr2));
                assert(wave_reader_get_sample_bits(wr) == wave_reader_get_sample_bits(wr2));
                assert(wave_reader_get_num_samples(wr) == wave_reader_get_num_samples(wr2));
            } else {
                printf("rerror=%d\n", rerror);
            }

            wave_reader_close(wr2);
            wave_reader_close(wr);
        } else {
            printf("werror=%d\n", werror);
        }
    } else {
        printf("rerror=%d\n", rerror);
    }
}

int main(void) {
    test_read_write("11k16bitpcm.wav");
    return 0;
}
