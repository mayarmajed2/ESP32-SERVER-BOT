#include "auth.h"
#include "../config.h"
#include "../notifications/notifications.h"
#include "../utils/utils.h"

// External variables
extern WebServer server;
extern UniversalTelegramBot bot;

// Session management
Session sessions[MAX_SESSIONS];
int sessionCount = 0;

void handleLogin() {
  String email = server.arg("email");
  String password = server.arg("password");
  
  if (email != "" && password != "") {
    if (verifyUser(email, password)) {
      // Create session
      if (sessionCount < MAX_SESSIONS) {
        String sessionId = generateToken();
        sessions[sessionCount].username = email;
        sessions[sessionCount].sessionId = sessionId;  // Store session ID
        sessions[sessionCount].expiry = millis() + SESSION_TIMEOUT;
        sessionCount++;
        
        // Set session cookie
        server.sendHeader("Set-Cookie", "session=" + sessionId + "; Max-Age=3600; Path=/");
        
        // Send login notification to Telegram
        String loginMsg = "✅ Successful Login!\n";
        loginMsg += "Email: " + email + "\n";
        loginMsg += "IP: " + server.client().remoteIP().toString();
        
        // Send notification based on user preferences
        sendUserNotification(email, "login", loginMsg);
        
        // Redirect to dashboard
        server.sendHeader("Location", "/dashboard");
        server.send(302);
      } else {
        server.send(503, "text/plain", "Too many active sessions");
      }
    } else {
      // Send failed login attempt to Telegram
      String failMsg = "❌ Failed Login Attempt!\n";
      failMsg += "Email: " + email + "\n";
      failMsg += "IP: " + server.client().remoteIP().toString();
      
      // Send notification based on user preferences (if user exists)
      if (userExists(email)) {
        sendUserNotification(email, "failed_login", failMsg);
      } else {
        bot.sendMessage(CHAT_ID, failMsg, "");
      }
      
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

void handleChangePassword() {
  if (!isAuthenticated()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  
  String username = getSessionUsername();
  String currentPassword = server.arg("current_password");
  String newPassword = server.arg("new_password");
  
  if (username == "" || currentPassword == "" || newPassword == "") {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }
  
  // Verify current password
  if (!verifyUser(username, currentPassword)) {
    server.send(401, "text/plain", "Current password is incorrect");
    return;
  }
  
  // Update password in users file
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    server.send(500, "text/plain", "Failed to open users file");
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, usersFile);
  usersFile.close();
  
  if (error) {
    server.send(500, "text/plain", "Failed to parse users file");
    return;
  }
  
  JsonArray users = doc.as<JsonArray>();
  bool passwordUpdated = false;
  
  for (JsonObject user : users) {
    if (user["username"] == username) {
      user["password"] = newPassword;
      passwordUpdated = true;
      break;
    }
  }
  
  if (!passwordUpdated) {
    server.send(404, "text/plain", "User not found");
    return;
  }
  
  // Save updated users file
  SD.remove(USERS_FILE);
  File newUsersFile = SD.open(USERS_FILE, FILE_WRITE);
  if (!newUsersFile) {
    server.send(500, "text/plain", "Failed to open users file for writing");
    return;
  }
  
  if (serializeJson(doc, newUsersFile) == 0) {
    newUsersFile.close();
    server.send(500, "text/plain", "Failed to write to users file");
    return;
  }
  
  newUsersFile.close();
  
  // Send notification
  String message = "🔐 Password Changed!\n";
  message += "Username: " + username + "\n";
  message += "IP: " + server.client().remoteIP().toString();
  bot.sendMessage(CHAT_ID, message, "");
  
  server.send(200, "text/plain", "Password changed successfully");
}

void handleResetPasswordRequest() {
  String email = server.arg("email");
  
  if (email == "") {
    server.send(400, "text/plain", "Email is required");
    return;
  }
  
  if (!userExists(email)) {
    // Don't reveal that user doesn't exist for security
    server.send(200, "text/plain", "If your email is registered, you will receive a password reset link");
    return;
  }
  
  // Generate reset token
  String token = generateToken();
  
  // Save token to file
  File resetTokensFile = SD.open(RESET_TOKENS_FILE, FILE_READ);
  DynamicJsonDocument doc(16384);
  
  if (resetTokensFile) {
    DeserializationError error = deserializeJson(doc, resetTokensFile);
    resetTokensFile.close();
    
    if (error) {
      doc.clear();
      doc.to<JsonObject>();
    }
  } else {
    doc.to<JsonObject>();
  }
  
  JsonObject tokens = doc.as<JsonObject>();
  tokens[email] = token;
  
  SD.remove(RESET_TOKENS_FILE);
  File newResetTokensFile = SD.open(RESET_TOKENS_FILE, FILE_WRITE);
  if (!newResetTokensFile) {
    server.send(500, "text/plain", "Failed to save reset token");
    return;
  }
  
  serializeJson(doc, newResetTokensFile);
  newResetTokensFile.close();
  
  // Send reset link via Telegram
  sendPasswordResetLink(email);
  
  server.send(200, "text/plain", "If your email is registered, you will receive a password reset link");
}

void sendPasswordResetLink(String email) {
  // Generate reset URL
  String resetUrl = "http://" + WiFi.localIP().toString() + "/reset-password?email=" + email;
  
  // Get token from reset tokens file
  File resetTokensFile = SD.open(RESET_TOKENS_FILE, FILE_READ);
  if (!resetTokensFile) {
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, resetTokensFile);
  resetTokensFile.close();
  
  if (error || !doc.containsKey(email)) {
    return;
  }
  
  String token = doc[email].as<String>();
  resetUrl += "&token=" + token;
  
  // Send message via Telegram
  String message = "🔑 Password Reset Request\n\n";
  message += "Email: " + email + "\n";
  message += "To reset your password, click the link below:\n\n";
  message += resetUrl + "\n\n";
  message += "This link will expire in 24 hours.";
  
  bot.sendMessage(CHAT_ID, message, "");
}

void handleResetPassword() {
  // Check if it's a GET request (reset password form)
  if (server.method() == HTTP_GET) {
    String token = server.arg("token");
    String email = server.arg("email");
    
    if (token == "" || email == "") {
      server.send(400, "text/plain", "Missing token or email");
      return;
    }
    
    // Verify token
    File resetTokensFile = SD.open(RESET_TOKENS_FILE, FILE_READ);
    if (!resetTokensFile) {
      server.send(400, "text/plain", "Invalid or expired token");
      return;
    }
    
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, resetTokensFile);
    resetTokensFile.close();
    
    if (error || !doc.containsKey(email) || doc[email] != token) {
      server.send(400, "text/plain", "Invalid or expired token");
      return;
    }
    
    // Show reset password form
    handleFileRead("/reset_password.html");
    return;
  }
  
  // POST request (process password reset)
  String email = server.arg("email");
  String token = server.arg("token");
  String newPassword = server.arg("new_password");
  
  if (email == "" || token == "" || newPassword == "") {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }
  
  // Verify token
  File resetTokensFile = SD.open(RESET_TOKENS_FILE, FILE_READ);
  if (!resetTokensFile) {
    server.send(400, "text/plain", "Invalid or expired token");
    return;
  }
  
  DynamicJsonDocument tokenDoc(16384);
  DeserializationError tokenError = deserializeJson(tokenDoc, resetTokensFile);
  resetTokensFile.close();
  
  if (tokenError || !tokenDoc.containsKey(email) || tokenDoc[email] != token) {
    server.send(400, "text/plain", "Invalid or expired token");
    return;
  }
  
  // Update password
  File usersFile = SD.open(USERS_FILE, FILE_READ);
  if (!usersFile) {
    server.send(500, "text/plain", "Failed to open users file");
    return;
  }
  
  DynamicJsonDocument userDoc(16384);
  DeserializationError userError = deserializeJson(userDoc, usersFile);
  usersFile.close();
  
  if (userError) {
    server.send(500, "text/plain", "Failed to parse users file");
    return;
  }
  
  JsonArray users = userDoc.as<JsonArray>();
  bool passwordUpdated = false;
  
  for (JsonObject user : users) {
    if (user["username"] == email) {
      user["password"] = newPassword;
      passwordUpdated = true;
      break;
    }
  }
  
  if (!passwordUpdated) {
    server.send(404, "text/plain", "User not found");
    return;
  }
  
  // Save updated users file
  SD.remove(USERS_FILE);
  File newUsersFile = SD.open(USERS_FILE, FILE_WRITE);
  if (!newUsersFile) {
    server.send(500, "text/plain", "Failed to open users file for writing");
    return;
  }
  
  if (serializeJson(userDoc, newUsersFile) == 0) {
    newUsersFile.close();
    server.send(500, "text/plain", "Failed to write to users file");
    return;
  }
  
  newUsersFile.close();
  
  // Remove used token
  tokenDoc.remove(email);
  SD.remove(RESET_TOKENS_FILE);
  File newResetTokensFile = SD.open(RESET_TOKENS_FILE, FILE_WRITE);
  if (newResetTokensFile) {
    serializeJson(tokenDoc, newResetTokensFile);
    newResetTokensFile.close();
  }
  
  // Send notification
  String message = "🔐 Password Reset Completed!\n";
  message += "Email: " + email + "\n";
  message += "IP: " + server.client().remoteIP().toString();
  bot.sendMessage(CHAT_ID, message, "");
  
  server.send(200, "text/plain", "Password has been reset successfully");
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
    if (user["username"] == email && user["password"] == password) {
      return true;
    }
  }
  return false;
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

bool isAuthenticated() {
  // تنظيف الجلسات منتهية الصلاحية أولاً
  cleanExpiredSessions();
  
  if (!server.hasHeader("Cookie")) {
    return false;
  }
  
  String cookie = server.header("Cookie");
  int sessionStart = cookie.indexOf("session=");
  if (sessionStart == -1) {
    return false;
  }
  
  int sessionEnd = cookie.indexOf(';', sessionStart);
  if (sessionEnd == -1) {
    sessionEnd = cookie.length();
  }
  
  String sessionId = cookie.substring(sessionStart + 8, sessionEnd);
  
  // التحقق من صلاحية الجلسة
  for (int i = 0; i < sessionCount; i++) {
    if (sessions[i].sessionId == sessionId && sessions[i].expiry > millis()) {
      return true;
    }
  }
  
  return false;
}

String getSessionUsername() {
  if (!server.hasHeader("Cookie")) {
    return "";
  }
  
  String cookie = server.header("Cookie");
  int sessionStart = cookie.indexOf("session=");
  if (sessionStart == -1) {
    return "";
  }
  
  int sessionEnd = cookie.indexOf(';', sessionStart);
  if (sessionEnd == -1) {
    sessionEnd = cookie.length();
  }
  
  String sessionId = cookie.substring(sessionStart + 8, sessionEnd);
  
  // البحث عن اسم المستخدم للجلسة
  for (int i = 0; i < sessionCount; i++) {
    // التحقق من صلاحية الجلسة ومطابقة معرف الجلسة
    if (sessions[i].expiry > millis() && sessions[i].sessionId == sessionId) {
      return sessions[i].username;
    }
  }
  
  return "";
}

String generateToken() {
  String token = "";
  for (int i = 0; i < 16; i++) {
    token += char(random(65, 90)); // A-Z
  }
  return token;
}

void cleanExpiredSessions() {
  unsigned long currentTime = millis();
  int validSessionCount = 0;
  
  // تحديد الجلسات الصالحة
  for (int i = 0; i < sessionCount; i++) {
    if (sessions[i].expiry > currentTime) {
      // إذا كانت الجلسة صالحة، نقلها إلى بداية المصفوفة
      if (i != validSessionCount) {
        sessions[validSessionCount] = sessions[i];
      }
      validSessionCount++;
    }
  }
  
  // تحديث عدد الجلسات
  if (validSessionCount != sessionCount) {
    Serial.print("Cleaned expired sessions. Active sessions: ");
    Serial.print(validSessionCount);
    Serial.print(" (was: ");
    Serial.print(sessionCount);
    Serial.println(")");
    sessionCount = validSessionCount;
  }
}