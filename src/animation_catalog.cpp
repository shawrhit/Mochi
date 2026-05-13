// animation_catalog.cpp — Animation playlist registry.

#include "animation_catalog.h"
#include "custom_animations.h"

const Animation kPlaylist[] = {
  {glow_allArray, glow_frames_count},
  {sleepy_allArray, sleepy_frames_count},
  {love_allArray, love_frames_count},
  {headlights_allArray, headlights_frames_count},
  {greeting_allArray, greeting_frames_count},
  {bored_allArray, bored_frames_count},
  {evil_allArray, evil_frames_count},
  {rev_allArray, rev_frames_count},
  {yawn_allArray, yawn_frames_count},
  {playful_allArray, playful_frames_count},
  {pingpong_allArray, pingpong_frames_count}
};

const int kPlaylistCount = sizeof(kPlaylist) / sizeof(kPlaylist[0]);
