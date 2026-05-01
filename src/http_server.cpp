#include "http_server.h"
#include <ArduinoJson.h>

namespace {

enum class ParseBodyResult { Ok, MissingBody, InvalidJson };

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

ParseBodyResult parseRequestBody(WebServer &server, JsonDocument &doc) {
  if (!server.hasArg("plain")) {
    return ParseBodyResult::MissingBody;
  }

  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    return ParseBodyResult::InvalidJson;
  }

  return ParseBodyResult::Ok;
}

const char *parseBodyResultToError(ParseBodyResult pb) {
  switch (pb) {
  case ParseBodyResult::MissingBody:
    return "missing_body";
  case ParseBodyResult::InvalidJson:
    return "invalid_json";
  case ParseBodyResult::Ok:
    return nullptr;
  }
  return "unknown_error";
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

  server_.onNotFound([this]() { handleDynamicCalibrationRoute(); });
}

void HttpServer::handleGetCalibration(SensorType st) {
  JsonDocument doc;

  if (!sensorManager_.getCalibration(st, doc)) {
    sendJsonError(server_, 400, "invalid_sensor_type");
    return;
  }
  sendJson(server_, 200, doc);
}

void HttpServer::handleDynamicCalibrationRoute() {
  String sensor;
  String field;

  if (!server_.uri().startsWith(kCalibrationPrefix) ||
      !server_.uri().endsWith(kCalibrationSuffix)) {
    sendJsonError(server_, 404, "not_found");
    return;
  }

  if (server_.method() == HTTP_GET) {
    if (!extractSensorCalibrationPath(server_.uri(), sensor)) {
      sendJsonError(server_, 400, "invalid_calibration_path");
      return;
    }

    SensorType st = sensorManager_.parseSensorType(sensor.c_str());
    handleGetCalibration(st);
    return;
  }

  if (server_.method() == HTTP_POST) {
    if (!extractCalibrationPath(server_.uri(), sensor, field)) {
      sendJsonError(server_, 400, "invalid_calibration_path");
      return;
    }

    SensorType st = sensorManager_.parseSensorType(sensor.c_str());
    processSetCalibration(st, field.c_str());
    return;
  }

  sendJsonError(server_, 405, "method_not_allowed");
}

void HttpServer::processSetCalibration(SensorType st, const char *field) {
  JsonDocument doc;

  ParseBodyResult result = parseRequestBody(server_, doc);
  const char *error = parseBodyResultToError(result);
  if (error) {
    sendJsonError(server_, 400, error);
    return;
  }

  if (!doc["reference"].is<float>()) {
    sendJsonError(server_, 400, "missing_reference");
    return;
  }

  float reference = doc["reference"].as<float>();

  if (!sensorManager_.setCalibration(st, field, reference)) {
    sendJsonError(server_, 400, "bad_request");
    return;
  }

  JsonDocument response;

  if (!sensorManager_.getCalibration(st, response)) {
    sendJsonError(server_, 500, "failed_to_read_calibration");
    return;
  }

  sendJson(server_, 200, response);
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
  server_.send(200, "text/plain; version=0.0.4", deviceService_.getMetrics());
}

void HttpServer::handleDeviceInfo() {
  JsonDocument doc;
  deviceService_.getDeviceInfo(doc);
  sendJson(server_, 200, doc);
}

void HttpServer::handleProvision() {
  JsonDocument request;

  ParseBodyResult res = parseRequestBody(server_, request);
  const char *error = parseBodyResultToError(res);
  if (error) {
    sendJsonError(server_, 400, error);
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

  ParseBodyResult res = parseRequestBody(server_, request);
  const char *error = parseBodyResultToError(res);
  if (error) {
    sendJsonError(server_, 400, error);
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

void HttpServer::handleReboot() {
  JsonDocument response;

  DeviceService::Result result = deviceService_.requestReboot(response);

  if (!result.ok) {
    sendJsonError(server_, result.statusCode, result.error);
    return;
  }
  sendJson(server_, result.statusCode, response);
}

void HttpServer::handleClient() {
  server_.handleClient();
  deviceService_.handlePendingReboot();
}

bool HttpServer::extractCalibrationPath(const String &uri, String &sensor,
                                        String &field) {

  const size_t prefixLen = strlen(kCalibrationPrefix);
  const size_t suffixLen = strlen(kCalibrationSuffix);

  if (!uri.startsWith(kCalibrationPrefix) ||
      !uri.endsWith(kCalibrationSuffix)) {
    return false;
  }

  String middle = uri.substring(prefixLen, uri.length() - suffixLen);

  int slash = middle.indexOf('/');
  if (slash < 0) {
    return false;
  }

  sensor = middle.substring(0, slash);
  field = middle.substring(slash + 1);

  return sensor.length() > 0 && field.length() > 0;
}

bool HttpServer::extractSensorCalibrationPath(const String &uri,
                                              String &sensor) {

  const size_t prefixLen = strlen(kCalibrationPrefix);
  const size_t suffixLen = strlen(kCalibrationSuffix);

  if (!uri.startsWith(kCalibrationPrefix) ||
      !uri.endsWith(kCalibrationSuffix)) {
    return false;
  }

  String middle = uri.substring(prefixLen, uri.length() - suffixLen);

  if (middle.indexOf('/') >= 0) {
    return false;
  }

  sensor = middle;
  return sensor.length() > 0;
}

void HttpServer::updateMetricsCache() { deviceService_.updateMetricsCache(); }
