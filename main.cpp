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
        loadReminderTime(); // Load reminder time from SQL database
        updateCalendar();

        // Set up timer for daily reminder
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MedicationReminder::dailyReminder);
        timer->start(60000); // Check every minute for time-based events
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

        if (isMedicationTaken(QDate::currentDate())) {
            qDebug() << "Medication already taken for today. Skipping reminder.";
            return;
        }

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
        // Get the current system time
        QTime currentTime = QTime::currentTime();

        // Check if the current hour matches the reminder hour
        if (currentTime.hour() == reminderTime.hour() && currentTime.minute() == reminderTime.minute() &&
            !reminderShown &&
            !isMedicationTaken(QDate::currentDate())) {

            reminderShown = true;
            showReminder();
        } else {
            reminderShown = false;
        }
    }

    void updateReminderTime() {
        reminderTime = reminderTimeEdit->time();
        saveReminderTime(); // Save reminder time to SQL database
    }

private:
    void createUI() {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        calendar = new QCalendarWidget(this);
        layout->addWidget(calendar);

        reminderTimeEdit = new QDateTimeEdit(this);
        reminderTimeEdit->setDisplayFormat("hh:mm AP");
        layout->addWidget(reminderTimeEdit);

        QPushButton *setReminderButton = new QPushButton("Set Reminder", this);
        layout->addWidget(setReminderButton);

        QPushButton *reminderButton = new QPushButton("Take Medication", this);
        layout->addWidget(reminderButton);

        centralWidget->setLayout(layout);

        connect(reminderButton, &QPushButton::clicked, this, &MedicationReminder::showReminder);
        connect(calendar, &QCalendarWidget::clicked, this, &MedicationReminder::dateClicked);
        connect(setReminderButton, &QPushButton::clicked, this, &MedicationReminder::updateReminderTime);

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
        query.exec("CREATE TABLE IF NOT EXISTS Settings ("
                   "ReminderHour TEXT PRIMARY KEY)");
    }

    void loadReminderTime() {
        QSqlQuery query("SELECT ReminderHour FROM Settings ORDER BY rowid DESC LIMIT 1");
        if (query.next()) {
            QString reminderHour = query.value(0).toString();
            qDebug() << "Reminder hour retrieved from database:" << reminderHour;
            reminderTime = QTime::fromString(reminderHour, "hh:mm AP");
            reminderTimeEdit->setTime(reminderTime);
        } else {
            qDebug() << "No reminder time found in the database.";
        }
    }

    void saveReminderTime() {
        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO Settings (ReminderHour) VALUES (:ReminderHour)");
        query.bindValue(":ReminderHour", reminderTime.toString("hh:mm AP"));

        qDebug() << "SQL Query:" << query.lastQuery();
        qDebug() << "Bind Values:" << query.boundValues();

        if (!query.exec()) {
            qDebug() << "Error saving reminder time:" << query.lastError().text();
        } else {
            qDebug() << "Reminder time saved successfully.";
        }
    }
    bool isMedicationTaken(const QDate &date) {
        QSqlQuery query;
        query.prepare("SELECT MedicationTaken FROM MedicationCalendar WHERE Date = :Date");
        query.bindValue(":Date", date.toString(Qt::ISODate));
        query.exec();
        if (query.next()) {
            return query.value(0).toBool();
        }
        return false;
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
    QDateTimeEdit *reminderTimeEdit;
    QTime reminderTime = QTime::currentTime(); // Default reminder time as current time
    bool reminderShown = false;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MedicationReminder reminder;
    reminder.show();

    return app.exec();
}

#include "main.moc"
