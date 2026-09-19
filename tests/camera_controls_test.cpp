#include "../includes/camera_controls.h"
#include <cassert>

struct Camera {
  bool failed = false;
  int contrast = 0, brightness = 0, saturation = 0, updates = 0;
  bool is_failed() const { return failed; }
  void set_contrast(int value) { contrast = value; }
  void set_brightness(int value) { brightness = value; }
  void set_saturation(int value) { saturation = value; }
  void update_camera_parameters() { ++updates; }
};

int main() {
  Camera camera;
  assert(camera_controls::apply(&camera, "contrast", -2));
  assert(camera_controls::apply(&camera, "brightness", 2));
  assert(camera_controls::apply(&camera, "saturation", 1));
  assert(camera.contrast == -2 && camera.brightness == 2 && camera.saturation == 1);
  assert(camera.updates == 3);

  for (auto name : {"contrast", "brightness", "saturation"}) {
    assert(!camera_controls::apply(&camera, name, -3));
    assert(!camera_controls::apply(&camera, name, 3));
  }
  assert(!camera_controls::apply(&camera, "resolution", 1));
  assert(!camera_controls::apply(&camera, "", 0));
  assert(!camera_controls::apply<Camera>(nullptr, "contrast", 1));
  camera.failed = true;
  assert(!camera_controls::apply(&camera, "contrast", 1));
  assert(camera.updates == 3);
  assert(camera.contrast == -2 && camera.brightness == 2 && camera.saturation == 1);
}
