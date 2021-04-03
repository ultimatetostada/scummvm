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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef ILLUSIONS_BBDOU_BBDOU_BOOTPARAMS_H
#define ILLUSIONS_BBDOU_BBDOU_BOOTPARAMS_H

namespace Illusions {

struct BootParam {
	int dsceneId;
	int dthreadId;
};

class BbdouBootParams {
public:
	BbdouBootParams();
	~BbdouBootParams();
	static BootParam getBootParam(int param);

private:
	static const int BOOT_PARAM_COUNT = 26;
	constexpr static BootParam BootParams[BOOT_PARAM_COUNT] = {
		{0x00000000, 0x00000000}, //Highland High - Default
		{0x00010006, 0x00020007}, //Highland High
		{0x00010007, 0x0002000C}, //Auditorium
		{0x00010079, 0x0002039C}, //Student Union cutscene
		{0x00010031, 0x00020036}, //MAP
		{0x00010019, 0x0002001E}, //Library
		{0x0001003E, 0x00020056}, //Library Computer
		{0x00010016, 0x0002001B}, //Dorms ext
		{0x00010017, 0x0002001C}, //Dorms int
		{0x00010028, 0x000202A1}, //Music room
		{0x00010012, 0x00020017}, //Dean
		{0x00010013, 0x00020018}, //Therapist
		{0x0001000B, 0x00020010}, //Quad
		{0x0001001C, 0x00020021}, //Agriculture
		{0x0001001D, 0x00020022}, //Agriculture Bull
		{0x0001001A, 0x0002001F}, //Communications
		{0x0001000E, 0x00020013}, //Java
		{0x0001003C, 0x00020054}, //Dumpster
		{0x0001003A, 0x00020052}, //Bathroom
		{0x0001003B, 0x00020053}, //Laundry
		{0x00010018, 0x0002001D}, //Dorm Room
		{0x0001000D, 0x00020012}, //Food minigame | Fails after dialog
		{0x00010067, 0x0002022A}, //Salad Bar
		{0x0001000C, 0x00020011}, //Cafeteria
		{0x00010047, 0x0002005F}, //Frat House - Shooting minigame | Scene crash when click to shoot
		{0x0001007D, 0x000203B9}  //Credits
	};
};

} // End of namespace Illusions

#endif // ILLUSIONS_BBDOU_BBDOU_BUBBLE_H
