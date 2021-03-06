#include <algorithm>

#include <QDebug>

#include "private/ibpp/core/ibpp.h"

#include "main.h"
#include "database_p.h"

#define DEBUG_LOG(message) qDebug() << "Thread [" << QThread::currentThreadId() << "] " << message

TsSqlBufferImpl::TsSqlBufferImpl():
   m_data(0),
   m_fetch(0),
   m_colCount(0)
{
   setStatements(0, 0);
}

TsSqlBufferImpl::TsSqlBufferImpl(TsSqlStatement &dataStatement):
   m_data(0),
   m_fetch(0),
   m_colCount(0)
{
   setStatements(&dataStatement);
}

TsSqlBufferImpl::TsSqlBufferImpl(
   TsSqlStatement &dataStatement, 
   TsSqlStatement &fetchStatement):
   m_data(0),
   m_fetch(0),
   m_colCount(0)
{
   setStatements(&dataStatement, &fetchStatement);
}

TsSqlBufferImpl::TsSqlBufferImpl(const TsSqlBufferImpl &copy): QObject(0)
{
   QMutexLocker lock(&m_mutex);
   QMutexLocker lockCopy(&copy.m_mutex);
   m_rows = copy.m_rows;
   setStatements(copy.m_data, copy.m_fetch);
}

void TsSqlBufferImpl::setStatements(
   TsSqlStatement *dataStatement, 
   TsSqlStatement *fetchStatement)
{
   if (m_data)
      disconnect(m_data, 0, this, 0);
   if (m_fetch)
      disconnect(m_fetch, 0, this, 0);
   // Only a fetch but no data statement is not allowed
   if (!dataStatement)
      fetchStatement = 0;
   m_data  = dataStatement;
   m_fetch = fetchStatement;
   if (m_data && m_fetch)
      connect(m_fetch, SIGNAL(fetched(TsSqlRow)), this, SLOT(appendEmptyRow(TsSqlRow)));
   else if (dataStatement)
      connect(m_data, SIGNAL(fetched(TsSqlRow)), this, SLOT(appendRow(TsSqlRow)));
   connect(dataStatement, SIGNAL(prepared()), this, SLOT(updateColumnCount()));
}

void TsSqlBufferImpl::updateColumnCount()
{
   m_colCount = m_data->columnCount();
   emit columnsChanged();
}

void TsSqlBufferImpl::validateRow(unsigned row)
{
   QPair<bool, TsSqlRow> &item = m_rows[row];
   if (!item.first)
   {
      m_data->setParam(1, item.second[0].asInt32());
      m_data->executeWaiting();
      m_data->fetchRow(item.second);
      item.first = true;
      emit rowFetched(item.second);
   }
}

void TsSqlBufferImpl::clear()
{
   QMutexLocker locker(&m_mutex);
   m_rows.clear();
   emit cleared();
}

void TsSqlBufferImpl::appendEmptyRow(const TsSqlRow &row)
{
   QMutexLocker locker(&m_mutex);
   m_rows.push_back(qMakePair(false, row));
   emit rowAppended();
}

void TsSqlBufferImpl::appendRow(const TsSqlRow &row)
{
   QMutexLocker locker(&m_mutex);
   m_rows.push_back(qMakePair(true, row));
   emit rowAppended();
}

void TsSqlBufferImpl::deleteRow(unsigned index)
{
   QMutexLocker locker(&m_mutex);
   m_rows.remove(index);
   emit rowDeleted();
}

void TsSqlBufferImpl::getRow(unsigned index, TsSqlRow &row)
{
   QMutexLocker locker(&m_mutex);
   validateRow(index);
   row = m_rows[index].second;
}

TsSqlRow TsSqlBufferImpl::getRow(unsigned index)
{
   QMutexLocker locker(&m_mutex);
   validateRow(index);
   return m_rows[index].second;
}

void TsSqlBufferImpl::setRow(unsigned index, const TsSqlRow &row)
{
   QMutexLocker locker(&m_mutex);
   m_rows[index].first   = true; // don't overwrite the content with fetched data
   m_rows[index].second  = row;
}

unsigned TsSqlBufferImpl::count() const
{
   QMutexLocker locker(&m_mutex);
   return m_rows.count();
}

unsigned TsSqlBufferImpl::columnCount() const
{
   return m_colCount;
}

TsSqlStatement *TsSqlBufferImpl::dataStatement()
{
   return m_data;
}

TsSqlStatement *TsSqlBufferImpl::fetchStatement()
{
   return m_fetch;
}


#define EMIT_ASYNC(object, signal) { TsSqlThreadEmitter emitter(object); emitter.signal(); }
#define EMIT_ERROR(object, errorMessage) {TsSqlThreadEmitter emitter(object); emitter.emitError(errorMessage); }

#define DEBUG_RECEIVE(message) DEBUG_OUT(message)

#define DBHANDLE(handle) (*reinterpret_cast<IBPP::Database*>   (handle))
#define TRHANDLE(handle) (*reinterpret_cast<IBPP::Transaction*>(handle))
#define STHANDLE(handle) (*reinterpret_cast<IBPP::Statement*>  (handle))

TsSqlThreadEmitter::TsSqlThreadEmitter(QObject *object):
   m_object(object)
{
}

void TsSqlThreadEmitter::emitDatabaseOpened()
{
   connect(this, SIGNAL(databaseOpened()), m_object, SIGNAL(opened()), Qt::QueuedConnection);
   emit databaseOpened();
}

void TsSqlThreadEmitter::emitDatabaseClosed()
{
   connect(this, SIGNAL(databaseClosed()), m_object, SIGNAL(closed()), Qt::QueuedConnection);
   emit databaseClosed();
}

void TsSqlThreadEmitter::emitTransactionStarted()
{
   connect(this, SIGNAL(transactionStarted()), m_object, SIGNAL(started()), Qt::QueuedConnection);
   emit transactionStarted();
}

void TsSqlThreadEmitter::emitTransactionCommited()
{
   connect(this, SIGNAL(transactionCommited()), m_object, SIGNAL(commited()), Qt::QueuedConnection);
   emit transactionCommited();
}

void TsSqlThreadEmitter::emitTransactionRolledBack()
{
   connect(this, SIGNAL(transactionRolledBack()), m_object, SIGNAL(rolledBack()), Qt::QueuedConnection);
   emit transactionRolledBack();
}

void TsSqlThreadEmitter::emitStatementPrepared()
{
//   reinterpret_cast<TsSqlStatementImpl*>(m_object)->emitPrepared();
   connect(this, SIGNAL(statementPrepared()), m_object, SLOT(emitPrepared()), Qt::QueuedConnection);
   emit statementPrepared();
}

void TsSqlThreadEmitter::emitStatementExecuted()
{
   connect(this, SIGNAL(statementExecuted()), m_object, SIGNAL(executed()), Qt::QueuedConnection);
   emit statementExecuted();
}

void TsSqlThreadEmitter::emitStatementFetchStarted()
{
   connect(this, SIGNAL(statementFetchStarted()), m_object, SIGNAL(fetchStarted()), Qt::QueuedConnection);
   emit statementFetchStarted();
}

void TsSqlThreadEmitter::emitStatementFetched(const TsSqlRow &row)
{
   connect(this, SIGNAL(statementFetched(TsSqlRow)), m_object, SLOT(fetchDataset(TsSqlRow)), Qt::QueuedConnection);
   emit statementFetched(row);
}

void TsSqlThreadEmitter::emitStatementFetchFinished()
{
   connect(this, SIGNAL(statementFetchFinished()), m_object, SIGNAL(fetchFinished()), Qt::QueuedConnection);
   emit statementFetchFinished();
}

void TsSqlThreadEmitter::emitError(const QString &errorMessage)
{
   connect(this, SIGNAL(error(QString)), m_object, SIGNAL(error(QString)), Qt::QueuedConnection);
   emit error(errorMessage);
}

TsSqlDatabaseThread::TsSqlDatabaseThread()
{
}

void TsSqlDatabaseThread::run()
{
   DEBUG_OUT("New thread " << this << " is running");
   QThread::run();
   DEBUG_OUT("Thread is stopping");
}

void setFromStatement(TsSqlVariant &variant, void *statement, int col)
{
   using namespace IBPP;
   Statement &st = *reinterpret_cast<Statement*>(statement);
   switch(st->ColumnType(col))
   {
      case sdBlob:
         {
            Blob b = BlobFactory(st->DatabasePtr(), st->TransactionPtr());
            if (st->Get(col, b))
               variant.setNull();
            else
            {
               std::string temp;
               b->Load(temp);
               variant.setVariant(QVariant(QString::fromStdString(temp)));
            }
            break;
         }
      case sdDate:
         {
            Date d;
            if (st->Get(col, d))
               variant.setNull();
            else
            {
               int year, month, day;
               d.GetDate(year, month, day);
               variant.setVariant(QVariant(QDate(year, month, day)));
            }
            break;
         }
      case sdTime:
         {
            Time d;
            if (st->Get(col, d))
               variant.setNull();
            else
            {
               int hour, minute, second, msec;
               d.GetTime(hour, minute, second, msec);
               variant.setVariant(QVariant(QTime(hour, minute, second, msec)));
            }
            break;
         }
      case sdTimestamp:
         {
            Timestamp d;
            if (st->Get(col, d))
               variant.setNull();
            else
            {
               int year, month, day, hour, minute, second, msec;
               d.GetTime(hour, minute, second, msec);
               d.GetDate(year, month, day);
               variant.setVariant(
                  QVariant(QDateTime(
                     QDate(year, month, day),
                     QTime(hour, minute, second, msec))));
            }
            break;
         }
      case sdString:
         {
            std::string temp;
            if (st->Get(col, temp))
               variant.setNull();
            else
               variant.setVariant(QVariant(QString::fromStdString(temp)));
            break;
         }
      case sdSmallint:
         {
            TsSqlSmallInt temp;
            if (st->Get(col, temp))
               variant.setNull();
            else
               variant.setVariant(QVariant(temp));
            break;
         }
      case sdInteger:
         {
            TsSqlInt temp;
            if (st->Get(col, temp))
               variant.setNull();
            else
               variant.setVariant(QVariant(temp));
            break;
         }
      case sdLargeint:
         {
            TsSqlLargeInt temp;
            if (st->Get(col, temp))
               variant.setNull();
            else
               variant.setVariant(QVariant(temp));
            break;
         }
      case sdFloat:
         {
            float temp;
            if (st->Get(col, temp))
               variant.setNull();
            else
               variant.setVariant(QVariant(temp));
            break;
         }
      case sdDouble:
         {
            double temp;
            if (st->Get(col, temp))
               variant.setNull();
            else
               variant.setVariant(QVariant(temp));
            break;
         }
      default:
         variant.setNull();
   }
}

void setStatementParam(const TsSqlVariant &variant, void *statement, int column)
{
   IBPP::Statement &st = STHANDLE(statement);
   switch(variant.m_type)
   {
      case stBlob:
         {
            IBPP::Blob blob = IBPP::BlobFactory(st->DatabasePtr(), st->TransactionPtr());
            QByteArray &arr = *reinterpret_cast<QByteArray*>(variant.m_data.asPointer);
            blob->Save(std::string(arr.constData(), arr.size()));
            st->Set(column, blob);
         }
         break;
      case stDate:
         {
            QDate &d = *reinterpret_cast<QDate*>(variant.m_data.asPointer);
            IBPP::Date date(d.year(), d.month(), d.day());
            st->Set(column, date);
         }
         break;
      case stTime:
         {
            QTime &t = *reinterpret_cast<QTime*>(variant.m_data.asPointer);
            IBPP::Time time(t.hour(), t.minute(), t.second(), t.msec());
            st->Set(column, time);
         }
         break;
      case stTimeStamp:
         {
            QDateTime &dt = *reinterpret_cast<QDateTime*>(variant.m_data.asPointer);
            IBPP::Timestamp timestamp(
               dt.date().year(),
               dt.date().month(),
               dt.date().day(),
               dt.time().hour(),
               dt.time().minute(),
               dt.time().second(),
               dt.time().msec());
            st->Set(column, timestamp);
         }
         break;
      case stString:
         st->Set(
            column,
            reinterpret_cast<QString*>(variant.m_data.asPointer)->toStdString());
         break;
      case stSmallInt:
         st->Set(column, variant.m_data.asInt16);
         break;
      case stInt:
         st->Set(column, variant.m_data.asInt32);
         break;
      case stLargeInt:
         st->Set(column, variant.m_data.asInt64);
         break;
      case stFloat:
         st->Set(column, variant.m_data.asFloat);
         break;
      case stDouble:
         st->Set(column, variant.m_data.asDouble);
         break;
   default:
      st->SetNull(column);
   }
}

void TsSqlDatabaseThread::test()
{
   IBPP::Database db = IBPP::DatabaseFactory(
      "",
      "melchior:/var/firebird/test.fdb",
      "sysdba",
      "5735");
   db->Connect();
   IBPP::Transaction tr = IBPP::TransactionFactory(db);
   tr->Start();

   IBPP::Statement st = IBPP::StatementFactory(
      db, 
      tr, 
      "select * from test2");
   st->Execute();
   if (st->Fetch())
   {
      int cols = st->Columns();
      for(int i = 1; i <= cols; ++i)
      {
         TsSqlVariant variant;
         setFromStatement(variant, &st, i);
         qDebug() << variant.asString();
      }
   }
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
   DEBUG_OUT("Database-handle " << handle << " created for object " << object);
}

void TsSqlDatabaseThread::destroyDatabase(DatabaseHandle handle)
{
   std::vector<DatabaseHandle>::iterator i = std::find(
      m_databaseHandles.begin(),
      m_databaseHandles.end(),
      handle);
   if (i != m_databaseHandles.end() )
      m_databaseHandles.erase(i);
}

void TsSqlDatabaseThread::databaseOpen(TsSqlDatabaseImpl *object, DatabaseHandle handle)
{
   DEBUG_RECEIVE("Received open request from " << object << " for database " << handle);
   try
   {
      DBHANDLE(handle)->Connect();
      TsSqlThreadEmitter emitter(object);
      EMIT_ASYNC(object, emitDatabaseOpened);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::databaseClose(TsSqlDatabaseImpl *object, DatabaseHandle handle)
{
   DEBUG_RECEIVE("Received close request from " << object << " for database " << handle);
   try
   {
      DBHANDLE(handle)->Disconnect();
      EMIT_ASYNC(object, emitDatabaseClosed);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what())
   }
}

void TsSqlDatabaseThread::databaseIsOpen(
   TsSqlDatabaseImpl *object, 
   DatabaseHandle handle, 
   bool *result)
{
   DEBUG_RECEIVE("Received isOpen request from " << object << " for database " << handle);
   try
   {
      *result = DBHANDLE(handle)->Connected();
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what())
   }
}

void TsSqlDatabaseThread::databaseInfo(
   TsSqlDatabaseImpl *object, 
   DatabaseHandle handle, 
   DatabaseInfo info,
   QString *result)
{
   DEBUG_RECEIVE("Received databaseInfo(" << info << ") request from " << object << " for database " << handle);
   try
   {
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
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::databaseConnectedUsers(
   TsSqlDatabaseImpl *object,
   DatabaseHandle handle,
   QVector<QString> *result)
{
   DEBUG_RECEIVE("Received connected users request from " << object << " for database " << handle);
   std::vector<std::string> users;
   try
   {
      DBHANDLE(handle)->Users(users);
      result->clear();
      for(std::vector<std::string>::const_iterator i = users.begin();
            i != users.end();
            ++i)
         result->push_back(QString::fromStdString(*i));
   } catch(std::exception  &e)
   {
      EMIT_ERROR(object, e.what());
   }
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
   try
   {
      TransactionHandle transaction = 
         reinterpret_cast<TransactionHandle>(
               new IBPP::Transaction(
                  IBPP::TransactionFactory(
                     DBHANDLE(database),
                     ibppMode)));
      object->m_handle = transaction;
   } catch(std::exception &e)
   {
      object->m_handle = 0;
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::destroyTransaction(TransactionHandle handle)
{
   std::vector<TransactionHandle>::iterator i = std::find(
      m_transactionHandles.begin(),
      m_transactionHandles.end(),
      handle);
   if (i != m_transactionHandles.end())
      m_transactionHandles.erase(i);
}

void TsSqlDatabaseThread::transactionStart(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction start from " << object << " for " << handle);
   try
   {
      TRHANDLE(handle)->Start();
      EMIT_ASYNC(object, emitTransactionStarted);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::transactionCommit(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction commit from " << object << " for " << handle);
   try
   {
      TRHANDLE(handle)->Commit();
      EMIT_ASYNC(object, emitTransactionCommited);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::transactionCommitRetaining(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction retaining commit from " << object << " for " << handle);
   try
   {
      TRHANDLE(handle)->CommitRetain();
      EMIT_ASYNC(object, emitTransactionCommited);
      EMIT_ASYNC(object, emitTransactionStarted);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::transactionRollBack(
   TsSqlTransactionImpl *object,
   TransactionHandle handle)
{
   DEBUG_OUT("Requested transaction rollback from " << object << " for " << handle);
   try
   {
      TRHANDLE(handle)->Rollback();
      EMIT_ASYNC(object, emitTransactionRolledBack);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::createStatement(
   TsSqlStatementImpl *object,
   DatabaseHandle database, 
   TransactionHandle transaction)
{
   DEBUG_OUT("Creating statement-handle");
   try
   {
      StatementHandle handle = 
         reinterpret_cast<StatementHandle>(
            new IBPP::Statement(
               IBPP::StatementFactory(
                  DBHANDLE(database),
                  TRHANDLE(transaction))));
      object->m_handle = handle;
   } catch(std::exception &e)
   {
      object->m_handle = 0;     
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::createStatement(
   TsSqlStatementImpl *object,
   DatabaseHandle database, 
   TransactionHandle transaction,
   QString sql)
{
   DEBUG_OUT("Creating statement-handle");
   try
   {
      StatementHandle handle = 
         reinterpret_cast<StatementHandle>(
               new IBPP::Statement(
                  IBPP::StatementFactory(
                     DBHANDLE(database),
                     TRHANDLE(transaction),
                     sql.toStdString())));
      object->m_handle = handle;
   } catch(std::exception &e)
   {
      object->m_handle = 0;     
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::destroyStatement(StatementHandle handle)
{
   std::vector<StatementHandle>::iterator i = std::find(
      m_statementHandles.begin(),
      m_statementHandles.end(),
      handle);
   if (i != m_statementHandles.end())
      m_statementHandles.erase(i);
}

void TsSqlDatabaseThread::statementPrepare(
   TsSqlStatementImpl *object,
   StatementHandle handle,
   const QString  &sql)
{
   DEBUG_RECEIVE("Received prepare request from " << object << " for statement " << handle);
   try
   {
      DEBUG_LOG("Preparing " << sql);
      STHANDLE(handle)->Prepare(sql.toStdString());
      EMIT_ASYNC(object, emitStatementPrepared);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementExecute(
   TsSqlStatementImpl *object,
   StatementHandle handle,
   bool startFetch)
{
   DEBUG_RECEIVE("Received execute request from " << object << " for statement " << handle);
   try
   {
      DEBUG_LOG("Executing " << STHANDLE(handle)->Sql().c_str());
      STHANDLE(handle)->Execute();
      EMIT_ASYNC(object, emitStatementExecuted);
      if (startFetch)
         statementStartFetch(object, handle);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementExecute(
   TsSqlStatementImpl *object,
   StatementHandle handle,
   const QString &sql,
   bool startFetch)
{
   DEBUG_RECEIVE("Received execute request from " << object << " for statement " << handle);
   try
   {
      DEBUG_LOG("Executing " << sql);
      STHANDLE(handle)->Execute(sql.toStdString());
      EMIT_ASYNC(object, emitStatementPrepared);
      EMIT_ASYNC(object, emitStatementExecuted);
      if (startFetch)
         statementStartFetch(object, handle);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementExecute(
   TsSqlStatementImpl *object,
   StatementHandle handle,
   const TsSqlRow &params,
   bool startFetch)
{
   DEBUG_RECEIVE("Received execute request from " << object << " for statement " << handle);
   try
   {
      DEBUG_LOG("Executing " << STHANDLE(handle)->Sql().c_str() << " with params");
      setParams(handle, params);
      STHANDLE(handle)->Execute();
      EMIT_ASYNC(object, emitStatementExecuted);
      if (startFetch)
         statementStartFetch(object, handle);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementExecute(
   TsSqlStatementImpl *object,
   StatementHandle handle,
   const QString &sql,
   const TsSqlRow &params,
   bool startFetch)
{
   DEBUG_RECEIVE("Received execute request from " << object << " for statement " << handle);
   try
   {
      DEBUG_LOG("Preparing " << sql);
      STHANDLE(handle)->Prepare(sql.toStdString());
      EMIT_ASYNC(object, emitStatementPrepared);
      setParams(handle, params);
      DEBUG_LOG("Executing " << sql << " with params");
      STHANDLE(handle)->Execute();
      EMIT_ASYNC(object, emitStatementExecuted);
      if (startFetch)
         statementStartFetch(object, handle);
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementSetParam(
   StatementHandle handle,
   int col,
   const TsSqlVariant &param)
{
   setStatementParam(param, handle, col);
}

void TsSqlDatabaseThread::emitStatementRow(
   TsSqlStatementImpl *receiver, 
   StatementHandle statement)
{
   TsSqlRow row;
   readRow(statement, row);
   TsSqlThreadEmitter emitter(receiver);
   emitter.emitStatementFetched(row);
}

void TsSqlDatabaseThread::readRow(StatementHandle statement, TsSqlRow &row)
{
   using namespace IBPP;
   Statement &st = STHANDLE(statement);
   int columns = st->Columns();
   row.resize(columns);
   for(int i = 1; i <= columns; ++i)
      setFromStatement(row[i-1], statement, i);
}

void TsSqlDatabaseThread::setParams(StatementHandle statement, const TsSqlRow &params)
{
   int col = 1;
   for(TsSqlRow::const_iterator i = params.begin();
       i != params.end();
       ++i)
   {
      statementSetParam(
         statement,
         col,
         *i);
      col++;
   }
}

void TsSqlDatabaseThread::statementStartFetch(
   TsSqlStatementImpl *object,
   StatementHandle handle)
{
   DEBUG_RECEIVE("Received start fetch request from " << object << " for statement " << handle);

   try
   {
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
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementFetchNext(
   TsSqlStatementImpl *object,
   StatementHandle handle)
{
   DEBUG_RECEIVE("Received fetch next request from " << object << " for statement " << handle);

   // Stop fetching when the transaction has been stopped or the
   // database connection is closed.
   try
   {
      object->m_stopFetchingMutex.lock();
      bool stopFetching = object->m_stopFetching;
      if (stopFetching)
         object->m_stopFetching = false;
      object->m_stopFetchingMutex.unlock();

      if (!STHANDLE(handle)->TransactionPtr()->Started() || 
          !STHANDLE(handle)->DatabasePtr()->Connected() ||
         stopFetching)
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
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

void TsSqlDatabaseThread::statementFetchSingleRow(
   StatementHandle handle,
   TsSqlRow *result)
{
   if (STHANDLE(handle)->Fetch())
      readRow(handle, *result);
   else
      result->resize(0);
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
   try
   {
      switch(info)
      {
         case siPlan:
            STHANDLE(handle)->Plan(temp);
            *result = QString::fromStdString(temp);
            break;
         case siColumnCount:
            {
               int iTemp = STHANDLE(handle)->Columns();
               *result = iTemp;
            }
            break;
         case siColumnName:
            *result = QString::fromAscii(STHANDLE(handle)->ColumnName(param.toInt() + 1));
            break;
         case siColumnIndex:
            *result = STHANDLE(handle)->ColumnNum(param.toString().toStdString());
            break;
         case siColumnAlias:
            *result = QString::fromAscii(STHANDLE(handle)->ColumnAlias(param.toInt() -+1));
            break;
         case siColumnTable:
            *result = QString::fromAscii(STHANDLE(handle)->ColumnTable(param.toInt() + 1));
            break;
         case siColumnType:
            *result = static_cast<int>(ibppTypeToTs(STHANDLE(handle)->ColumnType(param.toInt() + 1)));
            break;
         case siColumnSubType:
            *result = STHANDLE(handle)->ColumnSubtype(param.toInt() + 1);
            break;
         case siColumnSize:
            *result = STHANDLE(handle)->ColumnSize(param.toInt() + 1);
            break;
         case siColumnScale:
            *result = STHANDLE(handle)->ColumnScale(param.toInt() + 1);
            break;
         default:
            DEBUG_OUT("Unknown statement info(" << info << ") requested!");
      }
   } catch(std::exception &e)
   {
      EMIT_ERROR(object, e.what());
   }
}

TsSqlDatabaseImpl::TsSqlDatabaseImpl(
   const QString &server,
   const QString &database,
   const QString &user,
   const QString &password,
   const QString &characterSet,
   const QString &role,
   const QString &createParams):
   m_handle(0)
{
   m_thread.moveToThread(&m_thread);
   m_thread.start();
   DEBUG_OUT("New database object " << this);
   connect(this, SIGNAL(runTest()), &m_thread, SLOT(test()), Qt::QueuedConnection);
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
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(destroyHandle(DatabaseHandle)),
      &m_thread,
      SLOT(destroyDatabase(DatabaseHandle)),
      Qt::BlockingQueuedConnection);

   emit createHandle(
         this,
         server,
         database,
         user,
         password,
         characterSet,
         role,
         createParams);
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
      SIGNAL(databaseOpenWaiting(TsSqlDatabaseImpl*, DatabaseHandle)),
      &m_thread,
      SLOT(databaseOpen(TsSqlDatabaseImpl*, DatabaseHandle)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(databaseCloseWaiting(TsSqlDatabaseImpl*, DatabaseHandle)),
      &m_thread,
      SLOT(databaseClose(TsSqlDatabaseImpl*, DatabaseHandle)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(databaseIsOpen(
         TsSqlDatabaseImpl*, 
         DatabaseHandle, 
         bool*)),
      &m_thread,
      SLOT(databaseIsOpen(
         TsSqlDatabaseImpl*, 
         DatabaseHandle, 
         bool*)),
      Qt::BlockingQueuedConnection);
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
      Qt::BlockingQueuedConnection);
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
      Qt::BlockingQueuedConnection);
}

void TsSqlDatabaseImpl::test()
{
   emit runTest();
}

void TsSqlDatabaseImpl::open()
{
   emit databaseOpen(this, m_handle);
}

void TsSqlDatabaseImpl::close()
{
   emit databaseClose(this, m_handle);
}

void TsSqlDatabaseImpl::openWaiting()
{
   emit databaseOpenWaiting(this, m_handle);
}

void TsSqlDatabaseImpl::closeWaiting()
{
   emit databaseCloseWaiting(this, m_handle);
}

bool TsSqlDatabaseImpl::isOpen()
{
   bool result = false;
   emit databaseIsOpen(this, m_handle, &result);
   return result;
}

QString TsSqlDatabaseImpl::server()
{
   QString result;
   emit databaseInfo(this, m_handle, diServer, &result);
   return result;
}

QString TsSqlDatabaseImpl::database()
{
   QString result;
   emit databaseInfo(this, m_handle, diDatabase, &result);
   return result;
}

QString TsSqlDatabaseImpl::user()
{
   QString result;
   emit databaseInfo(this, m_handle, diUser, &result);
   return result;
}

QString TsSqlDatabaseImpl::password()
{
   QString result;
   emit databaseInfo(this, m_handle, diPassword, &result);
   return result;
}

QString TsSqlDatabaseImpl::role()
{
   QString result;
   emit databaseInfo(this, m_handle, diRole, &result);
   return result;
}

QString TsSqlDatabaseImpl::characterSet()
{
   QString result;
   emit databaseInfo(this, m_handle, diCharacterSet, &result);
   return result;
}

QString TsSqlDatabaseImpl::createParams()
{
   QString result;
   emit databaseInfo(this, m_handle, diCreateParams, &result);
   return result;
}

QVector<QString> TsSqlDatabaseImpl::connectedUsers()
{
   QVector<QString> result;
   emit databaseConnectedUsers(this, m_handle, &result);
   return result;
}

TsSqlDatabaseImpl::~TsSqlDatabaseImpl()
{
   emit destroyHandle(m_handle);
   m_thread.quit();
   // Wait for the thread to gracefully finish.
   // If this doesn't happen withing 10 seconds, try to terminate it
   if (!m_thread.wait(10000))
   {
      m_thread.terminate();
      // After it was tried to terminate it, wait another
      // 10 seconds for the thread to finish.
      m_thread.wait(10000);
   }
}

TsSqlTransactionImpl::TsSqlTransactionImpl(
   TsSqlDatabaseImpl &database, 
   TsSqlTransaction::TransactionMode mode):
   m_handle(0)
{
   DEBUG_OUT("Creating new transaction");
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
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(destroyTransaction(TransactionHandle)),
      &database.m_thread,
      SLOT(destroyTransaction(TransactionHandle)),
      Qt::BlockingQueuedConnection);

   emit createTransaction(
      this,
      database.m_handle,
      mode);
   DEBUG_OUT("Transaction-handle " << m_handle << " arrived for " << this);

   // asynchronous connections
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

   // synchronous connections
   connect(
      this,
      SIGNAL(transactionStartWaiting(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionStart(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(transactionCommitWaiting(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionCommit(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(transactionCommitRetainingWaiting(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionCommitRetaining(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(transactionRollBackWaiting(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      &database.m_thread,
      SLOT(transactionRollBack(
         TsSqlTransactionImpl *,
         TransactionHandle)),
      Qt::BlockingQueuedConnection);
}

TsSqlTransactionImpl::~TsSqlTransactionImpl()
{
   emit destroyTransaction(m_handle);
}

void TsSqlTransactionImpl::start()
{
   emit transactionStart(
      this,
      m_handle);
}

void TsSqlTransactionImpl::commit()
{
   emit transactionCommit(
      this,
      m_handle);
}

void TsSqlTransactionImpl::commitRetaining()
{
   emit transactionCommitRetaining(
      this,
      m_handle);
}

void TsSqlTransactionImpl::rollBack()
{
   emit transactionRollBack(
      this,
      m_handle);
}

void TsSqlTransactionImpl::startWaiting()
{
   emit transactionStartWaiting(
      this,
      m_handle);
}

void TsSqlTransactionImpl::commitWaiting()
{
   emit transactionCommitWaiting(
      this,
      m_handle);
}

void TsSqlTransactionImpl::commitRetainingWaiting()
{
   emit transactionCommitRetainingWaiting(
      this,
      m_handle);
}

void TsSqlTransactionImpl::rollBackWaiting()
{
   emit transactionRollBackWaiting(
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
   m_handle(0),
   m_stopFetching(false)
{
   DEBUG_OUT("Creating new statement");
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
      Qt::BlockingQueuedConnection);

   emit createStatement(
      this,
      database.m_handle,
      transaction.m_handle);
   DEBUG_OUT("Statement-handle " << m_handle << " arrived for " << this);

   connectSignals(&database.m_thread);
}


TsSqlStatementImpl::TsSqlStatementImpl(
   TsSqlDatabaseImpl &database, 
   TsSqlTransactionImpl &transaction, 
   const QString &sql)
{
   DEBUG_OUT("Creating new statement");
   connect(
      this,
      SIGNAL(createStatement(
         TsSqlStatementImpl*,
         DatabaseHandle,
         TransactionHandle,
         QString)),
      &database.m_thread,
      SLOT(createStatement(
         TsSqlStatementImpl*,
         DatabaseHandle,
         TransactionHandle,
         QString)),
      Qt::BlockingQueuedConnection);

   emit createStatement(
      this,
      database.m_handle,
      transaction.m_handle,
      sql);
   DEBUG_OUT("Statement-handle " << m_handle << " arrived for " << this);

   connectSignals(&database.m_thread);
}

TsSqlStatementImpl::~TsSqlStatementImpl()
{
   emit destroyStatement(m_handle);
}

void TsSqlStatementImpl::connectSignals(QObject *receiver)
{
   connect(
      this,
      SIGNAL(destroyStatement(StatementHandle)),
      receiver,
      SLOT(destroyStatement(StatementHandle)),
      Qt::BlockingQueuedConnection);

   /* asynchronous connections */
   connect(
      this,
      SIGNAL(statementPrepare(
         TsSqlStatementImpl *,
         StatementHandle,
         QString)),
      receiver,
      SLOT(statementPrepare(
         TsSqlStatementImpl *,
         StatementHandle,
         QString)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         bool)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         bool)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         TsSqlRow,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         TsSqlRow,
         bool)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         TsSqlRow,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         TsSqlRow,
         bool)),
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

   /* synchronous connections */
   connect(
      this,
      SIGNAL(statementPrepareWaiting(
         TsSqlStatementImpl *,
         StatementHandle,
         QString)),
      receiver,
      SLOT(statementPrepare(
         TsSqlStatementImpl *,
         StatementHandle,
         QString)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(statementExecuteWaiting(
         TsSqlStatementImpl *,
         StatementHandle,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         bool)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(statementExecuteWaiting(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         bool)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(statementExecuteWaiting(
         TsSqlStatementImpl *,
         StatementHandle,
         TsSqlRow,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         TsSqlRow,
         bool)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(statementExecuteWaiting(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         TsSqlRow,
         bool)),
      receiver,
      SLOT(statementExecute(
         TsSqlStatementImpl *,
         StatementHandle,
         QString,
         TsSqlRow,
         bool)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(statementSetParam(
         StatementHandle,
         int,
         const TsSqlVariant &)),
      receiver,
      SLOT(statementSetParam(
         StatementHandle,
         int,
         const TsSqlVariant &)),
      Qt::BlockingQueuedConnection);
   connect(
      this,
      SIGNAL(statementFetchNext(
         TsSqlStatementImpl *,
         StatementHandle)),
      receiver,
      SLOT(statementFetchNext(
         TsSqlStatementImpl *,
         StatementHandle)),
      Qt::QueuedConnection);
   connect(
      this,
      SIGNAL(statementFetchSingleRow(
         StatementHandle,
         TsSqlRow *)),
      receiver,
      SLOT(statementFetchSingleRow(
         StatementHandle,
         TsSqlRow *)),
      Qt::BlockingQueuedConnection);

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
      Qt::BlockingQueuedConnection);
}

void TsSqlStatementImpl::fetchDataset(const TsSqlRow &row)
{
   emit fetched(row);
   emit statementFetchNext(
      this,
      m_handle);
}

void TsSqlStatementImpl::prepare(const QString &sql)
{
   emit statementPrepare(this, m_handle, sql);
}

void TsSqlStatementImpl::execute(bool startFetch)
{
   emit statementExecute(this, m_handle, startFetch);
}

void TsSqlStatementImpl::execute(const QString &sql, bool startFetch)
{
   emit statementExecute(this, m_handle, sql, startFetch);
}

void TsSqlStatementImpl::execute(const TsSqlRow &params, bool startFetch)
{
   emit statementExecute(this, m_handle, params, startFetch);
}

void TsSqlStatementImpl::execute(
   const QString &sql, 
   const TsSqlRow &params,
   bool startFetch)
{
   emit statementExecute(this, m_handle, sql, params, startFetch);
}

void TsSqlStatementImpl::prepareWaiting(const QString &sql)
{
   emit statementPrepareWaiting(this, m_handle, sql);
}

void TsSqlStatementImpl::executeWaiting()
{
   emit statementExecuteWaiting(this, m_handle, false);
}

void TsSqlStatementImpl::executeWaiting(const QString &sql)
{
   emit statementExecuteWaiting(this, m_handle, sql, false);
}

void TsSqlStatementImpl::executeWaiting(const TsSqlRow &params)
{
   emit statementExecuteWaiting(this, m_handle, params, false);
}

void TsSqlStatementImpl::executeWaiting(
   const QString &sql, 
   const TsSqlRow &params)
{
   emit statementExecuteWaiting(this, m_handle, sql, params, false);
}

void TsSqlStatementImpl::setParam(int column, const TsSqlVariant &param)
{
   emit statementSetParam(
      m_handle,
      column,
      param);
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

bool TsSqlStatementImpl::fetchRow(TsSqlRow &row)
{
   emit statementFetchSingleRow(m_handle, &row);
   return row.size() > 0;
}

void TsSqlStatementImpl::stopFetching()
{
   m_stopFetchingMutex.lock();
   m_stopFetching = true;
   m_stopFetchingMutex.unlock();
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

         qRegisterMetaType<TsSqlVariant>();
         qRegisterMetaType<TsSqlRow>();
         qRegisterMetaType<TsSqlTransaction::TransactionMode>();
      }
   } g_sqlMetaTypeInitializer;
}
