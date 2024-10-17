#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wiringPi.h>

#define LED_PIN 27 // GPIO 16 (WiringPi pin 27)

#define DOT_DURATION 20 // Duration of a dot in milliseconds
#define DASH_DURATION (DOT_DURATION * 3)
#define SYMBOL_SPACE (DOT_DURATION)
#define LETTER_SPACE (DOT_DURATION * 3)
#define WORD_SPACE (DOT_DURATION * 7)

// Morse code representation
const char *morseTable[36] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", // A-J
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",   // K-T
    "..-", "...-", ".--", "-..-", "-.--", "--..",                         // U-Z
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----." // 0-9
};

void flashDot() {
    digitalWrite(LED_PIN, HIGH);
    delay(DOT_DURATION);
    digitalWrite(LED_PIN, LOW);
    delay(SYMBOL_SPACE);
}

void flashDash() {
    digitalWrite(LED_PIN, HIGH);
    delay(DASH_DURATION);
    digitalWrite(LED_PIN, LOW);
    delay(SYMBOL_SPACE);
}

void flashMorse(const char *morse) {
    for (int i = 0; morse[i] != '\0'; i++) {
        if (morse[i] == '.') {
            flashDot();
	    printf(".");
        } else if (morse[i] == '-') {
            flashDash();
	    printf("-");
        }
    }
}

void sendMorse(const char *message, int times) {
    for (int t = 0; t < times; t++) {
        for (int i = 0; message[i] != '\0'; i++) {
            if (message[i] == ' ') {
                delay(WORD_SPACE);
		printf("/ ");
            } else {
                if (message[i] >= 'a' && message[i] <= 'z') {
                    flashMorse(morseTable[message[i] - 'a']);
                } else if (message[i] >= 'A' && message[i] <= 'Z') {
                    flashMorse(morseTable[message[i] - 'A']);
                } else if (message[i] >= '0' && message[i] <= '9') {
                    flashMorse(morseTable[message[i] - '0' + 26]);
                }
                delay(LETTER_SPACE);
		printf(" ");
            }
        }
        delay(WORD_SPACE + 1000);
	printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <repeat_count> <message>\n", argv[0]);
        return 1;
    }

    int times = atoi(argv[1]);
    char *message = argv[2];

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "WiringPi setup failed\n");
        return 1;
    }

    pinMode(LED_PIN, OUTPUT);
    sendMorse(message, times);

    return 0;
}

