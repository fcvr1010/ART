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

#ifndef INCLUDE_NB_ARSS_3_HPP_
#define INCLUDE_NB_ARSS_3_HPP_

#include "ART.hpp"

#include <atomic>
#include <functional>

namespace ART
{

/**
 * Non-blocking single-writer, single-reader atomic register
 * implementation based on [1].
 *
 * The read-write protocol here implemented is exactly the one described in [1] but
 * I contextualized the procedure within the present Atomic Register Toolkit structure.
 *
 * @tparam T register content data type.
 *
 * [1] Jing Chen, and Alan Burns. "A three-slot asynchronous reader/writer mechanism
 * for multiprocessor real-time systems." Report - University of York (1997).
 */
template<typename T>
class NB_ARSS_3
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
	NB_ARSS_3(const T no_value_indicator,
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

		/* Initialize status. */
		m_reading = 3;
		m_latest = 0;
	}

	/** Destructor. */
	~NB_ARSS_3()
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
		static const int next[4][3] = {{1,2,1},{2,2,0},{1,0,0},{1,2,0}};
		unsigned char widx1, widx2, windex, tmp;

		widx1 = m_reading;
		widx2 = m_latest;
		windex = next[widx1][widx2];

		/* Release old object (if any) and write new one. */
		m_free_fnc(m_buffer[windex].obj);
		m_buffer[windex].obj = m_copy_fnc(obj);
		m_buffer[windex].ts = ts;

		m_latest = windex;
		tmp = 3;
		m_reading.compare_exchange_strong(tmp, windex);
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
		unsigned char rindex, tmp;

		m_reading = 3;
		rindex = m_latest;
		tmp = 3;
		m_reading.compare_exchange_strong(tmp, rindex);
		rindex = m_reading;

		/* Perform read. */
		m_get_fnc(m_buffer[rindex].obj, obj);
		*ts = m_buffer[rindex].ts;
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

	/** First control variable (see [1] for further information). */
	std::atomic<unsigned char> m_reading;

	/** Second control variable (see [1] for further information). */
	std::atomic<unsigned char> m_latest;
};

} /* namespace ART */

#endif /* INCLUDE_NB_ARSS_3_HPP_ */
