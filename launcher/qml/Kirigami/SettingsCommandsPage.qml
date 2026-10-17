// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Custom Commands: shell commands run around every launch, launcher-wide.
 * Each instance can replace them on its own Settings tab.
 */
SettingsPage {
    id: page

    FormCard.FormHeader {
        title: i18nc("@title:group", "Commands")
    }
    CommandsCard {
        source: page.store
    }
    FormCard.FormSectionText {
        text: i18n("You can use $INST_NAME, $INST_ID, $INST_DIR, $INST_MC_DIR, $INST_JAVA and $INST_JAVA_ARGS in them.")
    }
}
