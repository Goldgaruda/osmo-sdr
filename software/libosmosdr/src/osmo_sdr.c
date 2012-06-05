/*
 * sysmocom OsmoSDR
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <Windows.h>
#endif

#include "osmosdr.h"

#define DEFAULT_SAMPLE_RATE		500000
#define DEFAULT_ASYNC_BUF_NUMBER	32
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)

static int do_exit = 0;
static osmosdr_dev_t *dev = NULL;

void usage(void)
{
	#ifdef _WIN32
	fprintf(stderr,
		"Usage:\t osmo_sdr.exe [device_index] [samplerate in kHz] "
		"[gain] [frequency in Hz] [filename]\n");
	#else
	fprintf(stderr,
		"Usage:\t -f frequency_to_tune_to [Hz]\n"
		"\t[-s samplerate (default: 2048000 Hz)]\n"
		"\t[-d device_index (default: 0)]\n"
		"\t[-g gain (default: 0 for auto)]\n"
		"\t[-b output_block_size (default: 16 * 16384)]\n"
		"\t[-S force sync output (default: async)]\n"
		"\tfilename (a '-' dumps samples to stdout)\n\n");
#endif
	exit(1);
}

#ifdef _WIN32
BOOL WINAPI
sighandler(int signum)
{
	if (CTRL_C_EVENT == signum) {
		fprintf(stderr, "Signal caught, exiting!\n");
		do_exit = 1;
		osmosdr_cancel_async(dev);
		return TRUE;
	}
	return FALSE;
}
#else
static void sighandler(int signum)
{
	fprintf(stderr, "Signal caught, exiting!\n");
	do_exit = 1;
	osmosdr_cancel_async(dev);
}
#endif

static void osmosdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
	if (ctx) {
		if (fwrite(buf, 1, len, (FILE*)ctx) != len) {
			fprintf(stderr, "Short write, samples lost, exiting!\n");
			osmosdr_cancel_async(dev);
		}
	}
}

int main(int argc, char **argv)
{
#ifndef _WIN32
	struct sigaction sigact;
#endif
	char *filename = NULL;
	int n_read;
	int r, opt;
	int i, gain = 0;
	int sync_mode = 0;
	FILE *file;
	uint8_t *buffer;
	uint32_t dev_index = 0;
	uint32_t frequency = 100000000;
	uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
	uint32_t out_block_size = DEFAULT_BUF_LENGTH;
	int device_count;
	char vendor[256] = { 0 }, product[256] = { 0 }, serial[256] = { 0 };
	int count;
	int gains[100];
	uint32_t rates[100];

#ifndef _WIN32
	while ((opt = getopt(argc, argv, "d:f:g:s:b:S::")) != -1) {
		switch (opt) {
		case 'd':
			dev_index = atoi(optarg);
			break;
		case 'f':
			frequency = (uint32_t)atof(optarg);
			break;
		case 'g':
			gain = (int)(atof(optarg) * 10); /* tenths of a dB */
			break;
		case 's':
			samp_rate = (uint32_t)atof(optarg);
			break;
		case 'b':
			out_block_size = (uint32_t)atof(optarg);
			break;
		case 'S':
			sync_mode = 1;
			break;
		default:
			usage();
			break;
		}
	}

	if (argc <= optind) {
		usage();
	} else {
		filename = argv[optind];
	}
#else
	if(argc <6)
		usage();
	dev_index = atoi(argv[1]);
	samp_rate = atoi(argv[2])*1000;
	gain=(int)(atof(argv[3]) * 10); /* tenths of a dB */
	frequency = atoi(argv[4]);
	filename = argv[5];
#endif
	if(out_block_size < MINIMAL_BUF_LENGTH ||
	   out_block_size > MAXIMAL_BUF_LENGTH ){
		fprintf(stderr,
			"Output block size wrong value, falling back to default\n");
		fprintf(stderr,
			"Minimal length: %u\n", MINIMAL_BUF_LENGTH);
		fprintf(stderr,
			"Maximal length: %u\n", MAXIMAL_BUF_LENGTH);
		out_block_size = DEFAULT_BUF_LENGTH;
	}

	buffer = malloc(out_block_size * sizeof(uint8_t));

	device_count = osmosdr_get_device_count();
	if (!device_count) {
		fprintf(stderr, "No supported devices found.\n");
		exit(1);
	}

	fprintf(stderr, "Found %d device(s):\n", device_count);
	for (i = 0; i < device_count; i++) {
		osmosdr_get_device_usb_strings(i, vendor, product, serial);
		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Using device %d: %s\n",
		dev_index, osmosdr_get_device_name(dev_index));

	r = osmosdr_open(&dev, dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open osmosdr device #%d.\n", dev_index);
		exit(1);
	}
#ifndef _WIN32
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
#else
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
#endif
	count = osmosdr_get_tuner_gains(dev, NULL);
	fprintf(stderr, "Supported gain values (%d): ", count);

	count = osmosdr_get_tuner_gains(dev, gains);
	for (i = 0; i < count; i++)
		fprintf(stderr, "%.1f ", gains[i] / 10.0);
	fprintf(stderr, "\n");

	count = osmosdr_get_sample_rates(dev, NULL);
	fprintf(stderr, "Supported sample rates (%d): ", count);

	count = osmosdr_get_sample_rates(dev, rates);
	for (i = 0; i < count; i++)
		fprintf(stderr, "%u ", rates[i]);
	fprintf(stderr, "\n");

	r = osmosdr_get_usb_strings(dev, vendor, product, serial);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to read usb strings.\n");
	else
		fprintf(stderr, "%s, %s: SN: %s\n", vendor, product, serial);

	/* Set the sample rate */
	r = osmosdr_set_sample_rate(dev, samp_rate);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to set sample rate.\n");
	else {
		samp_rate = osmosdr_get_sample_rate(dev);
		fprintf(stderr, "Sample rate is set to %u Hz.\n", samp_rate);
	}

	/* Set the frequency */
	r = osmosdr_set_center_freq(dev, frequency);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to set center freq.\n");
	else
		fprintf(stderr, "Tuned to %u Hz.\n", frequency);

	if (0 == gain) {
		 /* Enable automatic gain */
		r = osmosdr_set_tuner_gain_mode(dev, 0);
		if (r < 0)
			fprintf(stderr, "WARNING: Failed to enable automatic gain.\n");
	} else {
		/* Enable manual gain */
		r = osmosdr_set_tuner_gain_mode(dev, 1);
		if (r < 0)
			fprintf(stderr, "WARNING: Failed to enable manual gain.\n");

		/* Set the tuner gain */
		r = osmosdr_set_tuner_gain(dev, gain);
		if (r < 0)
			fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
		else
			fprintf(stderr, "Tuner gain set to %f dB.\n", gain/10.0);
	}

	if(strcmp(filename, "-") == 0) { /* Write samples to stdout */
		file = stdout;
	} else {
		file = fopen(filename, "wb");
		if (!file) {
			fprintf(stderr, "Failed to open %s\n", filename);
			goto out;
		}
	}

	/* Reset endpoint before we start reading from it (mandatory) */
	r = osmosdr_reset_buffer(dev);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to reset buffers.\n");

	if (sync_mode) {
		fprintf(stderr, "Reading samples in sync mode...\n");
		while (!do_exit) {
			r = osmosdr_read_sync(dev, buffer, out_block_size, &n_read);
			if (r < 0) {
				fprintf(stderr, "WARNING: sync read failed.\n");
				break;
			}

			if (fwrite(buffer, 1, n_read, file) != (size_t)n_read) {
				fprintf(stderr, "Short write, samples lost, exiting!\n");
				break;
			}

			if ((uint32_t)n_read < out_block_size) {
				fprintf(stderr, "Short read, samples lost, exiting!\n");
				break;
			}
		}
	} else {
		fprintf(stderr, "Reading samples in async mode...\n");
		r = osmosdr_read_async(dev, osmosdr_callback, (void *)file,
				      DEFAULT_ASYNC_BUF_NUMBER, out_block_size);
	}

	if (do_exit)
		fprintf(stderr, "\nUser cancel, exiting...\n");
	else
		fprintf(stderr, "\nLibrary error %d, exiting...\n", r);

	if (file != stdout)
		fclose(file);

	osmosdr_close(dev);
	free (buffer);
out:
	return r >= 0 ? r : -r;
}
