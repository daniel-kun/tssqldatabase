#ifndef TS_DATABASE_H_27_08_2008
#define TS_DATABASE_H_27_08_2008

#include <QDate>
#include <QTime>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QDateTime>

enum TsSqlType
{
   stUnknown,
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

typedef short     TsSqlSmallInt;
typedef int       TsSqlInt;
typedef long long TsSqlLargeInt;

class TsSqlVariant
{
   private:
      union Data
      {
         void         *asPointer;
         TsSqlSmallInt asInt16;
         TsSqlInt      asInt32;
         TsSqlLargeInt asInt64;
         float         asFloat;
         double        asDouble;
      } m_data;
      TsSqlType m_type;
      bool      m_delete;
      template<typename T>
      void newValue(const T &value, TsSqlType type)
      {
         m_data.asPointer = new T(value);
         m_type = type;
         m_delete = true;
      }
      friend void setFromStatement(TsSqlVariant &variant, void *statement, int column);
      friend void setStatementParam(const TsSqlVariant &variant, void *statement, int column);
   public:
      TsSqlVariant();
      template<typename T>
      TsSqlVariant(const T &value):
         m_type(stUnknown),
         m_delete(false)
      {
         m_data.asPointer = 0;
         setVariant(QVariant(value));
      }
      ~TsSqlVariant();
      TsSqlType type() const;

      bool isNull() const;
      void setNull();
      void setVariant  (const QVariant &value);
      template<typename T>
         void set(const T &value)
      {
         setVariant(QVariant(value));
      }

      QVariant      asVariant()   const;
      QByteArray    asData()      const;
      QString       asString()    const;
      TsSqlSmallInt asInt16()     const;
      TsSqlInt      asInt32()     const;
      TsSqlLargeInt asInt64()     const;
      float         asFloat()     const;
      double        asDouble()    const;
      QDateTime     asTimeStamp() const;
      QDate         asDate()      const;
      QTime         asTime()      const;
      template<typename T>
      TsSqlVariant &operator=(const T &value)
      {
         setVariant(QVariant(value));
         return *this;
      }
};
Q_DECLARE_METATYPE(TsSqlVariant);

typedef QVector<TsSqlVariant> TsSqlRow;
Q_DECLARE_METATYPE(TsSqlRow);

class TsSqlDatabase: public QObject
{
   Q_OBJECT
   private:
      class TsSqlDatabaseImpl *m_impl;
      friend class TsSqlTransaction;
      friend class TsSqlStatement;
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
      void test();   // some test-functions for development, only
      void open();   // async
      void close();  // async
      void openWaiting();  // sync
      void closeWaiting(); // sync
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
      void opened();
      void closed();
      void error(const QString &errorMessage);
};

class TsSqlTransaction: public QObject
{
   Q_OBJECT
   private:
      class TsSqlTransactionImpl *m_impl;
      friend class TsSqlStatement;
   public:
      enum TransactionMode
      {
         tmRead,
         tmWrite
      };
      TsSqlTransaction(TsSqlDatabase &database, TransactionMode mode = tmWrite);
      ~TsSqlTransaction();
      void start();            // async
      void commit();           // async
      void commitRetaining();  // async
      void rollBack();         // async

      void startWaiting();           // sync
      void commitWaiting();          // sync
      void commitRetainingWaiting(); // sync
      void rollBackWaiting();        // sync
      bool isStarted();
   signals:
      void started();
      void commited();
      void rolledBack();
      void error(const QString &errorMessage);
};
Q_DECLARE_METATYPE(TsSqlTransaction::TransactionMode);

class TsSqlStatement: public QObject
{
   Q_OBJECT
   private:
      class TsSqlStatementImpl *m_impl;
      void connectSignals();
   public:
      TsSqlStatement(TsSqlDatabase &database, TsSqlTransaction &transaction);
      TsSqlStatement(TsSqlDatabase &database, TsSqlTransaction &transaction, const QString &sql);
      ~TsSqlStatement();
      void prepare(const QString &sql); // async
      void execute();                   // async
      void executeAndFetch();           // async
      void execute(const QString &sql, bool startFetch = false); // async
      void execute(const TsSqlRow &params, bool startFetch = false); // async
      void execute(
         const QString &sql, 
         const TsSqlRow &params, 
         bool startFetch = false); // async

      void prepareWaiting(const QString &sql); // sync
      void executeWaiting();                   // sync
      void executeWaiting(const QString &sql); // sync
      void executeWaiting(const TsSqlRow &params); // sync
      void executeWaiting(const QString &sql, const TsSqlRow &params); // sync

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
      void prepared();
      void executed();
      void fetchStarted();
      void fetched(TsSqlRow row);
      void fetchFinished();
      void error(const QString &errorMessage);
};

#endif
