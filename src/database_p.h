#ifndef TS_DATABASE_P_H_27_08_2008
#define TS_DATABASE_P_H_27_08_2008
#include "database.h"

#include <QThread>
#include <QMutex>

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

      void emitStatementPrepared();
      void emitStatementExecuted();
      void emitStatementFetchStarted();
      void emitStatementFetched(const TsSqlRow &row);
      void emitStatementFetchFinished();

      void emitError(const QString  &errorMessage);
   signals:
      void databaseOpened();
      void databaseClosed();

      void transactionStarted();
      void transactionCommited();
      void transactionRolledBack();

      void statementPrepared();
      void statementExecuted();
      void statementFetchStarted();
      void statementFetched(TsSqlRow);
      void statementFetchFinished();

      void error(QString);
};

class TsSqlDatabaseThread: public QThread
{
   Q_OBJECT
   private:
      std::vector<DatabaseHandle>    m_databaseHandles;
      std::vector<TransactionHandle> m_transactionHandles;
      std::vector<StatementHandle>   m_statementHandles;

      void readRow(StatementHandle statement, TsSqlRow &row);
      void emitStatementRow(TsSqlStatementImpl *receiver, StatementHandle statement);
      void setParams(StatementHandle statement, const TsSqlRow &params);
   protected:
      virtual void run();
   public:
      TsSqlDatabaseThread();
   public slots:
      void test();
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
      void statementPrepare(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         bool startFetch);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql,
         bool startFetch);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const TsSqlRow &params,
         bool startFetch);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql,
         const TsSqlRow &params,
         bool startFetch);
      void statementSetParam(
         StatementHandle handle,
         int column,
         const TsSqlVariant &param);

      void statementStartFetch(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementFetchNext(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementFetchSingleRow(
         StatementHandle handle,
         TsSqlRow *result);
      void statementInfo(
         TsSqlStatementImpl *object, 
         StatementHandle handle, 
         StatementInfo info,
         QVariant param,
         QVariant *result);
   signals:
      void emitStatementFetchNext(
         TsSqlStatementImpl *object,
         StatementHandle handle);
};

class TsSqlDatabaseImpl: public QObject
{
   Q_OBJECT
   private:
      DatabaseHandle m_handle;
      TsSqlDatabaseThread m_thread;
      friend class TsSqlDatabaseThread;
      friend class TsSqlTransactionImpl;
      friend class TsSqlStatementImpl;
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
      void test();          // some test-functions for development, only
      void open();          // async
      void close();         // async
      void openWaiting();   // sync
      void closeWaiting();  // sync
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
      void runTest();
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
      void databaseOpenWaiting(TsSqlDatabaseImpl *object, DatabaseHandle handle);
      void databaseCloseWaiting(TsSqlDatabaseImpl *object, DatabaseHandle handle);
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
      friend class TsSqlStatementImpl;
   public:
      TsSqlTransactionImpl(TsSqlDatabaseImpl &database, TsSqlTransaction::TransactionMode mode);

      void start();                  // async
      void commit();                 // async
      void commitRetaining();        // async
      void rollBack();               // async

      void startWaiting();           // sync
      void commitWaiting();          // sync
      void commitRetainingWaiting(); // sync
      void rollBackWaiting();        // sync

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
      void transactionStartWaiting(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionCommitWaiting(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionCommitRetainingWaiting(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);
      void transactionRollBackWaiting(
         TsSqlTransactionImpl *object,
         TransactionHandle handle);

      void started();
      void commited();
      void rolledBack();
      void error(const QString &error);
};

class TsSqlStatementImpl: public QObject
{
   Q_OBJECT
   private:
      StatementHandle m_handle;
      QMutex m_stopFetchingMutex;
      bool m_stopFetching;
      void connectSignals(QObject *receiver);
      friend class TsSqlDatabaseThread;
   private slots:
      void fetchDataset(const TsSqlRow &row);
   public:
      TsSqlStatementImpl(
         TsSqlDatabaseImpl &database, 
         TsSqlTransactionImpl &transaction);
      TsSqlStatementImpl(
         TsSqlDatabaseImpl &database, 
         TsSqlTransactionImpl &transaction, 
         const QString &sql);

      void prepare(const QString &sql); // async
      void execute(bool startFetch);                         // async
      void execute(const QString &sql, bool startFetch);     // async
      void execute(const TsSqlRow &params, bool startFetch); // async
      void execute(
         const QString &sql, 
         const TsSqlRow &params, 
         bool startFetch); // async

      void prepareWaiting(const QString &sql); // async
      void executeWaiting();                         // async
      void executeWaiting(const QString &sql);     // async
      void executeWaiting(const TsSqlRow &params); // async
      void executeWaiting(
         const QString &sql, 
         const TsSqlRow &params); // async

      void setParam(int column, const TsSqlVariant &param); // sync

      QString sql();
      QString plan();
      int affectedRows();
      void fetch();                 // async
      bool fetchRow(TsSqlRow &row); // sync
      void stopFetching();          // async

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

      void statementPrepare(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         bool startFetch);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql,
         bool startFetch);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const TsSqlRow &params,
         bool startFetch);
      void statementExecute(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql,
         const TsSqlRow &params,
         bool startFetch);

      void statementPrepareWaiting(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql);
      void statementExecuteWaiting(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         bool startFetch);
      void statementExecuteWaiting(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql,
         bool startFetch);
      void statementExecuteWaiting(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const TsSqlRow &params,
         bool startFetch);
      void statementExecuteWaiting(
         TsSqlStatementImpl *object,
         StatementHandle handle,
         const QString &sql,
         const TsSqlRow &params,
         bool startFetch);
      void statementSetParam(
         StatementHandle handle,
         int column,
         const TsSqlVariant &param);

      void statementStartFetch(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementFetchNext(
         TsSqlStatementImpl *object,
         StatementHandle handle);
      void statementFetchSingleRow(
         StatementHandle handle,
         TsSqlRow *result);
      void statementInfo(
         TsSqlStatementImpl *object, 
         StatementHandle handle, 
         StatementInfo info,
         QVariant param,
         QVariant *result);

      void prepared();
      void executed();
      void fetchStarted();
      void fetched(TsSqlRow row);
      void fetchFinished();
      void error(const QString &error);
};

Q_DECLARE_METATYPE(DatabaseHandle);
Q_DECLARE_METATYPE(TransactionHandle);
Q_DECLARE_METATYPE(StatementHandle);
Q_DECLARE_METATYPE(DatabaseInfo);
Q_DECLARE_METATYPE(StatementInfo);
Q_DECLARE_METATYPE(QVariant);

#endif
