#include <unistd.h>
#include <QDebug>

#include "private/ibpp/core/ibpp.h"

#include "main.h"
#include "database_p.h"

#define EMIT_ASYNC(object, signal) { TsSqlThreadEmitter emitter(object); emitter.signal(); }
#define DEBUG_RECEIVE(message) DEBUG_OUT(message)

#define DBHANDLE(handle) (*reinterpret_cast<IBPP::Database*>   (handle))
#define TRHANDLE(handle) (*reinterpret_cast<IBPP::Transaction*>(handle))
#define STHANDLE(handle) (*reinterpret_cast<IBPP::Statement*>  (handle))
#define CHECK_DEADLOCK   usleep(1000000); if (!m_waiter.wait(&m_mutex, 1000)) DEBUG_OUT("Deadlock detected")

TsSqlThreadEmitter::TsSqlThreadEmitter(QObject *object):
   m_object(object)
{
}

void TsSqlThreadEmitter::emitDatabaseOpened()
{
   connect(this, SIGNAL(databaseOpened()), m_object, SLOT(emitOpened()), Qt::QueuedConnection);
   emit databaseOpened();
}

void TsSqlThreadEmitter::emitDatabaseClosed()
{
   connect(this, SIGNAL(databaseClosed()), m_object, SLOT(emitClosed()), Qt::QueuedConnection);
   emit databaseClosed();
}

void TsSqlThreadEmitter::emitTransactionStarted()
{
   connect(this, SIGNAL(transactionStarted()), m_object, SLOT(emitStarted()), Qt::QueuedConnection);
   emit transactionStarted();
}

void TsSqlThreadEmitter::emitTransactionCommited()
{
   connect(this, SIGNAL(transactionCommited()), m_object, SLOT(emitCommited()), Qt::QueuedConnection);
   emit transactionCommited();
}

void TsSqlThreadEmitter::emitTransactionRolledBack()
{
   connect(this, SIGNAL(transactionRolledBack()), m_object, SLOT(emitRolledBack()), Qt::QueuedConnection);
   emit transactionRolledBack();
}

void TsSqlThreadEmitter::emitStatementExecuted()
{
   connect(this, SIGNAL(statementExecuted()), m_object, SLOT(emitExecuted()), Qt::QueuedConnection);
   emit statementExecuted();
}

void TsSqlThreadEmitter::emitStatementFetchStarted()
{
   connect(this, SIGNAL(statementFetchStarted()), m_object, SLOT(emitFetchStarted()), Qt::QueuedConnection);
   emit statementFetchStarted();
}

void TsSqlThreadEmitter::emitStatementFetched(TsSqlRow row)
{
   connect(this, SIGNAL(statementFetched(TsSqlRow)), m_object, SLOT(emitFetched(TsSqlRow)), Qt::QueuedConnection);
   emit statementFetched(row);
}

void TsSqlThreadEmitter::emitStatementFetchFinished()
{
   connect(this, SIGNAL(statementFetchFinished()), m_object, SLOT(emitFetchFinished()), Qt::QueuedConnection);
   emit statementFetchFinished();
}

TsSqlDatabaseThread::TsSqlDatabaseThread(QWaitCondition &waiter):
   m_waiter(waiter)
{
   // Send fetch-next signals queued so other commands can be processed
   // in the meantime.
   connect(
      this, 
      SIGNAL(statementFetchNext(
         TsSqlStatementImpl *,
         StatementHandle)),
      this,
      SLOT(statementFetchNext(
         TsSqlStatementImpl *,
         StatementHandle)),
      Qt::QueuedConnection);
}

void TsSqlDatabaseThread::run()
{
   DEBUG_OUT("New thread " << this << " is running");
   m_waiter.wakeAll();
   QThread::run();
   DEBUG_OUT("Thread is stopping");
}

void TsSqlDatabaseThread::createDatabase(
   TsSqlDatabaseImpl *object,
   const QString &server,
   const QString &database,
   const QString &user,
   const QString &password,
   const QString &characterSet,
   const QString &role,
   const QString &createParams)
{
   DEBUG_OUT("Creating database-handle");
   DatabaseHandle handle = 
      reinterpret_cast<DatabaseHandle>(
         new IBPP::Database(
            IBPP::DatabaseFactory(
               server.toStdString(),
               database.toStdString(),
               user.toStdString(),
               password.toStdString(),
               role.toStdString(),
               characterSet.toStdString(),
               createParams.toStdString())));
   m_databaseHandles.push_back(handle);
   object->m_handle = handle;
   object->m_waiter.wakeAll();
   DEBUG_OUT("Database-handle " << handle << " created for object " << object);
}

void TsSqlDatabaseThread::databaseOpen(TsSqlDatabaseImpl *object, DatabaseHandle handle)
{
   DEBUG_RECEIVE("Received open request from " << object << " for database " << handle);
   DBHANDLE(handle)->Connect();
   EMIT_ASYNC(object, emitDatabaseOpened);
}

void TsSqlDatabaseThread::databaseClose(TsSqlDatabaseImpl *object, DatabaseHandle handle)
{
   DEBUG_RECEIVE("Received close request from " << object << " for database " << handle);
   DBHANDLE(handle)->Disconnect();
   EMIT_ASYNC(object, emitDatabaseClosed);
}

void TsSqlDatabaseThread::databaseIsOpen(
   TsSqlDatabaseImpl *object, 
   DatabaseHandle handle, 
   bool *result)
{
   DEBUG_RECEIVE("Received isOpen request from " << object << " for database " << handle);
   *result = DBHANDLE(handle)->Connected();
   object->m_waiter.wakeAll();
}

void TsSqlDatabaseThread::databaseInfo(
   TsSqlDatabaseImpl *object, 
   DatabaseHandle handle, 
   DatabaseInfo info,
   QString *result)
{
   DEBUG_RECEIVE("Received databaseInfo(" << info << ") request from " << object << " for database " << handle);
   switch(info)
   {
      case diServer:
         *result = QString::fromStdString(DBHANDLE(handle)->ServerName());
         break;
      case diDatabase:
         *result = QString::fromStdString(DBHANDLE(handle)->DatabaseName());
         break;
      case diUser:
         *result = QString::fromStdString(DBHANDLE(handle)->Username());
         break;
      case diPassword:
         *result = QString::fromStdString(DBHANDLE(handle)->UserPassword());
         break;
      case diCharacterSet:
         *result = QString::fromStdString(DBHANDLE(handle)->CharSet());
         break;
      case diRole:
         *result = QString::fromStdString(DBHANDLE(handle)->RoleName());
         break;
      case diCreateParams:
         *result = QString::fromStdString(DBHANDLE(handle)->CreateParams());
         break;
      default:
         *result = QString();
   }
   object->m_waiter.wakeAll();
}

void TsSqlDatabaseThread::databaseConnectedUsers(
   TsSqlDatabaseImpl *object,
   DatabaseHandle handle,
   QVector<QString> *result)
{
   DEBUG_RECEIVE("Received connected users request from " << object << " for database " << handle);
   std::vector<std::string> users;
   DBHANDLE(handle)->Users(users);
   result->clear();
   for(std::vector<std::string>::const_iterator i = users.begin();
       i != users.end();
       ++i)
      result->push_back(QString::fromStdString(*i));
   object->m_waiter.wakeAll();
}

void TsSqlDatabaseThread::createTransaction(
   TsSqlTransactionImpl *object,
   DatabaseHandle database, 
   TsSqlTransaction::TransactionMode mode)
{
   DEBUG_OUT("Creating transaction-handle");
   IBPP::TAM ibppMode;
   switch(mode)
   {
      case TsSqlTransaction::tmRead:
         ibppMode = IBPP::amRead;
         break;
      default:
         ibppMode = IBPP::amWrite;
   }
   TransactionHandle transaction = 
      reinterpret_cast<TransactionHandle>(
         new IBPP::Transaction(
            IBPP::TransactionFactory(
               DBHANDLE(database),
               ibppMode)));
   object->m_handle = transaction;
   object->m_waiter.wakeAll();
}

void TsSqlDatabaseThread::transactionStart(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction start from " << object << " for " << handle);
   TRHANDLE(handle)->Start();
   EMIT_ASYNC(object, emitTransactionStarted);
}

void TsSqlDatabaseThread::transactionCommit(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction commit from " << object << " for " << handle);
   TRHANDLE(handle)->Commit();
   EMIT_ASYNC(object, emitTransactionCommited);
}

void TsSqlDatabaseThread::transactionCommitRetaining(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction retaining commit from " << object << " for " << handle);
   TRHANDLE(handle)->CommitRetain();
   EMIT_ASYNC(object, emitTransactionCommited);
   EMIT_ASYNC(object, emitTransactionStarted);
}

void TsSqlDatabaseThread::transactionRollBack(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction rollback from " << object << " for " << handle);
   TRHANDLE(handle)->Rollback();
   EMIT_ASYNC(object, emitTransactionRolledBack);
}

void TsSqlDatabaseThread::createStatement(
   TsSqlStatementImpl *object,
   DatabaseHandle database, 
   TransactionHandle transaction)
{
   DEBUG_OUT("Creating statement-handle");
   StatementHandle handle = 
      reinterpret_cast<StatementHandle>(
         new IBPP::Statement(
            IBPP::StatementFactory(
               DBHANDLE(database),
               TRHANDLE(transaction))));
   object->m_handle = handle;
   object->m_waiter.wakeAll();
}

void TsSqlDatabaseThread::createStatement(
   TsSqlStatementImpl *object,
   DatabaseHandle database, 
   TransactionHandle transaction,
   QString sql)
{
   DEBUG_OUT("Creating statement-handle");
   StatementHandle handle = 
      reinterpret_cast<StatementHandle>(
         new IBPP::Statement(
            IBPP::StatementFactory(
               DBHANDLE(database),
               TRHANDLE(transaction),
               sql.toStdString())));
   object->m_handle = handle;
   object->m_waiter.wakeAll();
}

void TsSqlDatabaseThread::statementExecute(
   TsSqlStatementImpl *object,
   StatementHandle handle)
{
   DEBUG_RECEIVE("Received execute request from " << object << " for statement " << handle);
   STHANDLE(handle)->Execute();
   EMIT_ASYNC(object, emitStatementExecuted);
}

void TsSqlDatabaseThread::statementExecute(
   TsSqlStatementImpl *object,
   StatementHandle handle,
   const QString &sql)
{
   DEBUG_RECEIVE("Received execute request from " << object << " for statement " << handle);
   STHANDLE(handle)->Execute(sql.toStdString());
   EMIT_ASYNC(object, emitStatementExecuted);
}

void TsSqlDatabaseThread::emitStatementRow(
   TsSqlStatementImpl *receiver, 
   StatementHandle statement)
{
   IBPP::Statement &st = STHANDLE(statement);
   try
   {
      int columns = st->Columns();
      QVector<QVariant> row(columns);
      std::string text;
      IBPP::Blob blob;
      IBPP::Date date;
      IBPP::Time time;
      IBPP::Timestamp timestamp;
      int  year, month, day, hour, minute, second, msecond;
      int64_t largeint;
      double dbl;
      for(int i = 1; i <= columns; ++i)
      {
         switch(st->ColumnType(i))
         {
            case IBPP::sdBlob:
               blob = IBPP::BlobFactory(st->DatabasePtr(), st->TransactionPtr());
               if (st->Get(i, blob))
                  row[i-1] = QVariant(QVariant::ByteArray);
               else
               {
                  blob->Load(text);
                  row[i-1] = QVariant(QByteArray::fromRawData(text.c_str(), text.size()));
               }
               break;
            case IBPP::sdDate:
               if (st->Get(i, date))
                  row[i-1] = QVariant(QVariant::Date);
               else
               {
                  date.GetDate(year, month, day);
                  row[i-1] = QVariant(QDate(year, month, day));
               }
               break;
            case IBPP::sdTime:
               if (st->Get(i, time))
                  row[i-1] = QVariant(QVariant::Time);
               else
               {
                  time.GetTime(hour, minute, second, msecond);
                  row[i-1] = QVariant(QTime(hour, minute, second, msecond));
               }
               break;
            case IBPP::sdTimestamp:
               if (st->Get(i, timestamp))
                  row[i-1] = QVariant(QVariant::DateTime);
               else
               {
                  timestamp.GetDate(year, month, day);
                  timestamp.GetTime(hour, minute, second, msecond);
                  row[i-1] = QVariant(QDateTime(
                     QDate(year, month, day), 
                     QTime(hour, minute, second, msecond)));
               }
               break;
            case IBPP::sdString:
               {
                  if (st->Get(i, text))
                     row[i-1] = QVariant(QVariant::String);
                  else
                     row[i-1] = QVariant(QString::fromStdString(text));
                  break;
               }
            case IBPP::sdSmallint:
            case IBPP::sdInteger:
            case IBPP::sdLargeint:
               if (st->Get(i, largeint))
                  row[i-1] = QVariant(QVariant::LongLong);
               else
                  row[i-1] = QVariant(largeint);
               break;
            case IBPP::sdFloat:
            case IBPP::sdDouble:
               if (st->Get(1, dbl))
                  row[i-1] = QVariant(QVariant::Double);
               else
                  row[i-1] = QVariant(dbl);
               break;
            default:
               qDebug() << "Unknown column type!";
         }
      }
      TsSqlThreadEmitter emitter(receiver);
      emitter.emitStatementFetched(row);

      emit statementFetchNext(receiver, statement);
   } catch(std::exception &e)
   {
      qDebug() << "Reading a row failed: " << e.what();
   }
}

void TsSqlDatabaseThread::statementStartFetch(
   TsSqlStatementImpl *object,
   StatementHandle handle)
{
   DEBUG_RECEIVE("Received start fetch request from " << object << " for statement " << handle);

   if (!STHANDLE(handle)->DatabasePtr()->Connected() ||
       !STHANDLE(handle)->TransactionPtr()->Started())
   {
      EMIT_ASYNC(object, emitStatementFetchStarted);
      EMIT_ASYNC(object, emitStatementFetchFinished);
   }
   else
   {
      bool atEnd = !STHANDLE(handle)->Fetch();
      EMIT_ASYNC(object, emitStatementFetchStarted);
      if (atEnd)
      {
         DEBUG_OUT("Finished fetching datasets for statement " << handle);
         EMIT_ASYNC(object, emitStatementFetchFinished);
      }
      else
         emitStatementRow(object, handle);
   }
}

void TsSqlDatabaseThread::statementFetchNext(
   TsSqlStatementImpl *object,
   StatementHandle handle)
{
   DEBUG_RECEIVE("Received fetch next request from " << object << " for statement " << handle);

   // Stop fetching when the transaction has been stopped or the
   // database connection is closed.
   if (!STHANDLE(handle)->TransactionPtr()->Started() || 
       !STHANDLE(handle)->DatabasePtr()->Connected())
   {
      EMIT_ASYNC(object, emitStatementFetchFinished);
   }
   else
   {
      if (STHANDLE(handle)->Fetch())
         emitStatementRow(object, handle);
      else
         EMIT_ASYNC(object, emitStatementFetchFinished);
   }
}

TsSqlType ibppTypeToTs(IBPP::SDT ibppType)
{
   switch(ibppType)
   {
      case IBPP::sdBlob:
         return stBlob;
      case IBPP::sdDate:
         return stDate;
      case IBPP::sdTime:
         return stTime;
      case IBPP::sdTimestamp:
         return stTimeStamp;
      case IBPP::sdString:
         return stString;
      case IBPP::sdSmallint:
         return stSmallInt;
      case IBPP::sdInteger:
         return stInt;
      case IBPP::sdLargeint:
         return stLargeInt;
      case IBPP::sdFloat:
         return stFloat;
      case IBPP::sdDouble:
         return stDouble;
      default:
         DEBUG_OUT("Unsupported column type detected, defaulting to string.");
         return stString;
   }
}

void TsSqlDatabaseThread::statementInfo(
   TsSqlStatementImpl *object, 
   StatementHandle handle, 
   StatementInfo info,
   QVariant param,
   QVariant *result)
{
   DEBUG_RECEIVE("Received statement info(" << info << " request from " << object << " for statement " << handle);
   std::string temp;
   switch(info)
   {
      case siPlan:
         STHANDLE(handle)->Plan(temp);
         *result = QString::fromStdString(temp);
         break;
      case siColumnCount:
         *result = STHANDLE(handle)->Columns();
         break;
      case siColumnName:
         *result = QString::fromAscii(STHANDLE(handle)->ColumnName(param.toInt()));
         break;
      case siColumnIndex:
         *result = STHANDLE(handle)->ColumnNum(param.toString().toStdString());
         break;
      case siColumnAlias:
         *result = QString::fromAscii(STHANDLE(handle)->ColumnAlias(param.toInt()));
         break;
      case siColumnTable:
         *result = QString::fromAscii(STHANDLE(handle)->ColumnTable(param.toInt()));
         break;
      case siColumnType:
         *result = static_cast<int>(ibppTypeToTs(STHANDLE(handle)->ColumnType(param.toInt())));
         break;
      case siColumnSubType:
         *result = STHANDLE(handle)->ColumnSubtype(param.toInt());
         break;
      case siColumnSize:
         *result = STHANDLE(handle)->ColumnSize(param.toInt());
         break;
      case siColumnScale:
         *result = STHANDLE(handle)->ColumnScale(param.toInt());
         break;
      default:
         DEBUG_OUT("Unknown statement info(" << info << ") requested!");
   }
   object->m_waiter.wakeAll();
}

TsSqlDatabaseImpl::TsSqlDatabaseImpl(
   const QString &server,
   const QString &database,
   const QString &user,
   const QString &password,
   const QString &characterSet,
   const QString &role,
   const QString &createParams):
   m_handle(0),
   m_thread(m_waiter)
{
   m_mutex.lock();
   m_thread.start();
   m_waiter.wait(&m_mutex);
   m_thread.moveToThread(&m_thread);
   DEBUG_OUT("New database object " << this);
   connect(
      this, 
      SIGNAL(createHandle(
            TsSqlDatabaseImpl *,
            QString,
            QString,
            QString,
            QString,
            QString,
            QString,
            QString)),
      &m_thread,
      SLOT(createDatabase(
            TsSqlDatabaseImpl *,
            QString,
            QString,
            QString,
            QString,
            QString,
            QString,
            QString)), 
      Qt::QueuedConnection);

   emit createHandle(
         this,
         server,
         database,
         user,
         password,
         characterSet,
         role,
         createParams);
   m_waiter.wait(&m_mutex);
   DEBUG_OUT("Database-handle " << m_handle << " arrived for " << this);

   /* connect the other needed signals */
   connect(
      this,
      SIGNAL(databaseOpen(TsSqlDatabaseImpl*, DatabaseHandle)),
      &m_thread,
      SLOT(databaseOpen(TsSqlDatabaseImpl*, DatabaseHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(databaseClose(TsSqlDatabaseImpl*, DatabaseHandle)),
      &m_thread,
      SLOT(databaseClose(TsSqlDatabaseImpl*, DatabaseHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(databaseIsOpen(TsSqlDatabaseImpl*, DatabaseHandle, bool*)),
      &m_thread,
      SLOT(databaseIsOpen(TsSqlDatabaseImpl*, DatabaseHandle, bool*)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(databaseInfo(
         TsSqlDatabaseImpl *, 
         DatabaseHandle, 
         DatabaseInfo,
         QString *)),
      &m_thread,
      SLOT(databaseInfo(
         TsSqlDatabaseImpl *, 
         DatabaseHandle, 
         DatabaseInfo,
         QString *)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(databaseConnectedUsers(
         TsSqlDatabaseImpl*,
         DatabaseHandle,
         QVector<QString>*)),
      &m_thread,
      SLOT(databaseConnectedUsers(
         TsSqlDatabaseImpl*,
         DatabaseHandle,
         QVector<QString>*)),
      Qt::QueuedConnection);
}

void TsSqlDatabaseImpl::emitOpened()
{
   emit opened();
}

void TsSqlDatabaseImpl::emitClosed()
{
   emit closed();
}

void TsSqlDatabaseImpl::open()
{
   emit databaseOpen(this, m_handle);
}

void TsSqlDatabaseImpl::close()
{
   DEBUG_OUT("Closing database " << this);
   emit databaseClose(this, m_handle);
}

bool TsSqlDatabaseImpl::isOpen()
{
   bool result = false;
   emit databaseIsOpen(this, m_handle, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::isOpen() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::server()
{
   QString result;
   emit databaseInfo(this, m_handle, diServer, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::server() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::database()
{
   QString result;
   emit databaseInfo(this, m_handle, diDatabase, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::database() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::user()
{
   QString result;
   emit databaseInfo(this, m_handle, diUser, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::user() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::password()
{
   QString result;
   emit databaseInfo(this, m_handle, diPassword, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::password() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::role()
{
   QString result;
   emit databaseInfo(this, m_handle, diRole, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::role() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::characterSet()
{
   QString result;
   emit databaseInfo(this, m_handle, diCharacterSet, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::characterSet() deadlocked");
   return result;
}

QString TsSqlDatabaseImpl::createParams()
{
   QString result;
   emit databaseInfo(this, m_handle, diCreateParams, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::createParams() deadlocked");
   return result;
}

QVector<QString> TsSqlDatabaseImpl::connectedUsers()
{
   QVector<QString> result;
   emit databaseConnectedUsers(this, m_handle, &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlDatabaseImpl::connectedUsers() deadlocked");
   return result;
}

TsSqlDatabaseImpl::~TsSqlDatabaseImpl()
{
   m_thread.quit();
   if (!m_thread.wait(10000))
      m_thread.terminate();
   m_thread.wait(10000);
}

TsSqlTransactionImpl::TsSqlTransactionImpl(
   TsSqlDatabaseImpl &database, 
   TsSqlTransaction::TransactionMode mode):
   m_handle(0)
{
   DEBUG_OUT("Creating new transaction");
   m_mutex.lock();
   connect(
      this,
      SIGNAL(createTransaction(
         TsSqlTransactionImpl *,
         DatabaseHandle,
         TsSqlTransaction::TransactionMode)),
      &database.m_thread,
      SLOT(createTransaction(
         TsSqlTransactionImpl *,
         DatabaseHandle,
         TsSqlTransaction::TransactionMode)),
      Qt::QueuedConnection);

   emit createTransaction(
      this,
      database.m_handle,
      mode);
   m_waiter.wait(&m_mutex);
   DEBUG_OUT("Transaction-handle " << m_handle << " arrived for " << this);

   connect(
      this,
      SIGNAL(transactionStart(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionStart(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(transactionCommit(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionCommit(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(transactionCommitRetaining(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionCommitRetaining(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(transactionRollBack(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionRollBack(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::QueuedConnection);
}

void TsSqlTransactionImpl::emitStarted()
{
   emit started();
}

void TsSqlTransactionImpl::emitCommited()
{
   emit commited();
}

void TsSqlTransactionImpl::emitRolledBack()
{
   emit rolledBack();
}

void TsSqlTransactionImpl::start()
{
   DEBUG_OUT("Starting transaction " << this);
   emit transactionStart(
      this,
      m_handle);
}

void TsSqlTransactionImpl::commit()
{
   DEBUG_OUT("Commiting transaction " << this);
   emit transactionCommit(
      this,
      m_handle);
}

void TsSqlTransactionImpl::commitRetaining()
{
   DEBUG_OUT("Commiting transaction retainingly " << this);
   emit transactionCommitRetaining(
      this,
      m_handle);
}

void TsSqlTransactionImpl::rollBack()
{
   DEBUG_OUT("Rolling back transaction " << this);
   emit transactionRollBack(
      this,
      m_handle);
}

bool TsSqlTransactionImpl::isStarted()
{
   return true;
}

TsSqlStatementImpl::TsSqlStatementImpl(
   TsSqlDatabaseImpl &database,
   TsSqlTransactionImpl &transaction):
   m_handle(0)
{
   DEBUG_OUT("Creating new statement");
   m_mutex.lock();
   connect(
      this,
      SIGNAL(createStatement(
         TsSqlStatementImpl*,
         DatabaseHandle,
         TransactionHandle)),
      &database.m_thread,
      SLOT(createStatement(
         TsSqlStatementImpl*,
         DatabaseHandle,
         TransactionHandle)),
      Qt::QueuedConnection);

   emit createStatement(
      this,
      database.m_handle,
      transaction.m_handle);
   m_waiter.wait(&m_mutex);
   DEBUG_OUT("Statement-handle " << m_handle << " arrived for " << this);

   connectSignals(&database.m_thread);
}

void TsSqlStatementImpl::connectSignals(QObject *receiver)
{
   connect(
      this,
      SIGNAL(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementStartFetch(
         TsSqlStatementImpl *,
         StatementHandle)),
      receiver,
      SLOT(statementStartFetch(
         TsSqlStatementImpl *,
         StatementHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementInfo(
         TsSqlStatementImpl *, 
         StatementHandle, 
         StatementInfo,
         QVariant,
         QVariant *)),
      receiver,
      SLOT(statementInfo(
         TsSqlStatementImpl *, 
         StatementHandle, 
         StatementInfo,
         QVariant,
         QVariant *)),
      Qt::QueuedConnection);
}

TsSqlStatementImpl::TsSqlStatementImpl(
   TsSqlDatabaseImpl &, 
   TsSqlTransactionImpl &, 
   const QString &)
{
}

void TsSqlStatementImpl::emitExecuted()
{
   emit executed();
}

void TsSqlStatementImpl::emitFetchStarted()
{
   emit fetchStarted();
}

void TsSqlStatementImpl::emitFetched(TsSqlRow row)
{
   emit fetched(row);
}

void TsSqlStatementImpl::emitFetchFinished()
{
   emit fetchFinished();
}

void TsSqlStatementImpl::prepare(const QString &)
{
}

void TsSqlStatementImpl::execute(const QString &sql)
{
   emit statementExecute(this, m_handle, sql);
}

void TsSqlStatementImpl::execute()
{
   emit statementExecute(this, m_handle);
}

QString TsSqlStatementImpl::sql()
{
   return QString();
}

QString TsSqlStatementImpl::plan()
{
   return QString();
}

int TsSqlStatementImpl::affectedRows()
{
   return 0;
}

void TsSqlStatementImpl::fetch()
{
   emit statementStartFetch(this, m_handle);
}

int TsSqlStatementImpl::columnCount()
{
   QVariant result;
   emit statementInfo(
         this, 
         m_handle, 
         siColumnCount, 
         0,
         &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnCount() deadlocked");
   return result.toInt();
}

QString TsSqlStatementImpl::columnName(int columnIndex)
{
   QVariant result;
   emit statementInfo(
         this, 
         m_handle, 
         siColumnName, 
         columnIndex,
         &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnName() deadlocked");
   return result.toString();
}

int TsSqlStatementImpl::columnIndex(const QString &columnName)
{
   QVariant result;
   emit statementInfo(
         this, 
         m_handle, 
         siColumnIndex, 
         columnName,
         &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnIndex() deadlocked");
   return result.toInt();
}

QString TsSqlStatementImpl::columnAlias(int columnIndex)
{
   QVariant result;
   emit statementInfo(
         this, 
         m_handle, 
         siColumnAlias, 
         columnIndex,
         &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnAlias() deadlocked");
   return result.toString();
}

QString TsSqlStatementImpl::columnTable(int columnIndex)
{
   QVariant result;
   emit statementInfo(
         this, 
         m_handle, 
         siColumnTable, 
         columnIndex,
         &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnTable() deadlocked");
   return result.toString();
}

TsSqlType TsSqlStatementImpl::columnType(int columnIndex)
{
   QVariant result;
   emit statementInfo(
         this, 
         m_handle, 
         siColumnType, 
         columnIndex,
         &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnType() deadlocked");
   return static_cast<TsSqlType>(result.toInt());
}

int TsSqlStatementImpl::columnSubType(int columnIndex)
{
   QVariant result;
   emit statementInfo(
      this,
      m_handle,
      siColumnSubType,
      columnIndex,
      &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnSubType() deadlocked");
   return result.toInt();
}

int TsSqlStatementImpl::columnSize(int columnIndex)
{
   QVariant result;
   emit statementInfo(
      this,
      m_handle,
      siColumnSize,
      columnIndex,
      &result);
   CHECK_DEADLOCK;
   return result.toInt();
}

int TsSqlStatementImpl::columnScale(int columnIndex)
{
   QVariant result;
   emit statementInfo(
      this,
      m_handle,
      siColumnScale,
      columnIndex,
      &result);
   if (!m_waiter.wait(&m_mutex, 1000))
      DEBUG_OUT("TsSqlStatementImpl::columnScale() deadlocked");
   return result.toInt();
}

namespace
{
   struct TsSqlMetaTypeInitializer
   {
      TsSqlMetaTypeInitializer()
      {
         qRegisterMetaType<QVariant>();
         qRegisterMetaType<DatabaseHandle>();
         qRegisterMetaType<TransactionHandle>();
         qRegisterMetaType<StatementHandle>();

         qRegisterMetaType<DatabaseInfo>();
         qRegisterMetaType<StatementInfo>();

         qRegisterMetaType<TsSqlRow>();
         qRegisterMetaType<TsSqlTransaction::TransactionMode>();
      }
   } g_sqlMetaTypeInitializer;
}