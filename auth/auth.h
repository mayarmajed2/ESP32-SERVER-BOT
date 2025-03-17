#ifndef AUTH_H
#define AUTH_H

#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Session structure
struct Session {
  String username;
  String sessionId;
  unsigned long expiry;
};

// Function declarations
void handleLogin();
void handleRegister();
void handleChangePassword();
void handleResetPasswordRequest();
void handleResetPassword();
void sendPasswordResetLink(String email);
bool verifyUser(String email, String password);
bool userExists(String email);
bool addUser(String username, String password, String email);
String getAllUsers();
bool isAuthenticated();
String getSessionUsername();
String generateToken();
void cleanExpiredSessions(); // إضافة إعلان الدالة الجديدة

#endif