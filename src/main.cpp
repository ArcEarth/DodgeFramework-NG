#include "Events.h"
#include "Hooks.h"
#include "Settings.h"
#include "Papyrus.h"

#include <stddef.h>

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    /**
     * Setup logging.
     *
     * <p>
     * Logging is important to track issues. CommonLibSSE bundles functionality for spdlog, a common C++ logging
     * framework. Here we initialize it, using values from the configuration file. This includes support for a debug
     * logger that shows output in your IDE when it has a debugger attached to Skyrim, as well as a file logger which
     * writes data to the standard SKSE logging directory at <code>Documents/My Games/Skyrim Special Edition/SKSE</code>
     * (or <code>Skyrim VR</code> if you are using VR).
     * </p>
     */
    void InitializeLogging() {
        auto path = log_directory();
        auto plugin = PluginDeclaration::GetSingleton();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= plugin->GetName();
        *path += L".log";

#ifndef NDEBUG
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
        const auto level = spdlog::level::trace;
#else
        const auto level = spdlog::level::info;
#endif

        auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
        log->set_level(level);
        log->flush_on(level);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    /**
     * Initialize our Papyrus extensions.
     *
     * <p>
     * A common use of SKSE is to add new Papyrus functions. You can call a registration callback to do this. This
     * callback will not necessarily be called immediately, if the Papyrus VM has not been initialized yet (in that case
     * it's execution is delayed until the VM is available).
     * </p>
     *
     * <p>
     * You can call the <code>Register</code> function as many times as you want and at any time you want to register
     * additional functions.
     * </p>
     */
    void InitializePapyrus() {
        log::trace("Initializing Papyrus binding...");
        if (GetPapyrusInterface()->Register(Papyrus::DodgeFramework_MCM::Register)) {
            log::debug("Papyrus functions bound.");
        } else {
            stl::report_and_fail("Failure to register Papyrus bindings.");
        }
    }

    /**
     * Initialize the trampoline space for function hooks.
     *
     * <p>
     * Function hooks are one of the most powerful features available to SKSE developers, allowing you to replace
     * functions with your own, or replace a function call with a call to another function. However, to do this, we
     * need a code snippet that replicates the first instructions of the original code we overwrote, in order to be
     * able to call back to the original control flow with all the same functionality.
     * </p>
     *
     * <p>
     * CommonLibSSE comes with functionality to allocate trampoline space, including a common singleton space we can
     * access from anywhere. While this is not necessarily the most advanced use of trampolines and hooks, this will
     * suffice for our demo project.
     * </p>
     */
    void InitializeHooking() {
        Hooks::Install();
    }

    /**
     * Register to listen for messages.
     *
     * <p>
     * SKSE has a messaging system to allow for loosely coupled messaging. This means you don't need to know about or
     * link with a message sender to receive their messages. SKSE itself will send messages for common Skyrim lifecycle
     * events, such as when SKSE plugins are done loading, or when all ESP plugins are loaded.
     * </p>
     *
     * <p>
     * Here we register a listener for SKSE itself (because we have not specified a message source). Plugins can send
     * their own messages that other plugins can listen for as well, although that is not demonstrated in this example
     * and is not common.
     * </p>
     *
     * <p>
     * The data included in the message is provided as only a void pointer. It's type depends entirely on the type of
     * message, and some messages have no data (<code>dataLen</code> will be zero).
     * </p>
     */
    void InitializeMessaging() {
        if (!GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) {
                switch (message->type) {
                    case SKSE::MessagingInterface::kDataLoaded:
                        Events::SinkEventHandlers();
                        Settings::ReadSettings();
                        break;
                }
        })) {
            stl::report_and_fail("Unable to register message listener.");
        }
    }
}

/**
 * This if the main callback for initializing your SKSE plugin, called just before Skyrim runs its main function.
 *
 * <p>
 * This is your main entry point to your plugin, where you should initialize everything you need. Many things can't be
 * done yet here, since Skyrim has not initialized and the Windows loader lock is not released (so don't do any
 * multithreading). But you can register to listen for messages for later stages of Skyrim startup to perform such
 * tasks.
 * </p>
 */
SKSEPluginLoad(const LoadInterface* skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);


    Init(skse);
    InitializeMessaging();
    InitializeHooking();
    InitializePapyrus();

    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
