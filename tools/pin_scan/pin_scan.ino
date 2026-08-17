#include <OneWire.h>

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("=== D0 (GPIO1) DS18B20 TEST ===");
  pinMode(1, INPUT_PULLUP);
}

void loop() {
  OneWire ow(1);
  int pres = 0;
  for (int r = 0; r < 5; r++) pres += ow.reset();
  Serial.printf("presence pulses: %d/5\n", pres);
  if (pres == 0) { Serial.println("no device answering\n"); delay(2000); return; }

  ow.skip();
  ow.write(0x44, 1);
  delay(760);
  if (!ow.reset()) { Serial.println("lost device during conversion\n"); return; }
  ow.skip();
  ow.write(0xBE);
  byte d[9];
  for (int i = 0; i < 9; i++) d[i] = ow.read();
  bool crc = OneWire::crc8(d, 8) == d[8];
  int16_t raw = (d[1] << 8) | d[0];
  Serial.printf("scratchpad crc=%d raw=%d temp=%.2f C\n", crc, raw, raw / 16.0f);
  delay(2000);
}
