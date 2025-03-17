#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <time.h>

// Function declarations
void handleScheduleTasks();
void handleGetTasks();
void handleDeleteTask();
void checkScheduledTasks();

#endif