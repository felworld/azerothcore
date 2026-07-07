# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)
[![CodeFactor](https://www.codefactor.io/repository/github/azerothcore/azerothcore-wotlk/badge)](https://www.codefactor.io/repository/github/azerothcore/azerothcore-wotlk)
[![StackOverflow](http://img.shields.io/badge/stackoverflow-azerothcore-blue.svg?logo=stackoverflow)](https://stackoverflow.com/questions/tagged/azerothcore?sort=newest "Ask / browse questions here")
[![Discord](https://img.shields.io/discord/217589275766685707?logo=discord&logoColor=white)](https://discord.gg/gkt4y2x "Our community hub on Discord")
[![Bounties on BountyHub](https://img.shields.io/badge/Bounties-on%20BountyHub-yellow)](https://www.bountyhub.dev/bounties?repo=azerothcore)

## Build Status

[![nopch-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/core-build-nopch.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/core-build-nopch.yml?query=branch%3Amaster)
[![pch-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/core-build-pch.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/core-build-pch.yml?query=branch%3Amaster)
[![core-modules-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/core_modules_build.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/core_modules_build.yml?query=branch%3Amaster)
[![windows-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/windows_build.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/windows_build.yml?query=branch%3Amaster)
[![macos-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/macos_build.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/macos_build.yml?query=branch%3Amaster)
[![docker-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/docker_build.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/docker_build.yml?query=branch%3Amaster)
[![tools-build](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/tools_build.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/tools_build.yml?query=branch%3Amaster)
[![dashboard-ci](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/dashboard-ci.yml/badge.svg?branch=master)](https://github.com/azerothcore/azerothcore-wotlk/actions/workflows/dashboard-ci.yml?query=branch%3Amaster)

## Introduction

AzerothCore is an open-source game server application and framework designed for hosting massively multiplayer online role-playing games (MMORPGs). It is based on the popular MMORPG World of Warcraft (WoW) and seeks to recreate the gameplay experience of the original game from patch 3.3.5a.

The original code is based on MaNGOS, TrinityCore, and SunwellCore and has since then had extensive development to improve stability, in-game mechanics, and modularity to the game. AC has also grown into a community-driven project with a significant number of contributors and developers. It is written in C++ and provides a solid foundation for creating private servers that mimic the mechanics and behavior of the official WoW servers.

## Philosophy

Our main goal is to create a playable game server, offering a fully working in-game experience.

Here are the main points we focus on:

* Stability
  * We make sure all changes pass the CIs before being merged into the master branch.

* Blizzlike content
  * We strive to make all in-game content to be blizzlike. Therefore we have a high standard for fixes being made.

* Customization
  * It is easy to customize your experience using [modules](#modules).

* Community driven
  * AzerothCore has an active community of developers, contributors, and users who collaborate, share knowledge, and provide support through forums, Discord channels, and other communication platforms. 

### Modules

AzerothCore is designed to be highly modular, allowing developers to extend and customize the game to suit their preferences or create unique gameplay experiences. This flexibility enables the addition of custom features, content, and modifications.

We have a lot of modules already made by the community, many of which can be found in the [Module Catalogue](https://www.azerothcore.org/catalogue.html#/).

## Installation

Detailed installation instructions are available [here](http://www.azerothcore.org/wiki/installation).

## Running: session modes

This server wires up playerbots (`mod-playerbots`) and LLM-driven bot chat
(`mod-ollama-chat`) behind three **session modes**, chosen at startup with a
per-mode env file. Bring the stack up with `podman compose` (Docker Compose
under the hood, on Podman) using one of:

| Mode          | Command                                        | Playerbots | LLM chat (Ollama) |
| ------------- | ---------------------------------------------- | :--------: | :---------------: |
| Solo          | `podman compose --env-file .env.solo up -d`     |    off     |        off        |
| Dumb bots     | `podman compose --env-file .env.dumbbots up -d` |     on     |        off        |
| Ollama + bots | `podman compose --env-file .env.ollama up -d`   |     on     |        on         |

Switching modes is just a different `--env-file` + `up -d`. It only recreates
`ac-worldserver` (the database and authserver keep running), and needs no
rebuild or `.conf` edit: each env file sets `AC_AI_PLAYERBOT_ENABLED` /
`AC_OLLAMA_CHAT_ENABLE`, which the server reads in preference to the
`AiPlayerbot.Enabled` / `OllamaChat.Enable` module options.

Notes:

- Only `.env.ollama` starts the GPU-backed `ac-ollama` container (gated by
  `COMPOSE_PROFILES=ollama`); the other modes never start it, so it uses no VRAM.
- A bare `podman compose up -d` (no `--env-file`) defaults to bots-on /
  Ollama-off.
- Switching *away* from Ollama does not stop an already-running `ac-ollama` —
  stop it explicitly with `podman compose --profile ollama stop ac-ollama` to
  free VRAM.

## Contributing

AzerothCore can also serve as a learning resource for aspiring developers who want to understand how WoW servers work, how MMORPGs are structured, how game server emulators are created, or to improve their C++ and SQL knowledge.

If you want to contribute to the project, you will find a lot of resources that will guide you in our [wiki](https://www.azerothcore.org/wiki/contribute).

We also recommend you read our [Contributor Covenant Code of Conduct](https://github.com/azerothcore/azerothcore-wotlk/blob/master/.github/CODE_OF_CONDUCT.md).

Feel free to join our [Discord server](https://discord.gg/gkt4y2x).

Click on the "⭐ Star" button to help us gain more visibility on Github!

## Authors & Contributors

The project was born in 2016 based on SunwellCore. Unfortunately, SunwellCore was published without any git history, so on git there are no credits for all the contributors before 2016.

You can check the [authors](https://github.com/azerothcore/azerothcore-wotlk/blob/master/AUTHORS) file for more details.

## Important Links

- [Doxygen documentation](https://www.azerothcore.org/pages/doxygen/index.html)
- [Website](http://www.azerothcore.org/)
- [AzerothCore catalogue](http://www.azerothcore.org/catalogue.html  "Modules, tools, and other stuff for AzerothCore") (modules, tools, etc...)
- [Our Discord server](https://discord.gg/gkt4y2x)
- [Our wiki](http://www.azerothcore.org/wiki "Easy to use and developed by AzerothCore founder")
- [Our forum](https://github.com/azerothcore/azerothcore-wotlk/discussions/)
- [Our Facebook page](https://www.facebook.com/AzerothCore/)
- [Our LinkedIn page](https://www.linkedin.com/company/azerothcore/)

## License

- The new AzerothCore source components are released under the [GNU AGPL v3](https://www.gnu.org/licenses/agpl-3.0.en.html)
- The old sources based on MaNGOS/TrinityCore are released under the [GNU GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

It's important to note that AzerothCore is not an official Blizzard Entertainment product, and it is not affiliated with or endorsed by World of Warcraft or Blizzard Entertainment. AzerothCore does not in any case sponsor nor support illegal public servers. If you use this project to run an illegal public server and not for testing and learning it is your own personal choice.

## Special thanks

[JetBrains](https://www.jetbrains.com/?from=AzerothCore) is providing free [open-source licenses](https://www.jetbrains.com/community/opensource/) to the AzerothCore developers.

[![JetBrains logo.](https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg)](https://jb.gg/OpenSourceSupport)
