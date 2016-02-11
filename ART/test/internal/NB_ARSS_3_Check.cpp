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
 * @file NB_ARSS_3_Check.cpp
 *
 * A program to check the non-blocking single-writer, single-reader
 * atomic register implementation in NB_ARSS_3.
 */

#include "NB_ARSS_3.hpp"

#include <cstdint>
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;
using namespace ART;

#define ARR_LEN 4096

NB_ARSS_3<int_fast64_t*>* reg;
int_fast64_t* no_value_indicator;

atomic<bool> start;
atomic<bool> stop;

bool error;

inline int_fast64_t* MY_COPY_FNC(const int_fast64_t* obj){
	int_fast64_t* cp = no_value_indicator;

	if(obj != no_value_indicator){
		cp = (int_fast64_t*)calloc(ARR_LEN, sizeof(int_fast64_t));
		for(unsigned int i = 0; i < ARR_LEN; i++) cp[i] = obj[i];
	}

	return cp;
}

inline void MY_GET_FNC(const int_fast64_t* source, int_fast64_t** dest){
	if(source != no_value_indicator){
		for(unsigned int i = 0; i < ARR_LEN; i++) (*dest)[i] = source[i];
	}
}

inline void MY_FREE_FNC(int_fast64_t* obj){
	if(obj != no_value_indicator) free(obj);
}

static void fill_value(int_fast64_t* value, int_fast64_t ts){
	for(unsigned int i = 0; i < ARR_LEN; i++){
		if(i == 0) value[i] = ts;
		else if(i == 1) value[i] = ts + 1;
		else value[i] = (value[i-1] + value[i-2]) & 0xFFFFFFFF;
	}
}

static bool check_value(const int_fast64_t* obj, int_fast64_t ts){
	int_fast64_t expected;

	for(unsigned int i = 0; i < ARR_LEN; i++){
		if(i == 0) expected = ts;
		else if (i == 1) expected = ts + 1;
		else expected = (obj[i-1] + obj[i-2]) & 0xFFFFFFFF;

		if(obj[i] != expected){
			cout << "Found " << obj[i] << "\tExpected " << expected << endl;
			return false;
		}
	}
	return true;
}

static void reader_run()
{
	int_fast64_t* curr_value;
	int_fast64_t curr_ts;

	curr_value = (int_fast64_t*)calloc(ARR_LEN, sizeof(int_fast64_t));

	while(!start); // spin

	while(!stop)
	{
		reg->read(&curr_value, &curr_ts);
		if(curr_ts != NO_VALUE_TS)
		{
			if(!check_value(curr_value, curr_ts)){
				error = true;
				return;
			}
		}
	}

	free(curr_value);
}

static void writer_run()
{
	int_fast64_t curr_value[ARR_LEN] = {0};
	int_fast64_t curr_ts = 0;

	while(!start); // spin

	while(!stop)
	{
		curr_ts++;
		fill_value(curr_value, curr_ts);
		reg->write(curr_value, curr_ts);
	}
}

int main (void)
{
	unsigned int runtime_sec = 60 * 10;

	/* Initialize. */
	no_value_indicator = NULL;
	reg = new NB_ARSS_3<int_fast64_t*>(no_value_indicator, MY_COPY_FNC, MY_GET_FNC, MY_FREE_FNC);
	start = false;
	stop = false;
	error = false;

	/* Create and start threads */
	thread* writer = new thread(writer_run);
	thread* reader = new thread(reader_run);
	start = true;

	/* Run for the predefined number of seconds. */
	std::this_thread::sleep_for(std::chrono::seconds(runtime_sec));

	/* Stop threads and join them. */
	stop = true;
	writer->join();
	reader->join();

	if(error) cout << "Errors occurred." << endl;
	else cout << "Everything's fine." << endl;

	delete writer;
	delete reader;
	delete reg;
}
