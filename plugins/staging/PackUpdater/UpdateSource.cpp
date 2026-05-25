/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * UpdateSource factory.
 *
 * Each concrete source exposes a `make<Provider>Source_internal()`
 * symbol from its own TU; the factory just picks the right one
 * based on the record's provider. Decoupled this way so adding a
 * fifth catalogue (Technic? FTB?) is a one-line change here.
 */

#include "UpdateSource.h"

namespace pack_updater
{

	/* Forward declarations — symbols defined in the per-provider TUs. */
	UpdateSource* makeModrinthSource_internal();
	UpdateSource* makeCurseForgeSource_internal();

	std::unique_ptr<UpdateSource> makeSource(Provider p)
	{
		switch (p) {
			case Provider::Modrinth:
				return std::unique_ptr<UpdateSource>(
					makeModrinthSource_internal());
			case Provider::CurseForge:
				return std::unique_ptr<UpdateSource>(
					makeCurseForgeSource_internal());
			case Provider::MultiMC:
			case Provider::Unknown:
			default:
				/* No upstream to ask. The page handles this by
				 * showing a "this provider has no update channel"
				 * note instead of calling fetchLatest. */
				return nullptr;
		}
	}

} /* namespace pack_updater */
