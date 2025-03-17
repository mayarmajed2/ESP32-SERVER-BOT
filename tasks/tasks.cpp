#include "tasks.h"
#include "../config.h"
#include "../auth/auth.h"
#include "../notifications/notifications.h"

// External variables
extern WebServer server;
extern UniversalTelegramBot bot;

void handleScheduleTasks() {
  if (!isAuthenticated()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  
  String username = getSessionUsername();
  String taskName = server.arg("task_name");
  String taskTime = server.arg("task_time"); // Format: HH:MM
  String taskAction = server.arg("task_action"); // on/off
  String taskDevice = server.arg("task_device"); // device identifier
  String taskDays = server.arg("task_days"); // Format: 1,2,3,4,5,6,7 (days of week)
  
  if (username == "" || taskName == "" || taskTime == "" || taskAction == "" || taskDevice == "") {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }
  
  // Validate time format
  if (taskTime.length() != 5 || taskTime.charAt(2) != ':') {
    server.send(400, "text/plain", "Invalid time format. Use HH:MM");
    return;
  }
  
  int hour = taskTime.substring(0, 2).toInt();
  int minute = taskTime.substring(3, 5).toInt();
  
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    server.send(400, "text/plain", "Invalid time values");
    return;
  }
  
  // Load existing tasks
  File tasksFile = SD.open(TASKS_FILE, FILE_READ);
  DynamicJsonDocument doc(16384);
  
  if (tasksFile) {
    DeserializationError error = deserializeJson(doc, tasksFile);
    tasksFile.close();
    
    if (error) {
      doc.clear();
      doc.to<JsonArray>();
    }
  } else {
    doc.to<JsonArray>();
  }
  
  // Add new task
  JsonArray tasks = doc.as<JsonArray>();
  JsonObject newTask = tasks.createNestedObject();
  newTask["name"] = taskName;
  newTask["time"] = taskTime;
  newTask["action"] = taskAction;
  newTask["device"] = taskDevice;
  newTask["username"] = username;
  newTask["created_at"] = millis() / 1000;
  newTask["days"] = taskDays;
  newTask["enabled"] = true;
  
  // Save updated tasks file
  SD.remove(TASKS_FILE);
  File newTasksFile = SD.open(TASKS_FILE, FILE_WRITE);
  if (!newTasksFile) {
    server.send(500, "text/plain", "Failed to open tasks file for writing");
    return;
  }
  
  if (serializeJson(doc, newTasksFile) == 0) {
    newTasksFile.close();
    server.send(500, "text/plain", "Failed to write to tasks file");
    return;
  }
  
  newTasksFile.close();
  
  // Send notification
  String message = "⏰ New Task Scheduled!\n";
  message += "User: " + username + "\n";
  message += "Task: " + taskName + "\n";
  message += "Time: " + taskTime + "\n";
  message += "Action: " + taskAction + " " + taskDevice + "\n";
  message += "Days: " + taskDays;
  bot.sendMessage(CHAT_ID, message, "");
  
  server.send(200, "text/plain", "Task scheduled successfully");
}

void handleGetTasks() {
  if (!isAuthenticated()) {
    server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
    return;
  }
  
  String username = getSessionUsername();
  
  // Load tasks
  File tasksFile = SD.open(TASKS_FILE, FILE_READ);
  if (!tasksFile) {
    server.send(200, "application/json", "[]");
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, tasksFile);
  tasksFile.close();
  
  if (error) {
    server.send(500, "application/json", "{\"error\":\"Failed to parse tasks file\"}");
    return;
  }
  
  // Filter tasks for current user
  JsonArray allTasks = doc.as<JsonArray>();
  DynamicJsonDocument userTasksDoc(16384);
  JsonArray userTasks = userTasksDoc.to<JsonArray>();
  
  for (JsonObject task : allTasks) {
    if (task["username"] == username) {
      JsonObject userTask = userTasks.createNestedObject();
      userTask["name"] = task["name"];
      userTask["time"] = task["time"];
      userTask["action"] = task["action"];
      userTask["device"] = task["device"];
      userTask["days"] = task["days"];
      userTask["enabled"] = task["enabled"];
      userTask["created_at"] = task["created_at"];
    }
  }
  
  String response;
  serializeJson(userTasksDoc, response);
  server.send(200, "application/json", response);
}

void handleDeleteTask() {
  if (!isAuthenticated()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  
  String username = getSessionUsername();
  String taskName = server.arg("task_name");
  
  if (username == "" || taskName == "") {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }
  
  // Load tasks
  File tasksFile = SD.open(TASKS_FILE, FILE_READ);
  if (!tasksFile) {
    server.send(404, "text/plain", "No tasks found");
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, tasksFile);
  tasksFile.close();
  
  if (error) {
    server.send(500, "text/plain", "Failed to parse tasks file");
    return;
  }
  
  // Find and remove task
  JsonArray tasks = doc.as<JsonArray>();
  bool taskFound = false;
  
  for (int i = 0; i < tasks.size(); i++) {
    if (tasks[i]["username"] == username && tasks[i]["name"] == taskName) {
      tasks.remove(i);
      taskFound = true;
      break;
    }
  }
  
  if (!taskFound) {
    server.send(404, "text/plain", "Task not found");
    return;
  }
  
  // Save updated tasks file
  SD.remove(TASKS_FILE);
  File newTasksFile = SD.open(TASKS_FILE, FILE_WRITE);
  if (!newTasksFile) {
    server.send(500, "text/plain", "Failed to open tasks file for writing");
    return;
  }
  
  if (serializeJson(doc, newTasksFile) == 0) {
    newTasksFile.close();
    server.send(500, "text/plain", "Failed to write to tasks file");
    return;
  }
  
  newTasksFile.close();
  
  // Send notification
  String message = "🗑️ Task Deleted!\n";
  message += "User: " + username + "\n";
  message += "Task: " + taskName;
  bot.sendMessage(CHAT_ID, message, "");
  
  server.send(200, "text/plain", "Task deleted successfully");
}

void checkScheduledTasks() {
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  int currentDay = timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday; // Convert Sunday from 0 to 7
  
  String currentTime = (currentHour < 10 ? "0" : "") + String(currentHour) + ":" + 
                       (currentMinute < 10 ? "0" : "") + String(currentMinute);
  
  // Load tasks
  File tasksFile = SD.open(TASKS_FILE, FILE_READ);
  if (!tasksFile) {
    return;
  }
  
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, tasksFile);
  tasksFile.close();
  
  if (error) {
    return;
  }
  
  // Check each task
  JsonArray tasks = doc.as<JsonArray>();
  for (JsonObject task : tasks) {
    if (task["time"] == currentTime && task["enabled"].as<bool>()) {
      // Check if task should run on current day
      String days = task["days"].as<String>();
      if (days != "") {
        bool runOnDay = false;
        int commaPos = -1;
        int nextCommaPos = days.indexOf(',');
        
        while (nextCommaPos != -1 || commaPos != -1) {
          String dayStr;
          if (nextCommaPos != -1) {
            dayStr = days.substring(commaPos + 1, nextCommaPos);
            commaPos = nextCommaPos;
            nextCommaPos = days.indexOf(',', commaPos + 1);
          } else {
            dayStr = days.substring(commaPos + 1);
            commaPos = -1;
          }
          
          if (dayStr.toInt() == currentDay) {
            runOnDay = true;
            break;
          }
        }
        
        if (!runOnDay) {
          continue;
        }
      }
      
      // Execute task
      String taskName = task["name"];
      String taskAction = task["action"];
      String taskDevice = task["device"];
      String username = task["username"];
      
      // Here you would add code to control actual devices
      // For now, we'll just send a notification
      
      String message = "🔔 Executing Scheduled Task!\n";
      message += "User: " + username + "\n";
      message += "Task: " + taskName + "\n";
      message += "Action: " + taskAction + " " + taskDevice;
      bot.sendMessage(CHAT_ID, message, "");
      
      // Send notification to user based on their preferences
      sendUserNotification(username, "task_execution", message);
    }
  }
}