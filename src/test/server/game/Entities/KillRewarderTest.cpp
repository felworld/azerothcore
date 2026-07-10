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

#include "KillRewarder.h"
#include "gtest/gtest.h"

// XP.Kill.GroupMode semantics: whether each group member is rewarded full,
// undivided kill XP instead of the default split-among-members behaviour.

// cppcheck-suppress syntaxError
TEST(KillRewarderTest, GroupModeZeroAlwaysDividesXP)
{
    for (bool isBattleGround : { false, true })
        for (bool isFullXP : { false, true })
            EXPECT_FALSE(KillRewarder::IsUndividedGroupXP(0, isBattleGround, isFullXP));
}

TEST(KillRewarderTest, GroupModeOneRewardsUndividedXPOutsideBattlegrounds)
{
    EXPECT_TRUE(KillRewarder::IsUndividedGroupXP(1, false, true));
    // Mode 1 applies even when some members are gray to the victim.
    EXPECT_TRUE(KillRewarder::IsUndividedGroupXP(1, false, false));
}

TEST(KillRewarderTest, GroupModeTwoRequiresFullXP)
{
    EXPECT_TRUE(KillRewarder::IsUndividedGroupXP(2, false, true));
    // Victim gray to an alive member in range: fall back to divided XP,
    // closing the power-leveling loophole.
    EXPECT_FALSE(KillRewarder::IsUndividedGroupXP(2, false, false));
}

TEST(KillRewarderTest, BattlegroundsAlwaysDivideXP)
{
    for (bool isFullXP : { false, true })
    {
        EXPECT_FALSE(KillRewarder::IsUndividedGroupXP(1, true, isFullXP));
        EXPECT_FALSE(KillRewarder::IsUndividedGroupXP(2, true, isFullXP));
    }
}
