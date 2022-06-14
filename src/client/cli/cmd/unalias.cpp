/*
 * Copyright (C) 2021-2022 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "unalias.h"
#include "common_cli.h"

#include <multipass/cli/argparser.h>
#include <multipass/platform.h>

namespace mp = multipass;
namespace cmd = multipass::cmd;

mp::ReturnCode cmd::Unalias::run(mp::ArgParser* parser)
{
    auto ret = parse_args(parser);
    if (ret != ParseCode::Ok)
    {
        return parser->returnCodeFrom(ret);
    }

    if (parser->isSet(all_option_name))
    {
        aliases.clear();

        QDir alias_dir = MP_PLATFORM.get_alias_scripts_folder();
        alias_dir.removeRecursively();
    }
    else
    {
        std::vector<std::string> bad_aliases;

        for (const auto& alias_to_remove : aliases_to_remove)
            if (!aliases.exists_alias(alias_to_remove))
                bad_aliases.push_back(alias_to_remove);

        if (!bad_aliases.empty())
        {
            auto i = bad_aliases.begin();
            if (bad_aliases.size() == 1)
                cerr << fmt::format("Unexisting alias: {}.\n", *i);
            else
            {
                cerr << fmt::format("Unexisting aliases: {}", *i);
                for (++i; i != bad_aliases.end(); ++i)
                    cerr << fmt::format(", {}", *i);
                cerr << ".\n";
            }

            return ReturnCode::CommandLineError;
        }

        for (const auto& alias_to_remove : aliases_to_remove)
        {
            aliases.remove_alias(alias_to_remove); // We know removal won't fail because the alias exists.
            try
            {
                MP_PLATFORM.remove_alias_script(alias_to_remove);
            }
            catch (std::runtime_error& e)
            {
                cerr << fmt::format("Warning: '{}' when removing alias script for {}\n", e.what(), alias_to_remove);
            }
        }
    }

    return ReturnCode::Ok;
}

std::string cmd::Unalias::name() const
{
    return "unalias";
}

QString cmd::Unalias::short_help() const
{
    return QStringLiteral("Remove an alias");
}

QString cmd::Unalias::description() const
{
    return QStringLiteral("Remove an alias");
}

mp::ParseCode cmd::Unalias::parse_args(mp::ArgParser* parser)
{
    parser->addPositionalArgument("name", "Name of aliases to remove, can be --all to remove all the aliases\n");

    QCommandLineOption all_option(all_option_name, "Remove all aliases");
    parser->addOption(all_option);

    auto status = parser->commandParse(this);
    if (status != ParseCode::Ok)
        return status;

    auto parse_code = check_for_name_and_all_option_conflict(parser, cerr, false);
    if (parse_code != ParseCode::Ok)
        return parse_code;

    if (!parser->isSet(all_option_name))
        for (const auto& arg : parser->positionalArguments())
            aliases_to_remove.push_back(arg.toStdString());

    return ParseCode::Ok;
}
