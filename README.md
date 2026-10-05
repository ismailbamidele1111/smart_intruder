Smart Intruder Security System

Overview

The Smart Intruder Security System is an IoT-enabled embedded security project designed to detect unauthorized entry and provide timely notifications to the user.

The system monitors intrusion events using sensors and communicates detected events to an online monitoring system.

Current Features

1. Intrusion Detection

The system uses sensors to detect possible unauthorized entry or movement.

When an intrusion is detected, the system records the event and triggers the notification system.

2. ntfy Notifications

The system currently uses **ntfy** to send intrusion notifications to the user's device.

When an intrusion event occurs, a notification can be sent through the configured ntfy topic.

Example notification:

🚨 Intrusion Detected!

The notification system allows the user to receive alerts without having to continuously monitor the dashboard.

3. Real-Time Monitoring Dashboard

The project also includes a dashboard for monitoring intrusion events.

The dashboard is designed to update when new intrusion events are detected and can display information such as:

- Intrusion status
- Date and time of the event
- Number of detected intrusion events
- Recent intrusion history
- Sensor/event information

This provides a centralized interface for monitoring the security system.

System Workflow

Sensors > ESP32 > Intrusion Detected > ntfy Notification > Online Dashboard > User's Device > Real-Time Monitoring

Hardware

1. ESP32
2. Reed switch
3. mmWave radar sensor
4. Buzzer
5. LEDs
6. Power supply
7. Other supporting electronic components

Software & Technologies
1. ESP32
2. Embedded C/C++
3. Arduino IDE
4. Wi-Fi
5. Firebase Realtime Database
6. ntfy
7. Web Dashboard
8. HTML
9. CSS
10. JavaScript
11. Data and Monitoring

When an intrusion is detected, the system can record information about the event.
Possible recorded information includes:

Event type
1. Date
2. Time
3. Sensor that detected the event
4. Intrusion status
The recorded information can then be displayed on the monitoring dashboard.

Project Status

Currently Implemented

1 Intrusion detection
2 ESP32-based monitoring
3 ntfy notification system
4 Online dashboard
5 Real-time/event-based monitoring
6 Firebase Realtime Database integration

Future Improvements

Possible future additions include:

1. SMS Notifications

Add GSM-based SMS alerts so that users can receive notifications even when internet connectivity is unavailable.

2. Phone Call Alerts

Add an automatic phone-call alert for critical intrusion events.

3. Camera Integration

Add an ESP32-CAM or another camera module to capture an image when an intrusion is detected.

4. Remote Camera Monitoring

Allow authorized users to view or request images from the security system remotely.

5. Multiple User Accounts

Add authentication so that multiple authorized users can monitor the system.

6. Event History and Analytics

Add more detailed historical records and statistics, such as:

Daily intrusion count
Weekly intrusion count
Most active sensor
Intrusion frequency
Event timelines
7. Multiple Security Zones

Allow multiple rooms, doors, windows, or areas to be monitored independently.

8. Battery Backup

Add battery backup so that the security system can continue operating during a power outage.

9. Local Alarm

Add a louder external siren and additional warning indicators.

10. AI-Based Intrusion Detection

Future versions could investigate machine-learning or computer-vision techniques to distinguish genuine intrusion events from false alarms.

Security Considerations

Sensitive information such as:

1. API keys
2. Firebase credentials
3. Passwords
4. Authentication tokens
5. Private configuration files

are kept out of this repository. To run the project:

1. Copy `secrets.example.h` to `secrets.h` and fill in your WiFi, Firebase and ntfy values (used by `smart_intruder.ino`).
2. Copy `firebase-config.example.js` to `firebase-config.js` and fill in your Firebase web config (used by `Dashboard.html`).

Both `secrets.h` and `firebase-config.js` are listed in `.gitignore`.


Deploying the dashboard to Vercel

1. On vercel.com, Add New > Project and import this repo. `vercel.json` already sets the build (`node build.js`) and output folder (`dist`).
2. Under Settings > Environment Variables, add `FIREBASE_API_KEY`, `FIREBASE_AUTH_DOMAIN`, `FIREBASE_DATABASE_URL` and `FIREBASE_PROJECT_ID` (the same values as `firebase-config.js`).
3. Deploy. `build.js` copies `Dashboard.html` to `dist/index.html` and generates `firebase-config.js` from those variables.
4. In Firebase Console > Authentication > Settings > Authorized domains, add your `.vercel.app` domain so sign-in works.

Author
Ridwan Ismail and Shehu Uthman