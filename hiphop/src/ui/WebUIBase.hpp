/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEB_UI_BASE_HPP
#define WEB_UI_BASE_HPP

#include "extra/UIEx.hpp"

START_NAMESPACE_DISTRHO

class WebUIBase : public UIEx
{
public:
    WebUIBase(uint width = 0, uint height = 0);
    virtual ~WebUIBase();

private:

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebUIBase)

};

END_NAMESPACE_DISTRHO

#endif  // WEB_UI_BASE_HPP