#include "animation_catalog.h"
#include "custom_animations.h"

const Animation kPlaylist[] = {
  {sleepy_allArray, sleepy_frames_count},
  {love_allArray, love_frames_count},
  {evil_allArray, evil_frames_count},
  {yawn_allArray, yawn_frames_count},
  {playful_allArray, playful_frames_count},
  {greeting_allArray, greeting_frames_count},
  {pingpong_allArray, pingpong_frames_count}
};

const int kPlaylistCount = sizeof(kPlaylist) / sizeof(kPlaylist[0]);
