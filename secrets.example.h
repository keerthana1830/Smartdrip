#ifndef SECRETS_H
#define SECRETS_H


// WiFi
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Firebase Realtime Database
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET_OR_AUTH_TOKEN"

// Telegram
const char* TELEGRAM_BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN"; // e.g. 123456:ABCdef...
const char* TELEGRAM_CHAT_ID = "YOUR_CHAT_ID"; // numeric chat id or @username

// Optional: root CA certificate for api.telegram.org in PEM format. If provided, the
// sketch will use it for TLS verification instead of disabling verification.
// Example:
// const char* TELEGRAM_ROOT_CERT = "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n";

#endif // SECRETS_H
