#ifndef TS_SQL_TREEVIEW_H_28092008
#define TS_SQL_TREEVIEW_H_28092008
#include <QAbstractTableModel>
#include <QTableView>
#include <QTimer>

#include "database.h"

class TsSqlTableModel: public QAbstractTableModel
{
   Q_OBJECT
   private:
      TsSqlBuffer &m_buffer;
      QTimer       m_updateTimer;
      int m_rowCount, m_colCount;
   public slots:
      void updateRowCount();
      void updateColumnCount();
   public:
      TsSqlTableModel(TsSqlBuffer &buffer);
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
