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

#include "Chat.h"
#include "CommandScript.h"
#include "Log.h"
#include "RBAC.h"
#include "World.h"

using namespace Acore::ChatCommands;

// A free function (not class-local) so the unit tests can call it directly:
// src/test compiles this file into the test binary (see src/test/CMakeLists.txt).
bool HandleGameplayPauseCommand(ChatHandler* handler, Optional<bool> enableArg)
{
    bool const pause = enableArg ? *enableArg : !sWorld->IsGameplayPaused();

    if (pause == sWorld->IsGameplayPaused())
    {
        handler->PSendSysMessage("Gameplay is already {}.", pause ? "paused" : "running");
        return true;
    }

    sWorld->SetGameplayPaused(pause);

    if (pause)
        handler->SendGlobalSysMessage("Gameplay has been paused by a Game Master. Chat remains available.");
    else
        handler->SendGlobalSysMessage("Gameplay has been resumed.");

    LOG_INFO("server.worldserver", "Gameplay {} via .pause command.", pause ? "paused" : "resumed");
    return true;
}

class pause_commandscript : public CommandScript
{
public:
    pause_commandscript() : CommandScript("pause_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "pause", HandleGameplayPauseCommand, rbac::RBAC_PERM_COMMAND_PAUSE, Console::Yes }
        };
        return commandTable;
    }
};

void AddSC_pause_commandscript()
{
    new pause_commandscript();
}
