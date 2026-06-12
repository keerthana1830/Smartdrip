# SmartDrip

Small ESP32-based drip monitor that counts drops, detects low fluid level, uploads telemetry to Firebase Realtime Database, and sends alerts to Telegram.
 
 
Files:

- `SmartDrip.ino` — main sketch
- `wiring_table.md` — pin mapping and connection notes
- `secrets.example.h` — example file showing where to put credentials (do not commit `secrets.h`)

Quick setup

1. Copy `secrets.example.h` to `secrets.h` and fill in your WiFi, Firebase, and Telegram credentials.
2. (Recommended) Obtain the root CA certificate (PEM) for `api.telegram.org` and paste it into `TELEGRAM_ROOT_CERT` in `secrets.h`. This enables TLS verification.
3. Open the project in Arduino IDE or PlatformIO, compile and upload to an ESP32 board.

Notes and security

- Do NOT commit `secrets.h`. A `.gitignore` entry is included.
- The sketch will fall back to an insecure TLS mode if no certificate is provided — avoid this in production.

Wiring
See `wiring_table.md` for pin mapping and connection notes.

If you want, I can generate a `secrets.h` from your inputs or help you fetch the Telegram root certificate.
