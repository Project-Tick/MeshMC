#include "SystemRequirements.h"

#include <QObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QProcess>
#include <QSysInfo>
#include <QVector>

#include <functional>

#include <sys.h>

#ifdef Q_OS_LINUX
#include <elf.h>
#include <cstring>
#endif

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
#include <gnu/libc-version.h>
#include <link.h>
#endif

#if defined(Q_OS_MAC)
#include <sys/sysctl.h>
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

#include <QDebug>

namespace {

bool parseVersion(const QString &version, QVector<uint> &parts)
{
    parts.clear();
    if(version.isEmpty())
    {
        return false;
    }
    for(auto &part: version.split('.'))
    {
        // at most 9 digits, so it fits
        if(part.isEmpty() || part.size() > 9 || (part.size() > 1 && part[0] == '0'))
        {
            return false;
        }
        for(auto c: part)
        {
            if(c < '0' || c > '9')
            {
                return false;
            }
        }
        parts.append(part.toUInt());
    }
    return true;
}

QString archName(const Sys::Architecture &arch)
{
    if(arch == Sys::Architecture(Sys::ArchitectureType::X86))
    {
        return "x86";
    }
    if(arch == Sys::Architecture(Sys::ArchitectureType::AMD64))
    {
        return "x86_64";
    }
    if(arch == Sys::Architecture(Sys::ArchitectureType::AARCH64))
    {
        return "arm64";
    }
    return arch.getRaw();
}

QString osDisplayName(const QString &os)
{
    if(os == "windows")
    {
        return "Windows";
    }
    if(os == "macos")
    {
        return "macOS";
    }
    if(os == "linux")
    {
        return "Linux";
    }
    return os;
}

QString windowsDisplayName(const QString &ntVersion)
{
    QVector<uint> parts;
    if(!parseVersion(ntVersion, parts))
    {
        return QString("Windows NT %1").arg(ntVersion);
    }
    parts.resize(2);
    QString name;
    if(parts[0] == 5 && parts[1] == 1)
    {
        name = "Windows XP";
    }
    else if(parts[0] == 5 && parts[1] == 2)
    {
        name = "Windows XP x64";
    }
    else if(parts[0] == 6 && parts[1] == 0)
    {
        name = "Windows Vista";
    }
    else if(parts[0] == 6 && parts[1] == 1)
    {
        name = "Windows 7";
    }
    else if(parts[0] == 6 && parts[1] == 2)
    {
        name = "Windows 8";
    }
    else if(parts[0] == 6 && parts[1] == 3)
    {
        name = "Windows 8.1";
    }
    else if(parts[0] == 10 && parts[1] == 0)
    {
        name = "Windows 10";
    }
    else
    {
        return QString("Windows NT %1").arg(ntVersion);
    }
    return QString("%1 (NT %2)").arg(name, ntVersion);
}

QString osVersionDisplayName(const QString &os, const QString &version)
{
    if(os == "windows")
    {
        return windowsDisplayName(version);
    }
    return QString("%1 %2").arg(osDisplayName(os), version);
}

// Whether a machine of this architecture runs builds for that one
bool archRuns(const QString &os, const QString &machine, const QString &build)
{
    if(machine == build)
    {
        return true;
    }
    if(os == "windows" && machine == "x86_64" && build == "x86")
    {
        return true;
    }
    if(os == "macos" && build == "universal" && (machine == "x86_64" || machine == "arm64"))
    {
        return true;
    }
    // Rosetta
    if(os == "macos" && machine == "arm64" && build == "x86_64")
    {
        return true;
    }
    return false;
}

#ifdef Q_OS_LINUX
template <typename T>
bool readElfRecord(QFile &file, quint64 offset, T &record)
{
    const auto size = static_cast<quint64>(file.size());
    return offset <= size && sizeof(record) <= size - offset &&
           file.seek(static_cast<qint64>(offset)) &&
           file.read(reinterpret_cast<char *>(&record), sizeof(record)) == static_cast<qint64>(sizeof(record));
}

bool readX86ElfHeader(QFile &file, Elf64_Ehdr &header)
{
    return QFileInfo(file).isFile() && file.open(QIODevice::ReadOnly) && readElfRecord(file, 0, header) &&
           std::memcmp(header.e_ident, ELFMAG, SELFMAG) == 0 &&
           header.e_ident[EI_CLASS] == ELFCLASS64 && header.e_ident[EI_DATA] == ELFDATA2LSB &&
           header.e_machine == EM_X86_64 && header.e_type == ET_DYN;
}

bool providesVersions(QFile &file, const Elf64_Ehdr &header, QSet<QString> missing)
{
    if(missing.isEmpty())
        return true;
    if(header.e_shentsize != sizeof(Elf64_Shdr))
        return false;

    const auto fileSize = static_cast<quint64>(file.size());
    if(header.e_shoff > fileSize || quint64(header.e_shnum) * sizeof(Elf64_Shdr) > fileSize - header.e_shoff)
        return false;

    for(unsigned int index = 0; index < header.e_shnum; ++index)
    {
        Elf64_Shdr definitions;
        if(!readElfRecord(file, header.e_shoff + index * sizeof(definitions), definitions))
            return false;
        if(definitions.sh_type != SHT_GNU_verdef)
            continue;
        Elf64_Shdr strings;
        if(definitions.sh_link >= header.e_shnum ||
           !readElfRecord(file, header.e_shoff + definitions.sh_link * sizeof(strings), strings) ||
           strings.sh_type != SHT_STRTAB || strings.sh_offset > fileSize ||
           strings.sh_size > fileSize - strings.sh_offset || strings.sh_size > 16 * 1024 * 1024 ||
           definitions.sh_offset > fileSize || definitions.sh_size > fileSize - definitions.sh_offset ||
           !file.seek(static_cast<qint64>(strings.sh_offset)))
            return false;
        const QByteArray names = file.read(static_cast<qint64>(strings.sh_size));
        if(static_cast<quint64>(names.size()) != strings.sh_size)
            return false;

        quint64 offset = 0;
        while(offset < definitions.sh_size)
        {
            Elf64_Verdef definition;
            Elf64_Verdaux name;
            const quint64 remaining = definitions.sh_size - offset;
            if(remaining < sizeof(definition) ||
               !readElfRecord(file, definitions.sh_offset + offset, definition) ||
               definition.vd_version != VER_DEF_CURRENT || definition.vd_cnt == 0 ||
               definition.vd_aux < sizeof(definition) || definition.vd_aux > remaining ||
               remaining - definition.vd_aux < sizeof(name) ||
               !readElfRecord(file, definitions.sh_offset + offset + definition.vd_aux, name) ||
               name.vda_name >= strings.sh_size)
                return false;

            const char *begin = names.constData() + name.vda_name;
            const auto *end = static_cast<const char *>(std::memchr(begin, 0, strings.sh_size - name.vda_name));
            if(!end)
                return false;
            missing.remove(QString::fromLatin1(begin, static_cast<int>(end - begin)));
            if(missing.isEmpty())
                return true;
            if(definition.vd_next == 0)
                break;
            if(definition.vd_next < sizeof(definition) || definition.vd_next > remaining)
                return false;
            offset += definition.vd_next;
        }
    }
    return false;
}

QHash<QString, QStringList> systemElfLibraries()
{
    QHash<QString, QStringList> libraries;
    const QString ldconfig = QStandardPaths::findExecutable("ldconfig", {"/sbin", "/usr/sbin"});
    if(ldconfig.isEmpty())
        return libraries;

    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
    process.setProcessEnvironment(environment);
    process.start(ldconfig, {"-p"});
    if(!process.waitForFinished(1500))
    {
        process.kill();
        process.waitForFinished(500);
        return libraries;
    }
    if(process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return libraries;

    const QRegularExpression entry(QStringLiteral(R"(\s*(\S+)\s+\([^
]*\) => (/[^
]+))"));
    const auto output = QString::fromLocal8Bit(process.readAllStandardOutput());
    auto matches = entry.globalMatch(output);
    while(matches.hasNext())
    {
        const auto match = matches.next();
        libraries[match.captured(1)].append(match.captured(2));
    }
    return libraries;
}

bool probeLinuxAbi(const QJsonObject &requirements)
{
    const QString interpreter = requirements.value("interpreter").toString();
    const auto dependencies = requirements.value("libs").toArray();
    if(requirements.value("format").toInt() != 1 || requirements.value("arch").toString() != "x86_64" ||
       !QDir::isAbsolutePath(interpreter) || dependencies.isEmpty())
        return false;
    QFile loader(interpreter);
    Elf64_Ehdr header;
    if(!readX86ElfHeader(loader, header))
        return false;

    const auto cache = systemElfLibraries();
    const auto overrides = QString::fromLocal8Bit(qgetenv("LD_LIBRARY_PATH")).split(':');
    const QStringList defaults = {"/lib64", "/usr/lib64", "/lib/x86_64-linux-gnu",
                                 "/usr/lib/x86_64-linux-gnu", "/lib", "/usr/lib"};
    for(const auto &dependency : dependencies)
    {
        const auto library = dependency.toObject();
        const QString name = library.value("soname").toString();
        if(name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
           !library.value("versions").isArray())
            return false;
        QSet<QString> versions;
        for(const auto &version : library.value("versions").toArray())
        {
            if(!version.isString() || version.toString().isEmpty())
                return false;
            versions.insert(version.toString());
        }

        QStringList candidates;
        for(const auto &directory : overrides)
        {
            if(!directory.isEmpty())
                candidates.append(QDir(directory).filePath(name));
        }
        candidates.append(cache.value(name));
        for(const auto &directory : defaults)
            candidates.append(QDir(directory).filePath(name));

        bool found = false;
        for(const auto &path : candidates)
        {
            QFile file(path);
            if(!readX86ElfHeader(file, header))
                continue;
            if(!providesVersions(file, header, versions))
                return false;
            found = true;
            break;
        }
        if(!found)
            return false;
    }
    return true;
}
#endif

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
/*
 * Finds the highest GLIBCXX_ version the loaded libstdc++ defines, from the version
 * definitions in its dynamic section. libstdc++ has no call that says this.
 */
int findGlibcxxVersion(struct dl_phdr_info *info, size_t, void *data)
{
    auto result = static_cast<QString *>(data);
    const char *slash = std::strrchr(info->dlpi_name, '/');
    const char *fileName = slash ? slash + 1 : info->dlpi_name;
    if(std::strncmp(fileName, "libstdc++.so", 12) != 0)
    {
        return 0;
    }

    const ElfW(Dyn) *dynamic = nullptr;
    for(int i = 0; i < info->dlpi_phnum; i++)
    {
        if(info->dlpi_phdr[i].p_type == PT_DYNAMIC)
        {
            dynamic = reinterpret_cast<const ElfW(Dyn) *>(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
        }
    }
    if(!dynamic)
    {
        return 1;
    }
    ElfW(Addr) strtab = 0;
    ElfW(Addr) verdef = 0;
    ElfW(Xword) verdefCount = 0;
    for(auto entry = dynamic; entry->d_tag != DT_NULL; entry++)
    {
        switch(entry->d_tag)
        {
            case DT_STRTAB:
                strtab = entry->d_un.d_ptr;
                break;
            case DT_VERDEF:
                verdef = entry->d_un.d_ptr;
                break;
            case DT_VERDEFNUM:
                verdefCount = entry->d_un.d_val;
                break;
        }
    }
    if(!strtab || !verdef)
    {
        return 1;
    }
    // The dynamic linker relocates these in place on most architectures, but not on all
    if(strtab < info->dlpi_addr)
    {
        strtab += info->dlpi_addr;
    }
    if(verdef < info->dlpi_addr)
    {
        verdef += info->dlpi_addr;
    }

    auto definition = reinterpret_cast<const ElfW(Verdef) *>(verdef);
    for(ElfW(Xword) i = 0; i < verdefCount; i++)
    {
        auto aux = reinterpret_cast<const ElfW(Verdaux) *>(reinterpret_cast<const char *>(definition) + definition->vd_aux);
        QString name = QString::fromLatin1(reinterpret_cast<const char *>(strtab + aux->vda_name));
        if(name.startsWith("GLIBCXX_"))
        {
            auto version = name.mid(8);
            if(SystemRequirements::isValidVersion(version) && (result->isEmpty() || SystemRequirements::compareVersions(version, *result) > 0))
            {
                *result = version;
            }
        }
        if(!definition->vd_next)
        {
            break;
        }
        definition = reinterpret_cast<const ElfW(Verdef) *>(reinterpret_cast<const char *>(definition) + definition->vd_next);
    }
    return 1;
}
#endif

}

bool SystemRequirements::isValidVersion(const QString &version)
{
    QVector<uint> parts;
    return parseVersion(version, parts);
}

int SystemRequirements::compareVersions(const QString &a, const QString &b)
{
    QVector<uint> partsA;
    QVector<uint> partsB;
    parseVersion(a, partsA);
    parseVersion(b, partsB);
    int size = qMax(partsA.size(), partsB.size());
    partsA.resize(size);
    partsB.resize(size);
    for(int i = 0; i < size; i++)
    {
        if(partsA[i] != partsB[i])
        {
            return partsA[i] < partsB[i] ? -1 : 1;
        }
    }
    return 0;
}

namespace {

#if defined(Q_OS_WIN)
// Compatibility mode makes Windows report the version it emulates to GetVersionEx (and so to
// Sys::getKernelInfo), whatever the manifest says, and to the registry's CurrentBuildNumber
// (tried 2026-11-12 on Windows 11 in XP, 7 and 8 modes). These two give the real version;
// they are tried in order, and the log lists them when they disagree.

// The kernel's own numbers, from the page it shares read-only with every process
// (KUSER_SHARED_DATA, laid out in the WDK's ntddk.h). 0x7FFE0000 is its user-mode address,
// the same in 32-bit and 64-bit Windows; the kernel's own addresses (0xFFDF0000,
// 0xFFFFF78000000000) differ, but aren't ours. ARM64 is believed to be the same and not
// checked yet, so detect() only uses this when it agrees with ntdll. The build number is
// only there since Windows 10.
QString kernelSharedVersion()
{
    auto shared = reinterpret_cast<const volatile BYTE *>(0x7FFE0000);
    auto value = [shared](int offset)
    {
        return *reinterpret_cast<const volatile ULONG *>(shared + offset);
    };
    ULONG build = value(0x260);
    ULONG major = value(0x26C);
    ULONG minor = value(0x270);
    if(major < 10 || build == 0)
    {
        return QString();
    }
    return QString("%1.%2.%3").arg(major).arg(minor).arg(build);
}

// ntdll's numbers, compiled into it (the build number's top bits say what kind of build it is)
QString ntdllVersion()
{
    typedef void(WINAPI * Numbers)(DWORD *, DWORD *, DWORD *);
    auto ntdll = GetModuleHandleW(L"ntdll.dll");
    auto numbers = ntdll ? reinterpret_cast<Numbers>(GetProcAddress(ntdll, "RtlGetNtVersionNumbers")) : nullptr;
    if(!numbers)
    {
        return QString();
    }
    DWORD major = 0;
    DWORD minor = 0;
    DWORD build = 0;
    numbers(&major, &minor, &build);
    build &= 0xFFFF;
    if(major == 0 || build == 0)
    {
        return QString();
    }
    return QString("%1.%2.%3").arg(major).arg(minor).arg(build);
}
#endif

#if defined(Q_OS_MAC)
// macOS 11 and later tell a program built with an older SDK that they are 10.16, through
// what Qt asks. This sysctl (macOS 10.13.4 and later) has the real version.
QString realMacOSVersion()
{
    char buffer[64] = {};
    size_t size = sizeof(buffer) - 1;
    if(sysctlbyname("kern.osproductversion", buffer, &size, nullptr, 0) != 0)
    {
        return QString();
    }
    return QString::fromLatin1(buffer).trimmed();
}
#endif

}

namespace {
SystemRequirements::System detectNow();
}

SystemRequirements::System SystemRequirements::detect()
{
    // the system doesn't change while the Launcher runs
    static const System system = detectNow();
    return system;
}

namespace {
SystemRequirements::System detectNow()
{
    using namespace SystemRequirements;
    System out;
    auto kernel = Sys::getKernelInfo();
#if defined(Q_OS_WIN)
    out.os = "windows";
    // GetVersionEx: truthful thanks to the manifest, except in compatibility mode
    out.osVersion = QString("%1.%2.%3").arg(kernel.kernelMajor).arg(kernel.kernelMinor).arg(kernel.kernelPatch);
    {
        auto reported = out.osVersion;
        auto shared = kernelSharedVersion();
        auto ntdll = ntdllVersion();
        // the shared page only when its major.minor is ntdll's: a wrong address or layout
        // (a new architecture) then can't make up a version
        auto majorMinor = [](const QString &version)
        {
            return version.section('.', 0, 1);
        };
        if(isValidVersion(shared) && isValidVersion(ntdll) && majorMinor(shared) == majorMinor(ntdll))
        {
            out.osVersion = shared;
        }
        else if(isValidVersion(ntdll))
        {
            out.osVersion = ntdll;
        }
        if(out.osVersion != reported || (!shared.isEmpty() && shared != ntdll))
        {
            qDebug() << "Windows version: reported" << reported << "kernel" << shared << "ntdll" << ntdll
                     << "(compatibility mode?), using" << out.osVersion;
        }
    }
    out.machineArch = archName(Sys::systemArchitecture());
#elif defined(Q_OS_MAC)
    out.os = "macos";
    out.osVersion = QSysInfo::productVersion();
    auto real = realMacOSVersion();
    if(isValidVersion(real) && real != out.osVersion)
    {
        qDebug() << "macOS reports version" << out.osVersion << "but kern.osproductversion says" << real << ", using" << real;
        out.osVersion = real;
    }
    out.machineArch = archName(Sys::systemArchitecture());
    // under Rosetta, uname says x86_64
    int translated = 0;
    size_t size = sizeof(translated);
    if(sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0 && translated == 1)
    {
        out.machineArch = "arm64";
    }
#else
#if defined(Q_OS_LINUX)
    out.os = "linux";
#else
    out.os = QSysInfo::kernelType();
#endif
    out.osVersion = QString("%1.%2.%3").arg(kernel.kernelMajor).arg(kernel.kernelMinor).arg(kernel.kernelPatch);
    out.machineArch = archName(Sys::systemArchitecture());
#endif
    out.launcherArch = archName(Sys::Architecture::fromOSArch(QSysInfo::buildCpuArchitecture()));
    if(!isValidVersion(out.osVersion) || out.osVersion == "0.0.0")
    {
        out.osVersion.clear();
    }

#if defined(Q_OS_LINUX) && defined(__GLIBC__)
    out.glibc = QString::fromLatin1(gnu_get_libc_version());
    if(!isValidVersion(out.glibc))
    {
        out.glibc.clear();
    }
    dl_iterate_phdr(findGlibcxxVersion, &out.glibcxx);
#endif

#ifdef Q_OS_LINUX
    out.probeAbi = probeLinuxAbi;
#endif
    out.qt = QString::fromLatin1(qVersion());
    return out;
}
}

QString SystemRequirements::requirementArch(const System &system)
{
    if(system.os == "linux")
    {
        return system.launcherArch;
    }
    return system.machineArch;
}

QStringList SystemRequirements::unmetRequirements(const Requirements &requirements, const System &system)
{
    QStringList out;
    const auto &os = requirements.os;
    auto incomplete = [&](const QString &what)
    {
        out.append(QObject::tr("The build's requirements are incomplete or not understood (%1).").arg(what));
    };

    if(os != "windows" && os != "macos" && os != "linux")
    {
        incomplete(QString("os %1").arg(os));
        return out;
    }
    if(system.os != os)
    {
        out.append(QObject::tr("The build is for %1, this computer runs %2.").arg(osDisplayName(os), osDisplayName(system.os)));
        return out;
    }

    if(requirements.arch.isEmpty())
    {
        incomplete("arch");
    }
    else if(!archRuns(os, requirementArch(system), requirements.arch))
    {
        auto arch = requirementArch(system);
        out.append(QObject::tr("The build needs a %1 processor, this computer has %2.").arg(requirements.arch, arch.isEmpty() ? QObject::tr("an unknown one") : arch));
    }
    for(auto &key: requirements.unknown)
    {
        incomplete(key);
    }

    // a minimum version: `what` names the requirement, `describe` makes a version readable
    auto checkMinimum = [&](const QString &what, const QString &required, const QString &found, std::function<QString(const QString &)> describe)
    {
        if(!isValidVersion(required))
        {
            incomplete(QString("%1 %2").arg(what, required));
        }
        else if(!isValidVersion(found))
        {
            out.append(QObject::tr("The build needs %1 or newer, and the version on this computer could not be found out.").arg(describe(required)));
        }
        else if(compareVersions(found, required) < 0)
        {
            out.append(QObject::tr("The build needs %1 or newer, this computer has %2.").arg(describe(required), describe(found)));
        }
    };

    if(os == "windows" || os == "macos")
    {
        checkMinimum("os_version", requirements.osVersion, system.osVersion, [&](const QString &version)
        {
            return osVersionDisplayName(os, version);
        });
    }
    else
    {
        checkMinimum("glibc", requirements.glibc, system.glibc, [](const QString &version)
        {
            return QString("glibc %1").arg(version);
        });
        checkMinimum("glibcxx", requirements.glibcxx, system.glibcxx, [](const QString &version)
        {
            return QString("libstdc++ GLIBCXX_%1").arg(version);
        });

        QVector<uint> required;
        QVector<uint> found;
        if(!parseVersion(requirements.qt, required))
        {
            incomplete(QString("qt %1").arg(requirements.qt));
        }
        else if(!requirements.abi.isEmpty())
        {
            if(out.isEmpty() && (!system.probeAbi || !system.probeAbi(requirements.abi)))
                out.append(QObject::tr("The required system libraries for Qt %1 could not be verified.").arg(requirements.qt));
        }
        else if(!parseVersion(system.qt, found))
        {
            out.append(QObject::tr("The build needs Qt %1 or a newer Qt %2, and the version on this computer could not be found out.").arg(requirements.qt).arg(required[0]));
        }
        else if(found[0] != required[0] || compareVersions(system.qt, requirements.qt) < 0)
        {
            out.append(QObject::tr("The build needs Qt %1 or a newer Qt %2, this computer has Qt %3.").arg(requirements.qt).arg(required[0]).arg(system.qt));
        }
    }
    return out;
}
