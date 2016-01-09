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
	return ( digitalRead(FACTOR_PIN) == HIGH ? 100 : 1000 );
}

inline unsigned mapRate(unsigned rate) {
	return map(rate, 0, getFactor(), 0, 255);
}

inline unsigned mapLimit(unsigned rate) {
	return map(rate, 0, 1023, 0, getFactor());
}


bool processCommand(unsigned char *cmd) {
	switch (cmd[0]) {
		case 0: Serial.println("siphon"); return false;

		case 1: analogWrite(DOWNLOAD_PIN, mapRate(makeWord(cmd[2], cmd[1]))); return true;  // high, low
		case 2: analogWrite(UPLOAD_PIN, mapRate(makeWord(cmd[2], cmd[1]))); return true;

		case 3: digitalWrite(HAS_DOWNLOAD_PIN, cmd[1] ? HIGH : LOW); return true;
		case 4: digitalWrite(HAS_UPLOAD_PIN, cmd[1] ? HIGH : LOW); return true;

		case 5: {
			int download = mapLimit(analogRead(DOWNLOAD_LIMIT_PIN));
			Serial.write(lowByte(download));
			Serial.write(highByte(download));
		}; return true;

		case 6: {
			int upload = mapLimit(analogRead(UPLOAD_LIMIT_PIN));
			Serial.write(lowByte(upload));
			Serial.write(highByte(upload));
		}; return true;

		default: break;
	}
	return false;
}


void setup() {
	Serial.begin(SERIAL_SPEED);

	analogWrite(DOWNLOAD_PIN, 0);
	analogWrite(UPLOAD_PIN, 0);

	pinMode(NO_DATA_PIN, OUTPUT);
	pinMode(FACTOR_PIN, INPUT);

	pinMode(HAS_DOWNLOAD_PIN, OUTPUT);
	pinMode(HAS_UPLOAD_PIN, OUTPUT);
}

void loop() {
	static unsigned long last_data_time = 0;
    unsigned char cmd[8];

    if ( Serial.available() >= 8 ) {
        for (int count = 0; count < 8; ++count) {
            cmd[count] = Serial.read();
        }
        if (processCommand(cmd)) {
            last_data_time = millis();
            digitalWrite(NO_DATA_PIN, LOW);
        }
    }

	unsigned long current_time = millis();
	if ( ( current_time >= last_data_time && current_time - last_data_time >= MAX_NO_DATA ) ||
		( current_time < last_data_time && ((unsigned long) -1) - last_data_time + current_time >= MAX_NO_DATA ) ) {
		digitalWrite(NO_DATA_PIN, HIGH);
	}

	delay(10);
}
