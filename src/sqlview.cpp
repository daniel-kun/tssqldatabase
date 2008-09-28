#include "sqlview.h"

TsSqlTableModel::TsSqlTableModel(TsSqlStatement &fetchIds, TsSqlStatement &fetchData):
   m_fetchIds(fetchIds),
   m_fetchData(fetchData),
   m_rowCount(0),
   m_colCount(0)
{
   connect(&m_fetchData, SIGNAL(prepared()),        this, SLOT(dataPrepared()));
   connect(&m_fetchData, SIGNAL(fetched(TsSqlRow)), this, SLOT(dataFetched(TsSqlRow)));

   connect(&m_fetchIds,  SIGNAL(fetched(TsSqlRow)), this, SLOT(idFetched(TsSqlRow)));
}

void TsSqlTableModel::dataPrepared()
{
   int colCount = m_fetchData.columnCount();
   if (colCount > m_colCount)
   {
      beginInsertColumns(QModelIndex(), m_colCount, m_colCount + colCount);
      endInsertColumns();
   }
   else if (colCount < m_colCount)
   {
      beginRemoveColumns(QModelIndex(), colCount, m_colCount);
      endRemoveColumns();
   }
   m_colCount = colCount;
}

void TsSqlTableModel::idFetched(TsSqlRow)
{
   int rows = m_rowCount;
   beginInsertRows(QModelIndex(), rows + 1, rows + 1);
   endInsertRows();
}

void TsSqlTableModel::dataFetched(TsSqlRow)
{
}

int TsSqlTableModel::rowCount(const QModelIndex &parent) const
{
   if (parent != QModelIndex())
      return 0;
   return m_rowCount;
}

int TsSqlTableModel::columnCount(const QModelIndex &parent) const
{
   if (parent != QModelIndex())
      return 0;
   return m_colCount;
}

QVariant TsSqlTableModel::data(const QModelIndex &index, int role) const
{
   if (index != QModelIndex())
      return QVariant();
   if (role == Qt::DisplayRole)
   {

   }
   return QVariant();
}

TsSqlTableView::TsSqlTableView(QWidget *parent): QTableView(parent)
{
}
