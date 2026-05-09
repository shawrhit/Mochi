#include "sound_manager.h"

#include <Arduino.h>
#include <math.h>

SoundManager::SoundManager()
    : buzzerPin_(-1),
      melody_(nullptr),
      melodyCursor_(nullptr),
      stage_(0),
      tokenIndex_(0),
      durationMs_(0),
      restMs_(0),
      frequency_(0),
      nextAtMs_(0),
      melodyActive_(false) {}

void SoundManager::begin(int buzzerPin) {
  buzzerPin_ = buzzerPin;
  pinMode(buzzerPin_, OUTPUT);
  digitalWrite(buzzerPin_, LOW);
}

void SoundManager::toneFor(int frequencyHz, int durationMs) const {
  if (buzzerPin_ < 0) {
    return;
  }
  tone(buzzerPin_, frequencyHz, durationMs);
  delay(durationMs);
}

int SoundManager::noteToFrequency(const char* note) const {
  if (note == nullptr || note[0] == '\0') {
    return 0;
  }

  char letter = note[0];
  int octave = 0;
  bool sharp = false;

  if (note[1] == '\0') {
    return 0;
  }

  octave = note[1] - '0';
  if (octave < 0 || octave > 9) {
    return 0;
  }

  if (note[2] == 'S') {
    sharp = true;
  }

  int semitone = 0;
  switch (letter) {
    case 'C': semitone = 0; break;
    case 'D': semitone = 2; break;
    case 'E': semitone = 4; break;
    case 'F': semitone = 5; break;
    case 'G': semitone = 7; break;
    case 'A': semitone = 9; break;
    case 'B': semitone = 11; break;
    default: return 0;
  }

  if (sharp) {
    semitone += 1;
  }

  const int midi = (octave + 1) * 12 + semitone;
  const double freq = 440.0 * pow(2.0, (midi - 69) / 12.0);
  return static_cast<int>(freq + 0.5);
}

void SoundManager::playMelody(const char* melody) const {
  if (melody == nullptr) {
    return;
  }

  char token[8] = {0};
  int tokenIndex = 0;
  int stage = 0;
  int durationMs = 0;
  int restMs = 0;
  int frequency = 0;

  for (const char* p = melody; *p != '\0'; ++p) {
    char c = *p;
    if (c == ',' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      if (tokenIndex == 0) {
        continue;
      }

      token[tokenIndex] = '\0';
      if (stage == 0) {
        frequency = noteToFrequency(token);
      } else if (stage == 1) {
        durationMs = atoi(token);
      } else if (stage == 2) {
        restMs = atoi(token);
      }

      stage++;
      tokenIndex = 0;

      if (stage >= 3) {
        if (frequency > 0 && durationMs > 0) {
          toneFor(frequency, durationMs);
        }
        if (restMs > 0) {
          delay(restMs);
        }
        stage = 0;
        durationMs = 0;
        restMs = 0;
        frequency = 0;
      }
      continue;
    }

    if (tokenIndex < static_cast<int>(sizeof(token) - 1)) {
      token[tokenIndex++] = c;
    }
  }
}

void SoundManager::startMelody(const char* melody) {
  melody_ = melody;
  melodyCursor_ = melody;
  stage_ = 0;
  tokenIndex_ = 0;
  durationMs_ = 0;
  restMs_ = 0;
  frequency_ = 0;
  nextAtMs_ = 0;
  melodyActive_ = (melody_ != nullptr);
}

bool SoundManager::nextToken() {
  if (melodyCursor_ == nullptr) {
    return false;
  }

  while (*melodyCursor_ != '\0') {
    const char c = *melodyCursor_;
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != ',') {
      break;
    }
    ++melodyCursor_;
  }

  if (*melodyCursor_ == '\0') {
    return false;
  }

  tokenIndex_ = 0;
  while (*melodyCursor_ != '\0') {
    const char c = *melodyCursor_;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',') {
      break;
    }
    if (tokenIndex_ < static_cast<int>(sizeof(token_) - 1)) {
      token_[tokenIndex_++] = c;
    }
    ++melodyCursor_;
  }

  token_[tokenIndex_] = '\0';
  return tokenIndex_ > 0;
}

bool SoundManager::updateMelody() {
  if (!melodyActive_) {
    return false;
  }

  const unsigned long now = millis();
  if (now < nextAtMs_) {
    return true;
  }

  while (stage_ < 3) {
    if (!nextToken()) {
      melodyActive_ = false;
      return false;
    }

    if (stage_ == 0) {
      frequency_ = noteToFrequency(token_);
    } else if (stage_ == 1) {
      durationMs_ = atoi(token_);
    } else if (stage_ == 2) {
      restMs_ = atoi(token_);
    }

    stage_++;
  }

  if (buzzerPin_ >= 0 && frequency_ > 0 && durationMs_ > 0) {
    tone(buzzerPin_, frequency_, durationMs_);
  }

  nextAtMs_ = now + static_cast<unsigned long>(durationMs_ + restMs_);
  stage_ = 0;
  durationMs_ = 0;
  restMs_ = 0;
  frequency_ = 0;
  return true;
}

bool SoundManager::isMelodyActive() const { return melodyActive_; }

void SoundManager::playStartupMelody() const {
  playMelody("G4 100 20, C5 100 20, E5 100 20, G5 100 20, C6 100 20, D6 100 20, E6 200 200");
}

void SoundManager::playTouchMelody() const {
  toneFor(880, 70);
  toneFor(1175, 90);
}

void SoundManager::playNotificationMelody() const {
  toneFor(740, 90);
  toneFor(740, 90);
  toneFor(988, 140);
}
