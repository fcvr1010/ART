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

#ifndef INCLUDE_ART_HPP_
#define INCLUDE_ART_HPP_

#include <cstdint>

/**
 * @file ART.hpp
 *
 * Elements used by all registers.
 */

namespace ART
{

/**
 * Class representing the content of a register.
 *
 * @tparam T register content data type.
 */
template<typename T>
class ARContent
{
public:
	/** Object stored into the register. */
	T obj;

	/** Timestamp associated to the object stored into the register. */
	int_fast64_t ts;
};

/** Timestamp value associated to a register entry that contains no data. */
const int_fast64_t NO_VALUE_TS = -1;

/**
 * Default register content copy function.
 *
 * It is intended to be used in registers whose content is of
 * an elementary data type. In case the content is of a
 * complex data type (e.g. a class), the user shall specify an
 * appropriate copy function.
 *
 * @tparam T register content data type.
 * @param obj object to be copied.
 * @return the new object that is a copy of @p obj.
 */
template<typename T>
static inline T COPY_FNC(const T obj){return obj;}

/**
 * Default register content get function.
 *
 * It is intended to be used in registers whose content is of
 * an elementary data type. In case the content is of a
 * complex data type (e.g. a class), the user shall specify an
 * appropriate get function.
 *
 * @tparam T register content data type.
 * @param source the object to get.
 * @param dest pointer to the object into which @p source will be copied.
 */
template<typename T>
static inline void GET_FNC(const T source, T* dest){*dest = source;}

/**
 * Default register content memory release function.
 *
 * It does nothing and is intended to be used in registers whose
 * content is of an elementary data type. In case the content is
 * of a complex data type (e.g. a class), the user shall specify
 * an appropriate memory release function.
 *
 * @tparam T register content data type.
 * @param obj the object to release.
 */
template<typename T>
static inline void FREE_FNC(T obj){}

} /* namespace ART */

#endif /* INCLUDE_ART_HPP_ */
