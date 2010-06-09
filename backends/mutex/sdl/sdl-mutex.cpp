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
 * $URL$
 * $Id$
 *
 */

#if defined(WIN32) || defined(UNIX) || defined(MACOSX)

#include "backends/mutex/sdl/sdl-mutex.h"

#if defined(__SYMBIAN32__)
#include <esdl\SDL.h>
#else
#include <SDL.h>
#endif

OSystem::MutexRef SdlMutexManager::createMutex() {
	return (OSystem::MutexRef) SDL_CreateMutex();
}

void SdlMutexManager::lockMutex(OSystem::MutexRef mutex) {
	SDL_mutexP((SDL_mutex *) mutex);
}

void SdlMutexManager::unlockMutex(OSystem::MutexRef mutex) {
	SDL_mutexV((SDL_mutex *) mutex);
}

void SdlMutexManager::deleteMutex(OSystem::MutexRef mutex) {
	SDL_DestroyMutex((SDL_mutex *) mutex);
}

#endif
