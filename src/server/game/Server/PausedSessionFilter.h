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

#ifndef __PAUSEDSESSIONFILTER_H
#define __PAUSEDSESSIONFILTER_H

#include "Opcodes.h"
#include "WorldSession.h"

//used by World::UpdateSessions() instead of WorldSessionFilter while gameplay
//is paused (GM .pause): Map::Update() is skipped then, so nothing drains
//map-bound (PROCESS_THREADSAFE) packets and the ordered receive queue would
//wedge on the first one, blocking the chat and GM command packets behind it
//(including the .pause off that ends the pause). This filter pops every
//packet and discards the map-bound ones: gameplay input (movement, casts,
//loot, ...) is dropped, which is what "paused" means, while world-thread
//packets keep flowing. Dropping movement is safe — every 3.3.5 movement
//packet carries the full MovementInfo, so the first one after resume
//re-establishes the client's position.
class PausedSessionFilter : public PacketFilter
{
public:
    explicit PausedSessionFilter(WorldSession* pSession) : PacketFilter(pSession) {}
    ~PausedSessionFilter() override = default;

    bool Process(WorldPacket* /*packet*/) override { return true; }
    [[nodiscard]] bool Discard(WorldPacket* packet) const override;

    //the decision itself, split out for unit testing. Packets for players not
    //(yet) in world are kept: WorldSessionFilter would process those even for
    //map-bound opcodes, and pausing must not break login flows.
    [[nodiscard]] static bool ShouldDiscard(PacketProcessing processingPlace, bool playerIsInWorld)
    {
        return processingPlace == PROCESS_THREADSAFE && playerIsInWorld;
    }
};

#endif
