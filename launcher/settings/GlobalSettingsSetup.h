/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

class GlobalSettings;
class KConfigSettingsObject;

/*!
 * Registers the global settings with the settings object: every entry of
 * GlobalSettings.kcfg, with the default the compiled GlobalSettings class
 * has for it, under the name it has in the file and the older names it had
 * before, and makes that class the skeleton of the settings object.
 */
void registerGlobalSettings(KConfigSettingsObject* settings);

/*!
 * The typed accessors of the global settings: settings->proxyPort() instead
 * of settings->get("ProxyPort").toInt(). The same values the settings object
 * has: a set() is seen here, and a setter followed by save() is seen by a get().
 * Only valid after registerGlobalSettings().
 */
GlobalSettings* typedGlobalSettings(KConfigSettingsObject* settings);
