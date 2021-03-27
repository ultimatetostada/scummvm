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

#include "engines/nancy/state/logo.h"

#include "engines/nancy/nancy.h"
#include "engines/nancy/sound.h"
#include "engines/nancy/input.h"

namespace Common {
DECLARE_SINGLETON(Nancy::State::Logo);
}

namespace Nancy {
namespace State {

void Logo::process() {
	switch (_state) {
	case kInit:
		init();
		break;
	case kStartSound:
		startSound();
		break;
	case kRun:
		run();
		break;
	case kStop:
		stop();
	}
}

bool Logo::onStateExit() {
	g_nancy->_sound->stopSound(_msnd);
	destroy();
	return true;
}

void Logo::init() {
	Common::SeekableReadStream *lg = g_nancy->getBootChunkStream("LG0");
	lg->seek(0);

	_logoImage.init(lg->readString());
	_logoImage.registerGraphics();

	_state = kStartSound;
}

void Logo::startSound() {
	_msnd.read(*g_nancy->getBootChunkStream("MSND"), SoundDescription::kMenu);
	g_nancy->_sound->loadSound(_msnd);
	g_nancy->_sound->playSound(_msnd);

	_startTicks = g_nancy->getTotalPlayTime();
	_state = kRun;
}

void Logo::run() {
	if (g_nancy->getTotalPlayTime() - _startTicks >= 7000 || (g_nancy->_input->getInput().input & NancyInput::kLeftMouseButtonDown)) {
		_state = kStop;
	}
}

void Logo::stop() {
	// The original engine checks for N+D and N+C key combos here.
	// For the N+C key combo it looks for some kind of cheat file
	// to initialize the game state with.

	g_nancy->_sound->stopSound(_msnd);

	g_nancy->setState(NancyState::kScene);
}

} // End of namespace State
} // End of namespace Nancy
