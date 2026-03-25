#!/bin/bash

INSTDIR="${XDG_DATA_HOME-$HOME/.local/share}/meshmc"

if [ `getconf LONG_BIT` = "64" ]
then
    PACKAGE="mmc-stable-lin64.tar.gz"
else
    PACKAGE="mmc-stable-lin32.tar.gz"
fi

deploy() {
    mkdir -p $INSTDIR
    cd ${INSTDIR}

    wget --progress=dot:force "https://files.projecttick.org/downloads/${PACKAGE}" 2>&1 | sed -u 's/.* \([0-9]\+%\)\ \+\([0-9.]\+.\) \(.*\)/\1\n# Downloading at \2\/s, ETA \3/' | zenity --progress --auto-close --auto-kill --title="Downloading MeshMC..."

    tar -xzf ${PACKAGE} --transform='s,MeshMC/,,'
    rm ${PACKAGE}
    chmod +x MeshMC
}

runmmc() {
    cd ${INSTDIR}
    exec ./MeshMC "$@"
}

if [[ ! -f ${INSTDIR}/MeshMC ]]; then
    deploy
    runmmc "$@"
else
    runmmc "$@"
fi
