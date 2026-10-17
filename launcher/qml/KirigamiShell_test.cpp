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
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QUrl>

#include <KIconTheme>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <functional>
#include <memory>

#include "qml/QmlShell.h"

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

/* A SettingsAdapter (the launcher's or one instance's), in memory: what
 * the setting delegates call, and a record of what was reset. */
class FakeSettings : public QObject
{
	Q_OBJECT

  public:
	Q_INVOKABLE QVariant value(const QString& id) const
	{
		return m_values.value(id, m_defaults.value(id));
	}
	Q_INVOKABLE QVariant defaultValue(const QString& id) const
	{
		return m_defaults.value(id);
	}
	Q_INVOKABLE void setValue(const QString& id, const QVariant& value)
	{
		m_values.insert(id, value);
		emit valueChanged(id, value);
	}
	Q_INVOKABLE void reset(const QString& id)
	{
		m_values.remove(id);
		resets.append(id);
		emit valueChanged(id, value(id));
	}

	QVariantMap m_values;
	QVariantMap m_defaults;
	QStringList resets;

  signals:
	void valueChanged(const QString& id, const QVariant& value);
};

/* The instance's servers.dat (ServersListModel), with one server. */
class FakeServers : public QStandardItemModel
{
	Q_OBJECT
	Q_PROPERTY(bool locked MEMBER m_locked CONSTANT)

  public:
	FakeServers()
	{
		setItemRoleNames({{Qt::UserRole + 1, "name"},
						  {Qt::UserRole + 2, "address"},
						  {Qt::UserRole + 3, "acceptTextures"},
						  {Qt::UserRole + 4, "iconUrl"}});
		auto* item = new QStandardItem;
		item->setData(QStringLiteral("Hypixel"), Qt::UserRole + 1);
		item->setData(QStringLiteral("mc.hypixel.net"), Qt::UserRole + 2);
		item->setData(0, Qt::UserRole + 3);
		item->setData(QString(), Qt::UserRole + 4);
		appendRow(item);
	}
	bool m_locked = false;
};

/* The instance page's InstanceDetails, as far as these tests read it. */
class FakeDetails : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString instanceId READ instanceId CONSTANT)
	Q_PROPERTY(QString name READ name CONSTANT)
	Q_PROPERTY(bool isLegacy READ isLegacy CONSTANT)
	Q_PROPERTY(bool isMinecraft READ isMinecraft CONSTANT)
	Q_PROPERTY(QObject* settings READ settings CONSTANT)
	Q_PROPERTY(QObject* servers READ servers CONSTANT)
	Q_PROPERTY(QString notes MEMBER m_notes NOTIFY notesChanged)

  public:
	bool isMinecraft() const
	{
		return true;
	}
	QObject* servers()
	{
		return &m_servers;
	}
	FakeServers m_servers;
	QString m_notes;
	QObject* settings()
	{
		return &m_settings;
	}
	FakeSettings m_settings;

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

  signals:
	void notesChanged();
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

/* A plugin's surfaces (PluginSurfaceModel) with one surface -- a section
 * holding a toggle and a button -- and a record of the events sent back. */
class FakePluginSurfaces : public QStandardItemModel
{
	Q_OBJECT

  public:
	FakePluginSurfaces()
	{
		setItemRoleNames({{Qt::UserRole + 1, "surfaceId"},
						  {Qt::UserRole + 2, "title"},
						  {Qt::UserRole + 3, "document"},
						  {Qt::UserRole + 4, "revision"}});
		const auto node = [](const char* type, const char* id, const QVariantMap& props,
							 const QVariantList& children = {}) {
			return QVariantMap{{QStringLiteral("type"), QLatin1String(type)},
							   {QStringLiteral("id"), QLatin1String(id)},
							   {QStringLiteral("props"), props},
							   {QStringLiteral("children"), children}};
		};
		const QVariantMap section = node(
			"section", "sec", {{QStringLiteral("title"), QStringLiteral("Tweaks")}},
			{node("toggle", "fast", {{QStringLiteral("label"), QStringLiteral("Go fast")},
									 {QStringLiteral("value"), false}}),
			 node("button", "go", {{QStringLiteral("label"), QStringLiteral("Run it")}})});
		auto* item = new QStandardItem;
		item->setData(QStringLiteral("p.surface"), Qt::UserRole + 1);
		item->setData(QStringLiteral("My plugin"), Qt::UserRole + 2);
		item->setData(section, Qt::UserRole + 3);
		item->setData(0, Qt::UserRole + 4);
		appendRow(item);

		/* A surface that is not a section: put in a card of its own. */
		const QVariantMap loose = node(
			"column", "col", {},
			{node("text", "note", {{QStringLiteral("text"), QStringLiteral("Everything is fine.")}}),
			 node("text_field", "args", {{QStringLiteral("label"), QStringLiteral("Extra arguments")},
										 {QStringLiteral("value"), QStringLiteral("-Xmx4G")}}),
			 node("number_field", "threads", {{QStringLiteral("label"), QStringLiteral("Threads")},
											  {QStringLiteral("value"), 4}}),
			 node("choice", "profile",
				  {{QStringLiteral("label"), QStringLiteral("Profile")},
				   {QStringLiteral("value"), QStringLiteral("b")},
				   {QStringLiteral("options"),
					QVariantList{QVariantMap{{QStringLiteral("id"), QStringLiteral("a")},
											 {QStringLiteral("label"), QStringLiteral("Low")}},
								 QVariantMap{{QStringLiteral("id"), QStringLiteral("b")},
											 {QStringLiteral("label"), QStringLiteral("Balanced")}}}}}),
			 node("progress", "load", {{QStringLiteral("value"), 40}}),
			 node("list", "mods",
				  {{QStringLiteral("columns"), QStringList{QStringLiteral("Name"), QStringLiteral("State")}},
				   {QStringLiteral("rows"),
					QVariantList{QVariantMap{{QStringLiteral("id"), QStringLiteral("1")},
											 {QStringLiteral("cells"), QStringList{QStringLiteral("Sodium"), QStringLiteral("ok")}}}}}}),
			 node("link", "docs", {{QStringLiteral("text"), QStringLiteral("Open the docs")},
								   {QStringLiteral("href"), QStringLiteral("https://example.org")}})});
		auto* second = new QStandardItem;
		second->setData(QStringLiteral("p.loose"), Qt::UserRole + 1);
		second->setData(QStringLiteral("Loose surface"), Qt::UserRole + 2);
		second->setData(loose, Qt::UserRole + 3);
		second->setData(0, Qt::UserRole + 4);
		appendRow(second);
	}
	/// "surface/node/event", the last one sent.
	Q_INVOKABLE void sendEvent(const QString& surface, const QString& node,
							   const QString& event, const QVariant&)
	{
		lastEvent = surface + QLatin1Char('/') + node + QLatin1Char('/') + event;
	}
	QString lastEvent;
};

/* Stands in for QmlShell: the members Main.qml calls here, and nothing
 * else -- Main.qml checks for each shell member before using it. */
class FakeShell : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int accountCount READ accountCount CONSTANT)
	Q_PROPERTY(QObject* modpackModel READ modpackModel CONSTANT)
	Q_PROPERTY(QObject* settings READ settings CONSTANT)
	Q_PROPERTY(QObject* accountsController MEMBER m_accountsController CONSTANT)
	Q_PROPERTY(QObject* newInstance MEMBER m_newInstance CONSTANT)
	Q_PROPERTY(QStringList setupSteps MEMBER m_setupSteps CONSTANT)

  public:
	QObject* settings()
	{
		return &m_settings;
	}
	FakeSettings m_settings;
	/// Set by a test before the window loads; see accountsStub().
	QObject* m_accountsController = nullptr;
	/// Likewise; see newInstanceStub().
	QObject* m_newInstance = nullptr;
	/// QmlShell::setupSteps(): what first run still has to set up.
	QStringList m_setupSteps;
	/// The instance "fabric"'s own settings.
	FakeSettings& instanceSettings()
	{
		return m_details.m_settings;
	}

	int accountCount() const
	{
		return m_accountCount;
	}
	QObject* modpackModel()
	{
		return &m_modpacks;
	}
	FakeDetails& details()
	{
		return m_details;
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

	/// What the Play options offer: two accounts, one profiler.
	Q_INVOKABLE QVariantMap launchOptions(const QString&) const
	{
		const auto account = [](const char* id, const char* name, bool isDefault) {
			return QVariantMap{{QStringLiteral("id"), QLatin1String(id)},
							   {QStringLiteral("name"), QLatin1String(name)},
							   {QStringLiteral("isDefault"), isDefault}};
		};
		return {{QStringLiteral("supportsDemo"), true},
				{QStringLiteral("profiler"), m_profiler},
				{QStringLiteral("profilers"),
				 QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("jprofiler")},
										  {QStringLiteral("name"), QStringLiteral("JProfiler")},
										  {QStringLiteral("ready"), true}}}},
				{QStringLiteral("accounts"),
				 QVariantList{account("a1", "Steve", true), account("a2", "Alex", false)}}};
	}
	/// "id:as:account", for Play As.
	Q_INVOKABLE void launchInstanceAs(const QString& id, const QString& accountId)
	{
		lastLaunch = id + QStringLiteral(":as:") + accountId;
	}
	Q_INVOKABLE void setInstanceProfiler(const QString&, const QString& key)
	{
		m_profiler = key;
	}

	int m_accountCount = 1;
	QString lastLaunch;
	QString m_profiler;
	FakeModpackModel m_modpacks;

  signals:
	void navigateRequested(const QString& target);
	void openInstanceLog(const QString& id);
	void importRequested(const QString& source);

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

	/* The settings window while it is open, else null. */
	static QQuickWindow* settingsWindow(QObject* root)
	{
		return qobject_cast<QQuickWindow*>(
			evaluate(root, QStringLiteral("root.settingsView.settingsWindow"))
				.value<QObject*>());
	}

	static QString settingsPage(QObject* root)
	{
		return evaluate(root, QStringLiteral("root.settingsView.currentModule()"))
			.toString();
	}

	/* The first item under @p window's content for which @p match holds. */
	static QQuickItem* findItem(QQuickWindow* window,
								const std::function<bool(QQuickItem*)>& match)
	{
		std::function<QQuickItem*(QQuickItem*)> search =
			[&](QQuickItem* item) -> QQuickItem* {
			if (match(item)) {
				return item;
			}
			for (QQuickItem* child : item->childItems()) {
				if (QQuickItem* found = search(child)) {
					return found;
				}
			}
			return nullptr;
		};
		return window ? search(window->contentItem()) : nullptr;
	}

	/* A setting delegate (SettingSwitch.qml and friends) by its key. */
	static QQuickItem* settingItem(QQuickWindow* window, const QString& key)
	{
		return findItem(window, [&](QQuickItem* item) {
			return item->property("key").toString() == key &&
				   item->property("checked").isValid();
		});
	}

	static QQuickItem* buttonWithText(QQuickWindow* window, const QString& text)
	{
		return findItem(window, [&](QQuickItem* item) {
			return item->inherits("QQuickAbstractButton") &&
				   item->property("text").toString() == text;
		});
	}

	static bool click(QQuickItem* item)
	{
		return item && QMetaObject::invokeMethod(item, "click");
	}

	/* AccountsController, as far as the Accounts and skin pages call it:
	 * no accounts model until a test sets `accounts`, and one skin library
	 * whose arm width changes only once `accept` is set. */
	static std::unique_ptr<QObject> accountsStub(QQmlEngine& engine)
	{
		QQmlComponent component(&engine);
		component.setData(
			"import QtQuick\n"
			"QtObject {\n"
			"  id: stub\n"
			"  signal defaultSkinsRevisionChanged()\n"
			"  property var accounts: null\n"
			"  property string defaultAccountId: ''\n"
			"  property bool skinDemoRequested: false\n"
			"  property string skinDemoPose: ''\n"
			"  property QtObject library: QtObject {\n"
			"    id: lib\n"
			"    property bool accept: false\n"
			"    property int asked: 0\n"
			"    property var skins: [{ name: 'Hero', path: '/hero.png', textureUrl: '',\n"
			"      thumbnailUrl: '', bodyUrl: '', slim: false, worn: true }]\n"
			"    property var defaultSkins: [{ name: 'Alex', slim: true, variants: [\n"
			"      { name: 'Alex', path: '/alex-classic.png', slim: false, ready: true, worn: false },\n"
			"      { name: 'Alex', path: '/alex-slim.png', slim: true, ready: true, worn: false } ] }]\n"
			"    property bool defaultSkinsLoading: false\n"
			"    function setSlim(name, slim) {\n"
			"      lib.asked++\n"
			"      if (lib.accept) lib.skins = [Object.assign({}, lib.skins[0], { slim: slim })]\n"
			"    }\n"
			"    function markWorn(name) {}\n"
			"  }\n"
			"  function accountSkinInfo(row) { return { valid: true, slim: false,\n"
			"    hasSkin: false, currentCapeId: '', currentCapeUrl: '', capes: [] } }\n"
			"  function skins(row) { return stub.library }\n"
			"}\n",
			QUrl(QStringLiteral("qrc:/test/AccountsStub.qml")));
		std::unique_ptr<QObject> stub(component.create());
		if (!stub) {
			qWarning() << component.errorString();
		}
		return stub;
	}

	/* NewInstanceController, as far as New Instance and Import call it:
	 * two Minecraft versions, the newest first, and one watcher that every
	 * create() and importFrom() hands back, still running until a test
	 * finishes it. What they were asked is kept in `created` and
	 * `imported`. */
	static std::unique_ptr<QObject> newInstanceStub(QQmlEngine& engine)
	{
		QQmlComponent component(&engine);
		component.setData(
			"import QtQuick\n"
			"QtObject {\n"
			"  id: stub\n"
			"  property string loader: ''\n"
			"  property string selectedMinecraftVersion: ''\n"
			"  property string selectedLoaderVersion: ''\n"
			"  property bool loaderLoading: false\n"
			"  property var created: []\n"
			"  property var imported: []\n"
			"  property ListModel minecraftVersions: ListModel {\n"
			"    property bool showSnapshots: false\n"
			"    property bool showOldVersions: false\n"
			"    property bool loading: false\n"
			"    property string error: ''\n"
			"    property string firstVersionId: '1.21.4'\n"
			"    ListElement { version: '1.21.4'; versionId: '1.21.4'; type: 'release'; recommended: true; time: 0 }\n"
			"    ListElement { version: '1.21.3'; versionId: '1.21.3'; type: 'release'; recommended: false; time: 0 }\n"
			"  }\n"
			"  property ListModel loaderVersions: ListModel {}\n"
			"  property QtObject watcher: QtObject {\n"
			"    property bool running: true\n"
			"    property bool succeeded: false\n"
			"    property bool failed: false\n"
			"    property string status: ''\n"
			"    property string error: ''\n"
			"    property string title: ''\n"
			"    property real progress: -1\n"
			"    property string instanceId: 'fabric'\n"
			"    signal finished(bool ok)\n"
			"  }\n"
			"  function selectMinecraftVersion(v) { stub.selectedMinecraftVersion = v }\n"
			"  function selectLoaderVersion(v) { stub.selectedLoaderVersion = v }\n"
			"  function suggestedName() {\n"
			"    return stub.selectedMinecraftVersion + (stub.loader.length > 0 ? ' Fabric' : '') }\n"
			"  function create(name, group, icon) { stub.created = [name, group, icon]; return stub.watcher }\n"
			"  function suggestedNameForImportSource(s) { return s.length > 0 ? s.replace(/^.*\\//, '').replace(/\\..*$/, '') : '' }\n"
			"  function isImportSourceValid(s) { return /\\.(zip|mrpack)$/.test(s) }\n"
			"  function importFrom(s, name, group, icon) { stub.imported = [s, name, group, icon]; return stub.watcher }\n"
			"}\n",
			QUrl(QStringLiteral("qrc:/test/NewInstanceStub.qml")));
		std::unique_ptr<QObject> stub(component.create());
		if (!stub) {
			qWarning() << component.errorString();
		}
		return stub;
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

	void rootWindowLoadsOnHome()
	{
		QQmlEngine engine;
		QObject shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		/* Guards against the component resolving to something default
		 * constructed: the property only exists on this module's root. */
		QCOMPARE(root->property("moduleName").toString(),
				 QStringLiteral("MeshMC.Kirigami"));
		QCOMPARE(root->property("section").toString(), QStringLiteral("home"));
		QCOMPARE(depth(root.get()), 1);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Home"));

		/* The library is one step away. */
		QVERIFY(navigate(root.get(), QStringLiteral("library")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("library"));
		QVERIFY(navigate(root.get(), QStringLiteral("home")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("home"));
	}

	/* Every section opens on its own, whole paths and the split form alike,
	 * and an unknown target changes nothing. */
	void navigateOpensSections()
	{
		QQmlEngine engine;
		QObject shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(navigate(root.get(), QStringLiteral("accounts")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("accounts"));
		QCOMPARE(topTitle(root.get()), QStringLiteral("Accounts"));
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

	/* A plugin's document is drawn with FormCard delegates, and a click on
	 * one goes back to the plugin as (surface, node, event). */
	void pluginSurfacesDrawADocumentAndSendEvents()
	{
		QQmlEngine engine;
		KLocalization::setupLocalizedContext(&engine);
		engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
		FakePluginSurfaces surfaces;
		QQmlComponent component(&engine);
		component.setData(
			"import QtQuick\n"
			"import MeshMC.Kirigami\n"
			"PluginSurfaces { width: 400 }\n",
			QUrl(QStringLiteral("qrc:/qt/qml/MeshMC/Kirigami/Test.qml")));
		std::unique_ptr<QObject> object(component.createWithInitialProperties(
			{{QStringLiteral("model"), QVariant::fromValue<QObject*>(&surfaces)}}));
		QVERIFY2(object, qPrintable(component.errorString()));
		auto* root = qobject_cast<QQuickItem*>(object.get());
		QVERIFY(root);
		QCOMPARE(root->property("count").toInt(), 2);

		const auto find = [&](const char* text) {
			std::function<QQuickItem*(QQuickItem*)> search = [&](QQuickItem* item) -> QQuickItem* {
				if (item->inherits("QQuickAbstractButton") &&
					item->property("text").toString() == QLatin1String(text)) {
					return item;
				}
				for (QQuickItem* child : item->childItems()) {
					if (QQuickItem* found = search(child)) {
						return found;
					}
				}
				return nullptr;
			};
			return search(root);
		};
		QQuickItem* toggle = nullptr;
		QTRY_VERIFY((toggle = find("Go fast")));
		QQuickItem* button = find("Run it");
		QVERIFY(button);

		QVERIFY(QMetaObject::invokeMethod(button, "click"));
		QCOMPARE(surfaces.lastEvent, QStringLiteral("p.surface/go/click"));
		/* The switch inside the delegate is what a user clicks. */
		QQuickItem* sw = toggle->findChild<QQuickItem*>(QStringLiteral("checkControl"));
		QVERIFY(sw);
		QVERIFY(QMetaObject::invokeMethod(sw, "click"));
		QCOMPARE(surfaces.lastEvent, QStringLiteral("p.surface/fast/change"));

		/* A picture for review, when asked for. */
		const QString shotDir = qEnvironmentVariable("MESHMC_TEST_SHOT_DIR");
		if (!shotDir.isEmpty()) {
			QQuickWindow window;
			window.resize(640, 820);
			root->setParentItem(window.contentItem());
			root->setWidth(640);
			window.show();
			QTest::qWait(500);
			window.grabWindow().save(shotDir + QStringLiteral("/plugin-surfaces.png"));
			root->setParentItem(nullptr);
		}
	}

	/* The tabs beyond the everyday five sit under "More": the bar shows
	 * the one that is open, the menu the rest. */
	void moreTabsSitUnderMore()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/worlds")));
		const QString ids = QStringLiteral("root.instancePage.%1.map(t => t.id).join(',')");
		QCOMPARE(evaluate(root.get(), ids.arg(QStringLiteral("barTabs"))).toString(),
				 QStringLiteral("content,worlds,screenshots,log,settings"));
		QCOMPARE(evaluate(root.get(), ids.arg(QStringLiteral("moreTabs"))).toString(),
				 QStringLiteral("version,datapacks,servers,backups,gameoptions,notes"));

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/servers")));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("servers"));
		QCOMPARE(evaluate(root.get(), ids.arg(QStringLiteral("barTabs"))).toString(),
				 QStringLiteral("content,worlds,screenshots,log,settings,servers"));
		QCOMPARE(evaluate(root.get(), ids.arg(QStringLiteral("moreTabs"))).toString(),
				 QStringLiteral("version,datapacks,backups,gameoptions,notes"));

		/* A tab this instance does not have shows the first one. */
		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/managedpack")));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("content"));
	}

	/* The Servers tab lists servers.dat's servers; Add Server opens its
	 * dialog (the dev route's review=add). */
	void serversTabListsTheServers()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		auto* main = qobject_cast<QQuickWindow*>(root.get());

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/servers")));
		QQuickItem* list = nullptr;
		QTRY_VERIFY((list = findItem(main, [](QQuickItem* item) {
			return item->objectName() == QLatin1String("serversList");
		})));
		QCOMPARE(list->property("count").toInt(), 1);

		QTRY_VERIFY(evaluate(root.get(), QStringLiteral("root.instancePage.review('add')")).toBool());
		QVERIFY(!evaluate(root.get(), QStringLiteral("root.instancePage.review('nonsense')")).toBool());
	}

	/* Notes typed in the Notes tab reach the instance a moment later. */
	void notesAreSavedIntoTheInstance()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		auto* main = qobject_cast<QQuickWindow*>(root.get());

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/notes")));
		QQuickItem* area = nullptr;
		QTRY_VERIFY((area = findItem(main, [](QQuickItem* item) {
			return item->objectName() == QLatin1String("notesArea");
		})));
		area->setProperty("text", QStringLiteral("seed: 1234"));
		QTRY_COMPARE_WITH_TIMEOUT(shell.details().m_notes, QStringLiteral("seed: 1234"), 5000);
	}

	/* The instance page's Play menu: Play Offline and Demo, Play As with
	 * an entry per account, and the Profiler. Both choices go to the shell. */
	void playOptionsOfferAccountsAndProfilers()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/content")));
		const QString find = QStringLiteral(
			"root.instancePage.actions.filter(a => a.text === '%1')[0]");
		QCOMPARE(evaluate(root.get(), find.arg(QStringLiteral("Play Offline")) + QStringLiteral(".visible")).toBool(), true);
		QCOMPARE(evaluate(root.get(), find.arg(QStringLiteral("Play As")) + QStringLiteral(".children.length")).toInt(), 2);
		QCOMPARE(evaluate(root.get(), find.arg(QStringLiteral("Play As")) + QStringLiteral(".children[1].text")).toString(),
				 QStringLiteral("Alex"));
		QCOMPARE(evaluate(root.get(), find.arg(QStringLiteral("Profiler")) + QStringLiteral(".children.length")).toInt(), 2);

		QVERIFY(evaluate(root.get(), find.arg(QStringLiteral("Play As")) + QStringLiteral(".children[1].trigger(), true")).toBool());
		QCOMPARE(shell.lastLaunch, QStringLiteral("fabric:as:a2"));

		QVERIFY(evaluate(root.get(), find.arg(QStringLiteral("Profiler")) + QStringLiteral(".children[1].trigger(), true")).toBool());
		QCOMPARE(shell.m_profiler, QStringLiteral("jprofiler"));
	}

	/* Settings open in a window of their own, on the page asked for, over
	 * whatever the main window shows -- which the review snapshots then
	 * picture instead -- and move to another page when asked again. */
	void settingsOpenInAWindowOfTheirOwn()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		auto* main = qobject_cast<QQuickWindow*>(root.get());
		QVERIFY(main);
		/* Offscreen. A window opened over another one only shows once
		 * that one does. */
		main->show();
		QCOMPARE(snapshotWindow(main), main);

		QVERIFY(navigate(root.get(), QStringLiteral("library")));
		QVERIFY(navigate(root.get(), QStringLiteral("settings/java")));
		QCOMPARE(root->property("section").toString(), QStringLiteral("library"));
		QCOMPARE(depth(root.get()), 1);
		QQuickWindow* settings = settingsWindow(root.get());
		QVERIFY(settings);
		QVERIFY(settings != main);
		QTRY_COMPARE(settingsPage(root.get()), QStringLiteral("java"));
		QTRY_VERIFY(settings->isVisible());
		QCOMPARE(snapshotWindow(main), settings);

		QVERIFY(navigate(root.get(), QStringLiteral("settings"), QStringLiteral("proxy")));
		QTRY_COMPARE(settingsPage(root.get()), QStringLiteral("proxy"));

		/* Asked for as a section, they open the same way, and change no
		 * section either. */
		QVERIFY(evaluate(root.get(), QStringLiteral("root.showSection('settings')")).toBool());
		QCOMPARE(root->property("section").toString(), QStringLiteral("library"));

		settingsWindow(root.get())->close();
		QTRY_VERIFY(!settingsWindow(root.get()));
		QCOMPARE(snapshotWindow(main), main);
	}

	/* A setting is saved the moment it is flipped, and a change made
	 * anywhere else shows on its row -- here the game window's
	 * "Start maximized", which also locks the size rows. */
	void aSettingIsSavedAndFollowed()
	{
		QQmlEngine engine;
		FakeShell shell;
		shell.m_settings.m_defaults.insert(QStringLiteral("LaunchMaximized"), false);
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		qobject_cast<QQuickWindow*>(root.get())->show();

		QVERIFY(navigate(root.get(), QStringLiteral("settings/minecraft")));
		QTRY_COMPARE(settingsPage(root.get()), QStringLiteral("minecraft"));
		QQuickItem* maximized = nullptr;
		QTRY_VERIFY((maximized = settingItem(settingsWindow(root.get()), QStringLiteral("LaunchMaximized"))));
		QQuickItem* width = settingItem(settingsWindow(root.get()), QStringLiteral("MinecraftWinWidth"));
		QVERIFY(width);
		QVERIFY(!maximized->property("checked").toBool());
		// A page takes input once the settings window has laid it out.
		QTRY_VERIFY(width->isEnabled());

		QVERIFY(click(maximized));
		QCOMPARE(shell.m_settings.value(QStringLiteral("LaunchMaximized")).toBool(), true);
		QVERIFY(maximized->property("checked").toBool());
		QVERIFY(!width->isEnabled());

		shell.m_settings.setValue(QStringLiteral("LaunchMaximized"), false);
		QVERIFY(!maximized->property("checked").toBool());
		QVERIFY(width->isEnabled());
	}

	/* An instance's Settings tab follows the launcher until a group is
	 * switched to its own settings; switching back drops them. */
	void instanceSettingsOverrideTheLauncher()
	{
		QQmlEngine engine;
		FakeShell shell;
		FakeSettings& own = shell.instanceSettings();
		own.m_defaults.insert(QStringLiteral("OverrideWindow"), false);
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		auto* main = qobject_cast<QQuickWindow*>(root.get());
		main->show();

		QVERIFY(navigate(root.get(), QStringLiteral("instance/fabric/settings")));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.instancePage.shownTab")).toString(),
				 QStringLiteral("settings"));
		QQuickItem* gate = nullptr;
		QTRY_VERIFY((gate = settingItem(main, QStringLiteral("OverrideWindow"))));
		QQuickItem* maximized = settingItem(main, QStringLiteral("LaunchMaximized"));
		QVERIFY(maximized);
		QVERIFY(!gate->property("checked").toBool());
		// The tab takes input once the window has laid out the page.
		QTRY_VERIFY(gate->isEnabled());
		QVERIFY(!maximized->isEnabled());

		QVERIFY(click(gate));
		QCOMPARE(own.value(QStringLiteral("OverrideWindow")).toBool(), true);
		QVERIFY(maximized->isEnabled());
		QVERIFY(click(maximized));
		QCOMPARE(own.value(QStringLiteral("LaunchMaximized")).toBool(), true);
		QVERIFY(shell.m_settings.m_values.isEmpty());

		QVERIFY(click(gate));
		QCOMPARE(own.value(QStringLiteral("OverrideWindow")).toBool(), false);
		QVERIFY(!maximized->isEnabled());
		QCOMPARE(own.resets, QStringList({QStringLiteral("LaunchMaximized"),
										  QStringLiteral("MinecraftWinWidth"),
										  QStringLiteral("MinecraftWinHeight")}));
	}

	/* Accounts lists every account, says which is the default, and opens
	 * a Microsoft account's skin and cape over itself. */
	void accountsListThePlayers()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto stub = accountsStub(engine);
		QVERIFY(stub);
		shell.m_accountsController = stub.get();
		QStandardItemModel accounts;
		const QStringList roles = {QStringLiteral("name"), QStringLiteral("profileName"),
								   QStringLiteral("isMSA"), QStringLiteral("stateKey"),
								   QStringLiteral("status"), QStringLiteral("accountId")};
		QHash<int, QByteArray> roleNames;
		for (int i = 0; i < roles.size(); ++i) {
			roleNames.insert(Qt::UserRole + 1 + i, roles.at(i).toUtf8());
		}
		accounts.setItemRoleNames(roleNames);
		auto addAccount = [&](const QString& name, bool msa, const QString& id) {
			auto* item = new QStandardItem;
			const QVariantList values = {name, name, msa, QStringLiteral("online"),
										 QString(), id};
			for (int i = 0; i < values.size(); ++i) {
				item->setData(values.at(i), Qt::UserRole + 1 + i);
			}
			accounts.appendRow(item);
		};
		addAccount(QStringLiteral("Steve"), true, QStringLiteral("steve-id"));
		addAccount(QStringLiteral("Alex"), false, QStringLiteral("alex-id"));
		stub->setProperty("accounts", QVariant::fromValue<QObject*>(&accounts));
		stub->setProperty("defaultAccountId", QStringLiteral("alex-id"));
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		auto* main = qobject_cast<QQuickWindow*>(root.get());
		main->show();

		QVERIFY(navigate(root.get(), QStringLiteral("accounts")));
		QCOMPARE(topTitle(root.get()), QStringLiteral("Accounts"));
		auto row = [&](const QString& id) {
			return findItem(main, [&](QQuickItem* item) {
				return item->property("accountId").toString() == id &&
					   item->property("isDefault").isValid();
			});
		};
		QTRY_VERIFY(row(QStringLiteral("steve-id")) && row(QStringLiteral("alex-id")));
		QVERIFY(!row(QStringLiteral("steve-id"))->property("isDefault").toBool());
		QVERIFY(row(QStringLiteral("alex-id"))->property("isDefault").toBool());

		QTRY_VERIFY(row(QStringLiteral("steve-id"))->isEnabled());
		QVERIFY(click(row(QStringLiteral("steve-id"))));
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Skin and Cape for Steve"));
		back(root.get());
		QCOMPARE(depth(root.get()), 1);
		/* An offline account has no skin page to open. */
		QTRY_VERIFY(row(QStringLiteral("alex-id"))->isEnabled());
		QVERIFY(click(row(QStringLiteral("alex-id"))));
		QCOMPARE(depth(root.get()), 1);
	}

	/* The skin page's arm width radios show what the skin says, also after
	 * a click: a click only asks the library for the change (refused
	 * first, then taken), and on a default skin it picks that skin's other
	 * texture instead of asking the library at all. Kept from the custom
	 * design's skin editor, where choosing Slim snapped back to Classic. */
	void skinArmsFollowTheSkin()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto stub = accountsStub(engine);
		QVERIFY(stub);
		shell.m_accountsController = stub.get();
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		auto* main = qobject_cast<QQuickWindow*>(root.get());
		main->show();
		QObject* library = stub->property("library").value<QObject*>();
		QVERIFY(library);

		QVERIFY(evaluate(root.get(), QStringLiteral("root.openAccountSkin(-1, '', 'Demo') !== null")).toBool());
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Skin and Cape for Demo"));
		// The review row (-1) opens on its first skin.
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.editor.selectedName")).toString(),
				 QStringLiteral("Hero"));

		QQuickItem* classic = buttonWithText(main, QStringLiteral("Classic arms"));
		QQuickItem* slim = buttonWithText(main, QStringLiteral("Slim arms"));
		QVERIFY(classic && slim);
		auto shows = [&](bool wantSlim) {
			return classic->property("checked").toBool() == !wantSlim &&
				   slim->property("checked").toBool() == wantSlim;
		};
		QVERIFY(shows(false));
		QTRY_VERIFY(slim->isEnabled());

		// Refused: the radios stay on what the skin says.
		QVERIFY(click(slim));
		QCOMPARE(library->property("asked").toInt(), 1);
		QVERIFY(shows(false));

		// Taken: they follow the skin there, and back.
		library->setProperty("accept", true);
		QVERIFY(click(slim));
		QVERIFY(shows(true));
		QVERIFY(click(classic));
		QVERIFY(shows(false));
		QCOMPARE(library->property("asked").toInt(), 3);

		// A default skin: shown with its own width (Alex is slim), and the
		// radios swap its texture, the library left alone.
		evaluate(root.get(), QStringLiteral(
			"root.pageStack.currentItem.editor.selectDefault(root.pageStack.currentItem.editor.defaultList[0])"));
		auto selectedPath = [&] {
			return evaluate(root.get(), QStringLiteral(
				"root.pageStack.currentItem.editor.selected ? root.pageStack.currentItem.editor.selected.path : ''"))
				.toString();
		};
		QCOMPARE(selectedPath(), QStringLiteral("/alex-slim.png"));
		QVERIFY(shows(true));
		QVERIFY(click(classic));
		QCOMPARE(selectedPath(), QStringLiteral("/alex-classic.png"));
		QVERIFY(shows(false));
		QCOMPARE(library->property("asked").toInt(), 3);
	}

	/* New Instance goes over the library on the newest version, named
	 * after it until a name is typed; a second ask finds the same page.
	 * Create hands the controller what the page shows, and once the
	 * instance is in, its page opens. */
	void newInstanceCreatesThenOpensTheInstance()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto stub = newInstanceStub(engine);
		QVERIFY(stub);
		shell.m_newInstance = stub.get();
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		evaluate(root.get(), QStringLiteral("root.openNewInstance()"));
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(topTitle(root.get()), QStringLiteral("New Instance"));
		QCOMPARE(root->property("section").toString(), QStringLiteral("library"));
		QCOMPARE(stub->property("selectedMinecraftVersion").toString(), QStringLiteral("1.21.4"));
		evaluate(root.get(), QStringLiteral("root.openNewInstance()"));
		QCOMPARE(depth(root.get()), 2);

		/* A loader renames it; a vanilla one carries the game's name. */
		QVERIFY(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.pickLoader('fabric')")).toBool());
		QCOMPARE(stub->property("loader").toString(), QStringLiteral("fabric"));
		QVERIFY(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.canCreate")).toBool());
		evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.create()"));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.shell.newInstance.created.join('|')")).toString(),
				 QStringLiteral("1.21.4 Fabric||default"));
		QVERIFY(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.creating")).toBool());

		evaluate(root.get(), QStringLiteral(
			"root.shell.newInstance.watcher.running = false, root.shell.newInstance.watcher.finished(true)"));
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		QCOMPARE(root->property("openedInstanceId").toString(), QStringLiteral("fabric"));
		QCOMPARE(depth(root.get()), 2);
	}

	/* Import opens on the file it was handed, named after it. Another
	 * file from outside the window (a drop, `meshmc --import`) goes into
	 * the same page, unless it is still importing the last one. */
	void importTakesFilesFromOutside()
	{
		QQmlEngine engine;
		FakeShell shell;
		auto stub = newInstanceStub(engine);
		QVERIFY(stub);
		shell.m_newInstance = stub.get();
		auto root = createRoot(engine, &shell);
		QVERIFY(root);

		evaluate(root.get(), QStringLiteral("root.openImport('file:///packs/Better%20MC.mrpack')"));
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(topTitle(root.get()), QStringLiteral("Import Instance"));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.source")).toString(),
				 QStringLiteral("file:///packs/Better%20MC.mrpack"));

		emit shell.importRequested(QStringLiteral("/packs/Skyblock.zip"));
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.source")).toString(),
				 QStringLiteral("/packs/Skyblock.zip"));

		/* Not a pack: Import stays off. */
		evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.setSource('/packs/readme.txt')"));
		QVERIFY(!evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.canImport")).toBool());
		evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.setSource('/packs/Skyblock.zip')"));
		evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.startImport()"));
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.shell.newInstance.imported.join('|')")).toString(),
				 QStringLiteral("/packs/Skyblock.zip|Skyblock||default"));

		emit shell.importRequested(QStringLiteral("/packs/Other.zip"));
		QCOMPARE(depth(root.get()), 2);
		QCOMPARE(evaluate(root.get(), QStringLiteral("root.pageStack.currentItem.source")).toString(),
				 QStringLiteral("/packs/Skyblock.zip"));
	}

	/* First run opens only while a setup step is still needed, with the
	 * account step only while there is no account. */
	void onboardingOnlyWhenSomethingIsNeeded()
	{
		QQmlEngine engine;
		FakeShell shell;
		shell.m_accountCount = 0;
		auto root = createRoot(engine, &shell);
		QVERIFY(root);
		QVERIFY(!evaluate(root.get(), QStringLiteral("root.openOnboarding(false)")).toBool());

		shell.m_setupSteps = {QStringLiteral("java")};
		QVERIFY(evaluate(root.get(), QStringLiteral("root.openOnboarding(false)")).toBool());
		QCOMPARE(evaluate(root.get(), QStringLiteral("onboarding.steps.join('|')")).toString(),
				 QStringLiteral("welcome|account|java|done"));
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
