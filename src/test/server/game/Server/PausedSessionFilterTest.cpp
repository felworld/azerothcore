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

#include "PausedSessionFilter.h"
#include "gtest/gtest.h"

// While gameplay is paused, Map::Update never runs, so the session update on
// the world thread must discard map-bound packets — otherwise the first one
// wedges the ordered receive queue and the chat/GM-command packets behind it
// (including ".pause off") are never processed.

// cppcheck-suppress syntaxError
TEST(PausedSessionFilterTest, DiscardsMapBoundPacketsOfInWorldPlayers)
{
    EXPECT_TRUE(PausedSessionFilter::ShouldDiscard(PROCESS_THREADSAFE, true));
}

TEST(PausedSessionFilterTest, KeepsWorldThreadPackets)
{
    // Chat and GM commands are PROCESS_THREADUNSAFE; they must keep flowing
    // while paused.
    EXPECT_FALSE(PausedSessionFilter::ShouldDiscard(PROCESS_THREADUNSAFE, true));
}

TEST(PausedSessionFilterTest, KeepsInPlacePackets)
{
    EXPECT_FALSE(PausedSessionFilter::ShouldDiscard(PROCESS_INPLACE, true));
}

TEST(PausedSessionFilterTest, KeepsMapBoundPacketsWhileNotInWorld)
{
    // WorldSessionFilter processes even map-bound packets for players not in
    // world (login/loading flows); pausing must not change that.
    EXPECT_FALSE(PausedSessionFilter::ShouldDiscard(PROCESS_THREADSAFE, false));
}

TEST(PausedSessionFilterTest, AcceptsEveryPacketForPopping)
{
    // Process() must accept everything: the queue only pops the front packet
    // if the filter accepts it, and the whole point is to never leave a
    // map-bound packet stuck at the head.
    PausedSessionFilter filter(nullptr);
    EXPECT_TRUE(filter.Process(nullptr));
}

TEST(PausedSessionFilterTest, OtherFiltersNeverDiscard)
{
    // The Discard() hook must stay a no-op for the normal filters, or packets
    // would be silently dropped during regular (unpaused) updates.
    MapSessionFilter mapFilter(nullptr);
    WorldSessionFilter worldFilter(nullptr);
    EXPECT_FALSE(mapFilter.Discard(nullptr));
    EXPECT_FALSE(worldFilter.Discard(nullptr));
}
