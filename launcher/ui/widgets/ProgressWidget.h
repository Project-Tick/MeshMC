/// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QPointer>
#include <QString>
#include <QWidget>
#include <memory>

class Task;
class QProgressBar;
class QLabel;

/* A label and a progress bar following one task.
 *
 * Two ways in: start() takes ownership of a task and runs it, while
 * watch() only follows one somebody else owns - which is what an inline
 * progress strip inside a page needs, since the task it reports on
 * belongs to a model. */
class ProgressWidget : public QWidget
{
	Q_OBJECT
  public:
	explicit ProgressWidget(QWidget* parent = nullptr);

	/* When on, the widget hides itself whenever no task is running.
	 * Off by default, so existing users keep a stable layout. */
	void hideIfInactive(bool hide);
	/* Text shown above the bar while a task runs. An empty format hides
	 * the label entirely, leaving just the bar. */
	void progressFormat(const QString& format);

  public slots:
	/* Follow a task owned by someone else. Safe if it is deleted while
	 * we are watching. */
	void watch(Task* task);
	/* Take a task, follow it, and start it if it has not started. */
	void start(std::shared_ptr<Task> task);
	bool exec(std::shared_ptr<Task> task);

  private slots:
	void handleTaskFinish();
	void handleTaskStatus(const QString& status);
	void handleTaskProgress(qint64 current, qint64 total);
	void taskDestroyed();

  private:
	void updateVisibility();

  private:
	QLabel* m_label;
	QProgressBar* m_bar;
	/* Non-owning view of whatever we are following, whether it arrived
	 * through watch() or start(). */
	QPointer<Task> m_watched;
	/* Only set when we were handed ownership. */
	std::shared_ptr<Task> m_task;

	bool m_hideIfInactive = false;
	QString m_progressFormat;
};
