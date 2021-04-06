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

#ifndef ILLUSIONS_DEBUGGER_H
#define ILLUSIONS_DEBUGGER_H

#include "gui/debugger.h"

namespace Illusions {

class IllusionsEngine;

enum {
	kIllusionsDebugVideo = 1 << 0
    // next new channel must be 1 << 1 (2)
	// next new channel must be 1 << 2 (4)
    // the current limitation is 32 debug channels (1 << 31 is the last one)
};

class Debugger : public GUI::Debugger {
	IllusionsEngine *_vm;

public:
	bool _videoPlaybackToggle;

	Debugger(IllusionsEngine *vm);
	~Debugger() override;

    bool cmdScene(int argc, const char **argv);
	bool cmdVideoPlaybackToggle(int argc, const char **argv);
};

} // End of namespace Illusions

#endif // ILLUSIONS_DEBUGGER_H
