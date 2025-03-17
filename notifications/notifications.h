#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>
#include <ArduinoJson.h>

// Function declarations
void handleNotificationSettings();
void handleGetNotificationSettings();
void sendUserNotification(String username, String notificationType, String message);

#endif