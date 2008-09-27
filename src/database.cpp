#include "database.h"
#include "database_p.h"
#include <QThread>

/* This source-file only includes pimpl-forwards */

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
   connect(m_impl, SIGNAL(opened()),       this, SLOT(emitOpened()));
   connect(m_impl, SIGNAL(closed()),       this, SLOT(emitClosed()));
   connect(m_impl, SIGNAL(error(QString)), this, SLOT(emitError(QString)));
}

TsSqlDatabase::~TsSqlDatabase()
{
   delete m_impl;
}

void TsSqlDatabase::emitOpened()
{
   emit opened();
}

void TsSqlDatabase::emitClosed()
{
   emit closed();
}

void TsSqlDatabase::emitError(const QString &errorString)
{
   emit error(errorString);
}

void TsSqlDatabase::open()
{
   m_impl->open();
}

void TsSqlDatabase::close()
{
   m_impl->close();
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
   connect(m_impl, SIGNAL(started()),    this, SLOT(emitStarted()));
   connect(m_impl, SIGNAL(commited()),   this, SLOT(emitCommited()));
   connect(m_impl, SIGNAL(rolledBack()), this, SLOT(emitRolledBack()));
}

TsSqlTransaction::~TsSqlTransaction()
{
   delete m_impl;
}

void TsSqlTransaction::emitStarted()
{
   emit started();
}

void TsSqlTransaction::emitCommited()
{
   emit commited();
}

void TsSqlTransaction::emitRolledBack()
{
   emit rolledBack();
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

void TsSqlStatement::emitExecuted()
{
   emit executed();
}

void TsSqlStatement::emitFetchStarted()
{
   emit fetchStarted();
}

void TsSqlStatement::emitFetched(TsSqlRow row)
{
   emit fetched(row);
}

void TsSqlStatement::emitFetchFinished()
{
   emit fetchFinished();
}

void TsSqlStatement::connectSignals()
{
   connect(m_impl, SIGNAL(executed()),        this, SLOT(emitExecuted()));
   connect(m_impl, SIGNAL(fetchStarted()),    this, SLOT(emitFetchStarted()));
   connect(m_impl, SIGNAL(fetched(TsSqlRow)), this, SLOT(emitFetched(TsSqlRow)));
   connect(m_impl, SIGNAL(fetchFinished()),   this, SLOT(emitFetchFinished()));
}

void TsSqlStatement::prepare(const QString &sql)
{
   m_impl->prepare(sql);
}

void TsSqlStatement::execute(const QString &sql)
{
   m_impl->execute(sql);
}

void TsSqlStatement::execute()
{
   m_impl->execute();
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

