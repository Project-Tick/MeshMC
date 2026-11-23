/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include "modplatform/ContentProviderModel.h"

/* Modrinth search results. Everything except the shape of the JSON
 * lives in the base class. */
class ModrinthContentModel final : public ContentProviderModel
{
	Q_OBJECT

  public:
	explicit ModrinthContentModel(ModPlatform::ContentType contentType,
								  ModPlatform::SearchFilters filters,
								  QObject* parent = nullptr);

  protected:
	QList<ModPlatform::IndexedProject>
	parseSearchResponse(const QByteArray& bytes, int& totalHits) const override;

	ModPlatform::IndexedProject
	parseProjectResponse(const QByteArray& bytes) const override;

	QList<ModPlatform::ContentVersion>
	parseVersionsResponse(const QByteArray& bytes,
						  const ModPlatform::IndexedProject& project)
		const override;

	QString parseBodyResponse(const QByteArray& bytes) const override;

	QList<ModPlatform::Category>
	parseCategoriesResponse(const QByteArray& bytes) const override;

	QString iconCacheName() const override;
};
