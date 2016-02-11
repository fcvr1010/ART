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

#ifndef INCLUDE_NB_ARSS_1_HPP_
#define INCLUDE_NB_ARSS_1_HPP_

#include "ART.hpp"

#include <atomic>
#include <functional>

namespace ART
{

/**
 * Non-blocking single-writer, single-reader atomic register
 * implementation based on a RW protocol I developed.
 *
 * This implementation is wait-free.
 *
 * @tparam T register content data type.
 */
template<typename T>
class NB_ARSS_1
{
public:

	/**
	 * Constructor.
	 *
	 * @param no_value_indicator constant used to denote the absence of a value into the register.
	 * @param copy_fnc register content copy function.
	 * @param get_fnc register content get function.
	 * @param free_fnc register content memory release function.
	 */
	NB_ARSS_1(const T no_value_indicator,
			std::function<T(const T)> copy_fnc = COPY_FNC<T>,
			std::function<void(const T, T*)> get_fnc = GET_FNC<T>,
			std::function<void(T)> free_fnc = FREE_FNC<T>) :
		m_no_value_indicator(no_value_indicator),
		m_copy_fnc(copy_fnc),
		m_get_fnc(get_fnc),
		m_free_fnc(free_fnc)
	{
		/* Initialize buffer. */
		for(unsigned int i=0; i < 4; i++){
			m_buffer[i].obj = m_no_value_indicator;
			m_buffer[i].ts = NO_VALUE_TS;
		}

		/* Initialize status: next read from slot 0, currently not reading. */
		m_next_read = 0;
	}

	/**
	 * Write into the atomic register.
	 *
	 * @param obj object whose copy has to be stored into the register.
	 * @param ts object timestamp.
	 */
	void write(const T obj, int_fast64_t ts)
	{
		unsigned char local_next_read, new_next_read;
		unsigned char write_slot;

		/* Buffer slot to write into (candidate), initially 1. */
		static unsigned char next_write = 1;

		/* Get space to write into. */
		local_next_read = m_next_read.fetch_and(0x3);
		if(local_next_read >> 2) next_write = ((next_write & 0x2) | (~local_next_read & 0x1)); // pair switch needed
		write_slot = next_write;

		/* Release old object (if any) and write new one. */
		m_free_fnc(m_buffer[write_slot].obj);
		m_buffer[write_slot].obj = m_copy_fnc(obj);
		m_buffer[write_slot].ts = ts;

		/* Indicate to the reader where to look for and adjust the next write position. */
		new_next_read = write_slot;
		next_write = (write_slot ^ 0x2);
		local_next_read = (local_next_read & 0x3);
		if(!m_next_read.compare_exchange_strong(local_next_read, new_next_read))
		{
			next_write = ((next_write & 0x2) | (~local_next_read & 0x1)); // pair switch needed
			m_next_read.compare_exchange_strong(local_next_read, new_next_read);
		}
	}

	/**
	 * Read from the atomic register.
	 *
	 * @param obj pointer to the variable that will be filled (using the register get function)
	 * with the register content.
	 * @param ts pointer to the variable where the timestamp associated to the register content will be stored.
	 */
	void read(T* obj, int_fast64_t* ts)
	{
		/* Get slot to read from. */
		unsigned char read_slot = (m_next_read.fetch_or(0x4) & 0x3);

		/* Perform read. */
		m_get_fnc(m_buffer[read_slot].obj, obj);
		*ts = m_buffer[read_slot].ts;
	}

protected:
	/** No-value indicator. */
	T m_no_value_indicator;

	/** Register content copy function. */
	std::function<T(const T)> m_copy_fnc;

	/** Register content get function. */
	std::function<void(const T, T*)> m_get_fnc;

	/** Register content memory release function. */
	std::function<void(T)> m_free_fnc;

	/** Buffer. */
	ARContent<T> m_buffer[4];

	/**
	 * Register status. 3 bits with the following meaning (order
	 * from LSB to MSB):
	 *
	 * 	- Bit 1 and bit 2 encode the index of the buffer slot
	 * 	  containing the most up-to-date value.
	 *
	 * 	- Bit 3 indicates whether the reader is reading from the
	 * 	  above described buffer slot.
	 */
	std::atomic<unsigned char> m_next_read;
};

} /* namespace ART */

#endif /* INCLUDE_NB_ARSS_1_HPP_ */
