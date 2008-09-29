#include <cmath>

#include <QApplication>
#include <QStringList>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

#include "main.h"
#include "database.h"

DatabaseTest::DatabaseTest():
   m_vlayout(this),
   m_btnOpen(this),
   m_btnExecute(this),
   m_btnClose(this),
   m_btnFill(this),
   m_database(
      "",
      "melchior:/var/firebird/test.fdb",
      "sysdba",
      "5735"),
   m_transaction(m_database),
   m_statement(m_database, m_transaction),
   m_insertStatement(m_database, m_transaction),
   m_syncDatabase(
      "",
      "melchior:/var/firebird/test.fdb",
      "sysdba",
      "5735"),
   m_syncTransaction(m_syncDatabase),
   m_syncStatement(m_syncDatabase, m_syncTransaction),
   m_dataCount(0)
{
   connect(&m_btnOpen,           SIGNAL(clicked()),  SLOT(openDatabase()));
   connect(&m_btnExecute,        SIGNAL(clicked()),  SLOT(executeStatement()));
   connect(&m_btnClose,          SIGNAL(clicked()),  SLOT(closeDatabase()));
   connect(&m_btnTest,           SIGNAL(clicked()),  SLOT(testSync()));
   connect(&m_btnFill,           SIGNAL(clicked()),  SLOT(fillTest2()));

   connect(&m_database,          SIGNAL(error(QString)), SLOT(displayError(QString)));
   connect(&m_transaction,       SIGNAL(error(QString)), SLOT(displayError(QString)));
   connect(&m_statement,         SIGNAL(error(QString)), SLOT(displayError(QString)));
   connect(&m_insertStatement,   SIGNAL(error(QString)), SLOT(displayError(QString)));

   connect(&m_database,          SIGNAL(opened()),   SLOT(startTransaction()));
   connect(&m_database,          SIGNAL(closed()),   SLOT(databaseClosed()));

   connect(&m_transaction,       SIGNAL(commited()), SLOT(restartTransaction()));
   connect(&m_transaction,       SIGNAL(started()),  SLOT(databaseOpened()));

   connect(&m_statement,         SIGNAL(executed()),        SLOT(fetchDatasets()));
   connect(&m_statement,         SIGNAL(fetchStarted()),    SLOT(fetchStarted()));
   connect(&m_statement,         SIGNAL(fetchFinished()),   SLOT(fetchFinished()));
   connect(&m_statement,         SIGNAL(fetched(TsSqlRow)), SLOT(printDataset(TsSqlRow)));

   m_btnOpen.setText   ("&Open");
   m_btnExecute.setText("&Execute query");
   m_btnClose.setText  ("&Close");
   m_btnTest.setText   ("&Test");
   m_btnFill.setText   ("&Fill");
   setIsOpen(false);

   m_hlayout.addWidget(&m_btnOpen);
   m_hlayout.addWidget(&m_btnExecute);
   m_hlayout.addWidget(&m_btnClose);
   m_hlayout.addWidget(&m_btnTest);
   m_hlayout.addWidget(&m_btnFill);

   m_vlayout.addLayout(&m_hlayout);
   m_vlayout.addWidget(&m_tblData);
   m_vlayout.addWidget(&m_lDataCount);
/*   m_database.open();
   DEBUG_OUT("Database is open? " << m_database.isOpen());*/
}

void DatabaseTest::openDatabase()
{
   DEBUG_OUT("Opening database");
   m_database.open();
}

void DatabaseTest::closeDatabase()
{
   DEBUG_OUT("Closing the database");
   m_database.close();
}

void DatabaseTest::setIsOpen(bool isOpen)
{
   m_btnOpen.   setEnabled(!isOpen);
   m_btnExecute.setEnabled( isOpen);
   m_btnClose.  setEnabled( isOpen);
}

void DatabaseTest::startTransaction()
{
   DEBUG_OUT("Database opened, now starting transaction");
   DEBUG_OUT("Server: "            << m_database.server());
   DEBUG_OUT("Database: "          << m_database.database());
   DEBUG_OUT("User: "              << m_database.user());
   DEBUG_OUT("Password: "          << m_database.password());
   DEBUG_OUT("Character set: "     << m_database.characterSet());
   DEBUG_OUT("Role: "              << m_database.role());
   DEBUG_OUT("Create parameters: " << m_database.createParams());
   
   DEBUG_OUT("Connected users:");
   QVector<QString> users = m_database.connectedUsers();
   for(QVector<QString>::const_iterator i = users.begin();
       i != users.end();
       ++i)
      DEBUG_OUT("- " << *i);
   m_transaction.start();
   setIsOpen(true);
}

void DatabaseTest::commitTransaction()
{
   setIsOpen(true);
   DEBUG_OUT("Transaction started, trying a commit");
   m_transaction.commit();
}

void DatabaseTest::restartTransaction()
{
   setIsOpen(false);
   DEBUG_OUT("Transaction commited, restarting it");
   m_transaction.start();
}

void DatabaseTest::executeStatement()
{
   DEBUG_OUT("Transaction restarted, trying to execute a statement");
   m_statement.execute("select * from test2");
}

void DatabaseTest::fetchDatasets()
{
   DEBUG_OUT("Statement executed, now describing and starting to fetch datasets");
   int columns = m_statement.columnCount();
   DEBUG_OUT("Statement has " << columns << " columns, now describing columns:");

   m_tblData.setRowCount(0);
   m_tblData.setColumnCount(columns);

   for(int i = 1; i <= columns; ++i)
   {
      DEBUG_OUT(i 
         << ": "          << m_statement.columnName(i) 
         << ", alias: "   << m_statement.columnAlias(i) 
         << ", table: "   << m_statement.columnTable(i)
         << ", type: "    << m_statement.columnType(i)
         << ", subType: " << m_statement.columnSubType(i)
         << ", size: "    << m_statement.columnSize(i)
         << ", scale: "   << m_statement.columnScale(i));
   }
   m_statement.fetch();
}

void DatabaseTest::databaseOpened()
{
   bool isOpen = m_database.isOpen();
/*   if (isOpen)
      DEBUG_OUT("Database is open");
   else
      DEBUG_OUT("DATABASE IS NOT OPEN!");*/
   setIsOpen(isOpen);
}

void DatabaseTest::databaseClosed()
{
   DEBUG_OUT("Database is closed");
   setIsOpen(false);
}

void DatabaseTest::exitApplication()
{
   DEBUG_OUT("Exiting application.");
   hide();
}

void DatabaseTest::fetchStarted()
{
   DEBUG_OUT("Fetching datasets started!");
}

void DatabaseTest::printDataset(TsSqlRow row)
{
//   m_statement.stopFetching();
   QStringList result;
   int rows = m_tblData.rowCount();
   m_tblData.setRowCount(rows + 1);

   int col = 1;
   for(TsSqlRow::const_iterator i = row.begin() + 1; i != row.end(); ++i)
   {
      result << i->asString();
      QTableWidgetItem *item = new QTableWidgetItem(i->asString());
      m_tblData.setItem(rows, col++, item);
   }
   DEBUG_OUT("Fetched row: " << result);
}

void DatabaseTest::fetchFinished()
{
   DEBUG_OUT("Fetching datasets finished, closing database!");
   m_database.close();
}

int random(int min, int max)
{
   static struct RandInitializer
   { 
      RandInitializer()
      { 
         qsrand(QDateTime::currentDateTime().toTime_t()); 
      } 
   } randInitializer;
   return min + (int) ((max-min) * (qrand() / (RAND_MAX + 1.0)));
}

QString randomString(int maxlen = 30)
{
   int len = random(10, maxlen);
   QString result;
   for(int i = 0; i < len; ++i)
      result += static_cast<char>(random(33, 127));
   return result;
}

QDateTime randomDateTime()
{
   int day    = random(1, 28);
   int month  = random(1, 12);
   int year   = random(1800, 2100);
   int hour   = random(1, 12);
   int minute = random(1, 60);
   int second = random(1, 60);
   return QDateTime(QDate(year, month, day), QTime(hour, minute, second));
}

double randomReal()
{
   return double(random(0, 100000)) / random(1, 10000);
}

void randomizeParams(TsSqlRow &params)
{
   params.resize(10);
   params[0] = randomString();
   params[1] = randomDateTime().date();
   params[2] = randomDateTime().time();
   params[3] = randomDateTime();
   params[4] = randomString();
   params[5] = random(0, std::pow(2.0, 15)-1);
   params[6] = random(0, std::pow(2.0, 31)-1);
   params[7] = random(0, std::pow(2.0, 63)-1);
   params[8] = randomReal();
   params[9] = randomReal();
}

void DatabaseTest::fillTest2()
{
   if (m_dataCount)
   {
      disconnect(&m_insertStatement, SIGNAL(executed()), this, SLOT(insertDataset()));
      m_btnFill.setText("&Fill");
      m_dataCount = 0;
      m_transaction.commitRetainingWaiting();
   }
   else
   {
      m_btnFill.setText("&Stop filling");
      m_database.openWaiting();
      m_transaction.startWaiting();
      connect(&m_insertStatement, SIGNAL(executed()), SLOT(insertDataset()));

      m_insertStatement.prepareWaiting(
            "insert into test2(id, field_blob, field_date, "
            "field_time, field_timestamp, field_string, "
            "field_smallint, field_int, field_largeint, field_float, "
            "field_double) values (gen_id(test2_gen, 1), ?, ?, "
            "?, ?, ?, "
            "?, ?, ?, ?, "
            "?)");
      insertDataset();
   }
}

void DatabaseTest::insertDataset()
{
   TsSqlRow params;
   randomizeParams(params);
   m_insertStatement.execute(params);
   m_lDataCount.setText(QString("%1 datasets inserted").arg(++m_dataCount));
   if ((m_dataCount % 1000) == 0)
      m_transaction.commitRetainingWaiting();
}

void DatabaseTest::testSync()
{
   m_syncDatabase.openWaiting();
   DEBUG_OUT("Waiting for database to open: " << m_syncDatabase.isOpen());
   m_syncTransaction.startWaiting();
   m_syncStatement.executeWaiting("delete from test");

   // 1. Prepare, then execute
   m_syncStatement.prepareWaiting("insert into test(id, text)values(gen_id(test_gen, 1), 'Sync, Step 1!')");
   m_syncStatement.executeWaiting();

   // 2. Execute immediately
   m_syncStatement.executeWaiting("insert into test(id, text)values(gen_id(test_gen, 1), 'Sync, Step 2!')");

   // 3. Prepare, set parameters, then execute
   m_syncStatement.prepareWaiting("insert into test(id, text)values(gen_id(test_gen, 1), ?)");
   m_syncStatement.setParam(1, TsSqlVariant("Sync, Step 3!"));
   m_syncStatement.executeWaiting();

   // 4. Prepare, then execute and set parameters in one step
   m_syncStatement.prepareWaiting("insert into test(id, text)values(gen_id(test_gen, 1), ?)");
   TsSqlRow params;
   params.push_back(TsSqlVariant("Sync, Step 4!"));
   m_syncStatement.executeWaiting(params);

   // 5. Execute and set parameters in one step
   params[0] = QVariant("Sync, Step 5!");
   m_syncStatement.executeWaiting("insert into test(id, text)values(gen_id(test_gen, 1), ?)", params);

   // 6. Read all rows
   m_syncStatement.executeWaiting("select id, text from test");
   m_tblData.setRowCount(0);
   m_tblData.setColumnCount(m_syncStatement.columnCount());

   TsSqlRow row;
   while (m_syncStatement.fetchRow(row))
   {
      printDataset(row);
/*      QStringList sl;
      for(TsSqlRow::const_iterator i = row.begin();
          i != row.end();
          ++i)
         sl << i->toString();
      DEBUG_OUT("Row: " << sl);*/
   }

   m_syncTransaction.commitWaiting();
   m_syncDatabase.closeWaiting();
}

void DatabaseTest::displayError(const QString &errorMessage)
{
   QMessageBox::critical(this, "Error", errorMessage);
}

DataGrid::DataGrid():
   m_database(
      "",
      "melchior:/var/firebird/test.fdb",
      "sysdba",
      "5735"),
   m_transaction(m_database, TsSqlTransaction::tmRead),
   m_fetchIds(m_database, m_transaction),
   m_fetchData(m_database, m_transaction),
   m_model(m_fetchIds, m_fetchData),
   m_layout(this),
   m_table(this)
{
/*   m_database.test();
   return;*/
   m_database.openWaiting();
   m_transaction.startWaiting();

   m_layout.addWidget(&m_table);
   m_table.setModel(&m_model);
   m_fetchData.prepare("select * from test2 where id=?");
   m_fetchIds.execute("select id from test2", true);

   m_table.show();
}

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);

   DataGrid dataGrid;
   dataGrid.show();
//   DatabaseTest test;
//   test.show();

   app.exec();
}

