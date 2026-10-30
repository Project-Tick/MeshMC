target_sources(MeshMC_logic PRIVATE
    AccountData.cpp
    AccountData.h
    AccountList.cpp
    AccountList.h
    AccountTask.cpp
    AccountTask.h
    AuthRequest.cpp
    AuthRequest.h
    AuthSession.cpp
    AuthSession.h
    AuthStep.cpp
    AuthStep.h
    MinecraftAccount.cpp
    MinecraftAccount.h
    Parsers.cpp
    Parsers.h
)

add_subdirectory(flows)
add_subdirectory(steps)
