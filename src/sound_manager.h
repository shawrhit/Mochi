#pragma once

class SoundManager {
 public:
  SoundManager();

  void begin(int buzzerPin);
  void playStartupMelody() const;
  void playTouchMelody() const;
  void playNotificationMelody() const;
  void playMelody(const char* melody) const;
  void startMelody(const char* melody);
  bool updateMelody();
  bool isMelodyActive() const;

 private:
  int buzzerPin_;
  const char* melody_;
  const char* melodyCursor_;
  int stage_;
  char token_[8];
  int tokenIndex_;
  int durationMs_;
  int restMs_;
  int frequency_;
  unsigned long nextAtMs_;
  bool melodyActive_;

  void toneFor(int frequencyHz, int durationMs) const;
  int noteToFrequency(const char* note) const;
  bool nextToken();
};
