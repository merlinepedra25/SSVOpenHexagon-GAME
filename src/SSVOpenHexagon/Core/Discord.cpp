// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0
#include "SSVOpenHexagon/Core/Discord.hpp"

#include <SSVUtils/Core/Log/Log.hpp>

#include <math.h>
#include <chrono>

#include "discord/discord.h"

namespace hg::Discord
{

[[nodiscard]] static bool initialize_discord(discord::Core** core)
{
    const auto result = discord::Core::Create(
        725763266110029964, DiscordCreateFlags_NoRequireDiscord, core);

    if(result != discord::Result::Ok)
    {
        ssvu::lo("Discord") << "Failed to initialize core\n";
        return false;
    }

    ssvu::lo("Discord") << "Successfully initialized core\n";
    return true;
}

discord_manager::discord_manager() : _initialized{initialize_discord(&_core)}
{
    if(!_initialized)
    {
        return;
    }

    _core->SetLogHook(discord::LogLevel::Debug,
        [](discord::LogLevel level, const char* message) {
            ssvu::lo("Discord")
                << static_cast<uint32_t>(level) << ": " << message << '\n';
        });

    if(_core->ActivityManager().RegisterCommand("SSVOpenHexagon.exe") !=
        discord::Result::Ok)
    {
        ssvu::lo("Discord") << "Failed to register command\n";
    }
    else
    {
        ssvu::lo("Discord") << "Successfully registered command\n";
    }

    if(_core->ActivityManager().RegisterSteam(1358090) != discord::Result::Ok)
    {
        ssvu::lo("Discord") << "Failed to register Steam app\n";
    }
    else
    {
        ssvu::lo("Discord") << "Successfully registered Steam app\n";
    }
}

discord_manager::~discord_manager()
{
}

bool discord_manager::run_callbacks()
{
    if(!_initialized)
    {
        return false;
    }

    if(_core->RunCallbacks() != discord::Result::Ok)
    {
        ssvu::lo("Discord") << "Failed to run callbacks\n";
        return false;
    }

    return true;
}

bool discord_manager::set_rich_presence_in_menu()
{
    if(!_initialized)
    {
        return false;
    }

    discord::Activity activity{};
    activity.SetState("Selecting Level");
    activity.SetDetails("");
    discord::ActivityTimestamps& currentTimestamp = activity.GetTimestamps();
    currentTimestamp.SetStart(static_cast<int64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count() /
        1000000000));
    _core->ActivityManager().UpdateActivity(activity, [](discord::Result r) {
        if(r != discord::Result::Ok)
        {
            ssvu::lo("Discord") << "Fail\n";
        }
    });

    return true;
}

bool discord_manager::set_rich_presence_on_replay()
{
    if(!_initialized)
    {
        return false;
    }

    discord::Activity activity{};
    activity.SetState("Watching Replay");
    activity.SetDetails("");
    discord::ActivityTimestamps& currentTimestamp = activity.GetTimestamps();
    currentTimestamp.SetStart(0);
    _core->ActivityManager().UpdateActivity(activity, [](discord::Result r) {
        if(r != discord::Result::Ok)
        {
            ssvu::lo("Discord") << "Fail\n";
        }
    });

    return true;
}

bool discord_manager::set_rich_presence_in_game(
    const std::string& level_info, const std::string& second_info, bool dead)
{
    if(!_initialized)
    {
        return false;
    }

    discord::Activity activity{};
    activity.SetDetails(("Playing " + level_info).data());
    activity.SetState(second_info.data());
    discord::ActivityTimestamps& currentTimestamp = activity.GetTimestamps();
    if(!dead)
    {
        // ssvu::lo("Debug") << "The current time: " <<
        // static_cast<int64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()
        // / 1000000000) << "\n";
        currentTimestamp.SetStart(
            static_cast<int64_t>(std::chrono::high_resolution_clock::now()
                                     .time_since_epoch()
                                     .count() /
                                 1000000000));
    }
    else
    {
        currentTimestamp.SetStart(0);
    }
    _core->ActivityManager().UpdateActivity(activity, [](discord::Result r) {
        if(r != discord::Result::Ok)
        {
            ssvu::lo("Discord") << "Fail\n";
        }
    });

    return true;
}

} // namespace hg::Discord
