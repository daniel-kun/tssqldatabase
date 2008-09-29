#include "sqlview.h"

TsSqlTableModel::TsSqlTableModel(TsSqlBuffer &buffer):
   m_buffer(buffer),
   m_rowCount(0),
   m_colCount(0)
{
   connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(updateRowCount()));
   m_updateTimer.start(500);
//   connect(&buffer, SIGNAL(cleared()),        this, SLOT(updateRowCount()));
//   connect(&buffer, SIGNAL(rowAppended()),    this, SLOT(updateRowCount()));
//   connect(&buffer, SIGNAL(rowDeleted()),     this, SLOT(updateRowCount()));
   connect(&buffer, SIGNAL(columnsChanged()), this, SLOT(updateColumnCount()));
}

void TsSqlTableModel::updateColumnCount()
{
   unsigned colCount = m_buffer.columnCount();
   if (colCount > m_colCount)
   {
      beginInsertColumns(QModelIndex(), m_colCount, m_colCount + colCount);
      m_colCount = colCount;
      endInsertColumns();
   }
   else if (colCount < m_colCount)
   {
      beginRemoveColumns(QModelIndex(), colCount, m_colCount);
      m_colCount = colCount;
      endRemoveColumns();
   }
}

void TsSqlTableModel::updateRowCount()
{
   int rowCount = m_buffer.count();
   if (rowCount > m_rowCount)
   {
      beginInsertRows(QModelIndex(), m_rowCount, m_rowCount + rowCount);
      m_rowCount = rowCount;
      endInsertRows();
   }
   else if (rowCount < m_rowCount)
   {
      beginRemoveRows(QModelIndex(), rowCount, m_rowCount);
      m_rowCount = rowCount;
      endRemoveRows();
   }
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
   if (role == Qt::DisplayRole)
      return m_buffer.getRow(index.row())[index.column()].asVariant();
   return QVariant();
}

TsSqlTableView::TsSqlTableView(QWidget *parent): QTableView(parent)
{
}
