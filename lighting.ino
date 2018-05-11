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

void loop() {
  int v[N_INPUTS];
  
  for (int i = 0; i < N_INPUTS; i++) {
     int raw = map(analogRead(analog_pins[i]), 1023, 0, 0, 1023);
     v[i] = compute_avg(&avgs[i], raw);
     Serial.print(v[i]);
     Serial.print(' ');
  }

  Serial.print('\n');

  // ColorKey
  DmxSimple.write(1, map(v[0], 0, 1023, 0, 255));
  DmxSimple.write(2, map(v[1], 0, 1023, 0, 255));
  DmxSimple.write(3, map(v[2], 0, 1023, 0, 255));
  DmxSimple.write(4, 255);

  // ADJ
  DmxSimple.write(5, map(v[0], 0, 1023, 0, 255));
  DmxSimple.write(6, map(v[1], 0, 1023, 0, 255));
  DmxSimple.write(7, map(v[2], 0, 1023, 0, 255));
  DmxSimple.write(8, 0);

  // wait for the ADC to settle (at least 2 ms):
  delay(10);
}
