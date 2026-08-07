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

#include "FelworldEvents.h"
#include "DatabaseEnv.h"
#include "World.h"

void Felworld::LogEvent(ObjectGuid playerGuid, std::string_view eventType, std::string_view detailsJson)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_FELWORLD_EVENT);
    stmt->SetData(0, playerGuid.GetCounter());
    stmt->SetData(1, eventType);
    stmt->SetData(2, detailsJson);
    CharacterDatabase.Execute(stmt);
}

void Felworld::PurgeOldEvents()
{
    uint32 retentionDays = sWorld->getIntConfig(CONFIG_FELWORLD_EVENTS_RETENTION_DAYS);
    if (!retentionDays)
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_FELWORLD_EVENTS_OLD);
    stmt->SetData(0, retentionDays);
    CharacterDatabase.Execute(stmt);
}
