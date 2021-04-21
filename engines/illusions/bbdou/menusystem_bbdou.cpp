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

#include "illusions/illusions.h"
#include "illusions/actor.h"
#include "illusions/cursor.h"
#include "illusions/sound.h"
#include "illusions/bbdou/illusions_bbdou.h"
#include "illusions/bbdou/menusystem_bbdou.h"

namespace Illusions {

// BBDOUMenuSystem

BBDOUMenuSystem::BBDOUMenuSystem(IllusionsEngine_BBDOU *vm)
	: BaseMenuSystem(vm), _vm(vm) {
	clearMenus();
}

BBDOUMenuSystem::~BBDOUMenuSystem() {
	freeMenus();
}

void BBDOUMenuSystem::runMenu(MenuChoiceOffsets menuChoiceOffsets, int16 *menuChoiceOffset,
	uint32 menuId, uint32 duration, uint timeOutMenuChoiceIndex, uint32 menuCallerThreadId) {
	
	debug(0, "BBDOUMenuSystem::runMenu(%08X)", menuId);

	setTimeOutDuration(duration, timeOutMenuChoiceIndex);
	setMenuCallerThreadId(menuCallerThreadId);
	setMenuChoiceOffsets(menuChoiceOffsets, menuChoiceOffset);

	int rootMenuId = convertRootMenuId(menuId);
	BaseMenu *rootMenu = getMenuById(rootMenuId);
	openMenu(rootMenu);

}

void BBDOUMenuSystem::clearMenus() {
	for (int i = 0; i < kBBDOULastMenuIndex; ++i) {
		_menus[i] = 0;
	}
}

void BBDOUMenuSystem::freeMenus() {
	for (int i = 0; i < kBBDOULastMenuIndex; ++i) {
		delete _menus[i];
	}
}

BaseMenu *BBDOUMenuSystem::getMenuById(int menuId) {
	if (!_menus[menuId])
		_menus[menuId] = createMenuById(menuId);
	return _menus[menuId];
}

BaseMenu *BBDOUMenuSystem::createMenuById(int menuId) {
	switch (menuId) {
	case kBBDOUMainMenu:
		return createMainMenu();
	case kBBDOUPauseMenu:
		return createPauseMenu();
	case kBBDOUOptionsMenu:
		return createOptionsMenu();
	case kBBDOUQueryQuitMenu:
		return createQueryQuitMenu();
	case kBBDOUQueryRestartMenu:
		return createQueryRestartMenu();
	case kBBDOULoadGameMenu:
		return createLoadGameMenu();
	case kBBDOUSaveGameMenu:
		return createSaveCompleteMenu(); //createSaveGameMenu();
	// TODO Other menus
	default:
		error("BBDOUMenuSystem::createMenuById() Invalid menu id %d", menuId);
	}
}

BaseMenu *BBDOUMenuSystem::createMainMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	menu->addMenuItem(new MenuItem("Start the Game", new MenuActionReturnChoice(this, 1))); //putting 6 here causes opcode 33 to be called...
	menu->addMenuItem(new MenuItem("Load Game", new MenuActionLoadGame(this, 2)));
	menu->addMenuItem(new MenuItem("Options", new MenuActionEnterMenu(this, kBBDOUOptionsMenu)));
	menu->addMenuItem(new MenuItem("Quit Game", new MenuActionEnterQueryMenu(this, kBBDOUQueryQuitMenu, 3)));
	return menu;
}

//this call isn't made in duckman, why is it needed here?
BaseMenu *BBDOUMenuSystem::createLoadGameMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	menu->addText("Loading OK");
	menu->addText("-------------");
	menu->addMenuItem(new MenuItem("Continue", new MenuActionReturnChoice(this, 1)));
	return menu;
}

BaseMenu *BBDOUMenuSystem::createLoadFailedMenu() {
	return 0; // TODO
}

BaseMenu *BBDOUMenuSystem::createOptionsMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	/*BBDOUMenuActionUpdateSlider *sfxSlider;
	BBDOUMenuActionUpdateSlider *musicSlider;
	BBDOUMenuActionUpdateSlider *speechSlider;
	BBDOUMenuActionUpdateSlider *textDurationSlider;*/

	menu->addText("              GAME OPTIONS             @@@@");
	menu->addText("--------------------------------------");

	//menu->addMenuItem(createOptionsSliderMenuItem(&sfxSlider, "Sound Volume   @@", SFX, menu));
	//NOT USED: menu->addMenuItem(createOptionsSliderMenuItem(&musicSlider, "Music Volume  @@@", MUSIC, menu));
	//menu->addMenuItem(createOptionsSliderMenuItem(&speechSlider, "Speech Volume ", VOICE, menu));
	//menu->addMenuItem(createOptionsSliderMenuItem(&textDurationSlider, "Text Duration @@@", TEXT_DURATION, menu));
	//menu->addMenuItem(new MenuItem("Restore Defaults", new MenuActionResetOptionSliders(this, sfxSlider, musicSlider, speechSlider, textDurationSlider)));
	menu->addMenuItem(new MenuItem("Back", new MenuActionLeaveMenu(this)));

	return menu;
}

BaseMenu *BBDOUMenuSystem::createPauseMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	menu->addText("   Game Paused");
	menu->addText("-------------------");
	menu->addMenuItem(new MenuItem("Resume", new MenuActionReturnChoice(this, 1)));
	menu->addMenuItem(new MenuItem("Load Game", new MenuActionLoadGame(this, 2)));
	menu->addMenuItem(new MenuItem("Save Game", new MenuActionSaveGame(this, 3)));//2 works but gets to load //11 orig
	menu->addMenuItem(new MenuItem("Options", new MenuActionEnterMenu(this, kBBDOUOptionsMenu)));
	menu->addMenuItem(new MenuItem("Restart Game", new MenuActionEnterQueryMenu(this, kBBDOUQueryRestartMenu, 4)));
	menu->addMenuItem(new MenuItem("Quit Game", new MenuActionEnterQueryMenu(this, kBBDOUQueryQuitMenu, 5)));
	return menu;
}

BaseMenu *BBDOUMenuSystem::createQueryRestartMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	menu->addText("Do you really want to restart?");
	menu->addText("-----------------------------------");
	menu->addMenuItem(new MenuItem("Yes, let's try again", new MenuActionReturnChoice(this, getQueryConfirmationChoiceIndex())));
	menu->addMenuItem(new MenuItem("No, just kidding", new MenuActionLeaveMenu(this)));
	return menu;
}

BaseMenu *BBDOUMenuSystem::createQueryQuitMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	
	menu->addText("Do you really want to quit?");
	menu->addText("-------------------------------");
	menu->addMenuItem(new MenuItem("Yes, I'm outta here", new MenuActionReturnChoice(this, getQueryConfirmationChoiceIndex())));
	menu->addMenuItem(new MenuItem("No, just kidding", new MenuActionLeaveMenu(this)));
	
	return menu;
}

BaseMenu *BBDOUMenuSystem::createSaveGameMenu() {
	return 0; // TODO
}

BaseMenu *BBDOUMenuSystem::createGameSavedMenu() {
	return 0; // TODO
}

BaseMenu *BBDOUMenuSystem::createSaveCompleteMenu() {
	BaseMenu *menu = new BaseMenu(this, 0x00120003, 218, 150, 80, 20, 1);
	menu->addText("Game Saved");
	menu->addText("-------------");
	menu->addMenuItem(new MenuItem("Continue", new MenuActionReturnChoice(this, 1)));
	return menu;
}

BaseMenu *BBDOUMenuSystem::createSaveFailedMenu() {
	return 0; // TODO
}

int BBDOUMenuSystem::convertRootMenuId(uint32 menuId) {
	switch (menuId) {
	case 0x1C0001:
		return kBBDOUMainMenu;
	case 0x1C0002:
		return kBBDOUPauseMenu;
	case 0x1C0006:
		return kBBDOULoadGameMenu;
	case 0x1C0007:
		return kBBDOUSaveGameMenu;
	case 0x1C0008:
		return kBBDOUGameSavedMenu;
	case 0x1C0009:
		return kBBDOUSaveFailedMenu;
	case 0x1C000A:
		return kBBDOULoadFailedMenu;
	/* Unused/unimplemented debug menus
	case 0x1C0003: debugStartMenu
	case 0x1C0004: debugPauseMenu
	case 0x1C0005: unitTestsMenu
	*/
	default:
		error("BBDOUMenuSystem() Menu ID %08X not found", menuId);
	}
}

bool BBDOUMenuSystem::initMenuCursor() {
	bool cursorInitialVisibleFlag = false;
	Control *cursorControl = _vm->getObjectControl(0x40004);
	if (cursorControl) {
		if (cursorControl->_flags & 1) {
			cursorInitialVisibleFlag = false;
		} else {
			cursorInitialVisibleFlag = true;
			cursorControl->appearActor();
		}
	} else {
		Common::Point pos = _vm->getNamedPointPosition(0x70023);
		_vm->_controls->placeActor(0x50001, pos, 0x60001, 0x40004, 0);
		cursorControl = _vm->getObjectControl(0x40004);
	}
	return cursorInitialVisibleFlag;
}

int BBDOUMenuSystem::getGameState() {
	return _vm->_cursor->_status;
}

void BBDOUMenuSystem::setMenuCursorNum(int cursorNum) {
	Control *mouseCursor = _vm->getObjectControl(0x40004);
	_vm->_cursor->setActorIndex(5, cursorNum, 0);
	mouseCursor->startSequenceActor(0x60001, 2, 0);
}

void BBDOUMenuSystem::setGameState(int gameState) {
	_vm->_cursor->_status = gameState;
}

void BBDOUMenuSystem::playSoundEffect(int sfxId) {
	// TODO
}

} // End of namespace Illusions
