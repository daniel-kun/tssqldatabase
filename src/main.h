#ifndef TS_MAIN_H_27092008
#define TS_MAIN_H_27092008

#include <QLabel>
#include <QWidget>
#include <QThread>
#include <QBoxLayout>
#include <QPushButton>
#include <QTableWidget>

#include "database.h"
#include "sqlview.h"

//#define DEBUG_OUT(message) qDebug() << "Thread " << QThread::currentThreadId() << ": " << message
#define DEBUG_OUT(message)

class DatabaseTest: public QWidget
{
   Q_OBJECT
   private:
      /* Widgets */
      QVBoxLayout      m_vlayout;
      QHBoxLayout      m_hlayout;
      QPushButton      m_btnOpen, 
                       m_btnExecute, 
                       m_btnClose, 
                       m_btnTest,
                       m_btnFill;
      QTableWidget     m_tblData;
      QLabel           m_lDataCount;

      /* Database access async */
      TsSqlDatabase    m_database;
      TsSqlTransaction m_transaction;
      TsSqlStatement   m_statement;
      TsSqlStatement   m_insertStatement;

      /* Database access sync */
      TsSqlDatabase    m_syncDatabase;
      TsSqlTransaction m_syncTransaction;
      TsSqlStatement   m_syncStatement;

      int              m_dataCount;
      void setIsOpen(bool isOpen);
   public:
      DatabaseTest();
   public slots:
      void testSync();
      void fillTest2();
      void insertDataset();

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

class DataGrid: public QWidget
{
   Q_OBJECT
   private:
      TsSqlDatabase    m_database;
      TsSqlTransaction m_transaction;
      TsSqlStatement   m_fetchIds, m_fetchData, m_insertStatement;
      TsSqlBuffer      m_buffer;
      TsSqlTableModel  m_model;

      QVBoxLayout      m_layout;
      QLabel           m_lDataCount;
      QPushButton      m_btnStart, m_btnFill, m_btnQuit;
      QTableView       m_table;

      int              m_idCount, m_dataCount;

      void updateCountText();
   public:
      DataGrid();
      ~DataGrid();
   public slots:
      void updateDataCount(const TsSqlRow  &);
      void updateRowCount();
      void test();
      void fill();
      void insertDataset();
      void startFetch();
      void displayError(const QString &error);
};

#endif
