#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ========================================
// LogoCar ESP32 - Demo v0.1
// Board: ESP32 DevKit V1
// ========================================

namespace Config {
  constexpr char WIFI_SSID[] = "LogoCar-Demo";
  constexpr char WIFI_PASSWORD[] = "logo12345";
  constexpr uint16_t WEB_PORT = 80;
}

WebServer server(Config::WEB_PORT);

String currentLogo = "DEFAULT";
unsigned long lastCommandTime = 0;

// Giao diện demo chạy trực tiếp từ ESP32
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LogoCar Control</title>
<style>
:root {
  color-scheme: dark;
  --bg: #0b0e13;
  --panel: #151a22;
  --line: #29313d;
  --text: #f3f5f8;
  --muted: #9ba6b5;
  --accent: #4da3ff;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  padding: 24px 16px;
  background: var(--bg);
  color: var(--text);
  font-family: Arial, sans-serif;
}
main { max-width: 720px; margin: auto; }
header { margin-bottom: 26px; }
.brand { color: var(--accent); font-size: 12px;
  font-weight: bold; letter-spacing: 3px; }
h1 { font-size: 28px; margin: 10px 0; }
p { color: var(--muted); line-height: 1.6; }
.panel {
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 14px;
  padding: 22px;
  margin-bottom: 16px;
}
.preview {
  width: 220px;
  height: 220px;
  max-width: 65vw;
  max-height: 65vw;
  margin: 14px auto 24px;
  border-radius: 50%;
  border: 7px solid #343d49;
  background: radial-gradient(circle, #242d39, #080a0e 72%);
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 0 30px #4da3ff12;
}
.emblem {
  width: 145px;
  height: 145px;
  border: 3px solid #d7e0eb;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
  font-size: 22px;
  font-weight: 800;
  letter-spacing: 1px;
  background: #111720;
  transition: all .25s ease;
}
.emblem.bmw {
  color: #fff;
  background: conic-gradient(#f7f9fc 0 25%, #2387df 25% 50%,
    #f7f9fc 50% 75%, #2387df 75%);
  border: 5px solid #b9c4d0;
  text-shadow: 0 1px 3px #000;
}
.emblem.mercedes {
  color: #e6edf5;
  background: radial-gradient(circle, #343c47, #090b0f 70%);
  border: 4px solid #aab4c1;
  font-size: 64px;
  font-weight: 400;
}
.status {
  display: flex;
  align-items: center;
  gap: 9px;
  font-size: 13px;
  color: var(--muted);
}
.dot {
  width: 9px; height: 9px; border-radius: 50%;
  background: #42d392;
}
.current {
  text-align: center;
  font-size: 14px;
  color: var(--muted);
}
.current strong { color: white; }
.buttons {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 10px;
  margin-top: 18px;
}
button {
  min-height: 48px;
  border: 1px solid #394454;
  border-radius: 7px;
  background: #202834;
  color: white;
  font-size: 14px;
  cursor: pointer;
  transition: background .2s, border-color .2s;
}
button:hover { background: #2b394b; border-color: var(--accent); }
button:disabled { opacity: .55; cursor: wait; }
button.active { background: #17365a; border-color: var(--accent); }
#message { min-height: 24px; font-size: 13px; margin-top: 16px; }
footer { text-align: center; font-size: 12px; color: #758193; }
@media (max-width: 440px) {
  .panel { padding: 16px; }
  .buttons { grid-template-columns: 1fr; }
  h1 { font-size: 24px; }
}
</style>
</head>
<body>
<main>
<header>
  <div class="brand">LOGOCAR / ESP32</div>
  <h1>Logo Control Center</h1>
  <p>Giao diện thử nghiệm điều khiển logo và màn hình xe.</p>
  <div class="status"><span class="dot"></span>
    <span id="connection">Đang kết nối...</span>
  </div>
</header>

<section class="panel">
  <div class="preview">
    <div class="emblem" id="emblem">LOGO</div>
  </div>
  <div class="current">Logo hiện tại: <strong id="current">DEFAULT</strong></div>
</section>

<section class="panel">
  <h3>Chọn logo</h3>
  <p>Chọn một chế độ để cập nhật trạng thái trên ESP32.</p>
  <div class="buttons">
    <button data-logo="BMW" onclick="selectLogo('BMW')">BMW</button>
    <button data-logo="MERCEDES" onclick="selectLogo('MERCEDES')">Mercedes-Benz</button>
    <button data-logo="DEFAULT" onclick="selectLogo('DEFAULT')">Mặc định</button>
  </div>
  <div id="message" role="status"></div>
</section>

<footer>LogoCar Demo v0.1 · ESP32 DevKit V1</footer>
</main>

<script>
const emblem = document.getElementById('emblem');
const current = document.getElementById('current');
const message = document.getElementById('message');
const connection = document.getElementById('connection');

function renderLogo(name) {
  emblem.className = 'emblem';
  if (name === 'BMW') {
    emblem.classList.add('bmw');
    emblem.textContent = 'BMW';
  } else if (name === 'MERCEDES') {
    emblem.classList.add('mercedes');
    emblem.textContent = '✳';
  } else {
    emblem.textContent = 'LOGO';
  }

  current.textContent = name;
  document.querySelectorAll('[data-logo]').forEach(button => {
    button.classList.toggle('active', button.dataset.logo === name);
  });
}

async function refreshStatus() {
  try {
    const response = await fetch('/api/status');
    if (!response.ok) throw new Error('HTTP ' + response.status);
    const data = await response.json();
    renderLogo(data.logo);
    connection.textContent = 'ESP32 đang hoạt động';
  } catch (error) {
    connection.textContent = 'Chưa kết nối được ESP32';
  }
}

async function selectLogo(name) {
  const buttons = [...document.querySelectorAll('button')];
  buttons.forEach(button => button.disabled = true);
  message.textContent = 'Đang gửi lệnh...';

  try {
    const response = await fetch('/api/logo?name=' + encodeURIComponent(name));
    const data = await response.json();

    if (!response.ok || !data.ok) {
      throw new Error(data.message || 'Lệnh thất bại');
    }

    renderLogo(data.logo);
    message.textContent = 'Đã cập nhật logo: ' + data.logo;
  } catch (error) {
    message.textContent = 'Lỗi gửi lệnh: ' + error.message;
  } finally {
    buttons.forEach(button => button.disabled = false);
  }
}

refreshStatus();
setInterval(refreshStatus, 5000);
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleStatus() {
  String json = "{\"ok\":true,\"logo\":\"" + currentLogo + "\"}";
  server.send(200, "application/json", json);
}

void handleSetLogo() {
  if (!server.hasArg("name")) {
    server.send(400, "application/json",
                "{\"ok\":false,\"message\":\"Missing name\"}");
    return;
  }

  String name = server.arg("name");
  name.toUpperCase();

  if (name != "BMW" &&
      name != "MERCEDES" &&
      name != "DEFAULT") {
    server.send(400, "application/json",
                "{\"ok\":false,\"message\":\"Unsupported logo\"}");
    return;
  }

  currentLogo = name;
  lastCommandTime = millis();

  Serial.printf("[LogoController] Logo changed to: %s\n",
                currentLogo.c_str());

  String json = "{\"ok\":true,\"logo\":\"" + currentLogo + "\"}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_AP);

  bool apStarted = WiFi.softAP(
    Config::WIFI_SSID,
    Config::WIFI_PASSWORD
  );

  if (!apStarted) {
    Serial.println("[ERROR] Failed to start Wi-Fi AP");
  } else {
    Serial.println();
    Serial.println("================================");
    Serial.println(" LogoCar Demo v0.1");
    Serial.println("================================");
    Serial.printf("Wi-Fi SSID: %s\n", Config::WIFI_SSID);
    Serial.printf("IP address: %s\n",
                  WiFi.softAPIP().toString().c_str());
    Serial.println("Web server: port 80");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/logo", HTTP_GET, handleSetLogo);

  server.onNotFound([]() {
    server.send(404, "application/json",
                "{\"ok\":false,\"message\":\"Not found\"}");
  });

  server.begin();
  Serial.println("[WebServer] Started");
}

void loop() {
  server.handleClient();
}
