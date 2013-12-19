#include <Arduino.h>

#define DOWNLOAD_PIN     5
#define UPLOAD_PIN       6
#define NO_DATA_PIN      10
#define HAS_DOWNLOAD_PIN 11
#define HAS_UPLOAD_PIN   12
#define FACTOR_PIN       7

#define DOWNLOAD_LIMIT_PIN A1
#define UPLOAD_LIMIT_PIN   A0

#define SERIAL_SPEED 115200

#define MAX_NO_DATA  5000


inline unsigned getFactor() {
	return ( digitalRead(FACTOR_PIN) == HIGH ? 1000 : 100 );
}

inline unsigned mapRate(unsigned rate) {
	return map(rate, 0, getFactor(), 0, 255);
}

inline unsigned mapLimit(unsigned rate) {
	return map(rate, 0, 1023, 0, getFactor());
}

inline unsigned readUnsigned() {
	unsigned char low = lowByte(Serial.read());
	unsigned char high = highByte(Serial.read());
	return makeWord(high, low);
}

void displaySpeed(unsigned download, unsigned upload) {
	analogWrite(DOWNLOAD_PIN, mapRate(download));
	analogWrite(UPLOAD_PIN, mapRate(upload));
}

void displayActives(unsigned char download, unsigned char upload) {
	digitalWrite(HAS_DOWNLOAD_PIN, download ? HIGH : LOW);
	digitalWrite(HAS_UPLOAD_PIN, upload ? HIGH : LOW);
}

void sendSpeedLimits() {
	int download = mapLimit(analogRead(DOWNLOAD_LIMIT_PIN));
	Serial.write(lowByte(download));
	Serial.write(highByte(download));

	int upload = mapLimit(analogRead(UPLOAD_LIMIT_PIN));
	Serial.write(lowByte(upload));
	Serial.write(highByte(upload));
}

void setup()
{
	Serial.begin(SERIAL_SPEED);

	analogWrite(DOWNLOAD_PIN, 0);
	analogWrite(UPLOAD_PIN, 0);

	pinMode(NO_DATA_PIN, OUTPUT);
	pinMode(FACTOR_PIN, INPUT);

	pinMode(HAS_DOWNLOAD_PIN, OUTPUT);
	pinMode(HAS_UPLOAD_PIN, OUTPUT);
}

void loop()
{
	static unsigned long last_data_time = 0;

	if ( Serial.available() >= 6 ) {
		unsigned download = readUnsigned();
		unsigned upload = readUnsigned();
		unsigned char d_state = Serial.read();
		unsigned char u_state = Serial.read();

		displaySpeed(download, upload);
		displayActives(d_state, u_state);

		last_data_time = millis();
		digitalWrite(NO_DATA_PIN, LOW);

		sendSpeedLimits();
	}

	unsigned long current_time = millis();
	if ( ( current_time >= last_data_time && current_time - last_data_time >= MAX_NO_DATA ) ||
		( current_time < last_data_time && ((unsigned long) -1) - last_data_time + current_time >= MAX_NO_DATA ) ) {
		digitalWrite(NO_DATA_PIN, HIGH);
	}

	delay(10);
}
