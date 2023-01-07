/**
* Copyright (c) 2011-2014 - Ashita Development Team
*
* Ashita is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Ashita is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Ashita.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ASHITA_DeepsDefines_H_INCLUDED__
#define __ASHITA_DeepsDefines_H_INCLUDED__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif 

#define RBUFP(p, pos) (((uint8_t*)(p)) + (pos))
#define Read8(p, pos) (*(uint8_t*)RBUFP((p), (pos)))
#define Read16(p, pos) (*(uint16_t*)RBUFP((p), (pos)))
#define Read32(p, pos) (*(uint32_t*)RBUFP((p), (pos)))
#define Read64(p, pos) (*(uint64_t*)RBUFP((p), (pos)))
#define ReadFloat(p, pos) (*(float_t*)RBUFP((p), (pos)))

#define WBUFP(p, pos) (((uint8_t*)(p)) + (pos))
#define Write8(p, pos) (*(uint8_t*)WBUFP((p), (pos)))
#define Write16(p, pos) (*(uint16_t*)WBUFP((p), (pos)))
#define Write32(p, pos) (*(uint32_t*)WBUFP((p), (pos)))
#define Write64(p, pos) (*(uint64_t*)WBUFP((p), (pos)))
#define WriteFloat(p, pos) (*(float_t*)WBUFP((p), (pos)))

enum REACTION
{
    REACTION_NONE  = 0x00,
    REACTION_MISS  = 0x01,
    REACTION_PARRY = 0x03,
    REACTION_BLOCK = 0x04,
    REACTION_HIT   = 0x08,
    REACTION_EVADE = 0x09,
    REACTION_HIT2  = 0x10,
    REACTION_GUARD = 0x14
};

enum SPECEFFECT
{
    SPECEFFECT_NONE         = 0x00,
    SPECEFFECT_BLOOD        = 0x02,
    SPECEFFECT_HIT          = 0x10,
    SPECEFFECT_RAISE        = 0x11,
    SPECEFFECT_RECOIL       = 0x20,
    SPECEFFECT_CRITICAL_HIT = 0x22
};


// holds details about a specific damage source
struct damage_t
{
    uint64_t total;
    uint32_t max;
    uint32_t min;
    uint32_t count;

    damage_t()
    {
        total = 0;
        max   = 0;
        min   = 0;
        count = 0;
    }
    bool operator>(const damage_t& o) const
    {
        return (count > o.count);
    }
    uint32_t avg()
    {
        return count > 0 ? (total / count) : 0;
    }
};

// Container for an attack that tracks hit, miss, crit, evade, parry as damage types
struct source_t
{
    std::string name;
    std::map<const char*, damage_t> damage;

    source_t()
    {
    }

    uint64_t total() const
    {
        uint64_t tot = 0;
        for (const auto d : damage)
        {
            tot += d.second.total;
        }
        return tot;
    }
    // Return the total number of times this damage source was used
    uint64_t getCount() const
    {
        uint64_t count = 0;
        for (const auto d : damage)
        {
            count += d.second.count;
        }
        return count;
    }
    // Return the count if this damage source is "Miss", otherwise return 0
    uint64_t getMissed() const
    {
        uint64_t missed = 0;
        for (const auto d : damage)
        {
            if (d.first == "Miss")
            {
                missed = d.second.count;
            }
        }
        return missed;
    }

    bool operator>(const source_t& o) const
    {
        return (total() > o.total());
    }
};

struct entitysources_t
{
    std::string name; // Player name
    uint32_t color;   // A player's bar color
    uint32_t id;
    uint32_t ownerid;   // Player's pet ID if they have one
    std::map<uint32_t, source_t> sources;

    // Returns total damage dealt
    uint64_t total() const
    {
        int64_t total = 0;
        for (auto s : sources)
        {
            total += s.second.total();
        }
        return total;
    }

    // Returns overall hitrate as a percentage
    float hitrate() const
    {
        uint64_t totalCount = 0;
        uint64_t totalMiss  = 0;
        for (auto s : sources)
        {
            if (s.second.name != "Skillchain" && s.second.name != "Pet")
            {
                totalCount += s.second.getCount();
                totalMiss += s.second.getMissed();
            }
        }
        return totalCount == 0 ? 0.0f : 100 * ((float)(totalCount - totalMiss) / (float)totalCount);
    }
    bool operator==(const entitysources_t& o) const
    {
        return (total() == o.total());
    }

    bool operator>(const entitysources_t& o) const
    {
        return (total() > o.total());
    }
};

static const std::vector<D3DCOLOR> JobColors = {
    D3DCOLOR_ARGB(0xFF, 0x19, 0x19, 0x70), // NON: Midnight blue
    D3DCOLOR_ARGB(0xFF, 0xFF, 0x00, 0x00), // WAR: Red
    D3DCOLOR_ARGB(0xFF, 0xFF, 0x8C, 0x00), // MNK: Dark orange
    D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0xFF), // WHM: White
    D3DCOLOR_ARGB(0xFF, 0x4B, 0x00, 0x82), // BLM: Indigo
    D3DCOLOR_ARGB(0xFF, 0xFF, 0x69, 0xB4), // RDM: Pink
    D3DCOLOR_ARGB(0xFF, 0x22, 0x8B, 0x22), // THF: Forest green
    D3DCOLOR_ARGB(0xFF, 0xD3, 0xD3, 0xD3), // PLD: Light grey
    D3DCOLOR_ARGB(0xFF, 0x44, 0x44, 0x44), // DRK: Dark grey
    D3DCOLOR_ARGB(0xFF, 0x8B, 0x45, 0x13), // BST: Saddle brown
    D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0x00), // BRD: Yellow
    D3DCOLOR_ARGB(0xFF, 0xAD, 0xFF, 0x2F), // RNG: Green yellow
    D3DCOLOR_ARGB(0xFF, 0x80, 0x00, 0x00), // SAM: Maroon
    D3DCOLOR_ARGB(0xFF, 0x70, 0x80, 0x90), // NIN: Slate grey
    D3DCOLOR_ARGB(0xFF, 0x93, 0x70, 0xDB), // DRG: Medium purple
    D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0xFF), // SMN: Cyan
    D3DCOLOR_ARGB(0xFF, 0x41, 0x69, 0xE1), // BLU: Royal blue
    D3DCOLOR_ARGB(0xFF, 0xFF, 0xD8, 0xB1), // COR Apricot
    D3DCOLOR_ARGB(0xFF, 0xFF, 0xFA, 0xC8), // PUP Beige
    D3DCOLOR_ARGB(0xFF, 0xE6, 0x19, 0x4B), // DNC Red
    D3DCOLOR_ARGB(0xFF, 0xCD, 0x85, 0x3F), // SCH Peru
    D3DCOLOR_ARGB(0xFF, 0x80, 0x80, 0x00), // GEO Olive
    D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00)  // RUN PLACEHOLDER
};

static const std::vector<D3DCOLOR> RandomColors = {
    D3DCOLOR_ARGB(255, 12, 0, 155),
    D3DCOLOR_ARGB(255, 140, 0, 0),
    D3DCOLOR_ARGB(255, 255, 177, 32),
    D3DCOLOR_ARGB(255, 143, 143, 143),
    D3DCOLOR_ARGB(255, 68, 68, 68),
    D3DCOLOR_ARGB(255, 255, 0, 0),
    D3DCOLOR_ARGB(255, 0, 164, 49),
    D3DCOLOR_ARGB(255, 198, 198, 0),
    D3DCOLOR_ARGB(255, 116, 0, 145),
    D3DCOLOR_ARGB(255, 165, 153, 10),
    D3DCOLOR_ARGB(255, 184, 128, 10),
    D3DCOLOR_ARGB(255, 224, 0, 230),
    D3DCOLOR_ARGB(255, 234, 100, 0),
    D3DCOLOR_ARGB(255, 119, 0, 0),
    D3DCOLOR_ARGB(255, 130, 17, 255),
    D3DCOLOR_ARGB(255, 79, 196, 0),
    D3DCOLOR_ARGB(255, 0, 16, 217),
    D3DCOLOR_ARGB(255, 136, 68, 0),
    D3DCOLOR_ARGB(255, 244, 98, 0),
    D3DCOLOR_ARGB(255, 15, 190, 220),
    D3DCOLOR_ARGB(255, 0, 123, 145)
};

static const std::vector<uint16_t> hitMessages   = {1, 2, 77, 132, 157, 161, 163, 185, 187, 197, 227, 264, 281, 317, 352, 413, 522, 576, 577};
static const std::vector<uint16_t> critMessages  = {67, 252, 265, 274, 353, 379};
static const std::vector<uint16_t> missMessages  = {15, 85, 158, 188, 245, 284, 324, 354};
static const std::vector<uint16_t> evadeMessages = {14, 30, 31, 32, 33, 189, 248, 282, 283, 323, 355};
static const std::vector<uint16_t> parryMessages = {69, 70};
#endif // __ASHITA_DeepsDefines_H_INCLUDED__
