# ESP32 Server Bot Features

This project includes several advanced features that make it a comprehensive remote control and monitoring system. Below is an explanation of the main features and their benefits:

## 1. Authentication and Security System

### User Management
- **Registration**: Ability to create new accounts with checks for duplicate usernames.
- **Login**: Verifying user credentials and establishing a secure session.
- **Session Management**: Storing session information and tracking its validity.
- **Benefit**: Protecting the system from unauthorized access and providing a personalized experience for each user.

### Expired Session Cleanup
- Automatic cleanup of expired sessions every 5 minutes.
- **Benefit**: Improves system performance, reduces memory consumption, and prevents session arrays from overflowing.

### Password Reset
- Ability to request a password reset through a temporary link.
- Sends the reset link via the Telegram bot.
- **Benefit**: Provides a secure way to regain access to the account in case of forgotten passwords.

## 2. Task Scheduling and Reminders

### Scheduled Task Creation
- Ability to create tasks with specified execution times.
- Supports daily, weekly, or specific day repetitions.
- **Benefit**: Automates processes and ensures execution at specified times.

### Automatic Task Checking
- Check scheduled tasks every minute to determine if tasks need to be executed.
- **Benefit**: Ensures tasks are executed on time accurately.

### Task Management
- Display, modify, and delete scheduled tasks.
- **Benefit**: Provides flexibility in managing and adjusting tasks as needed.

## 3. Notification System

### Telegram Notifications
- Sends notifications via the Telegram bot for important events.
- **Benefit**: Keeps you informed about important events from anywhere.

### Notification Customization
- Ability to customize the types of notifications the user wants to receive.
- **Benefit**: Reduces unnecessary notifications and focuses attention on important events.

### Security Notifications
- Notifications for failed login attempts and password changes.
- **Benefit**: Enhances system security and provides early detection of potential intrusions.

## 4. Integrated Web Interface

### Easy-to-Use Interface
- A simple, user-friendly web interface for controlling the system.
- **Benefit**: Makes system management easy from any web browser.

### Responsive Design
- A responsive design that works across various screen sizes.
- **Benefit**: Enables system use from mobile devices and tablets.

## 5. Data Storage

### SD Card Storage
- All data is stored on an SD card in JSON files.
- **Benefit**: Ensures data continuity even after the device is restarted.

### Automatic Backup
- Ability to create automatic backups of important data.
- **Benefit**: Protects data from loss in case of SD card failure.

## 6. System Monitoring

### Startup Notifications
- Sends a notification when the system starts with information about the IP address and available memory.
- **Benefit**: Ensures the system is functioning correctly after a reboot.

### Resource Monitoring
- Monitors memory usage and system performance.
- **Benefit**: Detects potential issues before they affect system performance.

## 7. Compatibility and Expandability

### Modular Project Structure
- Code is organized into separate modules (auth, tasks, notifications, utils).
- **Benefit**: Makes it easy to add new features or modify existing ones.

### Comprehensive Documentation
- Full documentation for the code, features, and usage.
- **Benefit**: Makes it easy to understand, maintain, and develop the system.
