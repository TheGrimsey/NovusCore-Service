/*
    MIT License

    Copyright (c) 2020 NovusCore

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once
#include <Utils/Message.h>
#include <Utils/srp.h>
#include <Utils/StringUtils.h>
#include <Database/DBConnection.h>
#include <entt.hpp>
#include "../EngineLoop.h"
#include "../ECS/Components/Singletons/DBSingleton.h"
#include "../Utils/ServiceLocator.h"

void AccountCommand(EngineLoop& engineLoop, std::vector<std::string> subCommands)
{
    if (subCommands.size() < 2)
    {
        DebugHandler::PrintError("Account Commands takes the following parameters. (Username, Password)");
        return;
    }

    std::string username = subCommands[0];
    std::string password = subCommands[1];

    std::shared_ptr<Bytebuffer> sBuffer = Bytebuffer::Borrow<4>();
    std::shared_ptr<Bytebuffer> vBuffer = Bytebuffer::Borrow<256>();
    SRPUtils::CreateAccount(username.c_str(), password.c_str(), sBuffer.get(), vBuffer.get());

    std::string sHex = StringUtils::BytesToHexStr(sBuffer->GetDataPointer(), sBuffer->size);
    std::string vHex = StringUtils::BytesToHexStr(vBuffer->GetDataPointer(), vBuffer->size);

    entt::registry* registry = ServiceLocator::GetRegistry();
    DBSingleton& dbSingleton = registry->ctx<DBSingleton>();

    std::stringstream ss;

    ss << "INSERT INTO `accounts` (`username`, `salt`, `verifier`) ";
    ss << "VALUES('" << username << "', '" << sHex << "', '" << vHex << "');";

    dbSingleton.auth.Execute(ss.str());
    DebugHandler::PrintSuccess("Created account %s", username.c_str());
}