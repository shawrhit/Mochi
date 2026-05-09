#pragma once

struct Animation {
  const unsigned char* const* frames;
  int frameCount;
};

extern const Animation kPlaylist[];
extern const int kPlaylistCount;
