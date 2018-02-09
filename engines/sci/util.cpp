/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "sci/util.h"
#include "sci/sci.h"

namespace Sci {

bool g_SciBE = false;
bool g_Sci11BE = false;
bool g_Sci32BE = false;

uint16 READ_SCIENDIAN_UINT16(const void *ptr) {
	if (g_SciBE)
		return READ_BE_UINT16(ptr);
	else
		return READ_LE_UINT16(ptr);
}

void WRITE_SCIENDIAN_UINT16(void *ptr, uint16 val) {
	if (g_Sci11BE)
		WRITE_BE_UINT16(ptr, val);
	else
		WRITE_LE_UINT16(ptr, val);
}

uint16 READ_SCI11ENDIAN_UINT16(const void *ptr) {
	if (g_Sci11BE)
		return READ_BE_UINT16(ptr);
	else
		return READ_LE_UINT16(ptr);
}

uint16 READ_SCI32ENDIAN_UINT16(const void *ptr) {
	if (g_Sci32BE)
		return READ_BE_UINT16(ptr);
	else
		return READ_LE_UINT16(ptr);
}

uint32 READ_SCI11ENDIAN_UINT32(const void *ptr) {
	if (g_Sci11BE)
		return READ_BE_UINT32(ptr);
	else
		return READ_LE_UINT32(ptr);
}

void WRITE_SCI11ENDIAN_UINT16(void *ptr, uint16 val) {
	if (g_Sci11BE)
		WRITE_BE_UINT16(ptr, val);
	else
		WRITE_LE_UINT16(ptr, val);
}

void WRITE_SCI11ENDIAN_UINT32(void *ptr, uint32 val) {
	if (g_Sci11BE)
		WRITE_BE_UINT32(ptr, val);
	else
		WRITE_LE_UINT32(ptr, val);
}

} // End of namespace Sci
