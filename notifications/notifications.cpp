#include "notifications.h"
#include "../config.h"
#include "../auth/auth.h"

// External variables
extern WebServer server;
extern UniversalTelegramBot bot;

void handleNotificationSettings() {
  if (!isAuthenticated()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  
  String username = getSessionUsername();
  bool notifyLogin = server.arg("notify_login") == "true";
  bool notifyTasks = server.arg("notify_tasks") == "true";
  bool notifyFailedLogin = server.arg("notify_failed_login") == "true";
  
  // Load notification settings
  File notificationsFile = SD.open(NOTIFICATIONS_FILE, FILE_READ);
  DynamicJsonDocument doc(16384);
  
  if (notificationsFile) {
    DeserializationError error = deserializeJson(doc, notificationsFile);
    notificationsFile.close();
    
    if (error) {
      doc.clear();
      doc.to<JsonObject>();
    }
  } else {
    doc.to<JsonObject>();
  }
  
  // Update settings for user
  JsonObject settings = doc.as<JsonObject>();
  JsonObject userSettings;
  
  if (settings.containsKey(username)) {
    userSettings = settings[username];
  } else {
    userSettings = settings.createNestedObject(username);
  }
  
  userSettings["notify_login"] = notifyLogin;
  userSettings["notify_tasks"] = notifyTasks;
  userSettings["notify_failed_login"] = notifyFailedLogin;
  
  // Save updated settings
  SD.remove(NOTIFICATIONS_FILE);
  File newNotificationsFile = SD.open(NOTIFICATIONS_FILE, FILE_WRITE);
  if (!newNotificationsFile) {
    server.send(500, "text/plain", "Failed to open notifications file for writing");
    return;
  }
  
  if (serializeJson(doc, newNotificationsFile) == 0) {
    newNotificationsFile.close();
    server.send(500, "text/plain", "Failed to write to notifications file");
    return;
  }
  
  newNotificationsFile.close();
  
  // Send notification
  String message = "⚙️ Notification Settings Updated!\n";
  message += "User: " + username + "\n";
  message += "Login notifications: " + String(notifyLogin ? "Enabled" : "Disabled") + "\n";
  message += "Task notifications: " + String(notifyTasks ? "Enabled" : "Disabled") + "\n";
  message += "Failed login notifications: " + String(notifyFailedLogin ? "Enabled" : "Disabled");
  bot.sendMessage(CHAT_ID, message, "");
  
  server.send(200, "text/plain", "Notification settings updated successfully");
}

void handleGetNotificationSettings() {
  if (!isAuthenticated()) {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  String username = getSessionUsername();
  
  // Load notification settings
  File notificationsFile = SD.open(NOTIFICATIONS_FILE, FILE_READ);
  if (!notificationsFile) {
    // Return default settings if file doesn't exist
    server.send(200, "application/json", "{\"notify_login\":true,\"notify_tasks\":true,\"notify_failed_login\":true}");
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, notificationsFile);
  notificationsFile.close();
  
  if (error) {
    server.send(500, "application/json", "{\"error\":\"Failed to parse notifications file\"}");
    return;
  }
  
  JsonObject settings = doc.as<JsonObject>();
  
  // If user has no settings, return default settings
  if (!settings.containsKey(username)) {
    server.send(200, "application/json", "{\"notify_login\":true,\"notify_tasks\":true,\"notify_failed_login\":true}");
    return;
  }
  
  // Return user's settings
  JsonObject userSettings = settings[username];
  DynamicJsonDocument responseDoc(1024);
  
  responseDoc["notify_login"] = userSettings.containsKey("notify_login") ? userSettings["notify_login"].as<bool>() : true;
  responseDoc["notify_tasks"] = userSettings.containsKey("notify_tasks") ? userSettings["notify_tasks"].as<bool>() : true;
  responseDoc["notify_failed_login"] = userSettings.containsKey("notify_failed_login") ? userSettings["notify_failed_login"].as<bool>() : true;
  
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
}

void sendUserNotification(String username, String notificationType, String message) {
  // Load notification settings
  File notificationsFile = SD.open(NOTIFICATIONS_FILE, FILE_READ);
  if (!notificationsFile) {
    // If file doesn't exist, use default settings (send all notifications)
    bot.sendMessage(CHAT_ID, message, "");
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, notificationsFile);
  notificationsFile.close();
  
  if (error) {
    // If error parsing file, use default settings
    bot.sendMessage(CHAT_ID, message, "");
    return;
  }
  
  JsonObject settings = doc.as<JsonObject>();
  
  // If user has no settings, use default settings
  if (!settings.containsKey(username)) {
    bot.sendMessage(CHAT_ID, message, "");
    return;
  }
  
  JsonObject userSettings = settings[username];
  
  // Check notification type and user preferences
  if (notificationType == "login" && (!userSettings.containsKey("notify_login") || userSettings["notify_login"].as<bool>())) {
    bot.sendMessage(CHAT_ID, message, "");
  } else if (notificationType == "task_execution" && (!userSettings.containsKey("notify_tasks") || userSettings["notify_tasks"].as<bool>())) {
    bot.sendMessage(CHAT_ID, message, "");
  } else if (notificationType == "failed_login" && (!userSettings.containsKey("notify_failed_login") || userSettings["notify_failed_login"].as<bool>())) {
    bot.sendMessage(CHAT_ID, message, "");
  }
}