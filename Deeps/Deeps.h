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
#include "C:\Ashita 3\plugins\ADK\Ashita.h"
#include <list>
#include <map>
#include <functional>
#include <stdint.h>

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

    bool operator > (const source_t& o) const
    {
        return (total() > o.total());
    }
};

struct entitysources_t
{
    std::string name;
    uint32_t color;
    std::map<uint32_t, source_t> sources;

    uint64_t total() const
    {
        int64_t total = 0;
        for (auto s : sources)
        {
            total += s.second.total();
        }
        return total;
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

static const std::vector<D3DCOLOR> Colors = { D3DCOLOR_ARGB(255, 12, 0, 155), D3DCOLOR_ARGB(255, 140, 0, 0), D3DCOLOR_ARGB(255, 255, 177, 32), D3DCOLOR_ARGB(255, 143, 143, 143),
                                              D3DCOLOR_ARGB(255, 68, 68, 68), D3DCOLOR_ARGB(255, 255, 0, 0), D3DCOLOR_ARGB(255, 0, 164, 49), D3DCOLOR_ARGB(255, 198, 198, 0),
                                              D3DCOLOR_ARGB(255, 116, 0, 145), D3DCOLOR_ARGB(255, 165, 153, 10), D3DCOLOR_ARGB(255, 184, 128, 10), D3DCOLOR_ARGB(255, 224, 0, 230),
                                              D3DCOLOR_ARGB(255, 234, 100, 0), D3DCOLOR_ARGB(255, 119, 0, 0), D3DCOLOR_ARGB(255, 130, 17, 255), D3DCOLOR_ARGB(255, 79, 196, 0),
                                              D3DCOLOR_ARGB(255, 0, 16, 217), D3DCOLOR_ARGB(255, 136, 68, 0), D3DCOLOR_ARGB(255, 244, 98, 0), D3DCOLOR_ARGB(255, 15, 190, 220),
                                              D3DCOLOR_ARGB(255, 0, 123, 145) };

void g_onClick(int, void*, float, float);

/**
 * @brief Global copy of our plugin data.
 */
plugininfo_t* g_PluginInfo = NULL;

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
	IAshitaCore*        m_AshitaCore;
	ILogManager*		m_LogManager;
	DWORD               m_PluginId;
	IDirect3DDevice8*   m_Direct3DDevice;
	std::list<void*>	m_Packets;

private:
    source_t* getDamageSource(entitysources_t* entityInfo, uint8_t actionType, uint16_t actionID);
    bool updateDamageSource(source_t* source, uint16_t message, uint32_t damage);
    void repairBars(IFontObject* deepsBase, uint8_t size);
    void report(char mode, int max);
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
     * @brief GetPluginData implementation.
     */
	plugininfo_t GetPluginInfo(void);

    /**
     * @brief PluginBase virtual overrides.
     */
    bool Initialize(IAshitaCore* core, ILogManager* log, uint32_t id);
    void Release(void);
    bool HandleCommand(const char* command, int32_t type);
	bool HandleIncomingText(int16_t mode, const char* message, int16_t* modifiedMode, char* modifiedMessage, bool blocked);
	bool HandleIncomingPacket(uint16_t id, uint32_t size, void* data, void* modified, bool blocked);
	bool HandleOutgoingPacket(uint16_t id, uint32_t size, void* data, void* modified, bool blocked);
    bool Direct3DInitialize(IDirect3DDevice8* device);
    void Direct3DRelease(void);
    void Direct3DPreRender(void);
    void Direct3DRender(void);
    void onClick(int, IFontObject*, float, float);
};

// Global pointer to this

Deeps* g_Deeps = NULL;

/**
 * @brief Required Plugin Exports
 */
__declspec(dllexport) double     __stdcall GetInterfaceVersion(void);
__declspec(dllexport) void       __stdcall CreatePluginInfo(plugininfo_t* lpBuffer);
__declspec(dllexport) IPlugin*   __stdcall CreatePlugin(void);

#endif // __ASHITA_Deeps_H_INCLUDED__
