#define FASTLED_INTERNAL
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

enum states {UP, DOWN};
int state;
int16_t place_in_span;

int hue;

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

  if (mode < 256) {
    // Colour fade
    int hue1 = map(v[1], 0, 1023, 0, 255);
    int spd = map(v[2], 0, 1023, 2048, 32);
    int hue2 = map(v[3], 0, 1023, 0, 255);

    if (hue1 > hue2) {
      int swap = hue1;
      hue1 = hue2;
      hue2 = swap;
    }

    int16_t scaled_span = (hue2 - hue1) * 128;

    if (state == UP) {
      place_in_span += scaled_span / spd;
    } else if (state == DOWN) {
      place_in_span -= scaled_span / spd;
    } else {
      // shouldn't happen, so let's fix it
      state = UP;
    }

    if (place_in_span > scaled_span) {
      state = DOWN;
    } else if (place_in_span < 0) {
      state = UP;
    }

    hue = hue1 + place_in_span / 256;
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
}
