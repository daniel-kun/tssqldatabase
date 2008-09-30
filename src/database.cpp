#include "database.h"
#include "database_p.h"

#include <QThread>

TsSqlVariant::TsSqlVariant(): 
   m_type(stUnknown),
   m_delete(false)
{
   m_data.asPointer = 0;
}

TsSqlVariant::TsSqlVariant(const TsSqlVariant &copy):
   m_type(copy.m_type)
{
   switch(m_type)
   {
      case stBlob:
         m_data.asPointer =
            new QByteArray(*reinterpret_cast<QByteArray*>(copy.m_data.asPointer));
         m_delete = true;
         break;
      case stDate:
         m_data.asPointer =
            new QDate(*reinterpret_cast<QDate*>(copy.m_data.asPointer));
         m_delete = true;
         break;
      case stTime:
         m_data.asPointer =
            new QTime(*reinterpret_cast<QTime*>(copy.m_data.asPointer));
         m_delete = true;
         break;
      case stTimeStamp:
         m_data.asPointer =
            new QDateTime(*reinterpret_cast<QDateTime*>(copy.m_data.asPointer));
         m_delete = true;
         break;
      case stString:
         m_data.asPointer =
            new QString(*reinterpret_cast<QString*>(copy.m_data.asPointer));
         m_delete = true;
         break;
      default:
         m_data.asDouble = copy.m_data.asDouble; // Copy the biggest union-member
         m_delete = false;
         break;
   }
}

TsSqlVariant::~TsSqlVariant()
{
   setNull();
}

TsSqlType TsSqlVariant::type() const
{
   return m_type;
}

bool TsSqlVariant::isNull() const
{
   return m_type == stUnknown && m_data.asPointer == 0;
}

void TsSqlVariant::setNull()
{
   if (m_delete)
   {
      switch(m_type)
      {
         case stBlob:
            delete reinterpret_cast<QByteArray*>(m_data.asPointer);
            break;
         case stDate:
            delete reinterpret_cast<QDate*>(m_data.asPointer);
            break;
         case stTime:
            delete reinterpret_cast<QTime*>(m_data.asPointer);
            break;
         case stTimeStamp:
            delete reinterpret_cast<QDateTime*>(m_data.asPointer);
            break;
         case stString:
            delete reinterpret_cast<QString*>(m_data.asPointer);
            break;
         default:
            break;
      }
   }
   m_data.asPointer = 0;
   m_type = stUnknown;
   m_delete = false;
}

void TsSqlVariant::setVariant(const QVariant &value)
{
   switch(m_type)
   {
      case stBlob:
         *reinterpret_cast<QByteArray*>(m_data.asPointer) = value.toByteArray();
         break;
      case stDate:
         *reinterpret_cast<QDate*>(m_data.asPointer) = value.toDate();
         break;
      case stTime:
         *reinterpret_cast<QTime*>(m_data.asPointer) = value.toTime();
         break;
      case stTimeStamp:
         *reinterpret_cast<QDateTime*>(m_data.asPointer) = value.toDateTime();
         break;
      case stString:
         *reinterpret_cast<QString*>(m_data.asPointer) = value.toString();
         break;
      case stSmallInt:
         m_data.asInt16 = value.toInt();
         break;
      case stInt:
         m_data.asInt32 = value.toInt();
         break;
      case stLargeInt:
         m_data.asInt64 = value.toLongLong();
         break;
      case stFloat:
         m_data.asFloat = value.toDouble(); // there's no toFloat
         break;
      case stDouble:
         m_data.asDouble = value.toDouble();
         break;
      default:
         // If none or an incompatible type is currently set
         setNull();
         switch(value.type())
         {
            case QVariant::ByteArray:
               newValue(value.toByteArray(), stBlob);
               break;
            case QVariant::Date:
               newValue(value.toDate(), stDate);
               break;
            case QVariant::DateTime:
               newValue(value.toDateTime(), stTimeStamp);
               break;
            case QVariant::Double:
               m_data.asDouble = value.toDouble();
               m_type = stDouble;
               m_delete = false;
               break;
            case QVariant::Int:
               m_data.asInt32 = value.toInt();
               m_type = stInt;
               m_delete = false;
               break;
            case QVariant::LongLong:
               m_data.asInt64 = value.toLongLong();
               m_type = stLargeInt;
               m_delete = false;
               break;
            case QVariant::Time:
               newValue(value.toTime(), stTime);
               break;
            case QVariant::UInt:
               m_data.asInt32 = value.toUInt();
               m_type = stInt;
               m_delete = false;
               break;
            case QVariant::ULongLong:
               m_data.asInt64 = value.toULongLong();
               m_type = stLargeInt;
               m_delete = false;
               break;
            default:
               // Save it as string, if nothing else matched
               newValue(value.toString(), stString);
               break;
         }
   }
}

void TsSqlVariant::set(float value)
{
   setNull();
   m_data.asFloat = value;
   m_type = stFloat;
}

QVariant TsSqlVariant::asVariant() const
{
   switch(m_type)
   {
      case stBlob:
         return QVariant(*reinterpret_cast<QByteArray*>(m_data.asPointer));
      case stDate:
         return QVariant(*reinterpret_cast<QDate*>(m_data.asPointer));
      case stTime:
         return QVariant(*reinterpret_cast<QTime*>(m_data.asPointer));
      case stTimeStamp:
         return QVariant(*reinterpret_cast<QDateTime*>(m_data.asPointer));
      case stString:
         return QVariant(*reinterpret_cast<QString*>(m_data.asPointer));
      case stSmallInt:
         return QVariant(m_data.asInt16);
      case stInt:
         return QVariant(m_data.asInt32);
      case stLargeInt:
         return QVariant(m_data.asInt64);
      case stFloat:
         return QVariant(m_data.asFloat);
      case stDouble:
         return QVariant(m_data.asDouble);
      default:
         return QVariant();
   }
}

QByteArray TsSqlVariant::asData() const
{
   return asVariant().toByteArray();
}

QString TsSqlVariant::asString() const
{
   return asVariant().toString();
}

TsSqlSmallInt TsSqlVariant::asInt16() const
{
   return asVariant().toInt();
}

TsSqlInt TsSqlVariant::asInt32() const
{
   return asVariant().toInt();
}

TsSqlLargeInt TsSqlVariant::asInt64() const
{
   return asVariant().toInt();
}

float TsSqlVariant::asFloat() const
{
   return asVariant().toDouble();
}

double TsSqlVariant::asDouble() const
{
   return asVariant().toDouble();
}

QDateTime TsSqlVariant::asTimeStamp() const
{
   return asVariant().toDateTime();
}

QDate TsSqlVariant::asDate() const
{
   return asVariant().toDate();
}

QTime TsSqlVariant::asTime() const
{
   return asVariant().toTime();
}

TsSqlBuffer::TsSqlBuffer():
   m_impl(new TsSqlBufferImpl())
{
   connectSignals();
}

TsSqlBuffer::TsSqlBuffer(TsSqlStatement &dataStatement):
   m_impl(new TsSqlBufferImpl(dataStatement))
{
   connectSignals();
}

TsSqlBuffer::TsSqlBuffer(
   TsSqlStatement &dataStatement,
   TsSqlStatement &fetchStatement):
   m_impl(new TsSqlBufferImpl(dataStatement, fetchStatement))
{
   connectSignals();
}

void TsSqlBuffer::setStatements(TsSqlStatement *dataStatement, TsSqlStatement *fetchStatement)
{
   m_impl->setStatements(dataStatement, fetchStatement);
}

void TsSqlBuffer::connectSignals()
{
   connect(m_impl, SIGNAL(cleared()),            this, SIGNAL(cleared()));
   connect(m_impl, SIGNAL(rowAppended()),        this, SIGNAL(rowAppended()));
   connect(m_impl, SIGNAL(rowDeleted()),         this, SIGNAL(rowDeleted()));
   connect(m_impl, SIGNAL(columnsChanged()),     this, SIGNAL(columnsChanged()));
   connect(m_impl, SIGNAL(rowFetched(TsSqlRow)), this, SIGNAL(rowFetched(TsSqlRow)));
}

TsSqlBuffer::~TsSqlBuffer()
{
   delete m_impl;
}

void TsSqlBuffer::clear()
{
   m_impl->clear();
}

void TsSqlBuffer::appendRow(const TsSqlRow &row)
{
   m_impl->appendRow(row);
}

void TsSqlBuffer::deleteRow(unsigned index)
{
   m_impl->deleteRow(index);
}

void TsSqlBuffer::getRow(unsigned index, TsSqlRow &row)
{
   m_impl->getRow(index, row);
}

TsSqlRow TsSqlBuffer::getRow(unsigned index)
{
   return m_impl->getRow(index);
}

void TsSqlBuffer::setRow(unsigned index, const TsSqlRow &row)
{
   m_impl->setRow(index, row);
}

unsigned TsSqlBuffer::count() const
{
   return m_impl->count();
}

unsigned TsSqlBuffer::columnCount() const
{
   return m_impl->columnCount();
}

TsSqlStatement *TsSqlBuffer::dataStatement()
{
   return m_impl->dataStatement();
}

TsSqlStatement *TsSqlBuffer::fetchStatement()
{
   return m_impl->fetchStatement();
}

/* The rest of this source-file only includes pimpl-forwards */

TsSqlDatabase::TsSqlDatabase(
   const QString &server,
   const QString &database,
   const QString &user,
   const QString &password,
   const QString &characterSet,
   const QString &role,
   const QString &createParams):
   m_impl(new TsSqlDatabaseImpl(
            server,
            database,
            user,
            password,
            role,
            characterSet,
            createParams))
{
   connect(m_impl, SIGNAL(opened()),       this, SIGNAL(opened()));
   connect(m_impl, SIGNAL(closed()),       this, SIGNAL(closed()));
   connect(m_impl, SIGNAL(error(QString)), this, SIGNAL(error(QString)));
}

TsSqlDatabase::~TsSqlDatabase()
{
   delete m_impl;
}

void TsSqlDatabase::test()
{
   m_impl->test();
}

void TsSqlDatabase::open()
{
   m_impl->open();
}

void TsSqlDatabase::close()
{
   m_impl->close();
}

void TsSqlDatabase::openWaiting()
{
   m_impl->openWaiting();
}

void TsSqlDatabase::closeWaiting()
{
   m_impl->closeWaiting();
}

bool TsSqlDatabase::isOpen()
{
   return m_impl->isOpen();
}

QString TsSqlDatabase::server()
{
   return m_impl->server();
}

QString TsSqlDatabase::database()
{
   return m_impl->database();
}

QString TsSqlDatabase::user()
{
   return m_impl->user();
}

QString TsSqlDatabase::password()
{
   return m_impl->password();
}

QString TsSqlDatabase::role()
{
   return m_impl->role();
}

QString TsSqlDatabase::characterSet()
{
   return m_impl->characterSet();
}

QString TsSqlDatabase::createParams()
{
   return m_impl->createParams();
}

QVector<QString> TsSqlDatabase::connectedUsers()
{
   return m_impl->connectedUsers();
}

TsSqlTransaction::TsSqlTransaction(
   TsSqlDatabase &database, 
   TransactionMode mode):
   m_impl(new TsSqlTransactionImpl(*database.m_impl, mode))
{
   connect(m_impl, SIGNAL(started()),      this, SIGNAL(started()));
   connect(m_impl, SIGNAL(commited()),     this, SIGNAL(commited()));
   connect(m_impl, SIGNAL(rolledBack()),   this, SIGNAL(rolledBack()));
   connect(m_impl, SIGNAL(error(QString)), this, SIGNAL(error(QString)));
}

TsSqlTransaction::~TsSqlTransaction()
{
   delete m_impl;
}

void TsSqlTransaction::start()
{
   m_impl->start();
}

void TsSqlTransaction::commit()
{
   m_impl->commit();
}

void TsSqlTransaction::commitRetaining()
{
   m_impl->commitRetaining();
}

void TsSqlTransaction::rollBack()
{
   m_impl->rollBack();
}

void TsSqlTransaction::startWaiting()
{
   m_impl->startWaiting();
}

void TsSqlTransaction::commitWaiting()
{
   m_impl->commitWaiting();
}

void TsSqlTransaction::commitRetainingWaiting()
{
   m_impl->commitRetainingWaiting();
}

void TsSqlTransaction::rollBackWaiting()
{
   m_impl->rollBackWaiting();
}

bool TsSqlTransaction::isStarted()
{
   return m_impl->isStarted();
}

TsSqlStatement::TsSqlStatement(
   TsSqlDatabase &database, 
   TsSqlTransaction &transaction):
   m_impl(new TsSqlStatementImpl(
            *database.m_impl,
            *transaction.m_impl))
{
   connectSignals();
}

TsSqlStatement::TsSqlStatement(
   TsSqlDatabase &database, 
   TsSqlTransaction &transaction, 
   const QString &sql):
   m_impl(new TsSqlStatementImpl(
            *database.m_impl,
            *transaction.m_impl,
            sql))
{
   connectSignals();
}

TsSqlStatement::~TsSqlStatement()
{
   delete m_impl;
}

void TsSqlStatement::connectSignals()
{
   connect(m_impl, SIGNAL(prepared()),        this, SIGNAL(prepared()));
   connect(m_impl, SIGNAL(executed()),        this, SIGNAL(executed()));
   connect(m_impl, SIGNAL(fetchStarted()),    this, SIGNAL(fetchStarted()));
   connect(m_impl, SIGNAL(fetched(TsSqlRow)), this, SIGNAL(fetched(TsSqlRow)));
   connect(m_impl, SIGNAL(fetchFinished()),   this, SIGNAL(fetchFinished()));
   connect(m_impl, SIGNAL(error(QString)),    this, SIGNAL(error(QString)));
}

void TsSqlStatement::prepare(const QString &sql)
{
   m_impl->prepare(sql);
}

void TsSqlStatement::execute()
{
   m_impl->execute(false);
}

void TsSqlStatement::executeAndFetch()
{
   m_impl->execute(true);
}

void TsSqlStatement::execute(const QString &sql, bool startFetch)
{
   m_impl->execute(sql, startFetch);
}

void TsSqlStatement::execute(const TsSqlRow &params, bool startFetch)
{
   m_impl->execute(params, startFetch);
}

void TsSqlStatement::execute(
   const QString &sql, 
   const TsSqlRow &params,
   bool startFetch)
{
   m_impl->execute(sql, params, startFetch);
}

void TsSqlStatement::prepareWaiting(const QString &sql)
{
   m_impl->prepareWaiting(sql);
}

void TsSqlStatement::executeWaiting()
{
   m_impl->executeWaiting();
}

void TsSqlStatement::executeWaiting(const QString &sql)
{
   m_impl->executeWaiting(sql);
}

void TsSqlStatement::executeWaiting(const TsSqlRow &params)
{
   m_impl->executeWaiting(params);
}

void TsSqlStatement::executeWaiting(
   const QString &sql, 
   const TsSqlRow &params)
{
   m_impl->executeWaiting(sql, params);
}

void TsSqlStatement::setParam(int column, const TsSqlVariant &param)
{
   m_impl->setParam(column, param);
}

QString TsSqlStatement::sql()
{
   return m_impl->sql();
}

QString TsSqlStatement::plan()
{
   return m_impl->plan();
}

int TsSqlStatement::affectedRows()
{
   return m_impl->affectedRows();
}

void TsSqlStatement::fetch()
{
   m_impl->fetch();
}

bool TsSqlStatement::fetchRow(TsSqlRow &row)
{
   return m_impl->fetchRow(row);
}

void TsSqlStatement::stopFetching()
{
   return m_impl->stopFetching();
}

int TsSqlStatement::columnCount()
{
   return m_impl->columnCount();
}

QString TsSqlStatement::columnName(int columnIndex)
{
   return m_impl->columnName(columnIndex);
}

int TsSqlStatement::columnIndex(const QString &columnName)
{
   return m_impl->columnIndex(columnName);
}

QString TsSqlStatement::columnAlias(int columnIndex)
{
   return m_impl->columnAlias(columnIndex);
}

QString TsSqlStatement::columnTable(int columnIndex)
{
   return m_impl->columnTable(columnIndex);
}

TsSqlType TsSqlStatement::columnType(int columnIndex)
{
   return m_impl->columnType(columnIndex);
}

int TsSqlStatement::columnSubType(int columnIndex)
{
   return m_impl->columnSubType(columnIndex);
}

int TsSqlStatement::columnSize(int columnIndex)
{
   return m_impl->columnSize(columnIndex);
}

int TsSqlStatement::columnScale(int columnIndex)
{
   return m_impl->columnScale(columnIndex);
}

