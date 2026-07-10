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

#include "DBCStores.h"
#include "Formulas.h"
#include "WorldMock.h"
#include "gtest/gtest.h"

using namespace Acore::Honor;
using namespace Acore::XP;

TEST(FormulasTest, hk_honor_at_level)
{
    EXPECT_EQ(hk_honor_at_level(80), 124);
    EXPECT_EQ(hk_honor_at_level(80, 2), 248);
    EXPECT_EQ(hk_honor_at_level(80, 0.5), 62);
    EXPECT_EQ(hk_honor_at_level(1), 2);
    EXPECT_EQ(hk_honor_at_level(1, 10), 16);
    EXPECT_EQ(hk_honor_at_level(2), 4);
    EXPECT_EQ(hk_honor_at_level(3), 5);
}

TEST(FormulasTest, GetGrayLevel)
{
    EXPECT_EQ(GetGrayLevel(0), 0);
    EXPECT_EQ(GetGrayLevel(5), 0);
    EXPECT_EQ(GetGrayLevel(6), 1);
    EXPECT_EQ(GetGrayLevel(39), 31);
    EXPECT_EQ(GetGrayLevel(40), 31);
    EXPECT_EQ(GetGrayLevel(59), 47);
    EXPECT_EQ(GetGrayLevel(60), 51);
    EXPECT_EQ(GetGrayLevel(80), 71);
}

TEST(FormulasTest, GetColorCode)
{
    EXPECT_EQ(GetColorCode(60, 80), XP_RED);
    EXPECT_EQ(GetColorCode(60, 65), XP_RED);
    EXPECT_EQ(GetColorCode(60, 64), XP_ORANGE);
    EXPECT_EQ(GetColorCode(60, 63), XP_ORANGE);
    EXPECT_EQ(GetColorCode(60, 62), XP_YELLOW);
    EXPECT_EQ(GetColorCode(60, 58), XP_YELLOW);
    EXPECT_EQ(GetColorCode(60, 57), XP_GREEN);
    EXPECT_EQ(GetColorCode(60, 52), XP_GREEN);
    EXPECT_EQ(GetColorCode(60, 51), XP_GRAY);
    EXPECT_EQ(GetColorCode(60, 1), XP_GRAY);
}

TEST(FormulasTest, GetZeroDifference)
{
    EXPECT_EQ(GetZeroDifference(1), 5);
    EXPECT_EQ(GetZeroDifference(7), 5);
    EXPECT_EQ(GetZeroDifference(8), 6);
    EXPECT_EQ(GetZeroDifference(9), 6);
    EXPECT_EQ(GetZeroDifference(10), 7);
    EXPECT_EQ(GetZeroDifference(11), 7);
    EXPECT_EQ(GetZeroDifference(12), 8);
    EXPECT_EQ(GetZeroDifference(15), 8);
    EXPECT_EQ(GetZeroDifference(16), 9);
    EXPECT_EQ(GetZeroDifference(19), 9);
    EXPECT_EQ(GetZeroDifference(20), 11);
    EXPECT_EQ(GetZeroDifference(29), 11);
    EXPECT_EQ(GetZeroDifference(30), 12);
    EXPECT_EQ(GetZeroDifference(39), 12);
    EXPECT_EQ(GetZeroDifference(40), 13);
    EXPECT_EQ(GetZeroDifference(44), 13);
    EXPECT_EQ(GetZeroDifference(45), 14);
    EXPECT_EQ(GetZeroDifference(49), 14);
    EXPECT_EQ(GetZeroDifference(50), 15);
    EXPECT_EQ(GetZeroDifference(54), 15);
    EXPECT_EQ(GetZeroDifference(55), 16);
    EXPECT_EQ(GetZeroDifference(59), 16);
    EXPECT_EQ(GetZeroDifference(60), 17);
    EXPECT_EQ(GetZeroDifference(80), 17);
}

TEST(FormulasTest, BaseGain)
{
    EXPECT_EQ(BaseGain(60, 40, CONTENT_1_60), 0);
    EXPECT_EQ(BaseGain(60, 60, CONTENT_1_60), 345);
    EXPECT_EQ(BaseGain(50, 60, CONTENT_1_60), 354);
    EXPECT_EQ(BaseGain(65, 66, CONTENT_61_70), 588);
    EXPECT_EQ(BaseGain(79, 78, CONTENT_71_80), 917);

    // check outError() has been called after passing an invalid ContentLevels content
    EXPECT_EQ(BaseGain(79, 1, ContentLevels(999)), 0);
}

TEST(FormulasTest, Gain)
{
    auto worldMock = new WorldMock();
    sWorld.reset((worldMock));
    /// @todo: create mocks of Player and Creature
    // Gain(nullptr, nullptr);
}

class ProfessionSkillUpXPTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _originalWorld = sWorld.release();
        _worldMock = new ::testing::NiceMock<WorldMock>();
        sWorld.reset(_worldMock);
    }

    void TearDown() override
    {
        IWorld* currentWorld = sWorld.release();
        delete currentWorld;
        _worldMock = nullptr;

        sWorld.reset(_originalWorld);
        _originalWorld = nullptr;
    }

    void SetRate(float rate)
    {
        ON_CALL(*_worldMock, getRate(RATE_XP_PROFESSION_SKILLUP)).WillByDefault(::testing::Return(rate));
    }

    IWorld* _originalWorld = nullptr;
    ::testing::NiceMock<WorldMock>* _worldMock = nullptr;
};

// cppcheck-suppress syntaxError
TEST_F(ProfessionSkillUpXPTest, ScalesBaseKillXPByConfiguredRate)
{
    SetRate(0.25f);

    // BaseGain(60, 60, CONTENT_1_60) = 345; 345 * 0.25 truncates to 86.
    EXPECT_EQ(ProfessionSkillUpXP(60), 86u);
}

TEST_F(ProfessionSkillUpXPTest, RateZeroDisablesTheReward)
{
    SetRate(0.0f);

    EXPECT_EQ(ProfessionSkillUpXP(1), 0u);
    EXPECT_EQ(ProfessionSkillUpXP(80), 0u);
}

TEST_F(ProfessionSkillUpXPTest, UsesContentBracketOfPlayerLevel)
{
    SetRate(1.0f);

    // With rate 1.0 the reward equals BaseGain(level, level, bracket) exactly.
    EXPECT_EQ(ProfessionSkillUpXP(1), 50u);
    EXPECT_EQ(ProfessionSkillUpXP(60), 345u);   // CONTENT_1_60
    EXPECT_EQ(ProfessionSkillUpXP(61), 540u);   // CONTENT_61_70
    EXPECT_EQ(ProfessionSkillUpXP(70), 585u);
    EXPECT_EQ(ProfessionSkillUpXP(71), 935u);   // CONTENT_71_80
    EXPECT_EQ(ProfessionSkillUpXP(80), 980u);
}
