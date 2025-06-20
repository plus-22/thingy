/*
Cloak V4
Copyright (C) 2025 Maintainer_(Ivan Shkatov) <ivanskatov672@gmail.com>
Copyright (C) 2025 Prounce <prouncedev@gmail.com>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/***************************************************************************************************
 *                                          NewMenu.cpp:                                           *
 *                                                                                                 *
 *          The menu supports various interactive elements, such as categories, sliders,           * 
 *      selection boxes, and dropdowns, using core::rect<s32> for mouse events and collisions      *
 *                                                                                                 *
 *                                  Core functionality includes:                                   *
 *                                                                                                 *
 * - Dynamic creation and cleanup of GUI elements via `create()` and `close()` methods.            *
 * - Drawing methods (`drawCategory`, `drawSelectionBox`, etc.) for rendering menu components.     *
 * - Event handling through `OnEvent()` for togglable cheats, sliders, and selection boxes.        *
 * - Helper methods for geometric calculations and slider value mapping.                           *
 *                                                                                                 *
 *                            This code is cursed as fuck, but it works.                           *
 *                       When you work on this file, increment the counter:                        *
 *                                   Total hours spent here: 96                                    *
 *                                                                                                 *
 ***************************************************************************************************/

#include "NewMenu.h"


 std::chrono::high_resolution_clock::time_point NewMenu::lastTime = std::chrono::high_resolution_clock::now();
 
 float NewMenu::getDeltaTime() {
     auto currentTime = std::chrono::high_resolution_clock::now();
     std::chrono::duration<float> deltaTime = currentTime - lastTime;
     lastTime = currentTime;
 
     return deltaTime.count();
 }
 
 Sprite NewMenu::coords_sprite = Sprite();
 
 NewMenu::NewMenu(gui::IGUIEnvironment* env, 
     gui::IGUIElement* parent, 
     s32 id, IMenuManager* menumgr, 
     Client* client)
     : IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, 
     core::rect<s32>(0, 0, 0, 0)), 
     m_menumgr(menumgr), isDragging(false), draggedRectIndex(-1),
     m_client(client)
 {    
     infostream << "[NEWMENU] Successfully created" << std::endl;
     this->env = env;
         
     coords_sprite.width = 140;
     coords_sprite.height = 30;
 
     if (!g_settings->exists("coords_sprite")) {
         coords_sprite.x = 5;
         coords_sprite.y = (Environment->getVideoDriver()->getScreenSize().Height - coords_sprite.height / 2 + g_fontengine->getTextHeight() - coords_sprite.height);
     } else {
         v2f data = g_settings->getV2F("coords_sprite");
         coords_sprite.x = data[0];
         coords_sprite.y = data[1];
     }
 }
 
 NewMenu::~NewMenu()
 {
     for (size_t i = 0; i < cheatSettingTextFields.size(); ++i) {
         for (size_t c = 0; c < cheatSettingTextFields[i].size(); ++c) {
             for (size_t s = 0; s < cheatSettingTextFields[i][c].size(); ++s) {
                 delete cheatSettingTextFields[i][c][s];
             }
         }
     }
 }
 
 void NewMenu::create()
 {
     GET_SCRIPT_POINTER
 
     if (script->m_cheat_categories.empty()) {
         std::cout << "No categories available." << std::endl;
         return;
     }
     if (!m_initialized) {
         video::IVideoDriver* driver = Environment->getVideoDriver();
         lastScreenSize = driver->getScreenSize();
         category_positions.resize(script->m_cheat_categories.size(), core::position2d<s32>(0, 0));
         categoryRects.resize(script->m_cheat_categories.size(), core::rect<s32>(0,0,0,0));
         dropdownRects.resize(script->m_cheat_categories.size(), core::rect<s32>(0,0,0,0));
         textRects.resize(script->m_cheat_categories.size(), core::rect<s32>(0,0,0,0));
         selectionBoxRects.resize(script->m_cheat_categories.size());
         selectedCategory.resize(script->m_cheat_categories.size(), false);
         selectedCheat.resize(script->m_cheat_categories.size());
         dropdownHovered.resize(script->m_cheat_categories.size(), false);
         textHovered.resize(script->m_cheat_categories.size(), false);
         cheatRects.resize(script->m_cheat_categories.size());
         cheatRectAnimationProgress.resize(script->m_cheat_categories.size());
         cheatTextRects.resize(script->m_cheat_categories.size());
         cheatDropdownRects.resize(script->m_cheat_categories.size());
         cheatDropdownHovered.resize(script->m_cheat_categories.size());
         cheat_positions.resize(script->m_cheat_categories.size());
         cheatTextHovered.resize(script->m_cheat_categories.size());
         cheatSettingRects.resize(script->m_cheat_categories.size());
         cheatSliderRects.resize(script->m_cheat_categories.size());
         cheatSliderBarRects.resize(script->m_cheat_categories.size());
         cheatSettingTextRects.resize(script->m_cheat_categories.size());
         cheatSettingTextFields.resize(script->m_cheat_categories.size());
         cheatSettingTextLasts.resize(script->m_cheat_categories.size());
         cheatSettingTextHovered.resize(script->m_cheat_categories.size());
         cheatSliderHovered.resize(script->m_cheat_categories.size());
         selectionBoxHovered.resize(script->m_cheat_categories.size());
         cheat_setting_positions.resize(script->m_cheat_categories.size());
         cheatSettingOptionHovered.resize(script->m_cheat_categories.size());
         cheatSettingOptionRects.resize(script->m_cheat_categories.size());
 
         for (size_t i = 0; i < script->m_cheat_categories.size(); ++i) {
             if (g_settings->exists("Category_Dropdown_" + std::to_string(i)) && g_settings->getBool("save_menu_category_positions")) {
                 selectedCategory[i] = g_settings->getBool("Category_Dropdown_" + std::to_string(i));
             }
             
             if (g_settings->exists("Category_Position_" + std::to_string(i)) && g_settings->getBool("save_menu_category_positions")) {
                 v2f position = g_settings->getV2F("Category_Position_" + std::to_string(i));
                 category_positions[i] = core::position2d<s32>(position.X, position.Y);
             } else {
                 category_positions[i] = core::position2d<s32>(category_height / 2, category_height / 2 + ((category_height + category_height / 2) * i));
             }
             categoryRects[i] = core::rect<s32>(category_positions[i].X, category_positions[i].Y, 
                                             category_positions[i].X + category_width, category_positions[i].Y + category_height);
 
             textRects[i] = core::rect<s32>(category_positions[i].X, category_positions[i].Y, 
                                         category_positions[i].X + (category_width - category_height), category_positions[i].Y + category_height);
 
             dropdownRects[i] = core::rect<s32>(category_positions[i].X + (category_width - category_height), category_positions[i].Y, 
                                             category_positions[i].X + category_width, category_positions[i].Y + category_height);
                                     
             cheatRects[i].resize(script->m_cheat_categories[i]->m_cheats.size(), core::rect<s32>(0,0,0,0));
             cheatRectAnimationProgress[i].resize(script->m_cheat_categories[i]->m_cheats.size(), 1.0f);
             cheatTextRects[i].resize(script->m_cheat_categories[i]->m_cheats.size(), core::rect<s32>(0,0,0,0));
             cheatDropdownRects[i].resize(script->m_cheat_categories[i]->m_cheats.size(), core::rect<s32>(0,0,0,0));
             cheatDropdownHovered[i].resize(script->m_cheat_categories[i]->m_cheats.size(), false);
             cheatTextHovered[i].resize(script->m_cheat_categories[i]->m_cheats.size(), false);
             selectedCheat[i].resize(script->m_cheat_categories[i]->m_cheats.size(), false);
             cheat_positions[i].resize(script->m_cheat_categories[i]->m_cheats.size(), core::position2d<s32>(0, 0));
             cheatSettingRects[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSliderRects[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSliderBarRects[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             selectionBoxRects[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSettingTextRects[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSettingTextFields[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSettingTextLasts[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSettingTextHovered[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSliderHovered[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             selectionBoxHovered[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheat_setting_positions[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSettingOptionHovered[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             cheatSettingOptionRects[i].resize(script->m_cheat_categories[i]->m_cheats.size());
             for (size_t c = 0; c < script->m_cheat_categories[i]->m_cheats.size(); ++c) {
                 if (g_settings->exists("Cheat_Dropdown_" + std::to_string(i) + "_" + std::to_string(c)) && g_settings->getBool("save_menu_category_positions")) {
                     selectedCheat[i][c] = g_settings->getBool("Cheat_Dropdown_" + std::to_string(i) + "_" + std::to_string(c));
                 }
                 cheatRectAnimationProgress[i][c] = static_cast<float>(!script->m_cheat_categories[i]->m_cheats[c]->is_enabled());
                 cheatSettingRects[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSliderRects[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 selectionBoxRects[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSliderBarRects[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSettingTextRects[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSettingTextFields[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSettingTextLasts[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSettingTextHovered[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(), false);
                 cheatSliderHovered[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(), false);
                 selectionBoxHovered[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(), false);
                 cheat_setting_positions[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSettingOptionHovered[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 cheatSettingOptionRects[i][c].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size());
                 for (size_t s = 0; s < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(); ++s) {
                     if (script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_type == "text") {
                         std::wstring wname = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(g_settings->get(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_setting));
                         cheatSettingTextLasts[i][c][s] = wname;
                         cheatSettingTextFields[i][c][s] = env->addEditBox(wname.c_str(), core::rect<s32>(0, 0, category_width, category_height), true);
                         cheatSettingTextFields[i][c][s]->setDrawBackground(false);
                         cheatSettingTextFields[i][c][s]->setWordWrap(true);
                         cheatSettingTextFields[i][c][s]->setOverrideColor(video::SColor(173, 35, 45, 56));
                         cheatSettingTextFields[i][c][s]->setVisible(false);
                     } else if (script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_type == "selectionbox") {
                         for (size_t o = 0; o < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options.size(); ++o) {
                             cheatSettingOptionHovered[i][c][s].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options.size(), false);
                             cheatSettingOptionRects[i][c][s].resize(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options.size());
                         }
                     }
                 }
                 
                 moveMenu(i, category_positions[i]);
             }
         }
         m_initialized = true;
     } else {
         for (size_t i = 0; i < cheatSettingTextFields.size(); ++i) {
             for (size_t c = 0; c < cheatSettingTextFields[i].size(); ++c) {
                 for (size_t s = 0; s < cheatSettingTextFields[i][c].size(); ++s) {
                     if (cheatSettingTextFields[i][c][s] != nullptr) {
                         cheatSettingTextFields[i][c][s]->setVisible(selectedCategory[i] && selectedCheat[i][c]);
                     }
                 }
             }
         }
     }
 
     core::rect<s32> screenRect(0, 0, 
         Environment->getVideoDriver()->getScreenSize().Width, 
         Environment->getVideoDriver()->getScreenSize().Height);
     setRelativePosition(screenRect);
 
     IGUIElement::setVisible(true);
     Environment->setFocus(this);
     m_menumgr->createdMenu(this);
     m_is_open = true;
 }
 
 void NewMenu::close()
 {
     for (size_t i = 0; i < cheatSettingTextFields.size(); ++i) {
         for (size_t c = 0; c < cheatSettingTextFields[i].size(); ++c) {
             for (size_t s = 0; s < cheatSettingTextFields[i][c].size(); ++s) {
                 if (cheatSettingTextFields[i][c][s] != nullptr) {
                     cheatSettingTextFields[i][c][s]->setVisible(false);
                 }
             }
         }
     }
     Environment->removeFocus(this);
     m_menumgr->deletingMenu(this);
     IGUIElement::setVisible(false);
     m_is_open = false;
 }
 
 
 double NewMenu::roundToNearestStep(double number, double m_min, double m_max, double m_steps)
 {
     double stepSize = (m_max - m_min) / (m_steps - 1);
     return m_min + stepSize * round((number - m_min) / stepSize);
 }
 
 void NewMenu::drawInterpolatedRectangle(video::IVideoDriver* driver, const core::rect<s32>& rect, video::SColor innerColor, video::SColor outerColor, float interpolation)
 {
     interpolation = core::clamp(interpolation, 0.0f, 1.0f);
 
     if (interpolation == 1.0f) {
         driver->draw2DRectangle(outerColor, rect);
         return;
     }
 
     if (interpolation == 0.0f) {
         driver->draw2DRectangle(innerColor, rect);
         return;
     }
 
     s32 midX = (rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2;
     s32 interpWidth = static_cast<s32>((rect.getWidth() * (1.0f - interpolation)) / 2);
 
     core::rect<s32> innerRect(midX - interpWidth, rect.UpperLeftCorner.Y, midX + interpWidth, rect.LowerRightCorner.Y);
     driver->draw2DRectangle(innerColor, innerRect);
     
     driver->draw2DRectangle(outerColor, core::rect<s32>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y, innerRect.UpperLeftCorner.X, rect.LowerRightCorner.Y));
     driver->draw2DRectangle(outerColor, core::rect<s32>(innerRect.LowerRightCorner.X, rect.UpperLeftCorner.Y, rect.LowerRightCorner.X, rect.LowerRightCorner.Y));
 }
 
 void NewMenu::calculateSliderSplit(
     const core::rect<s32>& sliderRect, 
     double value, 
     double minValue, 
     double maxValue, 
     core::rect<s32>& filledRect, 
     core::rect<s32>& remainingRect
 ) {
     if (minValue >= maxValue) {
         filledRect = sliderRect;
         remainingRect = core::rect<s32>(0, 0, 0, 0);
     }
 
     value = std::max(minValue, std::min(value, maxValue));
 
     double ratio = (value - minValue) / (maxValue - minValue);
     int splitX = sliderRect.UpperLeftCorner.X + static_cast<int>(sliderRect.getWidth() * ratio);
 
     filledRect = core::rect<s32>(
         sliderRect.UpperLeftCorner.X,
         sliderRect.UpperLeftCorner.Y,
         splitX,
         sliderRect.LowerRightCorner.Y
     );
 
     remainingRect = core::rect<s32>(
         splitX,
         sliderRect.UpperLeftCorner.Y,
         sliderRect.LowerRightCorner.X,
         sliderRect.LowerRightCorner.Y
     );
 }
 
 double mapValue(double value, double oldMin, double oldMax, double newMin, double newMax) {
     return newMin + (value - oldMin) * (newMax - newMin) / (oldMax - oldMin);
 }
 
 double NewMenu::calculateSliderValueFromPosition(const core::rect<s32>& sliderBarRect, const core::position2d<s32>& pointerPosition, double m_min, double m_max, double m_steps)
 {
 
     s32 clampedX = std::clamp(pointerPosition.X, sliderBarRect.UpperLeftCorner.X, sliderBarRect.LowerRightCorner.X);
 
 
     double sliderValue = mapValue(clampedX, sliderBarRect.UpperLeftCorner.X, sliderBarRect.LowerRightCorner.X, m_min, m_max);
 
     return roundToNearestStep(sliderValue, m_min, m_max, m_steps);
 }
 
 
 s32 NewMenu::respaceMenu(size_t i)
 {
     GET_SCRIPT_POINTER_S32
     s32 last_height = category_positions[i].Y + category_height + 1;
     categoryRects[i] = core::rect<s32>(category_positions[i].X, category_positions[i].Y, 
                                 category_positions[i].X + category_width, category_positions[i].Y + category_height);
 
     textRects[i] = core::rect<s32>(category_positions[i].X, category_positions[i].Y, 
                                 category_positions[i].X + (category_width - category_height), category_positions[i].Y + category_height);
 
     dropdownRects[i] = core::rect<s32>(category_positions[i].X + (category_width - category_height), category_positions[i].Y, 
                                     category_positions[i].X + category_width, category_positions[i].Y + category_height);
     
 
     for (size_t c = 0; c < script->m_cheat_categories[i]->m_cheats.size(); ++c) {
         cheat_positions[i][c] = core::position2d<s32>(category_positions[i].X, last_height);
         last_height += category_height;
         cheatRects[i][c] = core::rect<s32>(cheat_positions[i][c].X, cheat_positions[i][c].Y, 
                                             cheat_positions[i][c].X + category_width, cheat_positions[i][c].Y + category_height);
         if (script->m_cheat_categories[i]->m_cheats[c]->has_settings()) {
             
             cheatTextRects[i][c] = core::rect<s32>(cheat_positions[i][c].X, cheat_positions[i][c].Y, 
                                                 cheat_positions[i][c].X + (category_width - category_height), cheat_positions[i][c].Y + category_height);
 
             cheatDropdownRects[i][c] = core::rect<s32>(cheat_positions[i][c].X + (category_width - category_height), cheat_positions[i][c].Y, 
                                                     cheat_positions[i][c].X + category_width, cheat_positions[i][c].Y + category_height);
             if (selectedCheat[i][c]) {
                 for (size_t s = 0; s < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(); ++s) {
                     cheat_setting_positions[i][c][s] = core::position2d<s32>(category_positions[i].X, last_height);
                     if (script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_type == "selectionbox" || script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_type == "slider_int" || script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_type == "slider_float") {
                         cheatSettingRects[i][c][s] = core::rect<s32>(cheat_setting_positions[i][c][s].X,                  cheat_setting_positions[i][c][s].Y, 
                                                                      cheat_setting_positions[i][c][s].X + category_width, cheat_setting_positions[i][c][s].Y + category_height * 2);
                     } else if (script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_type == "text") {
                         cheatSettingRects[i][c][s] = core::rect<s32>(cheat_setting_positions[i][c][s].X,                  cheat_setting_positions[i][c][s].Y, 
                                                                      cheat_setting_positions[i][c][s].X + category_width, cheat_setting_positions[i][c][s].Y + category_height * 4);
                         cheatSettingTextFields[i][c][s]->setRelativePosition(core::rect<s32>(
                             cheatSettingRects[i][c][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width, 
                             cheatSettingRects[i][c][s].UpperLeftCorner.Y + category_height,
                             cheatSettingRects[i][c][s].LowerRightCorner.X - setting_bar_padding, 
                             cheatSettingRects[i][c][s].LowerRightCorner.Y - setting_bar_padding
                         ));
                     } else {
                         cheatSettingRects[i][c][s] = core::rect<s32>(cheat_setting_positions[i][c][s].X,                  cheat_setting_positions[i][c][s].Y, 
                                                                      cheat_setting_positions[i][c][s].X + category_width, cheat_setting_positions[i][c][s].Y + category_height);
                         cheatSettingTextRects[i][c][s] = core::rect<s32>(cheat_setting_positions[i][c][s].X + (setting_bar_padding * 2) + setting_bar_width, cheat_setting_positions[i][c][s].Y, 
                                                                          cheat_setting_positions[i][c][s].X + category_width,                                cheat_setting_positions[i][c][s].Y + category_height);
                     }
                     
                     last_height += cheatSettingRects[i][c][s].getHeight();
                 }
             }
         } else {
             cheatTextRects[i][c] = core::rect<s32>(cheat_positions[i][c].X, cheat_positions[i][c].Y, 
                                                 cheat_positions[i][c].X + category_width, cheat_positions[i][c].Y + category_height);
 
             cheatDropdownRects[i][c] = core::rect<s32>(cheat_positions[i][c].X + category_width, cheat_positions[i][c].Y, 
                                                     cheat_positions[i][c].X + category_width, cheat_positions[i][c].Y);
         }
     }
     return last_height;
 }
 
 void NewMenu::moveMenu(size_t i, core::position2d<s32> new_position)
 {
     s32 last_height = respaceMenu(i);
     s32 screenWidth = Environment->getVideoDriver()->getScreenSize().Width;
     s32 screenHeight = Environment->getVideoDriver()->getScreenSize().Height;
     category_positions[i] = new_position;
     s32 newX = category_positions[i].X;
     s32 newY = category_positions[i].Y;
 
     if (newX < 0) {
         newX = 0;
     } else if (newX + categoryRects[i].getWidth() > screenWidth) {
         newX = screenWidth - categoryRects[i].getWidth();
     }
     s32 cheats_height = categoryRects[i].getHeight();
     if (selectedCategory[i]){
         cheats_height = last_height - categoryRects[i].UpperLeftCorner.Y;
     }
     for (size_t c = 0; c < cheatSettingTextFields[i].size(); ++c) {
         g_settings->setBool("Category_Dropdown_" + std::to_string(i), selectedCategory[i]);
         for (size_t s = 0; s < cheatSettingTextFields[i][c].size(); ++s) {
             g_settings->setBool("Cheat_Dropdown_" + std::to_string(i) + "_" + std::to_string(c), selectedCheat[i][c]); 
             if (cheatSettingTextFields[i][c][s] != nullptr) {
                 cheatSettingTextFields[i][c][s]->setVisible(selectedCheat[i][c]);
             }
         }
     }
     if (newY < 0) {
         newY = 0;
     } else if (newY + cheats_height > screenHeight) {
         newY = screenHeight - cheats_height;
     }
     if (g_settings->exists("use_menu_grid") && g_settings->getBool("use_menu_grid") == true) {
         newX = std::round(newX / (category_height / 2)) * (category_height / 2);
         newY = std::round(newY / (category_height / 2)) * (category_height / 2);
     }
 
     category_positions[i] = core::position2d<s32>(newX, newY);
     
     respaceMenu(i);
 
     g_settings->setV2F("Category_Position_" + std::to_string(i), v2f(newX, newY));
 }
 
 bool NewMenu::OnEvent(const irr::SEvent& event) 
 {
     s32 screenWidth = Environment->getVideoDriver()->getScreenSize().Width, screenHeight = Environment->getVideoDriver()->getScreenSize().Height;
     if (!m_is_open) {
         return false;
     }
     GET_SCRIPT_POINTER_BOOL  
     if (event.EventType == irr::EET_KEY_INPUT_EVENT)
     {
         if (event.KeyInput.Key == KEY_ESCAPE)
         {
             close();
             return true; 
         }
     }
     
     if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
         if (event.MouseInput.Event == irr::EMIE_LMOUSE_PRESSED_DOWN) {
 
         if (coords_sprite.get_rect().isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
             coords_sprite.isDragging = true;
             offset = core::vector2d<s32>(event.MouseInput.X - coords_sprite.x, event.MouseInput.Y - coords_sprite.y);
             return true;
         }
             if (isSelecting) {
                 for (size_t o = 0; o < script->m_cheat_categories[selectingCategoryIndex]->m_cheats[selectingCheatIndex]->m_cheat_settings[selectingSettingIndex]->m_options.size(); ++o) {
                     if (cheatSettingOptionRects[selectingCategoryIndex][selectingCheatIndex][selectingSettingIndex][o].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                         g_settings->set(script->m_cheat_categories[selectingCategoryIndex]->m_cheats[selectingCheatIndex]->m_cheat_settings[selectingSettingIndex]->m_setting, 
                             script->m_cheat_categories[selectingCategoryIndex]->m_cheats[selectingCheatIndex]->m_cheat_settings[selectingSettingIndex]->m_options[o]->c_str());
                     }
                 }
 
                 isSelecting=false;
                 return true;
             }
             for (size_t i = 0; i < script->m_cheat_categories.size(); ++i) {
                 if (dropdownRects[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                     selectedCategory[i] = !selectedCategory[i];
 
                     moveMenu(i, category_positions[i]);
                     return true;
                 }
 
                 if (textRects[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                     isDragging = true;
                     draggedRectIndex = i;
                     offset = core::vector2d<s32>(event.MouseInput.X - textRects[i].UpperLeftCorner.X, event.MouseInput.Y - textRects[i].UpperLeftCorner.Y);
                     return true; 
                 }
                 if (selectedCategory[i]) {
                     for (size_t c = 0; c < script->m_cheat_categories[i]->m_cheats.size(); ++c) {
                         if (cheatTextRects[i][c].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                             script->toggle_cheat(script->m_cheat_categories[i]->m_cheats[c]);
                             return true; 
                         }
 
                         if (cheatDropdownRects[i][c].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                             selectedCheat[i][c] = !selectedCheat[i][c];
                             
                             moveMenu(i, category_positions[i]);
                             return true; 
                         }
 
                         for (size_t s = 0; s < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(); ++s) {
                             if (selectedCheat[i][c]) {
                                 if (cheatSettingTextRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                                     script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->toggle();
                                 }
                                 if (cheatSliderRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y)) || cheatSliderBarRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                                     draggedSliderCategoryIndex = i;
                                     draggedSliderCheatIndex = c;
                                     draggedSliderSettingIndex = s;
                                     isSliding = true;
                                     ScriptApiCheatsCheatSetting* cheatSetting = script->m_cheat_categories[draggedSliderCategoryIndex]->m_cheats[draggedSliderCheatIndex]->m_cheat_settings[draggedSliderSettingIndex];
                                     cheatSetting->set_value(calculateSliderValueFromPosition(cheatSliderBarRects[draggedSliderCategoryIndex][draggedSliderCheatIndex][draggedSliderSettingIndex], core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y), cheatSetting->m_min, cheatSetting->m_max, cheatSetting->m_steps));
                                 }
                                 if (selectionBoxRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
                                     selectingCategoryIndex = i;
                                     selectingCheatIndex = c;
                                     selectingSettingIndex = s;
                                     isSelecting = true;
                                 }
                             }
                         }
                     }
                 }
             }
             return false; 
         } else if (event.MouseInput.Event == irr::EMIE_LMOUSE_LEFT_UP) {
             coords_sprite.isDragging = false;
             isDragging = false;
             isSliding = false;
             draggedRectIndex = -1;
             draggedSliderCategoryIndex = 0;
             draggedSliderCheatIndex = 0;
             draggedSliderSettingIndex = 0;
             return true;
         } else if (event.MouseInput.Event == irr::EMIE_MOUSE_MOVED && isDragging && draggedRectIndex != -1) {
             moveMenu(draggedRectIndex, core::vector2d<s32>(event.MouseInput.X - offset.X, event.MouseInput.Y - offset.Y));
             return true;
         } else if (event.MouseInput.Event == irr::EMIE_MOUSE_MOVED && isSliding) {
             ScriptApiCheatsCheatSetting* cheatSetting = script->m_cheat_categories[draggedSliderCategoryIndex]->m_cheats[draggedSliderCheatIndex]->m_cheat_settings[draggedSliderSettingIndex];
             cheatSetting->set_value(calculateSliderValueFromPosition(cheatSliderBarRects[draggedSliderCategoryIndex][draggedSliderCheatIndex][draggedSliderSettingIndex], core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y), cheatSetting->m_min, cheatSetting->m_max, cheatSetting->m_steps));
         } else if (event.MouseInput.Event == irr::EMIE_MOUSE_MOVED) {
             if (coords_sprite.isDragging) {
                 coords_sprite.x = event.MouseInput.X - offset.X;
                 coords_sprite.y = event.MouseInput.Y - offset.Y;
                 coords_sprite.save(screenWidth, screenHeight);
             }
 
             for (size_t i = 0; i < script->m_cheat_categories.size(); ++i) {
 
                 dropdownHovered[i] = dropdownRects[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
 
                 textHovered[i] = textRects[i].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
 
                 for (size_t c = 0; c < script->m_cheat_categories[i]->m_cheats.size(); ++c) {
 
                     cheatTextHovered[i][c] = cheatTextRects[i][c].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
 
                     cheatDropdownHovered[i][c] = cheatDropdownRects[i][c].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
 
                     for (size_t s = 0; s < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings.size(); ++s) {
                         cheatSettingTextHovered[i][c][s] = cheatSettingTextRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
                         cheatSliderHovered[i][c][s] = cheatSliderRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
                         selectionBoxHovered[i][c][s] = selectionBoxRects[i][c][s].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
 
                         for (size_t o = 0; o < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options.size(); ++o) {
                             cheatSettingOptionHovered[i][c][s][o] = cheatSettingOptionRects[i][c][s][o].isPointInside(core::vector2d<s32>(event.MouseInput.X, event.MouseInput.Y));
                         }
                     }
                 }
             }
         }
     }
 
     return Parent ? Parent->OnEvent(event) : false; 
 }
 
 void NewMenu::drawCategory(video::IVideoDriver* driver, gui::IGUIFont* font, const size_t i, float dtime)
 {
     GET_SCRIPT_POINTER
     // TODO REMOVE THESE AND STORE THEM AS SETTINGS
     const s32 unit_size = category_height / 9;
     const video::SColor category_color = video::SColor(230, 30, 30, 40);
     const video::SColor cheat_color = video::SColor(210, 10, 15, 20);
     const video::SColor cheat_color_enabled = video::SColor(210, 50, 80, 175);
     const video::SColor outline_color = video::SColor(230, 75, 125, 250);
     
     ///////////////////////// DRAW CATEGORY HEADER /////////////////////////
     video::SColor arrow_color = video::SColor(255, 255, 255, 255);
     video::SColor text_color = video::SColor(255, 255, 255, 255);
     if (dropdownHovered[i]) {
         arrow_color = video::SColor(255, 127, 127, 127);
     }
     if (textHovered[i]) {
         text_color = video::SColor(255, 127, 127, 127);
     }
 
     driver->draw2DRectangle(category_color, categoryRects[i]);
     
     driver->draw2DRectangleOutline(core::rect<s32>(categoryRects[i].UpperLeftCorner.X, categoryRects[i].UpperLeftCorner.Y, categoryRects[i].LowerRightCorner.X - 1, categoryRects[i].LowerRightCorner.Y - 1), outline_color);
     driver->draw2DRectangleOutline(core::rect<s32>(categoryRects[i].UpperLeftCorner.X - 1, categoryRects[i].UpperLeftCorner.Y - 1, categoryRects[i].LowerRightCorner.X, categoryRects[i].LowerRightCorner.Y), outline_color);
 
     const std::string& categoryName = script->m_cheat_categories[i]->m_name;
     std::wstring wCategoryName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(categoryName);
 
     core::dimension2d<u32> textSizeU32 = font->getDimension(wCategoryName.c_str());
     core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
 
     s32 textX = textRects[i].UpperLeftCorner.X + (textRects[i].getWidth() - textSize.Width) / 2;
     s32 textY = textRects[i].UpperLeftCorner.Y + (textRects[i].getHeight() - textSize.Height) / 2;
 
 
     font->draw(wCategoryName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), text_color);
 
     core::position2d<s32> dropdown_center(dropdownRects[i].UpperLeftCorner.X + dropdownRects[i].getWidth() / 2 ,dropdownRects[i].UpperLeftCorner.Y + dropdownRects[i].getHeight() / 2);
 
     if (selectedCategory[i]) {
         
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, dropdown_center.Y - (unit_size * 1.5)), arrow_color);
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), 1 + dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y - (unit_size * 1.5)), arrow_color);
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X, dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), dropdown_center.Y + (unit_size * 1.5)), arrow_color);
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), 1 + dropdown_center.Y + (unit_size * 1.5)), arrow_color);
         ///////////////////////// DRAW CHEATS /////////////////////////
         for (size_t cheat_index = 0; cheat_index < script->m_cheat_categories[i]->m_cheats.size(); ++cheat_index) {
 
             arrow_color = video::SColor(255, 255, 255, 255);
             text_color = video::SColor(255, 255, 255, 255);
             if (cheatDropdownHovered[i][cheat_index]) {
                 arrow_color = video::SColor(255, 127, 127, 127);
             }
             if (cheatTextHovered[i][cheat_index]) {
                 text_color = video::SColor(255, 127, 127, 127);
             }
 
             
             if (!script->m_cheat_categories[i]->m_cheats[cheat_index]->is_enabled() && cheatRectAnimationProgress[i][cheat_index] <= 1) {
                 cheatRectAnimationProgress[i][cheat_index] += dtime * 8;
             } else if (script->m_cheat_categories[i]->m_cheats[cheat_index]->is_enabled() && cheatRectAnimationProgress[i][cheat_index] >= 0) {
                 cheatRectAnimationProgress[i][cheat_index] -= dtime * 8;
             }
 
             drawInterpolatedRectangle(driver, cheatRects[i][cheat_index], cheat_color_enabled, cheat_color, cheatRectAnimationProgress[i][cheat_index]);
 
             const std::string& cheatName = script->m_cheat_categories[i]->m_cheats[cheat_index]->m_name;
             std::wstring wCheatName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(cheatName);
 
             textSizeU32 = font->getDimension(wCheatName.c_str());
             core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
             s32 textX = cheatTextRects[i][cheat_index].UpperLeftCorner.X + (cheatTextRects[i][cheat_index].getWidth() - textSize.Width) / 2;
             s32 textY = cheatTextRects[i][cheat_index].UpperLeftCorner.Y + (cheatTextRects[i][cheat_index].getHeight() - textSize.Height) / 2;
 
             font->draw(wCheatName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), text_color);
             if (script->m_cheat_categories[i]->m_cheats[cheat_index]->has_settings()) {
                 core::position2d<s32> dropdown_center(cheatDropdownRects[i][cheat_index].UpperLeftCorner.X + cheatDropdownRects[i][cheat_index].getWidth() / 2, 
                                                       cheatDropdownRects[i][cheat_index].UpperLeftCorner.Y + cheatDropdownRects[i][cheat_index].getHeight() / 2);
 
                 if (selectedCheat[i][cheat_index]) {
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), 1 + dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X, dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), 1 + dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                     ///////////////////////// DRAW CHEAT SETTINGS /////////////////////////
 
                     driver->draw2DRectangle(settingBackgroundColor, core::rect<s32>(cheatSettingRects[i][cheat_index][0].UpperLeftCorner.X,
                                                                                     cheatSettingRects[i][cheat_index][0].UpperLeftCorner.Y, 
                                                                                     cheatSettingRects[i][cheat_index][cheatSettingRects[i][cheat_index].size() - 1].LowerRightCorner.X,
                                                                                     cheatSettingRects[i][cheat_index][cheatSettingRects[i][cheat_index].size() - 1].LowerRightCorner.Y));
                     driver->draw2DRectangle(settingBarColor, core::rect<s32>(cheatSettingRects[i][cheat_index][0].UpperLeftCorner.X + setting_bar_padding,
                                                                              cheatSettingRects[i][cheat_index][0].UpperLeftCorner.Y + setting_bar_padding, 
                                                                              cheatSettingRects[i][cheat_index][cheatSettingRects[i][cheat_index].size() - 1].UpperLeftCorner.X + setting_bar_width + setting_bar_padding,
                                                                              cheatSettingRects[i][cheat_index][cheatSettingRects[i][cheat_index].size() - 1].LowerRightCorner.Y - setting_bar_padding));
                     
                     for (size_t s = 0; s < script->m_cheat_categories[i]->m_cheats[cheat_index]->m_cheat_settings.size(); ++s) {
                         ScriptApiCheatsCheatSetting* cheatSetting = script->m_cheat_categories[i]->m_cheats[cheat_index]->m_cheat_settings[s];
 
                         text_color = video::SColor(255, 255, 255, 255);
                         if (cheatSetting->m_type == "selectionbox") {
                             const std::string& settingName = cheatSetting->m_name;
                             std::wstring wSettingName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(settingName);
 
                             textSizeU32 = font->getDimension(wSettingName.c_str());
                             core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
 
                             core::rect<s32> settingTextRect = core::rect<s32>(cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y, 
                                                                               cheatSettingRects[i][cheat_index][s].LowerRightCorner.X,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height);
 
                             s32 textX = settingTextRect.UpperLeftCorner.X + (settingTextRect.getWidth() - textSize.Width) / 2;
                             s32 textY = settingTextRect.UpperLeftCorner.Y + (settingTextRect.getHeight() - textSize.Height) / 2;
 
                             font->draw(wSettingName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), text_color);
 
                             arrow_color = video::SColor(255, 255, 255, 255);
                             if (selectionBoxHovered[i][cheat_index][s]) {
                                 arrow_color = video::SColor(255, 127, 127, 127);
                             }
 
                             selectionBoxRects[i][cheat_index][s] = core::rect<s32>( cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width + selection_box_padding,
                                                                                     cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height, 
                                                                                     cheatSettingRects[i][cheat_index][s].LowerRightCorner.X - selection_box_padding,
                                                                                     cheatSettingRects[i][cheat_index][s].LowerRightCorner.Y - selection_box_padding);
 
                             core::rect<s32> textBox = core::rect<s32>(  cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width + selection_box_padding,
                                                                         cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height, 
                                                                         cheatSettingRects[i][cheat_index][s].LowerRightCorner.X - category_height,
                                                                         cheatSettingRects[i][cheat_index][s].LowerRightCorner.Y);
 
                             core::rect<s32> dropdownBox = core::rect<s32>(  cheatSettingRects[i][cheat_index][s].LowerRightCorner.X - category_height,
                                                                             cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height, 
                                                                             (cheatSettingRects[i][cheat_index][s].LowerRightCorner.X - selection_box_padding),
                                                                             cheatSettingRects[i][cheat_index][s].LowerRightCorner.Y - selection_box_padding);
 
                             driver->draw2DRectangle(cheat_color, selectionBoxRects[i][cheat_index][s]);
                             driver->draw2DRectangleOutline(selectionBoxRects[i][cheat_index][s], settingBarColor);
                             driver->draw2DRectangleOutline(dropdownBox, settingBarColor);
 
                             core::position2d<s32> dropdown_center(dropdownBox.UpperLeftCorner.X + dropdownBox.getWidth() / 2 , dropdownBox.UpperLeftCorner.Y + dropdownBox.getHeight() / 2);
                             if (isSelecting && selectingCategoryIndex == i && selectingCheatIndex == cheat_index && selectingSettingIndex == s) {
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), 1 + dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X, dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), 1 + dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                             } else {
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), 1 + dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X, dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                                 driver->draw2DLine(core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), 1 + dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                             }
                             
 
                             if (!g_settings->exists(cheatSetting->m_setting)) {
                                 std::string defaultValue = *(cheatSetting->m_options[0]);
 
                                 cheatSetting->set_value(defaultValue);
                             }
                             std::wstring wSelectedOption = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(g_settings->get(cheatSetting->m_setting));
 
                             textSizeU32 = font->getDimension(wSelectedOption.c_str());
                             textSize = core::dimension2d<s32>(textSizeU32.Width, textSizeU32.Height);
 
                             textX = textBox.UpperLeftCorner.X + (textBox.getWidth() - textSize.Width) / 2;
                             textY = textBox.UpperLeftCorner.Y + (textBox.getHeight() - textSize.Height) / 2;
 
                             font->draw(wSelectedOption.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), arrow_color);
                         } else if (cheatSetting->m_type == "bool") {
                             if(g_settings->exists(cheatSetting->m_setting) && g_settings->getBool(cheatSetting->m_setting) == true) {
                                 text_color = video::SColor(255, 62, 100, 219);
                             }
                             if(cheatSettingTextHovered[i][cheat_index][s] == true) {
                                 if (g_settings->exists(cheatSetting->m_setting) && g_settings->getBool(cheatSetting->m_setting) == true) {
                                     text_color = video::SColor(255, 31, 50, 109);
                                 } else {
                                     text_color = video::SColor(255, 127, 127, 127);
                                 }
                             }
                             const std::string& settingName = cheatSetting->m_name;
                             std::wstring wSettingName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(settingName);
                             
                             gui::IGUIFont* dropFont = g_fontengine->getFont(g_fontengine->getFontSize(FM_HD) * 0.95, FM_HD);
                             textSizeU32 = dropFont->getDimension(wSettingName.c_str());
                             core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
 
                             core::rect<s32> settingTextRect = core::rect<s32>(cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y, 
                                                                               cheatSettingRects[i][cheat_index][s].LowerRightCorner.X,
                                                                               cheatSettingRects[i][cheat_index][s].LowerRightCorner.Y);
 
                             s32 textX = settingTextRect.UpperLeftCorner.X + (settingTextRect.getWidth() - textSize.Width) / 2;
                             s32 textY = settingTextRect.UpperLeftCorner.Y + (settingTextRect.getHeight() - textSize.Height) / 2;
 
                             dropFont->draw(wSettingName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), text_color);
                         } else if (cheatSetting->m_type == "slider_int" || cheatSetting->m_type == "slider_float") {
                             std::string settingName = cheatSetting->m_name;
 
                             if (!g_settings->exists(cheatSetting->m_setting)) {
                                 cheatSetting->set_value(roundToNearestStep((cheatSetting->m_min + cheatSetting->m_max) / 2, cheatSetting->m_min, cheatSetting->m_max, cheatSetting->m_steps));
                             }
 
                             if (cheatSetting->m_type == "slider_int") {
                                 std::ostringstream oss;
                                 oss << " ( " << round(g_settings->getFloat(cheatSetting->m_setting)) << " )"; 
                                 settingName = settingName + oss.str();
                             } else if (cheatSetting->m_type == "slider_float") {
                                 std::ostringstream oss;
                                 oss.precision(1);
                                 oss << std::fixed << g_settings->getFloat(cheatSetting->m_setting);
                                 settingName = settingName + "( " + oss.str() + " )";
                             }
                             std::wstring wSettingName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(settingName);
                             textSizeU32 = font->getDimension(wSettingName.c_str());
                             core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
 
                             core::rect<s32> settingTextRect = core::rect<s32>(cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y, 
                                                                               cheatSettingRects[i][cheat_index][s].LowerRightCorner.X,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height);
 
                             s32 textX = settingTextRect.UpperLeftCorner.X + (settingTextRect.getWidth() - textSize.Width) / 2;
                             s32 textY = settingTextRect.UpperLeftCorner.Y + (settingTextRect.getHeight() - textSize.Height) / 2;
 
                             font->draw(wSettingName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), text_color);
 
                             cheatSliderBarRects[i][cheat_index][s] = core::rect<s32>(cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width + slider_width_padding,
                                                                             cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height + slider_height_padding, 
                                                                             cheatSettingRects[i][cheat_index][s].LowerRightCorner.X - slider_width_padding,
                                                                             cheatSettingRects[i][cheat_index][s].LowerRightCorner.Y - slider_height_padding);
 
                             core::rect<s32> filledRect;
                             core::rect<s32> remainingRect;
                             calculateSliderSplit(cheatSliderBarRects[i][cheat_index][s], g_settings->getFloat(cheatSetting->m_setting), cheatSetting->m_min, cheatSetting->m_max, filledRect, remainingRect);
                             driver->draw2DRectangle(sliderBarColorActive, filledRect);
                             driver->draw2DRectangle(sliderBarColor, remainingRect);
 
                             cheatSliderRects[i][cheat_index][s] = core::rect<s32>(filledRect.LowerRightCorner.X - slider_width_padding / 2,
                                                                                   filledRect.UpperLeftCorner.Y - slider_height_padding / 1.5,
                                                                                   filledRect.LowerRightCorner.X + slider_width_padding / 2,
                                                                                   filledRect.LowerRightCorner.Y + slider_height_padding / 1.5);
 
                             if (cheatSliderHovered[i][cheat_index][s]) {
                                 driver->draw2DRectangle(sliderColorActive, cheatSliderRects[i][cheat_index][s]);
                             } else {
                                 driver->draw2DRectangle(sliderColor, cheatSliderRects[i][cheat_index][s]);
                             }
                         } else if (cheatSetting->m_type == "text") {
                             std::wstring currentText = cheatSettingTextFields[i][cheat_index][s]->getText();
                             if (currentText != cheatSettingTextLasts[i][cheat_index][s]) {
                                 g_settings->set(script->m_cheat_categories[i]->m_cheats[cheat_index]->m_cheat_settings[s]->m_setting, std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(cheatSettingTextFields[i][cheat_index][s]->getText()));
                             }
                             std::wstring wSettingName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(cheatSetting->m_name);
                             textSizeU32 = font->getDimension(wSettingName.c_str());
                             core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
 
                             core::rect<s32> settingTextRect = core::rect<s32>(cheatSettingRects[i][cheat_index][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y, 
                                                                               cheatSettingRects[i][cheat_index][s].LowerRightCorner.X,
                                                                               cheatSettingRects[i][cheat_index][s].UpperLeftCorner.Y + category_height);
 
                             s32 textX = settingTextRect.UpperLeftCorner.X + (settingTextRect.getWidth() - textSize.Width) / 2;
                             s32 textY = settingTextRect.UpperLeftCorner.Y + (settingTextRect.getHeight() - textSize.Height) / 2;
 
                             font->draw(wSettingName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), text_color);
                         }
                     }
                 } else {
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), 1 + dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y + (unit_size * 1.5)), arrow_color);
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X, dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                     driver->draw2DLine(core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), 1 + dropdown_center.Y - (unit_size * 1.5)), arrow_color);
                 }
                 
             }
         }
     } else {
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, dropdown_center.Y + (unit_size * 1.5)), arrow_color);
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X - (unit_size * 3), 1 + dropdown_center.Y - (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y + (unit_size * 1.5)), arrow_color);
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X, dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), dropdown_center.Y - (unit_size * 1.5)), arrow_color);
         driver->draw2DLine(core::position2d<s32>(dropdown_center.X, 1 + dropdown_center.Y + (unit_size * 1.5)), core::position2d<s32>(dropdown_center.X + (unit_size * 3), 1 + dropdown_center.Y - (unit_size * 1.5)), arrow_color);
     }
 }
 
 void NewMenu::drawSelectionBox(video::IVideoDriver * driver, gui::IGUIFont * font, const size_t i, const size_t c, const size_t s)
 {
     GET_SCRIPT_POINTER
 
     for (size_t o = 0; o < script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options.size(); o++) {
         cheatSettingOptionRects[i][c][s][o] = core::rect<s32>(cheatSettingRects[i][c][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width + selection_box_padding,
                                                               cheatSettingRects[i][c][s].UpperLeftCorner.Y + (category_height - selection_box_padding) + ((o+1) * category_height), 
                                                               (cheatSettingRects[i][c][s].LowerRightCorner.X - selection_box_padding),
                                                               cheatSettingRects[i][c][s].LowerRightCorner.Y + ((o+1) * category_height));
 
         driver->draw2DRectangle(option_color, cheatSettingOptionRects[i][c][s][o]);
         std::wstring wOptionName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options[o]->c_str());
         
         core::dimension2d<u32> textSizeU32 = font->getDimension(wOptionName.c_str());
         core::dimension2d<s32> textSize(textSizeU32.Width, textSizeU32.Height);
 
         s32 textX = cheatSettingOptionRects[i][c][s][o].UpperLeftCorner.X + (cheatSettingOptionRects[i][c][s][o].getWidth() - textSize.Width) / 2;
         s32 textY = cheatSettingOptionRects[i][c][s][o].UpperLeftCorner.Y + (cheatSettingOptionRects[i][c][s][o].getHeight() - textSize.Height) / 2;
         video::SColor option_text_color = video::SColor(255, 255, 255, 255);
         if (cheatSettingOptionHovered[i][c][s][o]) {
             option_text_color = video::SColor(255, 127, 127, 127);
         }
         font->draw(wOptionName.c_str(), core::rect<s32>(textX, textY, textX + textSize.Width, textY + textSize.Height), option_text_color);
     }
     core::rect<s32> outlineBox = core::rect<s32>(cheatSettingRects[i][c][s].UpperLeftCorner.X + (setting_bar_padding * 2) + setting_bar_width + selection_box_padding,
                                                   cheatSettingRects[i][c][s].UpperLeftCorner.Y + (category_height - selection_box_padding) + category_height, 
                                                   (cheatSettingRects[i][c][s].LowerRightCorner.X - selection_box_padding),
                                                   cheatSettingRects[i][c][s].LowerRightCorner.Y + (script->m_cheat_categories[i]->m_cheats[c]->m_cheat_settings[s]->m_options.size() * category_height));
     driver->draw2DRectangleOutline(outlineBox, settingBarColor);
 }
 
 void NewMenu::drawHints(video::IVideoDriver* driver, gui::IGUIFont* font, const size_t i)
 {
     GET_SCRIPT_POINTER
 
     if (selectedCategory[i]) {
         for (size_t cheat_index = 0; cheat_index < script->m_cheat_categories[i]->m_cheats.size(); ++cheat_index) {
             if (cheatTextHovered[i][cheat_index] && g_settings->getBool("use_hints")) {
                 std::wstring wCheatDes = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(script->m_cheat_categories[i]->m_cheats[cheat_index]->m_description);
                 core::dimension2d<u32> textSizeU32_des = font->getDimension(wCheatDes.c_str());
                 core::dimension2d<s32> textSize_des(textSizeU32_des.Width, textSizeU32_des.Height);
                 const s32 padding = 8;
 
                 // Ensure background accounts for padding
                 s32 backgroundWidth = textSize_des.Width + 2 * padding;
                 s32 backgroundHeight = textSize_des.Height + 2 * padding;
 
                 // Corrected centering using background width instead of text width
                 s32 backgroundX = cheatRects[i][cheat_index].UpperLeftCorner.X + 
                                   (cheatRects[i][cheat_index].getWidth() - backgroundWidth) / 2;
                 s32 backgroundY = cheatRects[i][cheat_index].LowerRightCorner.Y + padding / 2;
 
                 if (!wCheatDes.empty()) {
                     driver->draw2DRectangle(video::SColor(255, 40, 5, 20), core::rect<s32>(backgroundX, backgroundY, backgroundX + backgroundWidth, backgroundY + backgroundHeight));
                     driver->draw2DRectangleOutline(core::rect<s32>(backgroundX, backgroundY, backgroundX + backgroundWidth, backgroundY + backgroundHeight), video::SColor(255, 125, 15, 60));
                 }
 
                 // Text should be inside the padded area
                 s32 textXD = backgroundX + padding;
                 s32 textYD = backgroundY + padding;
 
                 font->draw(wCheatDes.c_str(), 
                         core::rect<s32>(textXD, textYD, textXD + textSize_des.Width, textYD + textSize_des.Height), 
                         video::SColor(255, 255, 255, 255));
             }
         }
     }
 }
 
 
 
 void NewMenu::draw() 
 {
     if (m_client == nullptr || m_client->isShutdown()) {
         return;
     }
 
     GET_SCRIPT_POINTER
 
     float dtime = getDeltaTime();
 
     video::IVideoDriver* driver = Environment->getVideoDriver();
     gui::IGUIFont* font = g_fontengine->getFont(FONT_SIZE_UNSPECIFIED, FM_HD);
     if (g_settings->exists("use_menu_grid") && g_settings->getBool("use_menu_grid") == true) {
         if (isDragging) {
             for (size_t x = 0; x <= driver->getScreenSize().Width; x += category_height / 2) {
                 driver->draw2DLine(core::position2d<s32>(x, 0), core::position2d<s32>(x, driver->getScreenSize().Height), video::SColor(50, 255, 255, 255));
             }
 
             for (size_t y = 0; y <= driver->getScreenSize().Height; y += category_height / 2) {
                 driver->draw2DLine(core::position2d<s32>(0, y), core::position2d<s32>(driver->getScreenSize().Width, y), video::SColor(50, 255, 255, 255));
             }
         }
     }
     
     if (m_is_open) {
         for (size_t i = 0; i < script->m_cheat_categories.size(); i++) {
             if (driver->getScreenSize() != lastScreenSize) {
                 moveMenu(i, category_positions[i]);
             }
             drawCategory(driver, font, i, dtime);
         }
         for (size_t i = 0; i < script->m_cheat_categories.size(); i++) {
             drawHints(driver, font, i);
         }
         if (isSelecting) {
             drawSelectionBox(driver, font, selectingCategoryIndex, selectingCheatIndex, selectingSettingIndex);
         }
         driver->draw2DRectangleOutline(core::rect<s32>(coords_sprite.x, coords_sprite.y, coords_sprite.x + coords_sprite.width, coords_sprite.y + coords_sprite.height), video::SColor(255, 255, 0, 255));
     }
 } 