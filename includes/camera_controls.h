#pragma once

#include <string_view>

namespace camera_controls {

// Point d'entrée commun aux commandes API et MQTT.
// Les valeurs invalides ne doivent provoquer aucune écriture sur la caméra.
template<typename Camera>
bool apply(Camera *camera, std::string_view name, int value) {
  if (camera == nullptr || camera->is_failed() || value < -2 || value > 2) return false;
  if (name == "contrast") {
    camera->set_contrast(value);
  } else if (name == "brightness") {
    camera->set_brightness(value);
  } else if (name == "saturation") {
    camera->set_saturation(value);
  } else {
    return false;
  }
  camera->update_camera_parameters();
  return true;
}

}  // namespace camera_controls
