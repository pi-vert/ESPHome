#pragma once

#include <array>
#include <cstdint>

namespace mqtt_standard {

class GestureWindow {
 public:
  struct Result {
    std::array<uint32_t, 4> scores{};
    uint32_t best{0};
    uint8_t winner{0};
    uint8_t ties{0};
    uint8_t first{0};
  };

  bool add(int direction) {
    if (direction < 1 || direction > 4) return false;
    const auto index = static_cast<unsigned>(direction - 1);
    if (scores_[index] == 0) order_[index] = ++distinct_;
    ++scores_[index];
    if (first_ == 0) first_ = static_cast<uint8_t>(direction);
    return true;
  }

  Result finish() {
    Result result;
    result.scores = scores_;
    result.first = first_;
    uint8_t first_winner_order = 255;
    for (unsigned index = 0; index < scores_.size(); ++index) {
      const auto score = scores_[index];
      if (score == 0 || score < result.best) continue;
      if (score > result.best) {
        result.best = score;
        result.ties = 0;
        first_winner_order = 255;
      }
      ++result.ties;
      // Départager seulement les directions ex æquo au score maximal.
      if (order_[index] < first_winner_order) {
        first_winner_order = order_[index];
        result.winner = static_cast<uint8_t>(index + 1);
      }
    }
    *this = GestureWindow{};
    return result;
  }

  const std::array<uint32_t, 4> &scores() const { return scores_; }

  static const char *label(int direction) {
    switch (direction) {
      case 1: return "UP";
      case 2: return "DOWN";
      case 3: return "LEFT";
      case 4: return "RIGHT";
      default: return "";
    }
  }

 private:
  std::array<uint32_t, 4> scores_{};
  std::array<uint8_t, 4> order_{};
  uint8_t distinct_{0};
  uint8_t first_{0};
};

// Une instance RAM par firmware, utilisée uniquement depuis la boucle ESPHome.
// Aucun GlobalsComponent ni écriture en flash n'est nécessaire.
inline GestureWindow gesture_votes;

}  // namespace mqtt_standard
