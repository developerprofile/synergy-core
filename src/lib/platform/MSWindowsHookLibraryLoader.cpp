/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/MSWindowsHookLibraryLoader.h"

#include "synergy/XScreen.h"
#include "base/Log.h"

CMSWindowsHookLibraryLoader::CMSWindowsHookLibraryLoader() :
	m_init(NULL),
	m_cleanup(NULL),
	m_setSides(NULL),
	m_setZone(NULL),
	m_setMode(NULL),
	m_getDraggingFilename(NULL),
	m_clearDraggingFilename(NULL)
{
}

CMSWindowsHookLibraryLoader::~CMSWindowsHookLibraryLoader()
{
	// TODO: take ownership of m_ and delete them.
}

HINSTANCE
CMSWindowsHookLibraryLoader::openHookLibrary(const char* name)
{
	// load the hook library
	HINSTANCE hookLibrary = LoadLibrary(name);
	if (hookLibrary == NULL) {
		LOG((CLOG_ERR "failed to load hook library, %s.dll is missing or invalid", name));
		throw XScreenOpenFailure();
	}

	// look up functions
	m_setSides  = (SetSidesFunc)GetProcAddress(hookLibrary, "setSides");
	m_setZone   = (SetZoneFunc)GetProcAddress(hookLibrary, "setZone");
	m_setMode   = (SetModeFunc)GetProcAddress(hookLibrary, "setMode");
	m_init      = (InitFunc)GetProcAddress(hookLibrary, "init");
	m_cleanup   = (CleanupFunc)GetProcAddress(hookLibrary, "cleanup");

	if (m_setSides == NULL ||
		m_setZone == NULL ||
		m_setMode == NULL ||
		m_init == NULL ||
		m_cleanup == NULL) {
		LOG((CLOG_ERR "failed to load hook function, %s.dll could be out of date", name));
		throw XScreenOpenFailure();
	}

	// initialize hook library
	if (m_init(GetCurrentThreadId()) == 0) {
		LOG((CLOG_ERR "failed to init %s.dll, another program may be using it", name));
		LOG((CLOG_INFO "restarting your computer may solve this error"));
		throw XScreenOpenFailure();
	}

	return hookLibrary;
}

HINSTANCE
CMSWindowsHookLibraryLoader::openShellLibrary(const char* name)
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    if (osvi.dwMajorVersion < 6) {
		LOG((CLOG_INFO "skipping shell extension library load, %s.dll not supported before vista", name));
        return NULL;
    }

	// load the hook library
	HINSTANCE shellLibrary = LoadLibrary(name);
	if (shellLibrary == NULL) {
		LOG((CLOG_ERR "failed to load shell extension library, %s.dll is missing or invalid", name));
		throw XScreenOpenFailure();
	}

	// look up functions
	m_getDraggingFilename = (GetDraggingFilenameFunc)GetProcAddress(shellLibrary, "getDraggingFilename");
	m_clearDraggingFilename = (ClearDraggingFilenameFunc)GetProcAddress(shellLibrary, "clearDraggingFilename");

	if (m_getDraggingFilename == NULL ||
		m_clearDraggingFilename == NULL) {
		LOG((CLOG_ERR "failed to load shell extension function, %s.dll could be out of date", name));
		throw XScreenOpenFailure();
	}

	return shellLibrary;
}