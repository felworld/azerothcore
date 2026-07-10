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

#include "LootMgr.h"
#include "WorldMock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

namespace
{
// Quests.MultiDropQuestItems: quest-required drops become free-for-all so every
// group member on the quest loots their own copy (LootItem::LootItem).
class LootItemMultiDropTest : public Test
{
protected:
    void SetUp() override
    {
        _originalWorld = sWorld.release();
        _worldMock = new NiceMock<WorldMock>();
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

    void SetMultiDropQuestItems(bool enabled)
    {
        ON_CALL(*_worldMock, getBoolConfig(CONFIG_QUEST_ITEMS_MULTI_DROP)).WillByDefault(Return(enabled));
    }

    // The item id is unknown to the (empty) ObjectMgr item store, matching an
    // ordinary quest item with no MULTI_DROP / FOLLOW_LOOT_RULES flags.
    static LootItem MakeLootItem(bool needsQuest)
    {
        LootStoreItem storeItem(12345, 0, 100.0f, needsQuest, 1, 0, 1, 1);
        return LootItem(storeItem);
    }

    IWorld* _originalWorld = nullptr;
    NiceMock<WorldMock>* _worldMock = nullptr;
};

// cppcheck-suppress syntaxError
TEST_F(LootItemMultiDropTest, QuestItemBecomesFreeForAllWhenEnabled)
{
    SetMultiDropQuestItems(true);

    EXPECT_TRUE(MakeLootItem(/*needsQuest=*/true).freeforall);
}

TEST_F(LootItemMultiDropTest, QuestItemStaysExclusiveWhenDisabled)
{
    SetMultiDropQuestItems(false);

    EXPECT_FALSE(MakeLootItem(/*needsQuest=*/true).freeforall);
}

TEST_F(LootItemMultiDropTest, NonQuestItemIsUnaffected)
{
    SetMultiDropQuestItems(true);

    EXPECT_FALSE(MakeLootItem(/*needsQuest=*/false).freeforall);
}
}
