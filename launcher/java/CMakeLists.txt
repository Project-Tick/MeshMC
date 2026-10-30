target_sources(MeshMC_logic PRIVATE
    JavaChecker.h
    JavaChecker.cpp
    JavaCheckerJob.h
    JavaCheckerJob.cpp
    JavaInstall.h
    JavaInstall.cpp
    JavaInstallList.h
    JavaInstallList.cpp
    JavaUtils.h
    JavaUtils.cpp
    JavaVersion.h
    JavaVersion.cpp
)

add_subdirectory(download)
