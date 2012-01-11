/* -*- c++ -*- */
/*
 * Copyright 2004,2010 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <osmosdr_source_c.h>
#include <gr_io_signature.h>
#include <gr_audio_source.h>
#include <gr_float_to_complex.h>

/*
 * Create a new instance of osmosdr_source_c and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
osmosdr_source_c_sptr
osmosdr_make_source_c (const std::string &device)
{
  return gnuradio::get_initial_sptr(new osmosdr_source_c (device));
}

/*
 * Specify constraints on number of input and output streams.
 * This info is used to construct the input and output signatures
 * (2nd & 3rd args to gr_block's constructor).  The input and
 * output signatures are used by the runtime system to
 * check that a valid number and type of inputs and outputs
 * are connected to this block.  In this case, we accept
 * only 1 input and 1 output.
 */
static const int MIN_IN = 0;	// mininum number of input streams
static const int MAX_IN = 0;	// maximum number of input streams
static const int MIN_OUT = 1;	// minimum number of output streams
static const int MAX_OUT = 1;	// maximum number of output streams

/*
 * The private constructor
 */
osmosdr_source_c::osmosdr_source_c (const std::string &device)
  : gr_hier_block2 ("source_c",
        gr_make_io_signature (MIN_IN, MAX_IN, sizeof (gr_complex)),
        gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (gr_complex)))
{
    std::string dev_name = "hw:0";
/*
    if (device.empty())
        pick first available device or throw an exception();
*/
    gr_float_to_complex_sptr f2c;

    /* Audio source; sample rate is 96kHz by default */
    audio_source::sptr src = audio_make_source(96000, device, true);

    /* block to convert stereo audio to a complex stream */
    f2c = gr_make_float_to_complex(1);

    connect(src, 0, f2c, 0);
    connect(src, 1, f2c, 1);
    connect(f2c, 0, self(), 0);
}

/*
 * Our virtual destructor.
 */
osmosdr_source_c::~osmosdr_source_c ()
{
  // nothing else required in this example
}
