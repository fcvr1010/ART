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

#ifndef INCLUDE_NB_ARSS_2_HPP_
#define INCLUDE_NB_ARSS_2_HPP_

#include "ART.hpp"

#include <atomic>
#include <functional>

namespace ART
{

/**
 * Non-blocking single-writer, single-reader atomic register
 * implementation based on a RW protocol I developed.
 *
 * This implementation is wait-free with respect to the write method,
 * and lock-free with respect to the read method.
 *
 * @tparam T register content data type.
 */
template<typename T>
class NB_ARSS_2
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
	NB_ARSS_2(const T no_value_indicator,
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

		/* Initialize status: read from 0 | write into 1. */
		m_status = 1;
	}

	/** Destructor. */
	~NB_ARSS_2()
	{
		for(unsigned int i=0; i < 4; i++){
			m_free_fnc(m_buffer[i].obj);
		}
	}

	/**
	 * Write into the atomic register.
	 *
	 * @param obj object whose copy has to be stored into the register.
	 * @param ts object timestamp.
	 */
	void write(const T obj, int_fast64_t ts)
	{
		unsigned char local_status, new_status;
		unsigned char write_slot;

		/* Get space to write into. */
		local_status = m_status;
		write_slot = (local_status & 0x3);

		/* Release old object (if any) and write new one. */
		m_free_fnc(m_buffer[write_slot].obj);
		m_buffer[write_slot].obj = m_copy_fnc(obj);
		m_buffer[write_slot].ts = ts;

		/* Indicate to the reader where to look for. */
		new_status = (write_slot << 2) | (write_slot ^ 0x2);
		if(!m_status.compare_exchange_strong(local_status, new_status))
			m_status = (write_slot << 2) | (local_status & 0x3);
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
		unsigned char local_status, new_status;
		unsigned char read_slot;

		/* Get read slot, possibly moving the next write to the other pair of slots if needed. */
		local_status = m_status;
		do new_status = (local_status & 0xE) | ((~local_status >> 2) & 0x1);
		while(!m_status.compare_exchange_weak(local_status, new_status));

		/* Perform read. */
		read_slot = (local_status >> 2);
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
	 * Register status. 4 bits with the following meaning (order
	 * from LSB to MSB):
	 *
	 * 	- Bit 1 and bit 2 encode the index of the buffer slot
	 * 	  that will be used by the next write operation.
	 *
	 *	- Bit 3 and bit 4 encode the index of the buffer slot
	 * 	  containing the most up-to-date value.
	 */
	std::atomic<unsigned char> m_status;
};

} /* namespace ART */

#endif /* INCLUDE_NB_ARSS_2_HPP_ */
