#ifndef DATABASE_P_H_27_08_2008
#define DATABASE_P_H_27_08_2008
#include "database.h"

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

// These fakes are necessary so the Qt meta-object system
// can distinguish the handle-types.
// When using typedef void * XyzHandle, Q_DECLARE_METATYPE
// won't work.
class FakeDatabase;
class FakeTransaction;
class FakeStatement;

typedef FakeDatabase *DatabaseHandle;
typedef FakeTransaction *TransactionHandle;
typedef FakeStatement *StatementHandle;

enum DatabaseInfo
{
   diServer,
   diDatabase,
   diUser,
   diPassword,
   diCharacterSet,
   diRole,
   diCreateParams
};

enum StatementInfo
{
   siPlan,
   siColumnCount,
   siColumnName,
   siColumnIndex,
   siColumnAlias,
   siColumnTable,
   siColumnType,
   siColumnSubType,
   siColumnSize,
   siColumnScale
};

class TsSqlThreadEmitter: public QObject
{
   Q_OBJECT
   private:
      QObject *m_object;

   public:
      TsSqlThreadEmitter(QObject *object);
      void emitDatabaseOpened();
      void emitDatabaseClosed();

      void emitTransactionStarted();
      void emitTransactionCommited();
      void emitTransactionRolledBack();

      void emitStatementExecuted();
      void emitStatementFetchStarted();
      void emitStatementFetched(TsSqlRow row);
      void emitStatementFetchFinished();
   signals:
      void databaseOpened();
      void databaseClosed();

      void transactionStarted();
      void transactionCommited();
      void transactionRolledBack();

      void statementExecuted();
      void statementFetchStarted();
      void statementFetched(TsSqlRow);
      void statementFetchFinished();
};

class TsSqlDatabaseThread: public QThread
{
   Q_OBJECT
   private:
      std::vector<DatabaseHandle>    m_databaseHandles;
      std::vector<TransactionHandle> m_transactionHandles;
      std::vector<StatementHandle>   m_statementHandles;
      QWaitCondition &m_waiter;

      void emitStatementRow(TsSqlStatementImpl *receiver, StatementHandle statement);
   protected:
      virtual void run();
   public:
      TsSqlDatabaseThread(QWaitCondition &waiter);
   public slots:
      void createDatabase(
         class TsSqlDatabaseImpl *object,
         const QString &server,
         const QString &database,
         const QString &user,
         const QString &password,
         const QString &characterSet,
         const QString &role,
         const QString &createParams);
      void databaseOpen(TsSqlDatabaseImpl *object, DatabaseHandle handle);
      void databaseClose(TsSqlDatabaseImpl *object, DatabaseHandle handle);
      void databaseIsOpen(
         TsSqlDatabaseImpl *object, 
         DatabaseHandle handle, 
         bool *result);
      void databaseInfo(
         TsSqlDatabaseImpl *object, 
         DatabaseHandle handle, 
         DatabaseInfo info,
         QString *result);
      void databaseConnectedUsers(
         TsSqlDatabaseImpl *object,
         DatabaseHandle handle,
         QVector<QString> *result);

      void createTransaction(
         TsSqlTransactionImpl *object,
         DatabaseHandle database, 
         TsSqlTransaction::TransactionMode mode);
      void transactionStart(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionCommit(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionCommitRetaining(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionRollBack(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);

      void createStatement(
         TsSqlStatementImpl *object,
         DatabaseHandle database, 
         TransactionHandle transaction);
      void createStatement(
         TsSqlStatementImpl *object,
         DatabaseHandle database, 
         TransactionHandle transaction,
         QString sql);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql);
      void statementStartFetch(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementFetchNext(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementInfo(
         TsSqlStatementImpl *object, 
         StatementHandle handle, 
         StatementInfo info,
         QVariant param,
         QVariant *result);
};

class TsSqlDatabaseImpl: public QObject
{
   Q_OBJECT
   private:
      DatabaseHandle m_handle;
      QMutex m_mutex;
      QWaitCondition m_waiter;
      TsSqlDatabaseThread m_thread;
      friend class TsSqlDatabaseThread;
      friend class TsSqlTransactionImpl;
      friend class TsSqlStatementImpl;
   private slots:
      void emitOpened();
      void emitClosed();
   public:
      TsSqlDatabaseImpl(
         const QString &server,
         const QString &database,
         const QString &user,
         const QString &password,
         const QString &characterSet,
         const QString &role,
         const QString &createParams);
      ~TsSqlDatabaseImpl();
      void open();       // async
      void close();      // async
      bool isOpen();
      QString server();
      QString database();
      QString user();
      QString password();
      QString role();
      QString characterSet();
      QString createParams();
      QVector<QString> connectedUsers();
   signals:
      /* These signals are used for internal communication */
      void createHandle(
         TsSqlDatabaseImpl *object,
         const QString &server,
         const QString &database,
         const QString &user,
         const QString &password,
         const QString &characterSet,
         const QString &role,
         const QString &createParams);
      void databaseOpen(TsSqlDatabaseImpl *object, DatabaseHandle handle);
      void databaseClose(TsSqlDatabaseImpl *object, DatabaseHandle handle);
      void databaseIsOpen(TsSqlDatabaseImpl*object, DatabaseHandle handle, bool *result);
      void databaseInfo(
         TsSqlDatabaseImpl *object, 
         DatabaseHandle handle, 
         DatabaseInfo info,
         QString *result);
      void databaseConnectedUsers(
         TsSqlDatabaseImpl *object,
         DatabaseHandle handle,
         QVector<QString> *result);

      /* These signals are forwarded to the interface */
      void opened();
      void closed();
      void error(const QString &error);
};

class TsSqlTransactionImpl: public QObject
{
   Q_OBJECT
   private:
      TransactionHandle m_handle;
      QMutex m_mutex;
      QWaitCondition m_waiter;
      friend class TsSqlStatementImpl;
   private slots:
      void emitStarted();
      void emitCommited();
      void emitRolledBack();
   public:
      TsSqlTransactionImpl(TsSqlDatabaseImpl &database, TsSqlTransaction::TransactionMode mode);
      void start();               // async
      void commit();              // async
      void commitRetaining();
      void rollBack();            // async
      bool isStarted();
      friend class TsSqlDatabaseThread;
   signals:
      void createTransaction(
         TsSqlTransactionImpl *object,
         DatabaseHandle database, 
         TsSqlTransaction::TransactionMode mode);
      void transactionStart(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionCommit(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionCommitRetaining(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionRollBack(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);

      void started();
      void commited();
      void rolledBack();
};

class TsSqlStatementImpl: public QObject
{
   Q_OBJECT
   private:
      QMutex m_mutex;
      QWaitCondition m_waiter;
      StatementHandle m_handle;
      void connectSignals(QObject *receiver);
      friend class TsSqlDatabaseThread;
   private slots:
      void emitExecuted();
      void emitFetchStarted();
      void emitFetched(TsSqlRow row);
      void emitFetchFinished();
   public:
      TsSqlStatementImpl(
         TsSqlDatabaseImpl &database, 
         TsSqlTransactionImpl &transaction);
      TsSqlStatementImpl(
         TsSqlDatabaseImpl &database, 
         TsSqlTransactionImpl &transaction, 
         const QString &sql);
      void prepare(const QString &sql); // async
      void execute(const QString &sql); // async
      void execute();                   // async
      QString sql();
      QString plan();
      int affectedRows();
      void fetch(); // async

      int        columnCount();
      QString    columnName(   int columnIndex);
      int        columnIndex(  const QString &columnName);
      QString    columnAlias(  int columnIndex);
      QString    columnTable(  int columnIndex);
      TsSqlType  columnType(   int columnIndex);
      int        columnSubType(int columnIndex);
      int        columnSize   (int columnIndex);
      int        columnScale  (int columnIndex);
   signals:
      void createStatement(
         TsSqlStatementImpl *object,
         DatabaseHandle database, 
         TransactionHandle transaction);
      void createStatement(
         TsSqlStatementImpl *object,
         DatabaseHandle database, 
         TransactionHandle transaction,
         QString sql);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql);
      void statementStartFetch(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementInfo(
         TsSqlStatementImpl *object, 
         StatementHandle handle, 
         StatementInfo info,
         QVariant param,
         QVariant *result);

      void executed();
      void fetchStarted();
      void fetched(TsSqlRow row);
      void fetchFinished();
};

Q_DECLARE_METATYPE(DatabaseHandle);
Q_DECLARE_METATYPE(TransactionHandle);
Q_DECLARE_METATYPE(StatementHandle);
Q_DECLARE_METATYPE(DatabaseInfo);
Q_DECLARE_METATYPE(StatementInfo);
Q_DECLARE_METATYPE(QVariant);

#endif
