# ESP32 Server with Telegram Bot

This project sets up a simple **ESP32-based web server** that allows users to **register** and **login** through a **web form**. The ESP32 is also connected to a **Telegram bot**, which sends notifications to a specified Telegram **group** when certain actions are triggered (such as login attempts or new registrations).

## Features

- **ESP32 Web Server**: A web server running on the ESP32 that serves an HTML form for user registration and login.
- **Telegram Bot Integration**: Sends notifications to a Telegram bot when users log in or register.
- **SD Card Integration**: Stores user data in a **JSON file** on an SD card connected to the ESP32.
- **Wi-Fi Connection**: The ESP32 connects to a specified Wi-Fi network to enable the web server and Telegram bot communication.
- **Port Forwarding**: The web server is accessible externally through port forwarding.

## Hardware Requirements

- **ESP32 Board**
- **Micro SD Card Module**
- **Micro SD Card**
- **Power Supply for the ESP32**

## Software Requirements

- **Arduino IDE** with ESP32 board support
- **Libraries**:
  - WebServer library
  - SD library
  - WiFi library
  - UniversalTelegramBot library
  - ArduinoJson library

## Installation Instructions

1. **Set up the Arduino IDE**:
   - Install the **ESP32 Board** package in Arduino IDE.
   - Install the required libraries from the Library Manager: **WebServer**, **WiFi**, **UniversalTelegramBot**, **ArduinoJson**, and **SD**.

2. **Configure the Wi-Fi settings**:
   - Replace the `ssid` and `password` values with your own Wi-Fi credentials in the Arduino code.

3. **Upload the code to your ESP32**:
   - Connect your ESP32 to your computer and select the correct **board** and **port** in the Arduino IDE.
   - Upload the provided code to your ESP32.

4. **Set up your Telegram Bot**:
   - Create a new bot on Telegram by talking to **BotFather** and get your **bot token**.
   - Replace the `BOT_TOKEN` and `CHAT_ID` values in the code with your actual bot token and chat ID.

5. **Set up Port Forwarding**:
   - Configure your router to forward the appropriate port (typically port `80`) to the IP address of your ESP32.
   - This allows external access to your web server.

6. **Access the Web Server**:
   - After uploading the code, the ESP32 will connect to your Wi-Fi.
   - Open the Serial Monitor in the Arduino IDE to get the **ESP32 IP address**.
   - Open the IP address in a web browser on your computer or phone to access the registration and login form.

7. **Monitor Notifications**:
   - When a user registers or logs in, notifications will be sent to your Telegram group.

## Example Usage

1. **Register** a new user using the web form.
2. **Login** with the registered username and password.
3. **Receive** Telegram notifications about successful or failed login attempts.

## Notes

- Make sure your **Telegram Bot** is set up properly and has permission to send messages to your group.
- The **SD card** will store the **users' data** in a `users.json` file.
- Ensure that port forwarding is correctly set up to make the server accessible externally.
