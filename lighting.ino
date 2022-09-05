// WARNING: FUN WEEKEND PROJECTS DO NOT REQUIRE CLEAN CODE!

#define FASTLED_INTERNAL
#include <FastLED.h>
#include <DmxSimple.h>

#define N_INPUTS 5  // note: HW supports 6 inputs as wired
static const uint8_t analog_pins[] = {A4,A3,A2,A1,A0};
#define SAMPLES 10
#define BAUD 57600
#define SEND_INTERVAL 1000 // ms

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

  Serial.begin(BAUD);
}

unsigned long last_send_time = 0;

enum states {UP, DOWN};
int state;
int16_t place_in_span;

uint8_t hue;
int sat;
int val;
uint16_t count;
int prev_step;
uint16_t doubleStrobeCounter;
int prev_spd;
CRGB rgb;

void loop() {
  int v[N_INPUTS];

  for (int i = 0; i < N_INPUTS; i++) {
     int raw = map(analogRead(analog_pins[i]), 1023, 0, 0, 1023);
     v[i] = compute_avg(&avgs[i], raw);
  }

  bool send = false;
  if (millis() - last_send_time >= SEND_INTERVAL) {
    send = true;
    last_send_time += SEND_INTERVAL;

    Serial.print("cat ");
    for (int i = 0; i < N_INPUTS; i++) {
      Serial.print(v[i]);
      Serial.print(' ');
    }
    Serial.print("meow\n");
  }

  int mode = v[0];

  if (mode < 8) {
    // gentle white
    hue = 255;
    sat = 0;
    val = 150;

    rgb = CHSV(hue, sat, val);
  } else if (mode < 256) {
    // Colour fade
    int hue1 = map(v[1], 0, 1023, 0, 255);
    int hue2 = map(v[2], 0, 1023, 0, 255);
    sat = map(v[3], 0, 1023, 0, 255);
    //sat = 255;
    int inc = map(v[4], 0, 1023, 16, 128);
    val = 255;

    if (hue1 > hue2) {
      int swap = hue1;
      hue1 = hue2;
      hue2 = swap;
    }

    int16_t scaled_span = (hue2 - hue1) * 128;

    if (state == UP) {
      place_in_span += inc;
    } else if (state == DOWN) {
      place_in_span -= inc;
    } else {
      // shouldn't happen, so let's fix it
      state = UP;
    }

    if (place_in_span > scaled_span) {
      place_in_span = scaled_span;
      state = DOWN;
    } else if (place_in_span < 0) {
      place_in_span = 0;
      state = UP;
    }

    hue = hue1 + place_in_span / 256;

    rgb = CHSV(hue, sat, val);
  } else if (mode < 640) {
    // Single colour strobe

    uint8_t set_hue = map(v[1], 0, 1023, 0, 255);
    int submode = v[2];
    sat = map(v[3], 0, 1023, 0, 255);
    int spd = constrain(map(v[4], 2, 1020, 66, 1), 1, 66);
    if (send) Serial.println(spd);

    int steps = 2;
    if (submode < 512) {
      // on/off ("in-phase")
      steps = 2;
    } else {
      // r/g/b ("out-of-phase")
      steps = 3;
    }

    if (spd != prev_spd) {
      count = count % (steps * prev_spd);
      prev_spd = spd;
    }

    count++;
    int step = (count % (steps * spd) / spd);

    if (submode > 256 and submode < 768) {
      // fixed hue
      hue = set_hue;
    } else if (step == 0 and prev_step != 0) {
      // increment hue (once per cycle)
      hue += set_hue;
    }

    if (submode < 512) {
      val = step == 0 ? 0 : 255;
      rgb = CHSV(hue, sat, val);
    } else {
      rgb = CHSV(hue, sat, 255);
      if (step == 0) {
        rgb.g = 0;
        rgb.b = 0;
      } else if (step == 1) {
        rgb.r = 0;
        rgb.b = 0;
      } else {
        rgb.r = 0;
        rgb.g = 0;
      }
    }

    prev_step = step;
  } else {
    // Dual colour strobe
    int spd = map(v[2], 0, 1023, 1, 75);
    int hue1 = map(v[1], 0, 1023, 0, 255);
    int hue2 = map(v[3], 0, 1023, 0, 255);

    if (spd != prev_spd) {
      doubleStrobeCounter = doubleStrobeCounter % (2*prev_spd);
      prev_spd = spd;
    }

    doubleStrobeCounter++;
    if((doubleStrobeCounter % (2*spd)) < spd) {
      hue = hue1;
    } else {
      hue = hue2;
    }

    sat = 255;
    val = 255;

    rgb = CHSV(hue, sat, val);
  }

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
