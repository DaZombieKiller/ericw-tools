/*
    Copyright (C) 1996-1997  Id Software, Inc.
    Copyright (C) 1997       Greg Lewis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#pragma once

#include <unordered_map>
#include <vector>
#include <list>
#include "common/cmdlib.hh"
#include "common/fs.hh"

// Texture data stored for quick searching
struct texture_t
{
    char name[16];
    int width, height;
};

// WAD Format
struct wadinfo_t
{
    char identification[4]; // should be WAD2
    int numlumps;
    int infotableofs;
};

struct lumpinfo_t
{
    int filepos;
    int disksize;
    int size; // uncompressed
    char type;
    char compression;
    char pad1, pad2;
    char name[16]; // must be null terminated
};

struct wad_t
{
    wadinfo_t header;
    int version;
    std::unordered_map<std::string, lumpinfo_t, case_insensitive_hash, case_insensitive_equal> lumps;
    std::unordered_map<std::string, texture_t, case_insensitive_hash, case_insensitive_equal> textures;
    qfile_t file = {nullptr, nullptr};
};

void WADList_Init(const char *wadstring);
void WADList_Process();
const texture_t *WADList_GetTexture(const char *name);
// for getting a texture width/height
