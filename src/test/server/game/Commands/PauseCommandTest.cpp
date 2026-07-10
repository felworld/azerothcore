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
#include "Optional.h"
#include "WorldMock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

using namespace testing;

// Defined in src/server/scripts/Commands/cs_pause.cpp, which is compiled into
// the test binary (see src/test/CMakeLists.txt). A signature mismatch here
// fails at link time.
bool HandleGameplayPauseCommand(ChatHandler* handler, ::Optional<bool> enableArg);

namespace
{
void CollectPrintedLine(void* arg, std::string_view text)
{
    static_cast<std::vector<std::string>*>(arg)->emplace_back(text);
}

// .pause [on|off] semantics: no argument toggles, an argument sets the state,
// and requesting the current state is a no-op that reports it.
class PauseCommandTest : public Test
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

    void SetPaused(bool paused)
    {
        ON_CALL(*_worldMock, IsGameplayPaused()).WillByDefault(Return(paused));
    }

    bool Invoke(::Optional<bool> enableArg)
    {
        CliHandler handler(&_output, &CollectPrintedLine);
        return HandleGameplayPauseCommand(&handler, enableArg);
    }

    std::string Output() const
    {
        std::string joined;
        for (std::string const& line : _output)
            joined += line;
        return joined;
    }

    IWorld* _originalWorld = nullptr;
    NiceMock<WorldMock>* _worldMock = nullptr;
    std::vector<std::string> _output;
};

// cppcheck-suppress syntaxError
TEST_F(PauseCommandTest, ToggleWithoutArgumentPausesARunningWorld)
{
    SetPaused(false);
    EXPECT_CALL(*_worldMock, SetGameplayPaused(true)).Times(1);

    EXPECT_TRUE(Invoke(std::nullopt));
}

TEST_F(PauseCommandTest, ToggleWithoutArgumentResumesAPausedWorld)
{
    SetPaused(true);
    EXPECT_CALL(*_worldMock, SetGameplayPaused(false)).Times(1);

    EXPECT_TRUE(Invoke(std::nullopt));
}

TEST_F(PauseCommandTest, ExplicitOnPausesARunningWorld)
{
    SetPaused(false);
    EXPECT_CALL(*_worldMock, SetGameplayPaused(true)).Times(1);

    EXPECT_TRUE(Invoke(true));
}

TEST_F(PauseCommandTest, ExplicitOnIsANoOpWhenAlreadyPaused)
{
    SetPaused(true);
    EXPECT_CALL(*_worldMock, SetGameplayPaused(_)).Times(0);

    EXPECT_TRUE(Invoke(true));
    EXPECT_NE(Output().find("already paused"), std::string::npos);
}

TEST_F(PauseCommandTest, ExplicitOffIsANoOpWhenAlreadyRunning)
{
    SetPaused(false);
    EXPECT_CALL(*_worldMock, SetGameplayPaused(_)).Times(0);

    EXPECT_TRUE(Invoke(false));
    EXPECT_NE(Output().find("already running"), std::string::npos);
}
}
