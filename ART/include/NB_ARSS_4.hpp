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

#ifndef INCLUDE_NB_ARSS_4_HPP_
#define INCLUDE_NB_ARSS_4_HPP_

#include "ART.hpp"

#include <atomic>
#include <functional>

namespace ART
{

/**
 * Non-blocking single-writer, single-reader atomic register
 * implementation based on a RW protocol I developed.
 *
 * This implementation is wait-free for the writer, and lock-free
 * for the reader.
 *
 * It is optimal with respect to the number of buffer slots (3),
 * and uses a single atomic control variable of 4 bits.
 *
 * @tparam T register content data type.
 */
template<typename T>
class NB_ARSS_4
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
	NB_ARSS_4(const T no_value_indicator,
			std::function<T(const T)> copy_fnc = COPY_FNC<T>,
			std::function<void(const T, T*)> get_fnc = GET_FNC<T>,
			std::function<void(T)> free_fnc = FREE_FNC<T>) :
		m_no_value_indicator(no_value_indicator),
		m_copy_fnc(copy_fnc),
		m_get_fnc(get_fnc),
		m_free_fnc(free_fnc)
	{
		/* Initialize buffer. */
		for(unsigned int i=0; i < 3; i++){
			m_buffer[i].obj = m_no_value_indicator;
			m_buffer[i].ts = NO_VALUE_TS;
		}

		/*
		 * Initialize status:
		 *
		 * 	- Most up-to-date value in slot 0, current/last read in slot 0.
		 * 	- Last slot written 0.
		 */
		m_status = 0;
		m_write_slot = 0;
	}

	/** Destructor. */
	~NB_ARSS_4()
	{
		for(unsigned int i=0; i < 3; i++){
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
		unsigned char local_status;

		/* Get slot to write into. */
		local_status = m_status;
		m_write_slot = ((m_write_slot + 1) % 3);
		if(m_write_slot == (local_status & 0x3)) m_write_slot = ((m_write_slot + 1) % 3);

		/* Release old object (if any) and write new one. */
		m_free_fnc(m_buffer[m_write_slot].obj);
		m_buffer[m_write_slot].obj = m_copy_fnc(obj);
		m_buffer[m_write_slot].ts = ts;

		/* Indicate to the reader where to look for. */
		if(!m_status.compare_exchange_strong(local_status, ((m_write_slot << 2) | (local_status & 0x3))))
		{
			/* Surely, this one will not fail. */
			m_status.compare_exchange_strong(local_status, ((m_write_slot << 2) | (local_status & 0x3)));
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
		unsigned char local_status, read_slot;

		/* Get slot to read from. */
		local_status = m_status;
		while(!m_status.compare_exchange_strong(local_status, ((local_status & 0xC) | (local_status >> 2))));
		read_slot = (local_status >> 2);

 		/* Perform read. */
		m_get_fnc(m_buffer[read_slot].obj, obj);
		*ts = m_buffer[read_slot].ts;
	}

	/**
	 * Read from the atomic register.
	 *
	 * This method is identical to the above one but takes a further
	 * output parameter used to inform the caller about the number
	 * of CAS-loop iterations that have been performed.
	 */
	void read(T* obj, int_fast64_t* ts, int_fast64_t* nretry)
	{
		unsigned char local_status, read_slot;

		/* Get slot to read from. */
		local_status = m_status;
		*nretry = 1;
		while(!m_status.compare_exchange_strong(local_status, ((local_status & 0xC) | (local_status >> 2)))) *nretry = *nretry + 1;
		read_slot = (local_status >> 2);

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
	ARContent<T> m_buffer[3];

	/**
	 * Register status. 4 bits with the following meaning (order
	 * from LSB to MSB):
	 *
	 * 	- Bit 1 and bit 2 contain the index of the buffer slot
	 * 	  the reader is currently reading, or has read, from.
	 *
	 * 	- Bit 3 and bit 4 contain the index of the buffer slot
	 * 	  containing the most up-to-date value.
	 */
	std::atomic<unsigned char> m_status;

	/**
	 * Write slot. This variable is used only by the writer,
	 * thus it needs not to be atomic.
	 */
	unsigned char m_write_slot;
};

} /* namespace ART */

#endif /* INCLUDE_NB_ARSS_4_HPP_ */
