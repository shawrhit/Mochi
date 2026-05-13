// sound_manager.h — Non-blocking melody player for piezo buzzer.

#pragma once

class SoundManager {
 public:
  SoundManager();

  void begin(int buzzerPin);
  void startMelody(const char* melody);
  void stopMelody();
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

  int noteToFrequency(const char* note) const;
  bool nextToken();
};
