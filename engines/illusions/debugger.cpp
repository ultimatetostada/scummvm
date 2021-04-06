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

#include "illusions/debugger.h"
#include "common/debug.h"

namespace Illusions {

Debugger::Debugger(IllusionsEngine *vm) : GUI::Debugger() {
    _vm = vm;

	_videoPlaybackToggle = true;

    registerCmd("scene", WRAP_METHOD(Debugger, cmdScene));
	registerCmd("video", WRAP_METHOD(Debugger, cmdVideoPlaybackToggle));
}

Debugger::~Debugger() {
	
}

bool Debugger::cmdScene(int argc, const char **argv) {
	if (argc != 2 && argc != 4) {
		debugPrintf("Loads the selected scene.\n");
		debugPrintf("Usage: %s [<sceneId> | <bootparam>]\n", argv[0]);
		return true;
	}

	return true;
}

bool Debugger::cmdVideoPlaybackToggle(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Toggles video playback.\n");
		debugPrintf("Usage: %s (on | off)\n", argv[0]);
		return true;
	}

	Common::String arg = argv[1];
	if (arg == "on" || arg == "On" || arg == "ON") {
		_videoPlaybackToggle = true;
	} else if (arg == "off" || arg == "Off" || arg == "OFF") {
		_videoPlaybackToggle = false;
	}

	return true;
}

}
