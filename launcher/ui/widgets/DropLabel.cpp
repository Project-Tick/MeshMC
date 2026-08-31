/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
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

#include "DropLabel.h"

#include <QMimeData>
#include <QDropEvent>

DropLabel::DropLabel(QWidget* parent) : QLabel(parent)
{
	setAcceptDrops(true);
}

void DropLabel::dragEnterEvent(QDragEnterEvent* event)
{
	event->acceptProposedAction();
}

void DropLabel::dragMoveEvent(QDragMoveEvent* event)
{
	event->acceptProposedAction();
}

void DropLabel::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}

void DropLabel::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();

	if (!mimeData) {
		return;
	}

	if (mimeData->hasUrls()) {
		auto urls = mimeData->urls();
		emit droppedURLs(urls);
	}

	event->acceptProposedAction();
}
