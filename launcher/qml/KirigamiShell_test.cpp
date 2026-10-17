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

#include <QtTest>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickStyle>
#include <QStandardItemModel>
#include <QUrl>

#include <KIconTheme>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <memory>

/*
 * The KDE interface (MeshMC.Kirigami) end to end, headlessly: the module is
 * built and at its resource path, its root window loads under the controls
 * style QmlShell gives it, and the window's navigation -- the one place every
 * page, the drawer and the C++ side go through -- opens what it is asked
 * for. Same shape as QmlModule_test, which keeps covering the custom design
 * until that is deleted.
 *
 * The URL is spelled out rather than using QQmlComponent::loadFromModule(),
 * which is Qt 6.5+; see QmlModule_test.cpp.
 */

/* The instance page's InstanceDetails, as far as these tests read it. */
class FakeDetails : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString instanceId READ instanceId CONSTANT)
	Q_PROPERTY(QString name READ name CONSTANT)
	Q_PROPERTY(bool isLegacy READ isLegacy CONSTANT)

  public:
	QString instanceId() const
	{
		return QStringLiteral("fabric");
	}
	QString name() const
	{
		return QStringLiteral("Fabric Optimized");
	}
	bool isLegacy() const
	{
		return false;
	}
};

/* A modpack's lazily loaded detail (ModrinthModpackDetail), empty. */
class FakePackDetail : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString projectId MEMBER m_projectId NOTIFY changed)
	Q_PROPERTY(QString bodyHtml MEMBER m_empty CONSTANT)
	Q_PROPERTY(QVariantList versions MEMBER m_none CONSTANT)
	Q_PROPERTY(QVariantList gallery MEMBER m_none CONSTANT)
	Q_PROPERTY(QVariantList links MEMBER m_none CONSTANT)
	Q_PROPERTY(bool loading MEMBER m_loading CONSTANT)
	Q_PROPERTY(QString error MEMBER m_empty CONSTANT)

  public:
	QString m_projectId;
	QString m_empty;
	QVariantList m_none;
	bool m_loading = true;

  signals:
	void changed();
};

/* One provider's modpack model (ModrinthModpackModel and its siblings), as
 * far as Discover reads it: one result row, and a record of what it was
 * asked to do. */
class FakeModpackModel : public QStandardItemModel
{
	Q_OBJECT
	Q_PROPERTY(QString query MEMBER m_query NOTIFY changed)
	Q_PROPERTY(QString sort MEMBER m_sort NOTIFY changed)
	Q_PROPERTY(QVariantList sortOptions MEMBER m_sortOptions CONSTANT)
	Q_PROPERTY(QString loader MEMBER m_loader NOTIFY changed)
	Q_PROPERTY(bool searching MEMBER m_searching CONSTANT)
	Q_PROPERTY(bool canFetchMore MEMBER m_canFetchMore CONSTANT)
	Q_PROPERTY(int count READ count CONSTANT)
	Q_PROPERTY(QString error MEMBER m_error CONSTANT)
	Q_PROPERTY(bool keyMissing MEMBER m_keyMissing CONSTANT)
	Q_PROPERTY(QObject* detail READ detail CONSTANT)

  public:
	FakeModpackModel()
	{
		setItemRoleNames({{Qt::UserRole + 1, "projectId"},
						  {Qt::UserRole + 2, "title"},
						  {Qt::UserRole + 3, "author"}});
		auto* item = new QStandardItem;
		item->setData(QStringLiteral("abc12345"), Qt::UserRole + 1);
		item->setData(QStringLiteral("Vault Hunters"), Qt::UserRole + 2);
		item->setData(QStringLiteral("iskallia"), Qt::UserRole + 3);
		appendRow(item);
		m_sortOptions = {QVariantMap{{QStringLiteral("id"), QStringLiteral("relevance")},
									 {QStringLiteral("label"), QStringLiteral("Relevance")}}};
	}
	int count() const
	{
		return rowCount();
	}
	QObject* detail()
	{
		return &m_detail;
	}
	Q_INVOKABLE void search()
	{
		++searches;
	}
	Q_INVOKABLE void fetchMore() {}
	Q_INVOKABLE void loadDetail(const QString& projectId)
	{
		m_detail.m_projectId = projectId;
		emit m_detail.changed();
		detailsAsked.append(projectId);
	}

	QString m_query;
	QString m_sort = QStringLiteral("relevance");
	QVariantList m_sortOptions;
	QString m_loader;
	bool m_searching = false;
	bool m_canFetchMore = false;
	QString m_error;
	bool m_keyMissing = false;
	FakePackDetail m_detail;
	int searches = 0;
	QStringList detailsAsked;

  signals:
	void changed();
};

/* Stands in for QmlShell: the members Main.qml calls here, and nothing
 * else -- Main.qml checks for each shell member before using it. */
class FakeShell : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int accountCount READ accountCount CONSTANT)
	Q_PROPERTY(QObject* modpackModel READ modpackModel CONSTANT)

  public:
	int accountCount() const
	{
		return m_accountCount;
	}
	QObject* modpackModel()
	{
		return &m_modpacks;
	}
	/// One instance exists: "fabric".
	Q_INVOKABLE QObject* instanceDetails(const QString& id)
	{
		return id == QLatin1String("fabric") ? &m_details : nullptr;
	}
	/// What QmlShell::launchInstanceMode() was last asked, as "id:mode".
	Q_INVOKABLE void launchInstanceMode(const QString& id, const QString& mode)
	{
		lastLaunch = id + QLatin1Char(':') + mode;
	}

	int m_accountCount = 1;
	QString lastLaunch;
	FakeModpackModel m_modpacks;

  signals:
	void navigateRequested(const QString& target);
	void openInstanceLog(const QString& id);

  private:
	FakeDetails m_details;
};

class KirigamiShellTest : public QObject
{
	Q_OBJECT

	/* Main.qml instantiated with @p shell, or null (with the reason
	 * already logged) if it fails to load. */
	std::unique_ptr<QObject> createRoot(QQmlEngine& engine, QObject* shell)
	{
		KLocalization::setupLocalizedContext(&engine);
		engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
		QQmlComponent component(
			&engine,
			QUrl(QStringLiteral("qrc:/qt/qml/MeshMC/Kirigami/Main.qml")));
		if (!component.isReady()) {
			qWarning() << component.errorString();
			return nullptr;
		}
		std::unique_ptr<QObject> root(component.createWithInitialProperties(
			{{QStringLiteral("instanceModel"),
			  QVariant::fromValue<QObject*>(&m_instances)},
			 {QStringLiteral("selection"),
			  QVariant::fromValue<QObject*>(&m_selection)},
			 {QStringLiteral("shell"), QVariant::fromValue<QObject*>(shell)}}));
		if (!root) {
			qWarning() << component.errorString();
		}
		return root;
	}

	/* @p expression evaluated in Main.qml's own scope. */
	static QVariant evaluate(QObject* root, const QString& expression)
	{
		QQmlExpression expr(qmlContext(root), root, expression);
		const QVariant result = expr.evaluate();
		if (expr.hasError()) {
			qWarning() << expr.error().toString();
		}
		return result;
	}

	/* Main.qml's navigate(page, sub); an invalid @p sub reaches it as
	 * undefined, the way a one-argument call from QML would. */
	static bool navigate(QObject* root, const QString& page,
						 const QVariant& sub = QVariant())
	{
		QVariant ok;
		const bool invoked = QMetaObject::invokeMethod(
			root, "navigate", Q_RETURN_ARG(QVariant, ok),
			Q_ARG(QVariant, page), Q_ARG(QVariant, sub));
		return invoked && ok.toBool();
	}

	static int depth(QObject* root)
	{
		return evaluate(root, QStringLiteral("root.pageStack.depth")).toInt();
	}

	static QString topTitle(QObject* root)
	{
		return evaluate(root, QStringLiteral("root.pageStack.currentItem.title")).toString();
	}

	/* Pops the top page and lets it go, as Back does. */
	static void back(QObject* root)
	{
		evaluate(root, QStringLiteral("root.pageStack.pop()"));
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	}

	QStandardItemModel m_instances;
	QObject m_selection;

  private slots:
	/* The style QmlShell::load() picks: the pages are written against
	 * KDE's desktop style, and loading them under it here catches what
	 * only it would reject. */
	void initTestCase()
	{
		QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
	}

	void rootWindowLoadsOnTheLibrary()
	{
		QQmlEngine engine;
		QObject shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		/* Guards against the component resolving to something default
		 * constructed: the property only exists on this module's root. */
		QCOMPARE(root->property("moduleName").toString(),
				 QStringLiteral("MeshMC.Kirigami"));
		QCOMPARE(root->property("section").toString(), QStringLiteral("library"));
		QCOMPARE(depth(root.get()), 1);
	}

	/* Every section opens on its own, whole paths and the split form alike,
	 * and an unknown target changes nothing. */
	void navigateOpensSections()
	{
		QQmlEngine engine;
		QObject shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(navigate(root.get(), QStringLiteral("settings/java")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("settings"));
		QVERIFY(navigate(root.get(), QStringLiteral("about")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("about"));
		QVERIFY(navigate(root.get(), QStringLiteral("discover"), QString()));
		QCOMPARE(root->property("section").toString(), QStringLiteral("discover"));
		QCOMPARE(depth(root.get()), 1);

		QVERIFY(!navigate(root.get(), QStringLiteral("nowhere/at/all")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("discover"));
	}

	/* An instance's page goes over the library, on the tab asked for; one
	 * that does not exist is refused and the page stays. */
	void navigateOpensAnInstanceOverTheLibrary()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(navigate(root.get(), QStringLiteral("discover")));
		QVERIFY(!navigate(root.get(), QStringLiteral("instance/missing")));
		QVERIFY(!navigate(root.get(), QStringLiteral("instance")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("discover"));

		QVERIFY(navigate(root.get(), QStringLiteral("instance"),
						 QStringLiteral("fabric/worlds")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("library"));
		QCOMPARE(root->property("openedInstanceId").toString(),
				 QStringLiteral("fabric"));
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("worlds"));

		/* Back (or Escape) closes it, and the window forgets it. */
		QVERIFY(evaluate(root.get(), QStringLiteral("root.pageStack.pop() !== null")).toBool());
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		QCOMPARE(depth(root.get()), 1);
		QCOMPARE(root->property("openedInstanceId").toString(), QString());
	}

	/* The launch flow's "show the console" (QmlShell::openInstanceLog())
	 * opens the instance on its Log tab; a navigate request from C++
	 * (QmlShell::requestNavigate()) is followed like any other. */
	void requestsFromTheShellAreFollowed()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		emit shell.openInstanceLog(QStringLiteral("fabric"));
		QCOMPARE(root->property("openedInstanceId").toString(),
				 QStringLiteral("fabric"));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("log"));

		emit shell.navigateRequested(QStringLiteral("accounts"));
		QCOMPARE(root->property("section").toString(), QStringLiteral("accounts"));
	}

	/* Discover searches its provider once, the first time it is shown;
	 * a pack opens on its own page over it, which asks for the pack's
	 * detail, and Back returns to the results. */
	void discoverOpensAModpackOverItself()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(navigate(root.get(), QStringLiteral("discover")));
		QCOMPARE(depth(root.get()), 1);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Discover"));
		QCOMPARE(shell.m_modpacks.searches, 1);
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.providers.length")).toInt(), 1);

		QVERIFY(evaluate(root.get(), QStringLiteral(
			"root.pageStack.currentItem.open({ projectId: 'abc12345', title: 'Vault Hunters', author: 'iskallia' }), true")).toBool());
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Vault Hunters"));
		QCOMPARE(shell.m_modpacks.detailsAsked, QStringList({QStringLiteral("abc12345")}));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.installKey")).toString(),
				 QStringLiteral("modrinth/abc12345"));

		back(root.get());
		QCOMPARE(depth(root.get()), 1);
		QCOMPARE(root->property("section").toString(), QStringLiteral("discover"));

		/* Shown again, it does not search again: the model kept the
		 * results. A provider in the path is the one shown. */
		QVERIFY(navigate(root.get(), QStringLiteral("library")));
		QVERIFY(navigate(root.get(), QStringLiteral("discover/curseforge")));
		QCOMPARE(shell.m_modpacks.searches, 1);
		QCOMPARE(root->property("discoverProvider").toString(), QStringLiteral("curseforge"));
	}

	/* Add Content goes over the instance's page, on the kind asked for; a
	 * project from it over that; the instance's own path goes back down
	 * to its page. An instance that does not exist has none. */
	void addContentOpensOverTheInstance()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(!navigate(root.get(), QStringLiteral("instance/missing/add")));
		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/add/resourcepacks")));
		QCOMPARE(depth(root.get()), 3);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Add Content"));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.initialKind")).toString(),
				 QStringLiteral("resourcepacks"));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("content"));

		QVERIFY(evaluate(root.get(), QStringLiteral(
			"root.openContentProject(root.pageStack.currentItem, 0, { title: 'Sodium', author: 'jellysquid3' }) !== null")).toBool());
		QCOMPARE(depth(root.get()), 4);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Sodium"));
		/* Only from the Add Content page on top. */
		QVERIFY(evaluate(root.get(), QStringLiteral(
			"root.openContentProject(root.instancePage, 0, { title: 'Iris' }) === null")).toBool());

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/worlds")));
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("worlds"));

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/add")));
		QCOMPARE(depth(root.get()), 3);
		back(root.get());
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(root->property("openedInstanceId").toString(), QStringLiteral("fabric"));
	}

	/* With no account at all, "Play Offline" goes to the Accounts page
	 * instead of launching; the demo needs no account and launches. */
	void launchModeNeedsAnAccountExceptForTheDemo()
	{
		QQmlEngine engine;
		FakeShell shell;
		shell.m_accountCount = 0;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVariant unused;
		QVERIFY(QMetaObject::invokeMethod(
			root.get(), "launchMode", Q_RETURN_ARG(QVariant, unused),
			Q_ARG(QVariant, QStringLiteral("fabric")),
			Q_ARG(QVariant, QStringLiteral("offline"))));
		QVERIFY(shell.lastLaunch.isEmpty());
		QCOMPARE(root->property("section").toString(), QStringLiteral("accounts"));

		QVERIFY(QMetaObject::invokeMethod(
			root.get(), "launchMode", Q_RETURN_ARG(QVariant, unused),
			Q_ARG(QVariant, QStringLiteral("fabric")),
			Q_ARG(QVariant, QStringLiteral("demo"))));
		QCOMPARE(shell.lastLaunch, QStringLiteral("fabric:demo"));
	}
};

int main(int argc, char* argv[])
{
	// Headless, as QmlModule_test is: a CI runner has no display.
	qputenv("QT_QPA_PLATFORM", "offscreen");

	// The launcher's own domain and icon theme setup, as main() makes them.
	KLocalizedString::setApplicationDomain("meshmc");
	KIconTheme::initTheme();

	QGuiApplication app(argc, argv);
	KirigamiShellTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "KirigamiShell_test.moc"
