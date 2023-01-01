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

#ifndef __ASHITA_Deeps_H_INCLUDED__
#define __ASHITA_Deeps_H_INCLUDED__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

enum REACTION
{
    REACTION_NONE = 0x00,
    REACTION_MISS = 0x01,
    REACTION_PARRY = 0x03,
    REACTION_BLOCK = 0x04,
    REACTION_HIT = 0x08,
    REACTION_EVADE = 0x09,
    REACTION_HIT2 = 0x10,
    REACTION_GUARD = 0x14
};

enum SPECEFFECT
{
    SPECEFFECT_NONE = 0x00,
    SPECEFFECT_BLOOD = 0x02,
    SPECEFFECT_HIT = 0x10,
    SPECEFFECT_RAISE = 0x11,
    SPECEFFECT_RECOIL = 0x20,
    SPECEFFECT_CRITICAL_HIT = 0x22
};

/**
 * @brief Required includes for an extension.
 */
#include "C:\code\Ashita-v4beta\plugins\sdk\Ashita.h"
#include <list>
#include <map>
#include <functional>
#include <stdint.h>

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
        max = 0;
        min = 0;
        count = 0;
    }
    bool operator > (const damage_t& o) const
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
        for (auto d : damage)
        {
            tot += d.second.total;
        }
        return tot;
    }
    // Return the total number of times this damage source was used
    uint64_t getCount() const
    {
        uint64_t count = 0;
        for (auto d : damage)
        {
            count += d.second.count;
        }
        return count;
    }
    // Return the count if this damage source is "Miss", otherwise return 0
    uint64_t getMissed() const
    {
        uint64_t missed = 0;
        for (auto d : damage)
        {
            if (d.first == "Miss")
            {
                missed = d.second.count;
            }
        }
        return missed;
    }

    bool operator > (const source_t& o) const
    {
        return (total() > o.total());
    }
};

struct entitysources_t
{
    std::string name; // Player name
    uint32_t color; // A player's bar color
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
        uint64_t totalMiss = 0;
        for (auto s : sources)
        {
            totalCount += s.second.getCount();
            totalMiss += s.second.getMissed();
        }
        return 100*((float)(totalCount - totalMiss)/(float)totalCount);
    }
    bool operator == (const entitysources_t& o) const
    {
        return (total() == o.total());
    }

    bool operator > (const entitysources_t& o) const
    {
        return (total() > o.total());
    }
};

std::map<uint32_t, entitysources_t> entities;
std::map<IFontObject*, std::string> clickMap;

static const std::vector<uint16_t> hitMessages = { 1, 2, 77, 132, 157, 161, 163, 185, 187, 197, 227, 264, 281, 317, 352, 413, 522, 576, 577 };
static const std::vector<uint16_t> critMessages = { 67, 252, 265, 274, 353, 379 };
static const std::vector<uint16_t> missMessages = { 15, 85, 158, 188, 245, 284, 324, 354 };
static const std::vector<uint16_t> evadeMessages = { 14, 30, 31, 32, 33, 189, 248, 282, 283, 323, 355 };
static const std::vector<uint16_t> parryMessages = { 69, 70 };


static const std::vector<D3DCOLOR> Colors = {
        D3DCOLOR_ARGB(0xFF, 0xFF, 0x00, 0x00), // WAR: Red
        D3DCOLOR_ARGB(0xFF, 0xFF, 0x8C, 0x00), // MNK: Dark orange
        D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0xFF), // WHM: White
        D3DCOLOR_ARGB(0xFF, 0x4B, 0x00, 0x82), // BLM: Indigo
        D3DCOLOR_ARGB(0xFF, 0xFF, 0x69, 0xB4), // RDM: Pink
        D3DCOLOR_ARGB(0xFF, 0x22, 0x8B, 0x22), // THF: Forest green
        D3DCOLOR_ARGB(0xFF, 0xD3, 0xD3, 0xD3), // PLD: Light grey
        D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00), // DRK: Black
        D3DCOLOR_ARGB(0xFF, 0x8B, 0x45, 0x13), // BST: Saddle brown
        D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0x00), // BRD: Yellow
        D3DCOLOR_ARGB(0xFF, 0xAD, 0xFF, 0x2F), // RNG: Green yellow
        D3DCOLOR_ARGB(0xFF, 0x80, 0x00, 0x00), // SAM: Maroon
        D3DCOLOR_ARGB(0xFF, 0x70, 0x80, 0x90), // NIN: Slate grey
        D3DCOLOR_ARGB(0xFF, 0x93, 0x70, 0xDB), // DRG: Medium purple
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0xFF), // SMN: Cyan
        D3DCOLOR_ARGB(0xFF, 0x41, 0x69, 0xE1), // BLU: Royal blue
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00), // COR PLACEHOLDER
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00), // PUP PLACEHOLDER
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00), // DNC PLACEHOLDER
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00), // SCH PLACEHOLDER
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00), // GEO PLACEHOLDER
        D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00)  // RUN PLACEHOLDER
    };

void g_onClick(Ashita::MouseEvent, void*, int32_t, int32_t);

/**
 * @brief Global copy of our plugin data.
 */
IPluginBase* g_PluginInfo = NULL;

/**
 * @brief Our Main Plugin Class
 *
 * @note    The main class of your plugin MUST use PluginBase as a base class. This is the
 *          internal base class that Ashita uses to communicate with your plugin!
 */
class Deeps : IPlugin
{
    /**
     * @brief Internal class variables.
     */
	IAshitaCore*          m_AshitaCore;
	ILogManager* 	      m_LogManager;
	DWORD                 m_PluginId;
	IDirect3DDevice8*     m_Direct3DDevice;
	std::list<void*>	  m_Packets;
    std::clock_t          m_LastRender;

private:
    source_t* getDamageSource(entitysources_t* entityInfo, uint8_t actionType, uint16_t actionID);
    bool updateDamageSource(source_t* source, uint16_t message, uint32_t damage);
    void repairBars(IFontObject* deepsBase, uint8_t size);
    void report(char mode, int max);
    void Direct3DRelease(void);
    uint16_t getIndex(std::function<bool(IEntity*, int)>);
    uint32_t m_charInfo;
    std::string m_sourceInfo;
    uint8_t m_bars;
    bool m_debug;

public:
    /**
     * @brief Constructor and deconstructor.
     */
    Deeps(void);
    virtual ~Deeps(void);


    /**
     * @brief PluginBase virtual overrides.
     */
    uint32_t GetFlags(void) const;
    const char* GetName(void) const;
    double GetVersion(void) const;
    const char* GetAuthor(void) const;
    const char* GetDescription(void) const;
    bool Initialize(IAshitaCore* core, ILogManager* log, uint32_t id);
    void Release(void);
    bool HandleCommand(int32_t mode, const char* command, bool injected);
	bool HandleIncomingText(int32_t mode, bool indent, const char* message, int32_t* modifiedMode, bool* modifiedIndent, char* modifiedMessage, bool injected, bool blocked);
	bool HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked);
	bool HandleOutgoingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked);
    bool Direct3DInitialize(IDirect3DDevice8* device);
    //void Direct3DEndScene(bool isRenderingBackBuffer);
    //void Direct3DBeginScene(bool isRenderingBackBuffer);
    void Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
    void onClick(Ashita::MouseEvent, IFontObject*, int32_t, int32_t);
};

// Global pointer to this

Deeps* g_Deeps = NULL;

/**
 * @brief Required Plugin Exports
 */
__declspec(dllexport) double     __stdcall expGetInterfaceVersion(void);
__declspec(dllexport) IPlugin*   __stdcall expCreatePlugin(const char* args);

#endif // __ASHITA_Deeps_H_INCLUDED__
