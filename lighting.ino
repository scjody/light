
#include <FastLED.h>
#include <DmxSimple.h>

#define N_INPUTS 6
static const uint8_t analog_pins[] = {A3,A2,A1,A0,A4,A5};
#define SAMPLES 10

typedef struct movingAvg {
  int values[SAMPLES];
  long sum;
  int pos;
  int samples_in;
} movingAvg;

movingAvg avgs[N_INPUTS] = {0};

int compute_avg(struct movingAvg *avg, int new_val) {
  avg->sum = avg->sum - avg->values[avg->pos] + new_val;
  avg->values[avg->pos] = new_val;
  avg->pos = (avg->pos + 1) % SAMPLES;
  if (avg->samples_in < SAMPLES) {
    avg->samples_in++;
  }
  return avg->sum / avg->samples_in;
}

void setup() {
  DmxSimple.usePin(3);  // double rainbow shield
  //DmxSimple.usePin(4);  // new, silver shield

  DmxSimple.maxChannel(8);

  Serial.begin(115200);

}

uint8_t cycle;
//enum states {UP, DOWN};
//int state;

void loop() {
  int v[N_INPUTS];
  
  for (int i = 0; i < N_INPUTS; i++) {
     int raw = map(analogRead(analog_pins[i]), 1023, 0, 0, 1023);
     v[i] = compute_avg(&avgs[i], raw);
     Serial.print(v[i]);
     Serial.print(' ');
  }

  Serial.print('\n');

  int mode = v[0];
  int hue;

  if (mode < 256) {
    // Colour fade
    int hue1 = map(v[1], 0, 1023, 0, 255);
    int speed = map(v[2], 0, 1023, 0, 255);
    int hue2 = map(v[3], 0, 1023, 0, 255);

    int step = hue1 - hue2;

    if (cycle < 128) {
      hue = hue1 - step * cycle / 128;
    } else {
      hue = hue2 + step * (cycle - 128) / 128;
    }
  } else if (mode < 640) {
    // Single colour strobe
    int spd = v[1];
    int hue = v[2];
    int sat = v[3];
  } else {
    // Dual colour strobe
    int spd = v[1];
    int hue1 = v[2];
    int hue2 = v[3];
  }

  const CRGB& rgb = CHSV(hue, 255, 255);

  // ColorKey
  DmxSimple.write(1, rgb.r);
  DmxSimple.write(2, rgb.g);
  DmxSimple.write(3, rgb.b);
  DmxSimple.write(4, 255);

  // ADJ
  DmxSimple.write(5, rgb.r);
  DmxSimple.write(6, rgb.g);
  DmxSimple.write(7, rgb.b);
  DmxSimple.write(8, 0);

  // wait for the ADC to settle (at least 2 ms):
  delay(2);

  cycle++;
}
