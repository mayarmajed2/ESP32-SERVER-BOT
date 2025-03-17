#include "utils.h"
#include "../config.h"
#include "../auth/auth.h"
#include "../tasks/tasks.h"
#include "../notifications/notifications.h"

// External variables
extern WebServer server;

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

void handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  
  // Check if path requires authentication
  if ((path.indexOf("dashboard") >= 0 || path.indexOf("settings") >= 0) && !isAuthenticated()) {
    server.sendHeader("Location", "/login");
    server.send(302);
    return;
  }
  
  String contentType = getContentType(path);
  
  if (SD.exists(path)) {
    File file = SD.open(path, FILE_READ);
    server.streamFile(file, contentType);
    file.close();
  } else {
    handleNotFound();
  }
}

void handleNotFound() {
  if (SD.exists("/404.html")) {
    File file = SD.open("/404.html", FILE_READ);
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "404: Not Found");
  }
}

void setupRoutes() {
  // Static files
  server.on("/", HTTP_GET, []() {
    handleFileRead("/index.html");
  });
  
  server.on("/login", HTTP_GET, []() {
    handleFileRead("/login.html");
  });
  
  server.on("/register", HTTP_GET, []() {
    handleFileRead("/register.html");
  });
  
  server.on("/dashboard", HTTP_GET, []() {
    if (!isAuthenticated()) {
      server.sendHeader("Location", "/login");
      server.send(302);
      return;
    }
    handleFileRead("/dashboard.html");
  });
  
  server.on("/forgot-password", HTTP_GET, []() {
    handleFileRead("/forgot_password.html");
  });
  
  // API endpoints
  server.on("/api/login", HTTP_POST, handleLogin);
  server.on("/api/register", HTTP_POST, handleRegister);
  server.on("/api/change-password", HTTP_POST, handleChangePassword);
  server.on("/api/reset-password-request", HTTP_POST, handleResetPasswordRequest);
  server.on("/api/reset-password", HTTP_POST, handleResetPassword);
  server.on("/reset-password", HTTP_GET, handleResetPassword);
  
  // Task management
  server.on("/api/tasks", HTTP_POST, handleScheduleTasks);
  server.on("/api/tasks", HTTP_GET, handleGetTasks);
  server.on("/api/tasks/delete", HTTP_POST, handleDeleteTask);
  
  // Notification settings
  server.on("/api/notifications/settings", HTTP_POST, handleNotificationSettings);
  server.on("/api/notifications/settings", HTTP_GET, handleGetNotificationSettings);
  
  // Handle not found
  server.onNotFound(handleNotFound);
}

void setupServer() {
  server.begin();
  Serial.println("HTTP server started");
}

void setupSDCard() {
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");
  
  // Create default users file if it doesn't exist
  if (!SD.exists(USERS_FILE)) {
    File usersFile = SD.open(USERS_FILE, FILE_WRITE);
    if (usersFile) {
      usersFile.println("[]");
      usersFile.close();
      Serial.println("Created default users file");
    }
  }
  
  // Create default tasks file if it doesn't exist
  if (!SD.exists(TASKS_FILE)) {
    File tasksFile = SD.open(TASKS_FILE, FILE_WRITE);
    if (tasksFile) {
      tasksFile.println("[]");
      tasksFile.close();
      Serial.println("Created default tasks file");
    }
  }
  
  // Create default notifications file if it doesn't exist
  if (!SD.exists(NOTIFICATIONS_FILE)) {
    File notificationsFile = SD.open(NOTIFICATIONS_FILE, FILE_WRITE);
    if (notificationsFile) {
      notificationsFile.println("{}");
      notificationsFile.close();
      Serial.println("Created default notifications file");
    }
  }
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();
  
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(timeinfo.tm_hour);
  Serial.print(":");
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  Serial.println(timeinfo.tm_sec);
}