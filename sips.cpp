// Copyright 2019 leoetlino <leo@leolam.fr>
// Licensed under GPLv2+

#include <array>
#include <cstdio>
#include <vector>

#include "common_types.h"
#include "file.h"

constexpr std::array<char, 5> IPS_MAGIC{{'I', 'P', 'S', '3', '2'}};
constexpr std::array<char, 4> IPS_EOF{{'E', 'E', 'O', 'F'}};
constexpr size_t IPS_MAX_FILE_SIZE = 0xffffffff;
constexpr size_t IPS_MAX_HUNK_SIZE = 0xffff;

enum class BuildHunkState {
  LookingForMismatch,
  LookingForMismatchEnd,
};

struct PatchHunk {
  size_t offset;
  size_t length;
};

template <typename WriteHunkFn>
static void buildHunks(const std::vector<u8>& original, const std::vector<u8>& patched,
                       WriteHunkFn writeHunk) {
  BuildHunkState state = BuildHunkState::LookingForMismatch;
  size_t mismatchIdx = -1;

  for (size_t i = 0; i < original.size(); ++i) {
    switch (state) {
    case BuildHunkState::LookingForMismatch:
      if (original[i] != patched[i]) {
        mismatchIdx = i;
        state = BuildHunkState::LookingForMismatchEnd;
      }
      break;

    case BuildHunkState::LookingForMismatchEnd:
      if (original[i] == patched[i]) {
        writeHunk(PatchHunk{mismatchIdx, i - mismatchIdx});
        state = BuildHunkState::LookingForMismatch;
      }
      break;
    }
  }
  if (state == BuildHunkState::LookingForMismatchEnd)
    writeHunk(PatchHunk{mismatchIdx, original.size() - mismatchIdx});
}

int main(int argc, const char** argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: sips ORIGINAL_FILE PATCHED_FILE PATCH\n");
    return 1;
  }

  util::File originalFile{argv[1], "rb"};
  util::File patchedFile{argv[2], "rb"};
  util::File patchFile{argv[3], "wb"};

  if (!originalFile || !patchedFile || !patchFile) {
    fprintf(stderr, "error: failed to open original and/or patched file\n");
    return 1;
  }

  if (originalFile.size() > IPS_MAX_FILE_SIZE) {
    fprintf(stderr, "error: original file is too large (>= 0x100000000 bytes)\n");
    return 1;
  }

  if (originalFile.size() != patchedFile.size()) {
    fprintf(stderr, "error: original file and patched file must have the same size\n");
    return 1;
  }

  // Read both files at the same time -- not going to bother with chunking reads
  // as executables are not that large.
  std::vector<u8> original(originalFile.size());
  std::vector<u8> patched(patchedFile.size());

  if (!originalFile.read(original.data(), original.size()) ||
      !patchedFile.read(patched.data(), patched.size())) {
    fprintf(stderr, "error: failed to read original and/or patched file\n");
    return 1;
  }

  // Generate the patch.
  patchFile.writeArray(IPS_MAGIC.data(), IPS_MAGIC.size());
  buildHunks(original, patched, [&](const PatchHunk hunk) {
    fprintf(stderr, "hunk: offset 0x%zx length %zu\n", hunk.offset, hunk.length);

    size_t offset = hunk.offset;
    size_t length = hunk.length;

    // Avoid generating hunks with offset 0x45454f46 because it could be misinterpreted as patch EOF.
    // Yes, the format is that bad.
    if (offset == 0x45454f46) {
      --offset;
      ++length;
    }

    // We don't care about RLE considering this is only used for patching executables.

    while (length) {
      const size_t chunkLength = std::min(IPS_MAX_HUNK_SIZE, length);

      patchFile.writeValue<u32>(offset);
      patchFile.writeValue<u16>(chunkLength);
      patchFile.writeArray(&patched[offset], chunkLength);

      offset += chunkLength;
      length -= chunkLength;
    }
  });
  patchFile.writeArray(IPS_EOF.data(), IPS_EOF.size());

  return 0;
}
