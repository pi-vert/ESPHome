#pragma once

#include "esphome.h"
#include "esphome/components/network/util.h"
#include <cmath>
#include <cstring>
#include <string>

namespace mqtt_standard {

inline void envelope(JsonObject root, const char *id, const char *type, const char *version) {
  root["schema_version"] = 1;
  root["id"] = id;
  root["type"] = type;
  root["firmware_version"] = version;
  root["uptime_s"] = esphome::millis_64() / 1000;
}

inline void announcement(JsonObject root, const char *id, const char *type,
                         const char *version, const char *prefix) {
  envelope(root, id, type, version);
  char ip[esphome::network::IP_ADDRESS_BUFFER_SIZE];
  esphome::network::get_ip_addresses()[0].str_to(ip);
  root["ip"] = ip;
  root["availability_topic"] = std::string(prefix) + "/status";
  root["state_topic"] = std::string(prefix) + "/state";
  root["heartbeat_interval_s"] = 60;
  root["outputs"].to<JsonArray>();
  root["inputs"].to<JsonArray>();
}

inline JsonObject output(JsonObject root, const char *prefix, const char *category,
                   const char *name, const char *datatype, const char *unit, bool retain) {
  JsonObject item = root["outputs"].as<JsonArray>().add<JsonObject>();
  item["name"] = name;
  item["category"] = category;
  item["datatype"] = datatype;
  if (unit != nullptr) item["unit"] = unit;
  item["topic"] = std::string(prefix) + "/" + category + "/" + name + "/state";
  item["qos"] = 0;
  item["retain"] = retain;
  item["kind"] = std::strcmp(category, "event") == 0 ? "event" : "state";
  item["ttl_s"] = 180;
  return item;
}

// Bounds describe the value transmitted on MQTT, after ESPHome filters.
inline JsonObject range(JsonObject port, float minimum, float maximum, float step) {
  port["min"] = minimum;
  port["max"] = maximum;
  port["step"] = step;
  return port;
}

inline JsonObject input(JsonObject root, const char *prefix, const char *name, const char *datatype) {
  JsonObject item = root["inputs"].as<JsonArray>().add<JsonObject>();
  item["name"] = name;
  item["datatype"] = datatype;
  item["topic"] = std::string(prefix) + "/actuator/" + name + "/set";
  item["qos"] = 0;
  item["retain"] = false;
  item["multiple"] = "single";
  return item;
}

inline void number(JsonObject root, const char *name, float value) {
  if (std::isfinite(value)) {
    root[name] = value;
  } else {
    root[name] = nullptr;
  }
}

}  // namespace mqtt_standard
