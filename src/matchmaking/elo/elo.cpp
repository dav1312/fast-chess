#include <matchmaking/elo/elo.hpp>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace fast_chess {

Elo::Elo(int wins, int losses, int draws) {
    diff_  = getDiff(wins, losses, draws);
    error_ = getError(wins, losses, draws);
}

double Elo::percToEloDiff(double percentage) noexcept {
    return -400.0 * std::log10(1.0 / percentage - 1.0);
}

double Elo::inverseError(double x) noexcept {
    constexpr double pi = 3.1415926535897;

    const double a = 8.0 * (pi - 3.0) / (3.0 * pi * (4.0 - pi));
    const double y = std::log(1.0 - x * x);
    const double z = 2.0 / (pi * a) + y / 2.0;

    const double ret = std::sqrt(std::sqrt(z * z - y / a) - z);

    if (x < 0.0) return -ret;
    return ret;
}

double Elo::phiInv(double p) noexcept { return std::sqrt(2.0) * inverseError(2.0 * p - 1.0); }

double Elo::getError(int wins, int losses, int draws) noexcept {
    const double n    = wins + losses + draws;
    const double w    = wins / n;
    const double l    = losses / n;
    const double d    = draws / n;
    const double perc = w + d / 2.0;

    const double devW  = w * std::pow(1.0 - perc, 2.0);
    const double devL  = l * std::pow(0.0 - perc, 2.0);
    const double devD  = d * std::pow(0.5 - perc, 2.0);
    const double stdev = std::sqrt(devW + devL + devD) / std::sqrt(n);

    const double devMin = perc + phiInv(0.025) * stdev;
    const double devMax = perc + phiInv(0.975) * stdev;
    return (percToEloDiff(devMax) - percToEloDiff(devMin)) / 2.0;
}

double Elo::getDiff(int wins, int losses, int draws) noexcept {
    const double n          = wins + losses + draws;
    const double score      = wins + draws / 2.0;
    const double percentage = (score / n);

    return percToEloDiff(percentage);
}

std::string Elo::getElo() const noexcept {
    std::stringstream ss;

    ss << std::fixed << std::setprecision(2) << diff_;
    ss << " +/- ";
    ss << std::fixed << std::setprecision(2) << error_;
    return ss.str();
}

std::string Elo::getLos(int wins, int losses) noexcept {
    const double los = (0.5 + 0.5 * std::erf((wins - losses) / std::sqrt(2.0 * (wins + losses))));
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << los * 100.0 << " %";
    return ss.str();
}

std::string Elo::getDrawRatio(int wins, int losses, int draws) noexcept {
    const double n = wins + losses + draws;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (draws / n) * 100.0 << " %";
    return ss.str();
}

std::string Elo::getScoreRatio(int wins, int losses, int draws) noexcept {
    const double n = wins + losses + draws;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << ((wins + draws / 2.0) / n) * 100.0 << " %";
    return ss.str();
}

}  // namespace fast_chess
