/* Copyright 2013-2021 MultiMC Contributors
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

#include "OpSys.h"

OpSys OpSys_fromString(QString name)
{
    if (name == "freebsd")
        return Os_FreeBSD;
    if (name == "linux")
        return Os_Linux;
    if (name == "windows")
        return Os_Windows;
    if (name == "osx")
        return Os_OSX;
    if (name == "osx-arm64")
        return Os_OSX_ARM64;

    return Os_Other;
}

QString OpSys_toString(OpSys name)
{
    switch (name)
    {
    case Os_FreeBSD:
        return "freebsd";
    case Os_Linux:
        return "linux";
    case Os_OSX:
        return "osx";
    case Os_OSX_ARM64:
        return "osx-arm64";
    case Os_Windows:
        return "windows";
    default:
        return "other";
    }
}

OpSys libraryRuleSystem(Sys::Architecture javaArchitecture)
{
#ifdef Q_OS_MAC
    if (!javaArchitecture.is64() && !javaArchitecture.is32())
        javaArchitecture = Sys::systemArchitecture();
    if (javaArchitecture == Sys::Architecture(Sys::ArchitectureType::AARCH64))
        return Os_OSX_ARM64;
#else
    Q_UNUSED(javaArchitecture);
#endif
    return currentSystem;
}
