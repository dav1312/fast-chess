#include <matchmaking/tournament/base/tournament.hpp>

#include <affinity/affinity_manager.hpp>
#include <engines/uci_engine.hpp>
#include <matchmaking/book/opening_book.hpp>
#include <matchmaking/match/match.hpp>
#include <matchmaking/output/output.hpp>
#include <matchmaking/output/output_factory.hpp>
#include <matchmaking/result.hpp>
#include <pgn/pgn_builder.hpp>
#include <types/tournament_options.hpp>
#include <util/cache.hpp>
#include <util/file_writer.hpp>
#include <util/logger/logger.hpp>
#include <util/threadpool.hpp>

namespace fast_chess {

BaseTournament::BaseTournament(const options::Tournament &config,
                               const std::vector<EngineConfiguration> &engine_configs) {
    tournament_options_ = config;
    engine_configs_     = engine_configs;
    output_             = OutputFactory::create(config.output);
    book_               = OpeningBook(config.opening);
    cores_              = std::make_unique<affinity::AffinityManager>(config.affinity,
                                                         getMaxAffinity(engine_configs));

    const auto filename = (config.pgn.file.empty() ? "fast-chess.pgn" : config.pgn.file);

    file_writer_ = std::make_unique<FileWriter>(filename);

    pool_.resize(config.concurrency);
}

void BaseTournament::start() {
    Logger::log<Logger::Level::TRACE>("Starting...");

    create();
}

void BaseTournament::stop() {
    Logger::log<Logger::Level::TRACE>("Stopped!");
    atomic::stop = true;
    Logger::log<Logger::Level::TRACE>("Stopping threads...");
    pool_.kill();
}

void BaseTournament::playGame(const std::pair<EngineConfiguration, EngineConfiguration> &configs,
                              start_callback start, finished_callback finish,
                              const Opening &opening, std::size_t game_id) {
    if (atomic::stop) return;

    const auto core = ScopeGuard(cores_->consume());

    auto engine_one = ScopeGuard(engine_cache_.getEntry(configs.first.name, configs.first));
    auto engine_two = ScopeGuard(engine_cache_.getEntry(configs.second.name, configs.second));

    start();

    auto match = Match(tournament_options_, opening);

    try {
        match.start(engine_one.get().get(), engine_two.get().get(), core.get().cpus);

        while (match.get().needs_restart) {
            if (atomic::stop) return;
            match.start(engine_one.get().get(), engine_two.get().get(), core.get().cpus);
        }

    } catch (const std::exception &e) {
        Logger::log<Logger::Level::ERR>("Exception RoundRobin::playGame: " + std::string(e.what()));

        return;
    }

    if (atomic::stop) return;

    const auto match_data = match.get();

    // If the game was stopped, don't write the PGN
    if (match_data.termination != MatchTermination::INTERRUPT) {
        file_writer_->write(PgnBuilder(match_data, tournament_options_, game_id).get());
    }

    finish({match_data}, match_data.reason);
}

int BaseTournament::getMaxAffinity(const std::vector<EngineConfiguration> &configs) const noexcept {
    constexpr auto transform = [](const auto &val) { return std::stoi(val); };
    const auto first_threads = configs[0].getOption<int>("Threads", transform).value_or(1);

    for (const auto &config : configs) {
        const auto threads = config.getOption<int>("Threads", transform).value_or(1);

        // thread count in all configs has to be the same for affinity to work,
        // otherwise we set it to 0 and affinity is disabled
        if (threads != first_threads) {
            return 0;
        }
    }

    return first_threads;
}

}  // namespace fast_chess