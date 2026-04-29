#include "http_server.h"
#include <ArduinoJson.h>

namespace {

void sendJson(WebServer &server, int statusCode, const JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  server.send(statusCode, "application/json", body);
}

void sendJsonError(WebServer &server, int statusCode, const char *error) {
  JsonDocument doc;
  doc["error"] = error;
  sendJson(server, statusCode, doc);
}

bool parseRequestBody(WebServer &server, JsonDocument &doc) {
  if (!server.hasArg("plain")) {
    return false;
  }

  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  return !err;
}

} // namespace

HttpServer::HttpServer(SensorManager &sensorManager, uint16_t port)
    : server_(port), sensorManager_(sensorManager),
      deviceService_(sensorManager) {}

void HttpServer::begin() {
  registerRoutes();
  server_.begin();
}

void HttpServer::registerRoutes() {
  server_.on("/", [this]() { handleRoot(); });
  server_.on("/healthz", [this]() { handleHealthz(); });
  server_.on("/json", [this]() { handleJson(); });
  server_.on("/metrics", [this]() { handleMetrics(); });

  server_.on("/api/device/info", [this]() { handleDeviceInfo(); });
  server_.on("/api/device/provision", HTTP_POST,
             [this]() { handleProvision(); });
  server_.on("/api/device/hostname", HTTP_POST,
             [this]() { handleSetHostname(); });
  server_.on("/api/device/reboot", HTTP_POST, [this]() { handleReboot(); });

  server_.on("/api/sensors/bme280/calibration", HTTP_GET,
           [this]() { handleGetCalibration(SensorType::Bme280); });
  server_.on("/api/sensors/bme280/temperature/calibration", HTTP_POST,
           [this]() { handleSetCalibration(SensorType::Bme280,"temperature"); });
  server_.on("/api/sensors/bme280/humidity/calibration", HTTP_POST,
             [this]() { handleSetCalibration(SensorType::Bme280,"humidity"); });
  server_.on("/api/sensors/bme280/pressure/calibration", HTTP_POST,
             [this]() { handleSetCalibration(SensorType::Bme280,"pressure"); });          
}

void HttpServer::handleGetCalibration(SensorType st){
  JsonDocument doc;

  if (!sensorManager_.getCalibration(st, doc)){
    server_.send(400, "application/json", "{\"error\":\"invalid sensor type\"}");
    return;
  }
  
  String body;
  serializeJson(doc, body);

  server_.send(200, "application/json", body);
}

void HttpServer::handleSetCalibration(SensorType st ,const char* field) {
  if (!server_.hasArg("plain")) {
    server_.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server_.arg("plain"));

  if (error) {
    server_.send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  if (!doc["offset"].is<float>()) {
    server_.send(400, "application/json", "{\"error\":\"missing offset\"}");
    return;
  }

  float offset = doc["offset"].as<float>();

  if ( ! sensorManager_.setCalibration(st, field, offset) ){
    server_.send(400, "application/json", "{\"error\":\"Bad Request\"}");
    return;
  }
  
  JsonDocument response;
  response["sensor"] = sensorManager_.sensorTypeToString(st);
  response["field"] = field;
  response["offset"] = offset;

  String body;
  serializeJson(response, body);

  server_.send(200, "application/json", body);
}


void HttpServer::handleRoot() {
  String body = deviceService_.getTextStatus();
  server_.send(200, "text/plain", body);
}

void HttpServer::handleJson() {
  JsonDocument doc;

  deviceService_.getJSONStatus(doc);

  sendJson(server_, 200, doc);
}

void HttpServer::handleHealthz() { server_.send(200, "text/plain", "OK\n"); }

void HttpServer::handleMetrics() {

  String metrics = deviceService_.getMetrics();
  server_.send(200, "text/plain; version=0.0.4", metrics);
}

void HttpServer::handleDeviceInfo() {
  JsonDocument doc;
  deviceService_.getDeviceInfo(doc);

  sendJson(server_, 200, doc);
}

void HttpServer::handleProvision() {
  JsonDocument request;

  if (!server_.hasArg("plain")) {
    sendJsonError(server_, 400, "missing_body");
    return;
  }

  if (!parseRequestBody(server_, request)) {
    sendJsonError(server_, 400, "invalid_json");
    return;
  }

  const char *newIdRaw = request["device_id"];
  const char *newHostnameRaw = request["hostname"];

  const String newId = newIdRaw ? String(newIdRaw) : "";
  const String newHostname = newHostnameRaw ? String(newHostnameRaw) : newId;

  JsonDocument response;

  DeviceService::Result result =
      deviceService_.provisionDevice(newId, newHostname, response);

  if (!result.ok) {
    sendJsonError(server_, result.statusCode, result.error);
    return;
  }

  sendJson(server_, result.statusCode, response);
}

void HttpServer::handleSetHostname() {
  JsonDocument request;

  if (!server_.hasArg("plain")) {
    sendJsonError(server_, 400, "missing_body");
    return;
  }

  if (!parseRequestBody(server_, request)) {
    sendJsonError(server_, 400, "invalid_json");
    return;
  }

  const char *newHostnameRaw = request["hostname"];
  const String newHostname = newHostnameRaw ? String(newHostnameRaw) : "";

  JsonDocument response;

  DeviceService::Result result =
      deviceService_.setHostname(newHostname, response);

  if (!result.ok) {
    sendJsonError(server_, result.statusCode, result.error);
    return;
  }

  sendJson(server_, result.statusCode, response);
}

// void HttpServer::handleReboot() {
//   JsonDocument response;
//
//   if (rebootRequested_) {
//     response["status"] = "ok";
//     response["message"] = "reboot_already_scheduled";
//     sendJson(server_, 202, response);
//     return;
//   }
//
//   Serial.println("[http] reboot requested");
//
//   rebootRequested_ = true;
//   rebootAtMs_ = millis() + kRebootDelayMs;
//
//   response["status"] = "ok";
//   response["message"] = "reboot_scheduled";
//   sendJson(server_, 202, response);
// }

void HttpServer::handleReboot() {
  JsonDocument response;

  DeviceService::Result result = deviceService_.requestReboot(response);

  if (!result.ok) {
    sendJsonError(server_, result.statusCode, result.error);
    return;
  }

  sendJson(server_, result.statusCode, response);
}

// void HttpServer::handleClient() {
//   server_.handleClient();
//
//   if (rebootRequested_ && static_cast<long>(millis() - rebootAtMs_) >= 0) {
//     Serial.println("[http] rebooting now...");
//     delay(kRebootFinalDelayMs);
//     ESP.restart();
//   }
// }

void HttpServer::handleClient() {
  server_.handleClient();
  deviceService_.handlePendingReboot();
}