/*
 * Copyright (C) 2015-2016 Francesco Cavrini.
 *
 * This file is part of the Atomic Register Toolkit (ART).
 *
 * *********************************************************************
 * The Atomic Register Toolkit is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *********************************************************************
 *
 * If you would like to use this software but cannot obey the GPL license
 * terms, you can contact me at: my-name my-surname at gmail.com (all lower
 * case, with no dots nor other characters in between) in order to arrange
 * for a release under a compatible license.
 *
 * Please feel free to contact me (email address above) to report a bug.
 * Moreover, feedback is welcome.
 */

/**
 * @file NB_ARSS_2_Perf.cpp
 *
 * A program to benchmark the non-blocking single-writer, single-reader
 * atomic register implementation in NB_ARSS_2.
 */

#include "NB_ARSS_2.hpp"

#include <cstdint>
#include <iostream>
#include <thread>
#include <atomic>
#include <stdexcept>

using namespace std;
using namespace ART;

NB_ARSS_2<int_fast64_t>* reg;
int_fast64_t no_value_indicator;

atomic<bool> start;
atomic<bool> stop;
atomic<bool> warm_up;

int_fast64_t nwrite;
int_fast64_t nread;
int_fast64_t nread_unique;

static void reader_run()
{
	int_fast64_t curr_value;
	int_fast64_t curr_ts;
	int_fast64_t prev_ts = NO_VALUE_TS;

	while(!start); // spin

	while(!stop)
	{
		if(!warm_up) nread++;

		reg->read(&curr_value, &curr_ts);
		if(curr_ts > prev_ts)
		{
			if(!warm_up) nread_unique++;
			prev_ts = curr_ts;
		}
		else if (curr_ts < prev_ts)
		{
			throw runtime_error("Read returned an object with timestamp lower than the previous one.");
		}
	}
}

static void writer_run()
{
	int_fast64_t curr_ts = 0;

	while(!start); // spin

	while(!stop)
	{
		if(!warm_up) nwrite++;

		curr_ts++;
		reg->write(curr_ts, curr_ts);
	}
}

int main (void)
{
	thread* writer = NULL;
	thread* reader = NULL;
	unsigned int runtime_sec = 60 * 10;
	bool error = false;

	cout << "millions of W op per sec; millions of R op per sec; millions of unique R op per sec";

	for(unsigned int cntTrial = 0; (cntTrial < 30) && (!error); cntTrial++){
		/* Initialize. */
		no_value_indicator = NO_VALUE_TS;
		reg = new NB_ARSS_2<int_fast64_t>(no_value_indicator);

		nwrite = 0;
		nread = 0;
		nread_unique = 0;

		start = false;
		stop = false;
		warm_up = true;

		try
		{
			/* Create and start threads */
			writer = new thread(writer_run);
			reader = new thread(reader_run);
			start = true;

			/* Warm-up */
			std::this_thread::sleep_for(std::chrono::seconds(60));
			warm_up = false;

			/* Run for the predefined number of seconds. */
			std::this_thread::sleep_for(std::chrono::seconds(runtime_sec));

			/* Stop threads and join them. */
			stop = true;
			writer->join();
			reader->join();

			/* Print output info. */
			cout	<< endl
					<< ((double)nwrite / (double)(runtime_sec * 1e6)) << ";"
					<< ((double)nread / (double)(runtime_sec * 1e6)) << ";"
					<< ((double)nread_unique / (double)(runtime_sec * 1e6));
		}
		catch(const exception& ex)
		{
			cout << "Error: " << ex.what() << endl;
			error = true;
		}

		delete writer;
		delete reader;
		delete reg;
	}

	if(error) return EXIT_FAILURE;
	else return EXIT_SUCCESS;
}
