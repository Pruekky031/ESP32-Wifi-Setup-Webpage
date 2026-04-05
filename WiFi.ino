#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

/* AP */
const char* ap_ssid = "ESP32_Setup";
const char* ap_pass = "12345678";

/* HTML PAGE */
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
<meta charset="UTF-8">
<title>WiFi Setup</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            background: #0f172a;
            color: #e2e8f0;
            padding: 20px;
        }

        * {
            box-sizing: border-box;
        }

        .container {
            max-width: 480px;
            margin: auto;
        }

        .topbar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
        }

        h2 {
            margin: 0;
            font-weight: 500;
        }

        /* refresh */
        .refresh-btn {
            padding: 6px 12px;
            border-radius: 8px;
            border: none;
            background: #38bdf8;
            cursor: pointer;
        }

        /* loader */
        .loader {
            width: 50px;
            aspect-ratio: 1;
            border-radius: 50%;
            margin: 30px auto;
            background:
                radial-gradient(farthest-side, #03c0f9 94%, #0000) top/8px 8px no-repeat,
                conic-gradient(#0000 30%, #16b5ff);
            -webkit-mask: radial-gradient(farthest-side, #0000 calc(100% - 8px), #000 0);
            animation: l13 1s infinite linear;
        }

        @keyframes l13 {
            100% {
                transform: rotate(1turn)
            }
        }

        /* card */
        .wifi-card {
            background: #1e293b;
            border-radius: 14px;
            margin-bottom: 12px;
            overflow: hidden;
            transition: 0.25s;
            border: 1px solid #334155;
        }

        .wifi-card:hover {
            border-color: #38bdf8;
        }

        .header {
            padding: 14px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            cursor: pointer;
        }

        .ssid {
            font-size: 16px;
            font-weight: 500;
        }

        .rssi {
            font-size: 13px;
            color: #94a3b8;
        }

        .signal {
            display: flex;
            gap: 3px;
        }

        .bar{width:4px;height:10px;background:#475569;display:inline-block;margin-right:2px;}
.bar.active{background:#38bdf8;}
.bar.lvl-4.active{background:#22c55e;}
.bar.lvl-3.active{background:#84cc16;}
.bar.lvl-2.active{background:#facc15;}
.bar.lvl-1.active{background:#ef4444;}

        /* expand */
        .expand {
            max-height: 0;
            overflow: hidden; 
            transition: 0.3s ease;
            padding: 0 14px;
        }

        .expand.active {
            max-height: 120px;
            padding-bottom: 14px;
        }

        input {
            width: 100%;
            padding: 10px;
            margin-top: 10px;
            border-radius: 8px;
            border: 1px solid #334155;
            background: #0f172a;
            color: white;
        }

        button {
            width: 100%;
            padding: 10px;
            margin-top: 10px;
            border-radius: 8px;
            border: none;
            background: #38bdf8;
            color: black;
            font-weight: 600;
            cursor: pointer;
        }

        .lock {
            font-size: 11px;
            color: #94a3b8;
            margin-left: 6px;
            opacity: 0.7;
        }

        /* สีตามระดับสัญญาณ */
        .bar.lvl-4.active {
            background: #22c55e;
        }

        /* เขียว */
        .bar.lvl-3.active {
            background: #84cc16;
        }

        /* เขียวอ่อน */
        .bar.lvl-2.active {
            background: #facc15;
        }

        /* เหลือง */
        .bar.lvl-1.active {
            background: #ef4444;
        }

        /* แดง */
        button:hover {
            background: #0ea5e9;
        }
    </style>
</head>
<body>

<div class="container">
<h3>WiFi Setup</h3>
<div id="wifiList"></div>
<button onclick="refreshWiFi()">Refresh</button>
</div>

<script>
let wifiListData = [];

function getLevel(rssi){
  if(rssi>-50)return 4;
  if(rssi>-65)return 3;
  if(rssi>-75)return 2;
  return 1;
}

function bars(level){
  let html="";
  for(let i=1;i<=4;i++){
    html += `<div class="bar ${i<=level?'active lvl-'+level:''}"></div>`;
  }
  return html;
}

function loadWiFi(){
  const c=document.getElementById("wifiList");
  c.innerHTML="";
  
  wifiListData.forEach((net,i)=>{
    c.innerHTML += `
    <div class="wifi-card">
      <div onclick="toggle(${i})">
        ${net.ssid} (${net.rssi})
        ${bars(getLevel(net.rssi))}
      </div>
      <div id="ex-${i}" style="display:none;">
        ${net.enc=="OPEN"
        ? `<button onclick="connectOpen('${net.ssid}')">Connect</button>`
        : `
        <input type="password" id="p-${i}" placeholder="Password">
        <button onclick="connectSecure('${net.ssid}',${i})">Connect</button>`}
      </div>
    </div>`;
  });
}

function toggle(i){
  let el=document.getElementById("ex-"+i);
  el.style.display = el.style.display=="none"?"block":"none";
}

async function refreshWiFi(){
  const res = await fetch("/scan");
  wifiListData = await res.json();
  loadWiFi();
}

async function connectSecure(ssid,i){
  const pass=document.getElementById("p-"+i).value;
  await fetch("/connect",{
    method:"POST",
    headers:{"Content-Type":"application/x-www-form-urlencoded"},
    body:`ssid=${ssid}&pass=${pass}`
  });
  alert("Connecting...");
}

async function connectOpen(ssid){
  await fetch("/connect",{
    method:"POST",
    headers:{"Content-Type":"application/x-www-form-urlencoded"},
    body:`ssid=${ssid}&pass=`
  });
  alert("Connecting...");
}

refreshWiFi();
</script>

</body>
</html>
)rawliteral";

/* ===== scan ===== */
void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";

  for (int i = 0; i < n; i++) {
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"enc\":\"" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "OPEN" : "WPA2") + "\"";
    json += "}";
    if (i < n - 1) json += ",";
  }

  json += "]";
  server.send(200, "application/json", json);
}

/* ===== connect ===== */
void handleConnect() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  server.send(200, "text/plain", "Connecting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int t = 20;
  while (WiFi.status() != WL_CONNECTED && t--) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.softAPdisconnect(true);
  }
}

/* ===== root ===== */
void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_pass);

  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/connect", HTTP_POST, handleConnect);

  server.begin();
}

void loop() {
  server.handleClient();
}