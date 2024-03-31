#include <QtWidgets>
#include <QtSql>

class MedicationReminder : public QMainWindow {
    Q_OBJECT
public:
    MedicationReminder(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        createUI();
        createConnection();
        createTable();
        updateCalendar();

        // Set up timer for daily reminder
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MedicationReminder::dailyReminder);
        timer->start(1000); // Check every second for time-based events
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (trayIcon->isVisible()) {
            hide();
            event->ignore();
        } else {
            event->accept();
        }
    }

private slots:
    void showReminder() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Medication Reminder", "Did you take your medication?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            markMedicationTaken(QDate::currentDate());
            updateCalendar();
        }
    }

    void dateClicked(const QDate &date) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Medication Reminder", "Would you like to take medication for this day?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            markMedicationTaken(date);
            updateCalendar();
        }
    }

    void showWindow() {
        this->show();
    }

    void quitApplication() {
        qApp->quit();
    }

    void dailyReminder() {
        // Check if it's time for the daily reminder
        QTime currentTime = QTime::currentTime();
        if (currentTime.hour() == reminderTime.hour() && currentTime.minute() == reminderTime.minute() && !reminderShown) {
            reminderShown = true;
            showReminder();
        } else {
            reminderShown = false;
        }
    }

private:
    void createUI() {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        calendar = new QCalendarWidget(this);
        layout->addWidget(calendar);

        QPushButton *reminderButton = new QPushButton("Take Medication", this);
        layout->addWidget(reminderButton);

        centralWidget->setLayout(layout);

        connect(reminderButton, &QPushButton::clicked, this, &MedicationReminder::showReminder);
        connect(calendar, &QCalendarWidget::clicked, this, &MedicationReminder::dateClicked);

        createSystemTrayIcon();
    }

    void createConnection() {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("medication.db");

        if (!db.open()) {
            qDebug() << "Error: Couldn't open database";
            return;
        }
    }

    void createTable() {
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS MedicationCalendar ("
                   "Date DATE PRIMARY KEY,"
                   "MedicationTaken BOOLEAN)");
    }

    void markMedicationTaken(const QDate &date) {
        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO MedicationCalendar (Date, MedicationTaken) "
                      "VALUES (:Date, :MedicationTaken)");
        query.bindValue(":Date", date.toString(Qt::ISODate));
        query.bindValue(":MedicationTaken", true);
        query.exec();
    }

    void updateCalendar() {
        QSqlQuery query;
        query.exec("SELECT Date FROM MedicationCalendar WHERE MedicationTaken = 1");

        QList<QDate> dates;
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
            dates.append(date);
        }

        QTextCharFormat format;
        format.setBackground(Qt::green); // Color for dates with medication taken
        for (const QDate &date : dates) {
            calendar->setDateTextFormat(date, format);
        }
    }

    void createSystemTrayIcon() {
        trayIconMenu = new QMenu(this);
        trayIconMenu->addAction("Show Window", this, &MedicationReminder::showWindow);
        trayIconMenu->addAction("Quit", this, &MedicationReminder::quitApplication);

        trayIcon = new QSystemTrayIcon(QIcon(":/icon.png"), this);
        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->show();
    }

    QCalendarWidget *calendar;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QTime reminderTime = QTime(12, 0); // Set the default reminder time to 12:00 PM
    bool reminderShown = false;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MedicationReminder reminder;
    reminder.show();

    return app.exec();
}

#include "main.moc"
