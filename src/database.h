#ifndef DATABASE_H_27_08_2008
#define DATABASE_H_27_08_2008

#include <stdexcept>

#include <QDate>
#include <QTime>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QDateTime>

class TsSqlException: public std::runtime_error
{
   public:
      TsSqlException(const std::string &what): runtime_error(what)
   { }
};

enum TsSqlType
{
   stBlob,
   stDate,
   stTime,
   stTimeStamp,
   stString,
   stSmallInt,
   stInt,
   stLargeInt,
   stFloat,
   stDouble
};

typedef QVector<QVariant> TsSqlRow;
Q_DECLARE_METATYPE(TsSqlRow);

class TsSqlDatabase: public QObject
{
   Q_OBJECT
   private:
      class TsSqlDatabaseImpl *m_impl;
      friend class TsSqlTransaction;
      friend class TsSqlStatement;
   private slots:
      void emitOpened();
      void emitClosed();
      void emitError(const QString &errorString);
   public:
      TsSqlDatabase(
         const QString &server,
         const QString &database,
         const QString &user,
         const QString &password,
         const QString &characterSet = QString(),
         const QString &role         = QString(),
         const QString &createParams = QString());
      ~TsSqlDatabase();
      virtual void open();   // async
      virtual void close();  // async
      virtual bool isOpen();
      QString server();
      QString database();
      QString user();
      QString password();
      QString role();
      QString characterSet();
      QString createParams();
      QVector<QString> connectedUsers();
   signals:
      void opened();
      void closed();
      void error(const QString &errorString);
};

class TsSqlTransaction: public QObject
{
   Q_OBJECT
   private:
      class TsSqlTransactionImpl *m_impl;
      friend class TsSqlStatement;
   private slots:
      void emitStarted();
      void emitCommited();
      void emitRolledBack();
   public:
      enum TransactionMode
      {
         tmRead,
         tmWrite
      };
      TsSqlTransaction(TsSqlDatabase &database, TransactionMode mode = tmWrite);
      ~TsSqlTransaction();
      virtual void start();               // async
      virtual void commit();              // async
      virtual void commitRetaining();
      virtual void rollBack();            // async
      virtual bool isStarted();
   signals:
      void started();
      void commited();
      void rolledBack();
};
Q_DECLARE_METATYPE(TsSqlTransaction::TransactionMode);

class TsSqlStatement: public QObject
{
   Q_OBJECT
   private:
      class TsSqlStatementImpl *m_impl;
      void connectSignals();
   private slots:
      void emitExecuted();
      void emitFetchStarted();
      void emitFetched(TsSqlRow row);
      void emitFetchFinished();
   public:
      TsSqlStatement(TsSqlDatabase &database, TsSqlTransaction &transaction);
      TsSqlStatement(TsSqlDatabase &database, TsSqlTransaction &transaction, const QString &sql);
      ~TsSqlStatement();
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
      void executed();
      void fetchStarted();
      void fetched(TsSqlRow row);
      void fetchFinished();
};

#endif
