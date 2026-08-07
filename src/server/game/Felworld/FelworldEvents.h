/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FELWORLD_EVENTS_H
#define _FELWORLD_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"

#include <string_view>

// Per-character event history (acore_characters.felworld_events), surfaced in
// the observability dashboards. Callable from core and modules.
namespace Felworld
{
    // Fire-and-forget async insert. detailsJson is free-form (JSON by
    // convention) and may be empty.
    AC_GAME_API void LogEvent(ObjectGuid playerGuid, std::string_view eventType, std::string_view detailsJson);

    // Deletes events older than Felworld.Events.RetentionDays (0 = keep all).
    AC_GAME_API void PurgeOldEvents();
}

#endif
