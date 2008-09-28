#ifndef MAIN_H_27092008
#define MAIN_H_27092008

#include <QWidget>
#include <QThread>
#include <QBoxLayout>
#include <QPushButton>
#include <QTableWidget>

#include "database.h"

#define DEBUG_OUT(message) qDebug() << "Thread " << QThread::currentThreadId() << ": " << message

class DatabaseTest: public QWidget
{
   Q_OBJECT
   private:
      /* Widgets */
      QVBoxLayout      m_vlayout;
      QHBoxLayout      m_hlayout;
      QPushButton      m_btnOpen, m_btnExecute, m_btnClose;
      QTableWidget     m_tblData;

      /* Database access */
      TsSqlDatabase    m_database;
      TsSqlTransaction m_transaction;
      TsSqlStatement   m_statement;

      void setIsOpen(bool isOpen);
   public:
      DatabaseTest();
   public slots:
      void openDatabase();
      void executeStatement();
      void closeDatabase();

      void startTransaction();
      void commitTransaction();
      void restartTransaction();
      void fetchDatasets();
      void databaseOpened();
      void databaseClosed();

      void fetchStarted();
      void printDataset(TsSqlRow);
      void fetchFinished();

      void exitApplication();

      void displayError(const QString &errorMessage);
};

#endif
