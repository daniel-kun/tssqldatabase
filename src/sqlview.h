#ifndef TS_SQL_TREEVIEW_H_28092008
#define TS_SQL_TREEVIEW_H_28092008
#include <QAbstractTableModel>
#include <QTableView>

#include "database.h"

class TsSqlTableModel: public QAbstractTableModel
{
   Q_OBJECT
   private:
      TsSqlStatement &m_fetchIds, &m_fetchData;
      int m_rowCount, m_colCount;
   private slots:
      void dataPrepared();
      void idFetched(TsSqlRow row);
      void dataFetched(TsSqlRow row);
   public:
      TsSqlTableModel(TsSqlStatement &fetchIds, TsSqlStatement &fetchData);
      virtual int rowCount(   const QModelIndex &parent) const;
      virtual int columnCount(const QModelIndex &parent) const;
      virtual QVariant data(  const QModelIndex &index, int role) const;
};

class TsSqlTableView: public QTableView
{
   public:
      TsSqlTableView(QWidget *parent);
};

#endif
