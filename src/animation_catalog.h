// animation_catalog.h — Frame-based animation data structure and playlist.

#pragma once

struct Animation {
  const unsigned char* const* frames;
  int frameCount;
};

extern const Animation kPlaylist[];
extern const int kPlaylistCount;
