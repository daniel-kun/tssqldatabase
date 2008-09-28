#include <QApplication>
#include <QStringList>
#include <QMessageBox>
#include <QDebug>

#include "main.h"
#include "database.h"

DatabaseTest::DatabaseTest():
   m_vlayout(this),
   m_btnOpen(this),
   m_btnExecute(this),
   m_btnClose(this),
   m_database(
      "",
      "/var/firebird/test.fdb",
      "sysdba",
      "5735"),
   m_transaction(m_database),
   m_statement(m_database, m_transaction)
{
   connect(&m_btnOpen,     SIGNAL(clicked()),  SLOT(openDatabase()));
   connect(&m_btnExecute,  SIGNAL(clicked()),  SLOT(executeStatement()));
   connect(&m_btnClose,    SIGNAL(clicked()),  SLOT(closeDatabase()));

   connect(&m_database,    SIGNAL(error(QString)), SLOT(displayError(QString)));
   connect(&m_transaction, SIGNAL(error(QString)), SLOT(displayError(QString)));
   connect(&m_statement,   SIGNAL(error(QString)), SLOT(displayError(QString)));

   connect(&m_database,    SIGNAL(opened()),   SLOT(startTransaction()));
   connect(&m_database,    SIGNAL(closed()),   SLOT(databaseClosed()));

   connect(&m_transaction, SIGNAL(commited()), SLOT(restartTransaction()));
   connect(&m_transaction, SIGNAL(started()),  SLOT(databaseOpened()));

   connect(&m_statement,   SIGNAL(executed()),        SLOT(fetchDatasets()));
   connect(&m_statement,   SIGNAL(fetchStarted()),    SLOT(fetchStarted()));
   connect(&m_statement,   SIGNAL(fetchFinished()),   SLOT(fetchFinished()));
   connect(&m_statement,   SIGNAL(fetched(TsSqlRow)), SLOT(printDataset(TsSqlRow)));

   m_btnOpen.setText   ("Open");
   m_btnExecute.setText("Execute query");
   m_btnClose.setText  ("Close");
   setIsOpen(false);

   m_hlayout.addWidget(&m_btnOpen);
   m_hlayout.addWidget(&m_btnExecute);
   m_hlayout.addWidget(&m_btnClose);

   m_vlayout.addLayout(&m_hlayout);
   m_vlayout.addWidget(&m_tblData);
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
   m_statement.execute("select * from test");
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
   if (isOpen)
      DEBUG_OUT("Database is open");
   else
      DEBUG_OUT("DATABASE IS NOT OPEN!");
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
   QStringList result;
   int rows = m_tblData.rowCount();
   m_tblData.setRowCount(rows + 1);

   int col = 0;
   for(TsSqlRow::const_iterator i = row.begin(); i != row.end(); ++i)
   {
      result << i->toString();
      QTableWidgetItem *item = new QTableWidgetItem(i->toString());
      m_tblData.setItem(rows, col++, item);
   }
   DEBUG_OUT("Fetched row: " << result);
}

void DatabaseTest::fetchFinished()
{
   DEBUG_OUT("Fetching datasets finished, closing database!");
   m_database.close();
}

void DatabaseTest::displayError(const QString &errorMessage)
{
   QMessageBox::critical(this, "Error", errorMessage);
}

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);

   DatabaseTest test;
   test.show();

   app.exec();
}

