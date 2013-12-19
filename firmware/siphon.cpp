#include <Arduino.h>

//#define DEBUG

#define DOWNLOAD_PIN     5
#define UPLOAD_PIN       6
#define NO_DATA_PIN      10
#define HAS_DOWNLOAD_PIN 11
#define HAS_UPLOAD_PIN   12
#define FACTOR_PIN       7

#define DOWNLOAD_LIMIT_PIN A1
#define UPLOAD_LIMIT_PIN   A0

#define SERIAL_SPEED 115200
#define CMD_BUF_SIZE 16

#define MAX_NO_DATA  5000


inline int getFactor() {
	return ( digitalRead(FACTOR_PIN) == HIGH ? 1000 : 100 );
}

inline int mapRate(unsigned char rate) {
	return map(rate, 0, getFactor(), 0, 255);
}

inline int mapLimit(int rate) {
	return map(rate, 0, 1023, 0, getFactor());
}

void cmdSetDownload(const char *rate) {
	analogWrite(DOWNLOAD_PIN, mapRate((unsigned char)rate[0]));
}

void cmdSetUpload(const char *rate) {
	analogWrite(UPLOAD_PIN, mapRate((unsigned char)rate[0]));
}

void cmdSetHasDownload(const char *state) {
	digitalWrite(HAS_DOWNLOAD_PIN, (unsigned char)state[0] ? HIGH : LOW);
}

void cmdSetHasUpload(const char *state) {
	digitalWrite(HAS_UPLOAD_PIN, (unsigned char)state[0] ? HIGH : LOW);
}

void cmdGetLimits(void) {
	Serial.print("D=");
	Serial.print(mapLimit(analogRead(DOWNLOAD_LIMIT_PIN)));
	Serial.print(" U=");
	Serial.println(mapLimit(analogRead(UPLOAD_LIMIT_PIN)));
}

void setup(void)
{
	Serial.begin(SERIAL_SPEED);

	analogWrite(DOWNLOAD_PIN, 0);
	analogWrite(UPLOAD_PIN, 0);

	pinMode(NO_DATA_PIN, OUTPUT);
	pinMode(FACTOR_PIN, INPUT);

	pinMode(HAS_DOWNLOAD_PIN, OUTPUT);
	pinMode(HAS_UPLOAD_PIN, OUTPUT);
}

void loop(void)
{
	static char cmd_buf[CMD_BUF_SIZE];
	static int cmd_buf_pos = 0;
	static int msg_size = 0;
	static unsigned long last_data_time = 0;
	char ch;

	if ( Serial.available() ) {
		if ( msg_size == 0 ) {
			msg_size = Serial.read();
		} else {
			if ( cmd_buf_pos < sizeof(cmd_buf) - 1 && cmd_buf_pos <= msg_size ) {
				cmd_buf[cmd_buf_pos] = Serial.read();
				++cmd_buf_pos;
			}

			if ( cmd_buf_pos > msg_size ) {
				cmd_buf[cmd_buf_pos] = 0;

# ifdef DEBUG
				Serial.print("I: CMD received (len 1+");
				Serial.print(msg_size);
				Serial.print("): ");
				Serial.println(cmd_buf);
# endif // DEBUG

				cmd_buf_pos = 0;
				msg_size = 0;

				switch ( cmd_buf[0] ) {
					case 'D' : cmdSetDownload(cmd_buf + 1);    break;
					case 'U' : cmdSetUpload(cmd_buf + 1);      break;
					case 'd' : cmdSetHasDownload(cmd_buf + 1); break;
					case 'u' : cmdSetHasUpload(cmd_buf + 1);   break;
					case 'L' : cmdGetLimits(); break;
				}
				last_data_time = millis();
				digitalWrite(NO_DATA_PIN, LOW);
			}
		}
	}

	unsigned long current_time = millis();
	if ( ( current_time >= last_data_time && current_time - last_data_time >= MAX_NO_DATA ) ||
		( current_time < last_data_time && ((unsigned long) -1) - last_data_time + current_time >= MAX_NO_DATA ) ) {
		digitalWrite(NO_DATA_PIN, HIGH);
	}

	delay(100);
}
