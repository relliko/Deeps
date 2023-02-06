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

/**
 * @brief Required includes for an extension.
 */
// #include "C:\code\Ashita-v4beta\plugins\sdk\Ashita.h"
#include "D:\HorizonXI\Game\plugins\sdk\Ashita.h"
#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <stdint.h>
#include <thread>
#include <windowsx.h>
#include "Defines.h"

/**
 * @brief Our Main Plugin Class
 *
 * @note    The main class of your plugin MUST use PluginBase as a base class. This is the
 *          internal base class that Ashita uses to communicate with your plugin!
 */
class Deeps : IPlugin
{
    // Internal class variables
	IAshitaCore*          m_AshitaCore;
	ILogManager* 	      m_LogManager;
	DWORD                 m_PluginId;
	IDirect3DDevice8*     m_Direct3DDevice;

    // Configuration
    bool                  m_Debug;
    bool                  m_JobColors;
    int32_t               m_MaxBars;
    bool                  m_PartyOnly;
    bool                  m_TVMode;
    float                 m_GUIScale;

    // Packet Deduplication reference
	std::list<void*>	  m_Packets;

    // Display State
    IFontObject*                        m_Background;
    std::vector<IFontObject*>           m_Bars;
    uint32_t                            m_CharInfo;
    std::map<IFontObject*, std::string> m_ClickMap;
    std::unordered_map<uint32_t, entitysources_t> m_Entities;
    std::string                         m_SourceInfo;

    // Mouse Handler
    bool                  m_Drag;
    int32_t               m_LastX;
    int32_t               m_LastY;

private:
    //damage.cpp
    uint16_t GetIndexFromId(int id);
    source_t* GetDamageSource(entitysources_t* entityInfo, uint8_t actionType, uint16_t actionID, bool isPet);
    bool IsParsedActionType(uint8_t actionType);
    bool UpdateDamageSource(source_t* source, uint16_t message, uint32_t damage);

    //main.cpp
    void Report(char mode, int max);

    //render.cpp
    void Direct3DRelease(void);
    void RepairBars(IFontObject* deepsBase, uint8_t size);
    uint32_t CheckColorSetting(uint32_t id, uint32_t random);
    bool CheckPartySetting(uint32_t id);
    bool HitTestBar(IFontObject* bar, int32_t x, int32_t y);

public:
    //main.cpp
    Deeps(void);
    virtual ~Deeps(void);
    uint32_t GetFlags(void) const;
    const char* GetName(void) const;
    double GetVersion(void) const;
    const char* GetAuthor(void) const;
    const char* GetDescription(void) const;
    bool Initialize(IAshitaCore* core, ILogManager* log, uint32_t id);
    void Release(void);
    bool HandleCommand(int32_t mode, const char* command, bool injected);

    //damage.cpp
	bool HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked);

    //render.cpp
    bool Direct3DInitialize(IDirect3DDevice8* device) override;
    void Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) override;
    bool OnClick(uint32_t uMsg, WPARAM wParam, LPARAM lParam, bool handled);
};

/**
 * @brief Required Plugin Exports
 */
__declspec(dllexport) double     __stdcall expGetInterfaceVersion(void);
__declspec(dllexport) IPlugin*   __stdcall expCreatePlugin(const char* args);

#endif // __ASHITA_Deeps_H_INCLUDED__
