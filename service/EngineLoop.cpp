#include "EngineLoop.h"
#include <thread>
#include <Utils/Timer.h>
#include "Utils/ServiceLocator.h"
#include <Networking/InputQueue.h>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkClient.h>
#include <tracy/Tracy.hpp>

// Component Singletons
#include "ECS/Components/Singletons/TimeSingleton.h"
#include "ECS/Components/Network/ConnectionDeferredSingleton.h"
#include "ECS/Components/Network/LoadBalanceSingleton.h"

// Components
#include "ECS/Components/Network/ConnectionComponent.h"
#include "ECS/Components/GameEntityInfo.h"
#include "ECS/Components/Transform.h"

// Systems
#include "ECS/Systems/Network/ConnectionSystems.h"
#include "ECS/Systems/Network/InitializePlayerSystem.h"

// Handlers
#include "Network/Handlers/Auth/AuthHandlers.h"
#include "Network/Handlers/General/GeneralHandlers.h"

EngineLoop::EngineLoop()
    : _isRunning(false), _inputQueue(256), _outputQueue(16)
{
    _network.asioService = std::make_shared<asio::io_service>(2);
    _network.internalServer = std::make_shared<NetworkServer>(_network.asioService, 8000);
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Start()
{
    if (_isRunning)
        return;

    // Setup Input Queue for libraries
    ServiceLocator::SetInputQueue(&_inputQueue);

    std::thread threadRunIoService = std::thread(&EngineLoop::RunIoService, this);
    threadRunIoService.detach();

    std::thread threadRun = std::thread(&EngineLoop::Run, this);
    threadRun.detach();
}

void EngineLoop::Stop()
{
    if (!_isRunning)
        return;

    Message message;
    message.code = MSG_IN_EXIT;
    PassMessage(message);
}

void EngineLoop::PassMessage(Message& message)
{
    _inputQueue.enqueue(message);
}

bool EngineLoop::TryGetMessage(Message& message)
{
    return _outputQueue.try_dequeue(message);
}

void EngineLoop::RunIoService()
{
    asio::io_service::work ioWork(*_network.asioService.get());
    _network.asioService->run();
}
void EngineLoop::Run()
{
    _isRunning = true;

    SetupUpdateFramework();
    _updateFramework.gameRegistry.create();

    TimeSingleton& timeSingleton = _updateFramework.gameRegistry.set<TimeSingleton>();
    ConnectionDeferredSingleton& connectionDeferredSingleton = _updateFramework.gameRegistry.set<ConnectionDeferredSingleton>();
    LoadBalanceSingleton& loadBalanceSingleton = _updateFramework.gameRegistry.set<LoadBalanceSingleton>();

    connectionDeferredSingleton.networkServer = _network.internalServer;

    _network.internalServer->SetConnectionHandler(std::bind(&ConnectionUpdateSystem::HandleConnection, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _network.internalServer->Start();

    /*DBConnection conn("localhost", 3306, "root", "ascent", "novuscore", 0, 2);
    
    for (i32 i = 0; i < 11; i++)
    {
        conn.QueryAsync("SELECT id, value, date FROM test",
            [&](std::shared_ptr<QueryResult> result)
            {
                while (result->GetNextRow())
                {
                    const Field& idField = result->GetField(0);
                    const Field& valueField = result->GetField(1);
                    const Field& dateField = result->GetField(2);

                    PrintMessage("WorkerThread completed query");
                }
            });
    }*/

    Timer timer;
    f32 targetDelta = 1.0f / 60.0f;
    while (true)
    {
        f32 deltaTime = timer.GetDeltaTime();
        timer.Tick();

        timeSingleton.lifeTimeInS = timer.GetLifeTime();
        timeSingleton.lifeTimeInMS = timeSingleton.lifeTimeInS * 1000;
        timeSingleton.deltaTime = deltaTime;

        if (!Update())
            break;

        {
            ZoneScopedNC("WaitForTickRate", tracy::Color::AntiqueWhite1)

            // Wait for tick rate, this might be an overkill implementation but it has the even tickrate I've seen - MPursche
            {
                ZoneScopedNC("Sleep", tracy::Color::AntiqueWhite1) for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta - 0.0025f; deltaTime = timer.GetDeltaTime())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            {
                ZoneScopedNC("Yield", tracy::Color::AntiqueWhite1) for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta; deltaTime = timer.GetDeltaTime())
                {
                    std::this_thread::yield();
                }
            }
        }


        FrameMark
    }

    // Clean up stuff here

    Message exitMessage;
    exitMessage.code = MSG_OUT_EXIT_CONFIRM;
    _outputQueue.enqueue(exitMessage);
}

bool EngineLoop::Update()
{
    ZoneScopedNC("Update", tracy::Color::Blue2)
    {
        ZoneScopedNC("HandleMessages", tracy::Color::Green3)
            Message message;

        while (_inputQueue.try_dequeue(message))
        {
            if (message.code == -1)
                assert(false);

            if (message.code == MSG_IN_EXIT)
            {
                return false;
            }
            else if (message.code == MSG_IN_PING)
            {
                ZoneScopedNC("Ping", tracy::Color::Green3)
                    Message pongMessage;
                pongMessage.code = MSG_OUT_PRINT;
                pongMessage.message = new std::string("PONG!");
                _outputQueue.enqueue(pongMessage);
            }
        }
    }

    UpdateSystems();
    return true;
}

void EngineLoop::SetupUpdateFramework()
{
    tf::Framework& framework = _updateFramework.framework;
    entt::registry& gameRegistry = _updateFramework.gameRegistry;

    ServiceLocator::SetRegistry(&gameRegistry);
    SetMessageHandler();

    // @TODO: Temporary fix to allow taskflow to run multiple tasks at the same time when using Entt to construct views
    gameRegistry.prepare<ConnectionComponent>();

    // ConnectionUpdateSystem
    tf::Task connectionUpdateSystemTask = framework.emplace([&gameRegistry]()
    {
        ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue2)
        ConnectionUpdateSystem::Update(gameRegistry);
    });

    // InitializePlayerSystem
    tf::Task initializePlayerSystemTask = framework.emplace([&gameRegistry]()
    {
        ZoneScopedNC("InitializePlayerSystem::Update", tracy::Color::Blue2)
            InitializePlayerSystem::Update(gameRegistry);
    });
    initializePlayerSystemTask.gather(connectionUpdateSystemTask);

    // ConnectionDeferredSystem
    tf::Task connectionDeferredSystemTask = framework.emplace([&gameRegistry]()
    {
        ZoneScopedNC("ConnectionDeferredSystem::Update", tracy::Color::Blue2)
            ConnectionDeferredSystem::Update(gameRegistry);
    });
    connectionDeferredSystemTask.gather(initializePlayerSystemTask);
}
void EngineLoop::SetMessageHandler()
{
    auto messageHandler = new MessageHandler();
    ServiceLocator::SetNetworkMessageHandler(messageHandler);

    Network::AuthHandlers::Setup(messageHandler);
    Network::GeneralHandlers::Setup(messageHandler);
}
void EngineLoop::UpdateSystems()
{
    ZoneScopedNC("UpdateSystems", tracy::Color::Blue2)
    {
        ZoneScopedNC("Taskflow::Run", tracy::Color::Blue2)
            _updateFramework.taskflow.run(_updateFramework.framework);
    }
    {
        ZoneScopedNC("Taskflow::WaitForAll", tracy::Color::Blue2)
            _updateFramework.taskflow.wait_for_all();
    }
}
