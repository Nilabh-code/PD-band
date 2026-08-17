#pragma once
#include <SD.h>
#include <SPI.h>
#include "config.h"

namespace Store {

SPIClass* spi = nullptr;
bool mounted = false;
File logFile;
uint32_t fileIndex = 0;

bool begin() {
  spi = new SPIClass();
  spi->begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  for (int i = 0; i < 3; i++) {
    if (SD.begin(PIN_SD_CS, *spi, 20000000)) break;
    delay(100);
  }
  mounted = SD.cardSize() > 0;
  return mounted;
}

size_t cardMB() { return mounted ? SD.cardSize() / (1024 * 1024) : 0; }
size_t cardFreeMB() { return mounted ? (SD.cardSize() - SD.usedBytes()) / (1024 * 1024) : 0; }

void rotate() {
  if (logFile) logFile.close();
  String name = "/log_" + String(fileIndex++) + ".csv";
  logFile = SD.open(name, FILE_WRITE);
  if (logFile) logFile.println("epoch,uptime_ms,temp_c,g_rms,band_rms,band_max,freeze_score,cv_pct,dom_freq,dom_amp,flags");
}

bool ensureFile() {
  if (!mounted) return false;
  if (!logFile || logFile.position() > 500000) { rotate(); return (bool)logFile; }
  return true;
}

void append(const Sample& s) {
  if (!ensureFile()) return;
  logFile.printf("%ld,%lu,%.2f,%.3f,%.3f,%.3f,%.2f,%.1f,%.2f,%.3f,%u\n",
                 (long)s.epoch, (unsigned long)s.uptime_ms, s.temp_c,
                 s.g_rms, s.tremor_band, s.band_max, s.freeze_score,
                 s.cv_pct, s.dom_freq, s.dom_amp, s.flags);
  logFile.flush();
}

void appendAlert(AlertType t, float temp, float band, const char* detail) {
  if (!mounted) return;
  File f = SD.open("/alerts.csv", FILE_APPEND);
  if (f) {
    f.printf("%ld,%d,%.2f,%.3f,%s\n", (long)time(nullptr), (int)t, temp, band, detail);
    f.close();
  }
}

}
