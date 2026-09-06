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

#include "Deeps.h"
namespace
{
std::mutex g_ClickMutex;
std::condition_variable g_ClickDrained;
Deeps* g_Deeps = NULL;
std::size_t g_ClicksInFlight = 0;
}

/**
 * Global function to serve as mouse callback
 */
BOOL __stdcall g_OnClick(uint32_t uMsg, WPARAM wParam, LPARAM lParam, bool handled)
{
    Deeps* deeps = NULL;
    {
        std::lock_guard<std::mutex> lock(g_ClickMutex);
        deeps = g_Deeps;
        if (deeps == NULL)
            return FALSE;
        ++g_ClicksInFlight;
    }

    BOOL result = FALSE;
    try
    {
        result = deeps->OnClick(uMsg, wParam, lParam, handled);
    }
    catch (...)
    {
        // Never let an exception cross the host's C callback boundary. The
        // in-flight guard still has to drain before teardown can continue.
    }
    {
        std::lock_guard<std::mutex> lock(g_ClickMutex);
        --g_ClicksInFlight;
        if (g_ClicksInFlight == 0)
            g_ClickDrained.notify_all();
    }
    return result;
}

/**
 * @brief Direct3D release call to allow this plugin to cleanup any Direct3D objects.
 */
void Deeps::Direct3DRelease(void)
{
    if (m_AshitaCore == NULL)
        return;
    auto input = m_AshitaCore->GetInputManager();
    if (input != NULL && input->GetMouse() != NULL)
        input->GetMouse()->RemoveCallback("deeps_click");

    // Remove the callback without holding the guard: the host may wait for a
    // callback that needs the same mutex. Then prevent new acquisitions and
    // drain callbacks that already acquired this object's lifetime.
    {
        std::unique_lock<std::mutex> lock(g_ClickMutex);
        if (g_Deeps == this)
            g_Deeps = NULL;
        g_ClickDrained.wait(lock, []() { return g_ClicksInFlight == 0; });
    }

    auto config = m_AshitaCore->GetConfigurationManager();
    if (config != NULL && m_Background != NULL)
    {
        config->SetValue("Deeps", "guipos", "xpos", std::to_string(m_Background->GetPositionX()).c_str());
        config->SetValue("Deeps", "guipos", "ypos", std::to_string(m_Background->GetPositionY()).c_str());
        config->SetValue("Deeps", "tvmode", "enabled", std::to_string(m_TVMode).c_str());
        config->SetValue("Deeps", "sc", "enabled", std::to_string(m_CountSkillchains).c_str());
        config->Save("Deeps", "Deeps");
    }

    auto fonts = m_AshitaCore->GetFontManager();
    if (fonts != NULL && m_Background != NULL)
        fonts->Delete(m_Background->GetAlias());
    m_Background = NULL;
    m_ClickMap.clear();

    while (!m_Bars.empty())
    {
        auto bar = m_Bars.back();
        if (fonts != NULL && bar != NULL)
            fonts->Delete(bar->GetAlias());
        m_Bars.pop_back();
    }
}


/**
 * @brief Direct3D initialize call to prepare this plugin for Direct3D calls.
 *
 * @param lpDevice              The Direct3D device currently wrapped by Ashita.
 *
 * @return True on success, false otherwise.
 *
 * @note    Plugins that do not return true on this call will not receive any other
 *          Direct3D calls listed below!
 */
bool Deeps::Direct3DInitialize(IDirect3DDevice8* device)
{
    if (device == NULL || m_AshitaCore == NULL || m_AshitaCore->GetConfigurationManager() == NULL ||
        m_AshitaCore->GetFontManager() == NULL || m_AshitaCore->GetInputManager() == NULL ||
        m_AshitaCore->GetInputManager()->GetMouse() == NULL)
        return false;
    this->m_Direct3DDevice = device;
    m_Drag                 = false;

    float xpos = m_AshitaCore->GetConfigurationManager()->GetFloat("Deeps", "guipos", "xpos", 300.0f);
    float ypos = m_AshitaCore->GetConfigurationManager()->GetFloat("Deeps", "guipos", "ypos", 300.0f);
    m_TVMode = m_AshitaCore->GetConfigurationManager()->GetBool("Deeps", "tvmode", "enabled", false);
    m_GUIScale = m_AshitaCore->GetConfigurationManager()->GetBool("Deeps", "tvmode", "enabled", false) ? 1.5f : 1.0f;
    m_CountSkillchains = m_AshitaCore->GetConfigurationManager()->GetBool("Deeps", "sc", "enabled", true);

    m_Background = m_AshitaCore->GetFontManager()->Create("DeepsBackground");
    if (m_Background == NULL || m_Background->GetBackground() == NULL)
        return false;
    m_Background->SetFontFamily("Arial");
    m_Background->SetFontHeight(TITLE_FONT_HEIGHT * m_GUIScale);
    m_Background->SetAutoResize(false);
    m_Background->GetBackground()->SetColor(D3DCOLOR_ARGB(0xCC, 0x00, 0x00, 0x00));
    m_Background->GetBackground()->SetVisible(true);
    m_Background->GetBackground()->SetWidth(WINDOW_WIDTH * m_GUIScale);
    m_Background->GetBackground()->SetHeight(TITLEBAR_HEIGHT * m_GUIScale);
    m_Background->GetBackground()->SetCanFocus(false);
    m_Background->SetColor(D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0xFF));
    m_Background->SetBold(false);
    m_Background->SetText("");
    m_Background->SetPositionX(xpos);
    m_Background->SetPositionY(ypos);
    m_Background->SetVisible(true);

    {
        std::lock_guard<std::mutex> lock(g_ClickMutex);
        g_Deeps = this;
    }
    m_AshitaCore->GetInputManager()->GetMouse()->AddCallback("deeps_click", g_OnClick);

    return true;
}

void Deeps::Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    if (m_Background == NULL || m_Background->GetBackground() == NULL)
        return;
    clock_t now = clock();
    if (!(now - m_LastRender > 0.1*CLOCKS_PER_SEC))
    {
        return;
    }

    // Making sure the background heights and widths are good for when the size toggle is switched
    m_Background->SetFontHeight(TITLE_FONT_HEIGHT * m_GUIScale);
    m_Background->GetBackground()->SetWidth(WINDOW_WIDTH * m_GUIScale);
    m_Background->GetBackground()->SetHeight(TITLEBAR_HEIGHT * m_GUIScale);

    if (m_CharInfo == 0)
    {
        m_Background->SetText(" Deeps - Damage Done");
        std::vector<entitysources_t> temp;
        uint64_t total = 0;
        for (auto iter = m_Entities.begin(); iter != m_Entities.end(); iter++)
        {
            if ((iter->second.total() != 0) && (temp.size() < m_MaxBars) && (this->CheckPartySetting(iter->first)))
            {
                temp.push_back(iter->second);
                total += iter->second.total();
            }
        }
        std::sort(temp.begin(), temp.end(), [](entitysources_t a, entitysources_t b) { return a > b; });
        RepairBars(m_Background, temp.size());

        int i        = 0;
        uint64_t max = 0;
        m_ClickMap.clear();
        for (auto iter = temp.begin(); iter != temp.end(); iter++)
        {
            IFontObject* bar = m_Bars[i];
            if (iter->total() > max)
                max = iter->total();
            bar->GetBackground()->SetWidth((BAR_WIDTH * m_GUIScale) * (total == 0 ? 1 : ((float)iter->total() / (float)max)));
            bar->GetBackground()->SetColor(this->CheckColorSetting(iter->id, iter->color));
            char string[256];
            sprintf_s(string, 256, " %d. %-10.10s %6llu (%03.1f%%)  -  Hit: %03.1f%% \n",
                i + 1, iter->name.c_str(), static_cast<unsigned long long>(iter->total()), total == 0 ? 0 : 100 * ((float)iter->total() / (float)total), iter->hitrate());
            bar->SetText(string);
            m_ClickMap.insert(std::pair<IFontObject*, std::string>(bar, iter->name));
            i++;
        }
    }
    else
    {
        auto it = m_Entities.find(m_CharInfo);
        if (it != m_Entities.end())
        {
            if (m_SourceInfo == "") // Updating top layer of the plugin
            {
                std::vector<source_t> temp;
                uint64_t total = 0;
                for (auto s : it->second.sources)
                {
                    if (s.second.total() != 0 && temp.size() < 15)
                    {
                        temp.push_back(s.second);
                        total += s.second.total();
                    }
                }
                std::sort(temp.begin(), temp.end(), [](source_t a, source_t b) { return a > b; });
                char string[256];
                sprintf_s(string, 256, " %s - Sources\n", it->second.name.c_str());
                m_Background->SetText(string);
                RepairBars(m_Background, temp.size());
                int i        = 0;
                uint64_t max = 0;
                m_ClickMap.clear();
                for (auto s : temp)
                {
                    IFontObject* bar = m_Bars[i];
                    if (s.total() > max)
                        max = s.total();
                    bar->GetBackground()->SetWidth((BAR_WIDTH * m_GUIScale) * (total == 0 ? 1 : ((float)s.total() / (float)max)));
                    bar->GetBackground()->SetColor(this->CheckColorSetting(it->first, it->second.color));
                    char string[256];
                    sprintf_s(string, 256, " %d. %-10.10s %6llu (%03.1f%%)\n",
                        i + 1, s.name.c_str(), static_cast<unsigned long long>(s.total()), total == 0 ? 0 : 100 * ((float)s.total() / (float)total));
                    bar->SetText(string);
                    m_ClickMap.insert(std::pair<IFontObject*, std::string>(bar, s.name));
                    i++;
                }
            }
            else // This is when a player's bar has been clicked into for additional details about their damage
            {
                for (auto s : it->second.sources)
                {
                    if (s.second.name == m_SourceInfo)
                    {
                        std::vector<std::pair<std::string, damage_t>> temp;
                        uint32_t count = 0;
                        for (const auto d : s.second.damage)
                        {
                            if (d.second.count != 0 && temp.size() < 15)
                            {
                                temp.push_back(d);
                                count += d.second.count;
                            }
                        }

                        std::sort(temp.begin(), temp.end(), [](const std::pair<std::string, damage_t>& a, const std::pair<std::string, damage_t>& b) { return a.second > b.second; });
                        char string[256];
                        sprintf_s(string, 256, " %s - %s\n", it->second.name.c_str(), s.second.name.c_str());
                        m_Background->SetText(string);
                        RepairBars(m_Background, temp.size());

                        int i        = 0;
                        uint32_t max = 0;
                        for (auto s : temp)
                        {
                            IFontObject* bar = m_Bars[i];
                            if (s.second.count > max)
                                max = s.second.count;
                            bar->GetBackground()->SetWidth((BAR_WIDTH * m_GUIScale) * (count == 0 ? 1 : 1 * ((float)s.second.count / (float)max)));
                            bar->GetBackground()->SetColor(this->CheckColorSetting(it->first, it->second.color));
                            char string[256];
                            sprintf_s(string, 256, " %-5sCnt:%4u  Avg:%5u  Max:%5u (%3.1f%%)\n", s.first.c_str(), s.second.count, s.second.avg(), s.second.max, count == 0 ? 0 : 100 * ((float)s.second.count / (float)count));
                            bar->SetText(string);
                            i++;
                        }
                        break;
                    }
                }
            }
        }
    }
    m_Background->GetBackground()->SetHeight(m_Bars.size() * (BAR_BACKGROUND_HEIGHT * m_GUIScale) + (TITLEBAR_HEIGHT * m_GUIScale));
    m_LastRender = clock();
}

/**
 * @brief Starts from the font base and creates or deletes bars as necessary.
 *
 * @param deepsBase The base font background
 * @param size The number of bars that should be displaying
 */
void Deeps::RepairBars(IFontObject* deepsBase, uint8_t size)
{
    auto barCount = m_Bars.size();
    while (m_Bars.size() < size)
    {
        barCount = m_Bars.size();
        char buffer[256];
        sprintf_s(buffer, 256, "DeepsBar%u", static_cast<unsigned int>(barCount));
        auto newBar = m_AshitaCore->GetFontManager()->Create(buffer);
        if (newBar == NULL || newBar->GetBackground() == NULL)
            return;
        newBar->SetAutoResize(false);
        newBar->SetFontFamily("Arial");
        if (m_TVMode)
        {
            newBar->SetCreateFlags(Ashita::FontCreateFlags::Bold);
        }
        newBar->SetFontHeight(BAR_FONT_HEIGHT * m_GUIScale);
        newBar->GetBackground()->SetColor(D3DCOLOR_ARGB(0xFF, 0x00, 0x7C, 0x5C));
        newBar->GetBackground()->SetVisible(true);
        sprintf_s(buffer, 256, "%s\\Resources\\Deeps\\bar.tga", m_AshitaCore->GetInstallPath());
        newBar->GetBackground()->SetTextureFromFile(buffer);
        newBar->GetBackground()->SetWidth(BAR_WIDTH * m_GUIScale);
        newBar->GetBackground()->SetHeight(BAR_HEIGHT * m_GUIScale);
        newBar->SetVisible(true);
        newBar->SetCanFocus(false);
        if (barCount == 0)
        {
            newBar->SetParent(m_Background);
            newBar->SetPositionX(BAR_HORIZONTAL_PADDING * m_GUIScale);
            newBar->SetPositionY((TITLEBAR_HEIGHT * m_GUIScale) - (1 * m_GUIScale));
        }
        else
        {
            newBar->SetParent(m_Bars[barCount - 1]);
            newBar->SetAnchorParent(Ashita::FrameAnchor::BottomLeft);
            newBar->SetPositionX(0);
            newBar->SetPositionY(BETWEEN_BAR_PADDING * m_GUIScale);
        }
        m_Bars.push_back(newBar);
    }

    while (m_Bars.size() > size)
    {
        auto bar = m_Bars.back();
        bar->SetParent(nullptr);
        m_AshitaCore->GetFontManager()->Delete(bar->GetAlias());
        m_Bars.pop_back();
    }
}

bool Deeps::OnClick(uint32_t uMsg, WPARAM wParam, LPARAM lParam, bool handled)
{
    if (m_Background == NULL || m_Background->GetBackground() == NULL)
        return false;
    int32_t xpos = GET_X_LPARAM(lParam);
    int32_t ypos = GET_Y_LPARAM(lParam);

    if (m_Drag)
    {
        auto newX = m_Background->GetPositionX() + (xpos - m_LastX);
        auto newY = m_Background->GetPositionY() + (ypos - m_LastY);
        m_Background->SetPositionX(newX);
        m_Background->SetPositionY(newY);
        m_LastX = xpos;
        m_LastY = ypos;
        if ((uMsg == 514) || ((GetKeyState(VK_SHIFT) & 0x8000) == 0))
        {
            m_Drag = false;
            return true;
        }
    }

    //Left click down..
    if (uMsg == 513)
    {
        if ((GetKeyState(VK_SHIFT) & 0x8000) && (m_Background->GetBackground()->HitTest(xpos, ypos)))
        {
            m_Drag = true;
            m_LastX = xpos;
            m_LastY = ypos;
        }
    }

    // Don't block input if it didn't fall into the deeps object
    if (!m_Background->GetBackground()->HitTest(xpos, ypos))
        return false;

    // Left click release handler checks for bars to focus..
    if (uMsg == 514)
    {
        for (auto iter = m_Bars.begin(); iter != m_Bars.end(); iter++)
        {
            if (HitTestBar(*iter, xpos, ypos))
            {
                if (m_CharInfo == 0)
                {
                    auto name = m_ClickMap.find(*iter);
                    if (name != m_ClickMap.end())
                    {
                        for (auto entity : m_Entities)
                        {
                            if (entity.second.name == name->second)
                            {
                                m_CharInfo = entity.first;
                                break;
                            }
                        }
                    }
                }
                else if (m_SourceInfo == "")
                {
                    auto name = m_ClickMap.find(*iter);
                    if (name != m_ClickMap.end())
                        m_SourceInfo.assign(name->second);
                }
                return true;
            }
        }
    }

    // Right click release handler cancels focused bar
    if (uMsg == 517)
    {
        if (m_SourceInfo != "")
        {
            m_SourceInfo = "";
        }
        else
        {
            m_CharInfo = 0;
        }
        return true;
    }

    // Block left and right click down as well.
    return ((uMsg == 513) || (uMsg == 516));
}

bool Deeps::HitTestBar(IFontObject* bar, int32_t x, int32_t y)
{
    if (bar == nullptr)
        return false;

    auto bg = bar->GetBackground();
    auto min = bg->GetPositionY();
    auto max = min + bg->GetHeight();
    return ((y >= min) && (y < max));
}

uint32_t Deeps::CheckColorSetting(uint32_t id, uint32_t randomColor)
{
    if (this->m_JobColors == false || m_AshitaCore == NULL || m_AshitaCore->GetMemoryManager() == NULL)
        return randomColor;

    IParty* party = m_AshitaCore->GetMemoryManager()->GetParty();
    if (party == NULL)
        return randomColor;
    for (auto i = 0; i < 18; i++)
    {
        if (party->GetMemberServerId(i) == id)
        {
            auto job = party->GetMemberMainJob(i);
            if (static_cast<size_t>(job) < JobColors.size())
                return JobColors[job];
            break;
        }
    }
    return randomColor;
}

bool Deeps::CheckPartySetting(uint32_t id)
{
    if (this->m_PartyOnly == false)
        return true;
    if (m_AshitaCore == NULL || m_AshitaCore->GetMemoryManager() == NULL)
        return false;

    IParty* party = m_AshitaCore->GetMemoryManager()->GetParty();
    if (party == NULL)
        return false;
    for (auto i = 0; i < 18; i++)
    {
        if (party->GetMemberServerId(i) == id)
            return true;
    }

    return false;
}