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

#ifndef INCLUDE_B_AR_1_HPP_
#define INCLUDE_B_AR_1_HPP_

#include "ART.hpp"

#include <mutex>

namespace ART
{

/**
 * Blocking atomic register implementation based on the use
 * of a mutex (can be shared among any number of readers and
 * writers)
 *
 * This implementation is mostly provided for reference and
 * performance comparison.
 *
 * @tparam T register content data type.
 */
template<typename T>
class B_AR_1
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
	B_AR_1(const T no_value_indicator,
			std::function<T(const T)> copy_fnc = COPY_FNC<T>,
			std::function<void(const T, T*)> get_fnc = GET_FNC<T>,
			std::function<void(T)> free_fnc = FREE_FNC<T>) :
		m_no_value_indicator(no_value_indicator),
		m_copy_fnc(copy_fnc),
		m_get_fnc(get_fnc),
		m_free_fnc(free_fnc)
	{
		/* Initialize register. */
		m_reg.obj = m_no_value_indicator;
		m_reg.ts = NO_VALUE_TS;
	}

	/** Destructor. */
	~B_AR_1()
	{
		m_free_fnc(m_reg.obj);
	}

	/**
	 * Write into the atomic register.
	 *
	 * @param obj object whose copy has to be stored into the register.
	 * @param ts object timestamp.
	 */
	void write(const T obj, int_fast64_t ts)
	{
		/* Get lock. */
		m_mutex.lock();

		/* Release old object (if any) and write new one. */
		m_free_fnc(m_reg.obj);
		m_reg.obj = m_copy_fnc(obj);
		m_reg.ts = ts;

		/* Release lock. */
		m_mutex.unlock();
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
		/* Get lock. */
		m_mutex.lock();

		/* Perform read. */
		m_get_fnc(m_reg.obj, obj);
		*ts = m_reg.ts;

		/* Release lock. */
		m_mutex.unlock();
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

	/** The register .*/
	ARContent<T> m_reg;

	/** Lock for exclusive access to the register. */
	std::mutex m_mutex;
};

} /* namespace ART */

#endif /* INCLUDE_B_AR_1_HPP_ */
