# Smart Helmet-SOS Alert
This prototype is designed to detect accidents and alert emergency contacts. It uses an
Arduino board, SIM800 GSM module, and GPS module to achieve the following:
1. ️ Continuous Monitoring: Tracks accelerometer data from an MPU-6050 sensor.
2. 🔊 Alert System: When the accelerometer values exceed a set threshold, the system
activates a buzzer to alert the rider.
3. 📞 Emergency Call: If the rider doesn’t turn off the buzzer within 20 seconds, the
system considers it an accident and makes an emergency call.
4. 📍 Location Notification: After disconnecting the call, it sends an SMS with a
Google Maps link containing the rider's location.