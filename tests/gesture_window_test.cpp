#include "../includes/gesture_window.h"
#include <cassert>
#include <cstring>

int main() {
  mqtt_standard::GestureWindow window;
  assert(window.finish().winner == 0);
  assert(!window.add(0));
  assert(!window.add(5));

  // Régression : le tout premier geste n'est pas forcément parmi les gagnants.
  for (int gesture : {1, 2, 3, 2, 3}) assert(window.add(gesture));
  const auto tied = window.finish();
  assert(tied.winner == 2);
  assert(tied.best == 2);
  assert(tied.ties == 2);
  assert(tied.first == 1);
  assert(window.finish().winner == 0);

  // L'ordre d'arrivée, et non l'ordre UP/DOWN/LEFT/RIGHT, départage les ex æquo.
  for (int gesture : {4, 1, 4, 1}) window.add(gesture);
  assert(window.finish().winner == 4);

  for (int gesture : {1, 4, 4, 4, 2}) window.add(gesture);
  assert(window.finish().winner == 4);

  // Deux fenêtres successives contenant UP donnent deux résultats distincts.
  window.add(1);
  assert(window.finish().winner == 1);
  window.add(1);
  assert(window.finish().winner == 1);
  assert(std::strcmp(mqtt_standard::GestureWindow::label(1), "UP") == 0);
  assert(std::strcmp(mqtt_standard::GestureWindow::label(4), "RIGHT") == 0);
}
