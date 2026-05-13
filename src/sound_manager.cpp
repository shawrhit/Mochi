// sound_manager.cpp — Non-blocking melody player for piezo buzzer.
// Parses "note duration rest" triplet strings and plays them
// asynchronously via updateMelody() called from the main loop.

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

void SoundManager::stopMelody() {
  melody_ = nullptr;
  melodyCursor_ = nullptr;
  stage_ = 0;
  tokenIndex_ = 0;
  durationMs_ = 0;
  restMs_ = 0;
  frequency_ = 0;
  nextAtMs_ = 0;
  melodyActive_ = false;
  if (buzzerPin_ >= 0) {
    noTone(buzzerPin_);
  }
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
