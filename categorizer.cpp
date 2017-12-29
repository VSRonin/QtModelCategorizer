#include "categorizer.h"
#include <QList>
#include <QPersistentModelIndex>
class TreeRow;
class TreeRowData{
    Q_DISABLE_COPY(TreeRowData)
    TreeRowData(TreeRow* par, int parCol, const QList<QPersistentModelIndex>& cols);
    TreeRowData(TreeRow* par = Q_NULLPTR, int parCol = 0);
    TreeRow* parent;
    int parentCol;
    QList<TreeRow*> children;
    QList<QPersistentModelIndex> columns;
    QVariant category;
    ~TreeRowData();
    friend TreeRow;
};

class TreeRow{
    Q_DISABLE_COPY(TreeRow)
public:
    TreeRow(TreeRow* par, int parCol, const QList<QPersistentModelIndex>& cols);
    TreeRow(TreeRow* par, int parCol, const QPersistentModelIndex& col);
    TreeRow(TreeRow* par, int parCol);
    ~TreeRow() { delete m_data; }
    TreeRow* parent() const;
    void setParent(TreeRow* par);
    int parentColumn() const;
    void setParentColumn(int parCol);
    const QVariant& category() const;
    void setCategory(const QVariant& cat);
    const QList<TreeRow*>& children() const;
    QList<TreeRow*>& children();
    const QList<QPersistentModelIndex>& columns() const;
    QList<QPersistentModelIndex>& columns();
private:
    TreeRowData* m_data;
};

TreeRow::TreeRow(TreeRow* par, int parCol, const QList<QPersistentModelIndex>& cols) 
    : m_data(new TreeRowData(par, parCol, cols))
{
    if (par)
        par->children().append(this);
}

TreeRow::TreeRow(TreeRow* par, int parCol, const QPersistentModelIndex& col) 
    :TreeRow(par, parCol, QList<QPersistentModelIndex>() << col)
{}

TreeRow::TreeRow(TreeRow* par, int parCol) 
    : TreeRow(par, parCol, QList<QPersistentModelIndex>())
{}

TreeRow* TreeRow::parent() const
{
    return m_data->parent;
}

void TreeRow::setParent(TreeRow* par)
{
    m_data->parent = par;
}

int TreeRow::parentColumn() const
{
    return m_data->parentCol;
}

void TreeRow::setParentColumn(int parCol)
{
    m_data->parentCol = parCol;
}

const QVariant& TreeRow::category() const
{
    return m_data->category;
}

void TreeRow::setCategory(const QVariant& cat)
{
    m_data->category = cat;
}

const QList<TreeRow*>& TreeRow::children() const
{
    return m_data->children;
}

QList<TreeRow*>& TreeRow::children()
{
    return m_data->children;
}

const QList<QPersistentModelIndex>& TreeRow::columns() const
{
    return m_data->columns;
}

QList<QPersistentModelIndex>& TreeRow::columns()
{
    return m_data->columns;
}

TreeRowData::TreeRowData(TreeRow* par, int parCol, const QList<QPersistentModelIndex>& cols) 
    : parent(par)
    , columns(cols)
    , parentCol(parCol)
{}

TreeRowData::TreeRowData(TreeRow* par, int parCol) 
    :parent(par)
    , parentCol(parCol)
{}

TreeRowData::~TreeRowData()
{
    for (auto i = children.begin(); i != children.end(); ++i)
        delete (*i);
}

class CategorizerPrivate{
    Q_DISABLE_COPY(CategorizerPrivate)
    Q_DECLARE_PUBLIC(Categorizer);
    CategorizerPrivate(Categorizer* q);
    ~CategorizerPrivate();
    QList<QMetaObject::Connection> m_sourceConnections;
    int m_keyColumn;
    int m_keyRole;
    Categorizer* q_ptr;
    QHash<QPersistentModelIndex, TreeRow*> m_mapping;
    QList<TreeRow*> m_treeStructure;
    TreeRow* itemForIndex(const QModelIndex& idx) const;
    QModelIndex indexForItem(TreeRow* const item, int col) const;
    void clearTreeStructure();
    void rebuildMapping();
    void rebuildTreeStructure(const QModelIndex &sourceParent, TreeRow* currParent, int parentCol);
    int indexForKey(const QVariant& key) const;
    void removeFromMapping(TreeRow* item);
    void onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onSourceRowsInserted(const QModelIndex &parent, int first, int last);
    void onSourceColumnsInserted(const QModelIndex &parent, int first, int last);
    void onSourceColumnsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void onSourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onSourceRowsRemoved(const QModelIndex &parent, int first, int last);
    enum {RootDataRole = Qt::UserRole};
};

QModelIndex Categorizer::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasChildren(parent))
        return QModelIndex();
    if (row<0 || row>=rowCount(parent))
        return QModelIndex();
    if (column<0 || column>=columnCount(parent))
        return QModelIndex();
    if (!parent.isValid())
        return createIndex(row, column, Q_NULLPTR);
    Q_ASSERT(parent.model() == this);
    Q_D(const Categorizer);
    return createIndex(row, column, d->itemForIndex(parent));
}

bool Categorizer::insertRows(int row, int count, const QModelIndex &parent) 
{
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool Categorizer::removeRows(int row, int count, const QModelIndex &parent) 
{
    if (!parent.isValid() || !sourceModel() || row<0 || count <= 0)
        return false;
    Q_ASSERT(parent.model() == this);
    if (parent.parent().isValid())
        return sourceModel()->removeRows(row, count, mapToSource(parent));
    Q_D(Categorizer);
    const TreeRow* const parentItem = d->itemForIndex(parent);
    if (row >= parentItem->children().size())
        return false;
    const QList<QPersistentModelIndex>& childCols = parentItem->children().at(row)->columns();
    Q_ASSERT(!childCols.isEmpty());
    return sourceModel()->removeRows(childCols.first().row(), count, QModelIndex());
/*


    TreeRow* const parentItem = d->itemForIndex(parent);
    if (row + count - 1 >= parentItem->children().size())
        return false;
    const QModelIndex sourceParent = mapToSource(parent);
    Q_ASSERT(parentItem);
    bool result = true;
    if (!parentItem->parent() && parentItem->children().size() == count){ //removing everything in the category
        const int catRow = d->m_treeStructure.indexOf(parentItem);
        Q_ASSERT(catRow >= 0);
        beginRemoveRows(QModelIndex(), catRow, catRow);
        const auto childEnd = parentItem->children().cend();
        for (auto i = parentItem->children().cbegin(); i != childEnd && result; ++i) 
            result = sourceModel()->removeRow((*i)->columns().first().row(), sourceParent);
        if (result) {
            d->removeFromMapping(parentItem);
            delete d->m_treeStructure.takeAt(catRow);
        }
        endRemoveRows();
    }
    else{
        beginRemoveRows(parent, row, row + count - 1);
        const auto childEnd = parentItem->children().begin()+row+count;
        const auto childBegin = parentItem->children().begin() + row;
        for (auto i = childBegin; i != childEnd && result; ) {
            result = sourceModel()->removeRow((*i)->columns().first().row(), sourceParent);
            if (Q_LIKELY(result)) {
                d->removeFromMapping(*i);
                i = parentItem->children().erase(i);
                continue;
            }
            ++i;
        }
        endRemoveRows();
    }
    return result;*/
}

bool Categorizer::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) 
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(count)
    Q_UNUSED(sourceRow)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationChild)
    return false;
}

bool Categorizer::insertColumns(int column, int count, const QModelIndex &parent) 
{
    Q_UNUSED(column)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

CategorizerPrivate::~CategorizerPrivate(){
    clearTreeStructure();
}
CategorizerPrivate::CategorizerPrivate(Categorizer* q)
    :q_ptr(q)
    , m_keyColumn(0)
    , m_keyRole(Qt::DisplayRole)
{
    Q_ASSERT(q_ptr);
}

TreeRow* CategorizerPrivate::itemForIndex(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return Q_NULLPTR;
    const TreeRow* const internalParent = static_cast<const TreeRow*>(idx.internalPointer());
    if (internalParent)
        return internalParent->children().value(idx.row(), Q_NULLPTR);
    return m_treeStructure.value(idx.row(), Q_NULLPTR);
}



QModelIndex CategorizerPrivate::indexForItem(TreeRow* const item, int col) const
{
    if (!item || col <0)
        return QModelIndex();
    TreeRow* const parentItem = item->parent();
    int rowIdx = -1;
    if (parentItem)
        rowIdx = parentItem->children().indexOf(item);
    else
        rowIdx = m_treeStructure.indexOf(item);
    if (rowIdx < 0)
        return QModelIndex();
    Q_Q(const Categorizer);
    return q->createIndex(rowIdx, col, parentItem);
}

void CategorizerPrivate::clearTreeStructure()
{
    for (auto i = m_treeStructure.begin(); i != m_treeStructure.end(); ++i)
        delete (*i);
    m_treeStructure.clear();
}

void CategorizerPrivate::rebuildMapping()
{
    Q_Q(Categorizer);
    m_mapping.clear();
    clearTreeStructure();
    q->beginResetModel();
    if (q->sourceModel()) {
        const int rowCnt = q->sourceModel()->rowCount();
        const int colCnt = q->sourceModel()->columnCount();
        for (int i = 0; i < rowCnt; ++i) {
            const QVariant idxData = q->sourceModel()->index(i, m_keyColumn).data(m_keyRole);
            const int mappingIndex = indexForKey(idxData);
            TreeRow* catParent = Q_NULLPTR;
            if (mappingIndex < 0) {
                catParent = new TreeRow(Q_NULLPTR, 0);
                catParent->setCategory(idxData);
                m_treeStructure.append(catParent);
            }
            else {
                catParent = m_treeStructure.at(mappingIndex);
            }
            TreeRow* const currItm = new TreeRow(catParent, 0);
            for (int j = 0; j < colCnt; ++j) {
                const QPersistentModelIndex currIdx = q->sourceModel()->index(i, j);
                m_mapping.insert(currIdx, currItm);
                currItm->columns().append(currIdx);
                if (q->sourceModel()->hasChildren(currIdx))
                    rebuildTreeStructure(currIdx, currItm, j);
            }
            Q_ASSERT(currItm->columns().size() == colCnt);
        }
    }
    q->endResetModel();
}

void CategorizerPrivate::onSourceRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_Q(Categorizer);
    const int colCnt = q->sourceModel()->columnCount(parent);
    if (!parent.isValid()) {
        for (int i = first; i <= last; ++i) {
            const QVariant idxData = q->sourceModel()->index(i, m_keyColumn).data(m_keyRole);
            const int mappingIndex = indexForKey(idxData);
            TreeRow* catParent = Q_NULLPTR;
            if (mappingIndex < 0) {
                q->beginInsertRows(QModelIndex(), m_treeStructure.size(), m_treeStructure.size());
                catParent = new TreeRow(Q_NULLPTR, 0);
                catParent->setCategory(idxData);
                m_treeStructure.append(catParent);
                q->endInsertRows();
            }
            else {
                catParent = m_treeStructure.at(mappingIndex);
            }
            const int catChildSize = catParent->children().size();
            int insertIndex = catChildSize;
            for (int j = 0; j < catChildSize;++j){
                const QList<QPersistentModelIndex>& columnList = catParent->children().at(j)->columns();
                Q_ASSERT(!columnList.isEmpty());
                if (columnList.first().row() >= i) {
                    insertIndex = j;
                    break;
                }
            }
            q->beginInsertRows(indexForItem(catParent, 0), insertIndex, insertIndex);
            TreeRow* const currItm = new TreeRow(catParent, 0);
            if (insertIndex != catChildSize)
                catParent->children().move(catChildSize, insertIndex);
            for (int j = 0; j < colCnt; ++j) {
                const QPersistentModelIndex currIdx = q->sourceModel()->index(i, j);
                m_mapping.insert(currIdx, currItm);
                currItm->columns().append(currIdx);
                if (q->sourceModel()->hasChildren(currIdx))
                    rebuildTreeStructure(currIdx, currItm, j);
            }
            q->endInsertRows();
        }
    }
    else{
        Q_ASSERT(parent.model() == q->sourceModel());
        const QModelIndex proxyParent = q->mapFromSource(parent);
        TreeRow* const itemParent = itemForIndex(proxyParent);
        q->beginInsertRows(proxyParent, first, last);
        for (int i = first; i <= last; ++i) {
            TreeRow* const currItm = new TreeRow(itemParent, 0);
            itemParent->children().move(itemParent->children().size() - 1, i);
            for (int j = 0; j < colCnt; ++j) {
                const QPersistentModelIndex currIdx = q->sourceModel()->index(i, j, parent);
                m_mapping.insert(currIdx, currItm);
                currItm->columns().append(currIdx);
                if (q->sourceModel()->hasChildren(currIdx))
                    rebuildTreeStructure(currIdx, currItm, j);
            }
        }
        q->endInsertRows();
    }
}
void CategorizerPrivate::onSourceColumnsAboutToBeInserted(const QModelIndex &parent, int first, int last){
    Q_Q(Categorizer);
    if(!parent.isValid())
        q->beginInsertColumns(QModelIndex(), first, last);
}
void CategorizerPrivate::onSourceColumnsInserted(const QModelIndex &parent, int first, int last)
{
    Q_Q(Categorizer);
    if(parent.isValid()){
        const QModelIndex proxyParent = q->mapFromSource(parent);
        TreeRow* const parentItem = itemForIndex(proxyParent);
        Q_ASSERT(parentItem);
        const int childSize = parentItem->children().size();
        q->beginInsertColumns(proxyParent, first, last);
        for (int i = 0; i < childSize; ++i) {
            for (int j = first; j <= last;++j){
                parentItem->children().at(i)->columns().insert(j, q->sourceModel()->index(i, j, parent));
            }
        }
        q->endInsertColumns();
    }
    else {
        const int catSize = m_treeStructure.size();
        q->endInsertColumns(); // started in onSourceColumnsAboutToBeInserted
        // insert within categories
        for (int catIter = 0; catIter < catSize; ++catIter) {
            const QModelIndex catParent = q->index(catIter, 0);
            Q_ASSERT(m_treeStructure.at(catIter)->columns().isEmpty());
            q->beginInsertColumns(catParent, first, last);
            const int childSize = m_treeStructure.at(catIter)->children().size();
            for (int childIter = 0; childIter < childSize; ++childIter) {
                QList<QPersistentModelIndex>& currColumns = m_treeStructure.at(catIter)->children().at(childIter)->columns();
                Q_ASSERT(!currColumns.isEmpty());
                const int currRow = currColumns.first().row();
                for (int j = first; j <= last; ++j) {
                    currColumns.insert(j, q->sourceModel()->index(currRow, j, parent));
                }
            }
            q->endInsertColumns();
        }
    }    
}

void CategorizerPrivate::rebuildTreeStructure(const QModelIndex &sourceParent, TreeRow* currParent, int parentCol)
{
    Q_Q(const Categorizer);
    const int rowCnt = q->sourceModel()->rowCount(sourceParent);
    const int colCnt = q->sourceModel()->columnCount(sourceParent);
    for (int i = 0; i< rowCnt;++i){
        TreeRow* const currItm = new TreeRow(currParent, parentCol);
        for (int j = 0; j < colCnt; ++j) {
            const QModelIndex currIdx = q->sourceModel()->index(i, j, sourceParent);
            m_mapping.insert(currIdx, currItm);
            currItm->columns().append(currIdx);
            if (q->sourceModel()->hasChildren(currIdx))
                rebuildTreeStructure(currIdx, currItm, j);
        }
        Q_ASSERT(currItm->columns().size() == colCnt);
    }
}

int CategorizerPrivate::indexForKey(const QVariant& key) const
{
    Q_Q(const Categorizer);
    const int treeSize = m_treeStructure.size();
    for (int i = 0; i < treeSize; ++i) {
        if (q->sameKey(m_treeStructure.at(i)->category(), key))
            return i;
    }
    return -1;
}


void CategorizerPrivate::removeFromMapping(TreeRow* item)
{
    const auto colEnd = item->columns().cend();
    for (auto i = item->columns().cbegin(); i != colEnd; ++i)
        m_mapping.remove(*i);
    const auto childEnd = item->children().end();
    for (auto i = item->children().begin(); i != childEnd; ++i)
        removeFromMapping(*i);
}

void CategorizerPrivate::onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_Q(Categorizer);
    if (!topLeft.isValid() || !bottomRight.isValid() || !q->sourceModel())
        return;
    Q_ASSERT(topLeft.model() == q->sourceModel());
    Q_ASSERT(bottomRight.model() == q->sourceModel());
    const QModelIndex& sourceParent = topLeft.parent();
    Q_ASSERT(sourceParent == bottomRight.parent());
    if (topLeft.parent().isValid())
        return;
    if (!((roles.isEmpty() || roles.contains(m_keyRole)) && topLeft.column() <= m_keyColumn && bottomRight.column() >= m_keyColumn))
        return;
    const int bottomRow = bottomRight.row();
    for (int i = topLeft.row(); i <= bottomRow;++i){
        const QModelIndex proxyIdx = q->mapFromSource(q->sourceModel()->index(i, m_keyColumn, sourceParent));
        Q_ASSERT(proxyIdx.isValid());
        const int catRow = proxyIdx.parent().row();
        Q_ASSERT(catRow >= 0);
        const QVariant& newData = proxyIdx.data();
        if (q->sameKey(m_treeStructure.at(catRow)->category(), newData))
            continue;
        const int destinationRow = indexForKey(newData);
        TreeRow* destinationCat = Q_NULLPTR;
        if(destinationRow<0){
            q->beginInsertRows(QModelIndex(), m_treeStructure.size(), m_treeStructure.size());
            destinationCat = new TreeRow(Q_NULLPTR, 0);
            destinationCat->setCategory(newData);
            m_treeStructure.append(destinationCat);
            q->endInsertRows();
        }
        else {
            destinationCat = m_treeStructure.at(destinationRow);
        }
        const int catChildSize = destinationCat->children().size();
        int insertIndex = catChildSize;
        for (int j = 0; j < catChildSize; ++j) {
            const QList<QPersistentModelIndex>& columnList = destinationCat->children().at(j)->columns();
            Q_ASSERT(!columnList.isEmpty());
            if (columnList.first().row() >= i) {
                insertIndex = j;
                break;
            }
        }
        q->beginMoveRows(proxyIdx.parent(), proxyIdx.row(), proxyIdx.row(), q->index(destinationRow, 0), insertIndex);
        TreeRow* const proxyItem = itemForIndex(proxyIdx);
        TreeRow* const oldParentItem = proxyItem->parent();
        Q_ASSERT(oldParentItem);
        Q_ASSERT(!oldParentItem->parent());
        oldParentItem->children().removeAll(proxyItem);
        proxyItem->setParent(destinationCat);
        destinationCat->children().insert(insertIndex,proxyItem);
        q->endMoveRows();
        if(oldParentItem->children().isEmpty()){
            q->beginRemoveRows(QModelIndex(), catRow, catRow);
            delete m_treeStructure.takeAt(catRow);
            q->endRemoveRows();
        }
    }
}




Categorizer::Categorizer(QObject* parent)
    :QAbstractProxyModel(parent)
    , m_dptr(new CategorizerPrivate(this))
{

}

Categorizer::~Categorizer()
{
    delete m_dptr;
}

void Categorizer::setSourceModel(QAbstractItemModel* newSourceModel) 
{
    Q_D(Categorizer);
    if (sourceModel()) {
        const auto connEnd = d->m_sourceConnections.cend();
        for (auto discIter = d->m_sourceConnections.cbegin(); discIter != connEnd; ++discIter)
            QObject::disconnect(*discIter);
    }
    d->m_sourceConnections.clear();
    QAbstractProxyModel::setSourceModel(newSourceModel);
    d->rebuildMapping();
    if (sourceModel()) {
        d->m_sourceConnections
            << connect(sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, &QAbstractItemModel::modelAboutToBeReset)
            << connect(sourceModel(), &QAbstractItemModel::modelReset, this, std::bind(&CategorizerPrivate::rebuildMapping, d))
            << connect(sourceModel(), &QAbstractItemModel::dataChanged, this, std::bind(&CategorizerPrivate::onSourceDataChanged, d, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            << connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, std::bind(&CategorizerPrivate::onSourceRowsInserted, d, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            << connect(sourceModel(), &QAbstractItemModel::columnsInserted, this, std::bind(&CategorizerPrivate::onSourceColumnsInserted, d, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            << connect(sourceModel(), &QAbstractItemModel::columnsAboutToBeInserted, this, std::bind(&CategorizerPrivate::onSourceColumnsAboutToBeInserted, d, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            << connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, std::bind(&CategorizerPrivate::onSourceRowsRemoved, d, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            << connect(sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this, std::bind(&CategorizerPrivate::onSourceRowsAboutToBeRemoved, d, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            << connect(sourceModel(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
                dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
            })
            << connect(sourceModel(), &QAbstractItemModel::headerDataChanged, this, &QAbstractItemModel::headerDataChanged)
            ;
    }
    Q_ASSERT(std::all_of(d->m_sourceConnections.cbegin(), d->m_sourceConnections.cend(), [](const QMetaObject::Connection &connection)->bool {return connection; }));
}

QModelIndex Categorizer::buddy(const QModelIndex &index) const 
{
    if (!sourceModel())
        return QModelIndex();
    return mapFromSource(sourceModel()->buddy(mapToSource(index)));
}

bool Categorizer::hasChildren(const QModelIndex &parent) const 
{
    if (!sourceModel())
        return false;
    Q_D(const Categorizer);
    if (!parent.isValid())
        return d->m_treeStructure.size()>0;
    Q_ASSERT(parent.model() == this);
    if (!parent.parent().isValid())
        return parent.column()==0;
    return sourceModel()->hasChildren(mapToSource(parent));
}

int Categorizer::rowCount(const QModelIndex &parent) const
{
    if (!sourceModel())
        return 0;
    Q_D(const Categorizer);
    if (!parent.isValid()) {
        const int mapSize = d->m_treeStructure.size();
        return mapSize;
    }
    Q_ASSERT(parent.model() == this);
    const TreeRow* const parentItem = d->itemForIndex(parent);
    if (!parentItem)
        return 0;
    return parentItem->children().size();
}

int Categorizer::columnCount(const QModelIndex &parent) const 
{
    if (!sourceModel())
        return 0;
    if (!parent.isValid() || !parent.parent().isValid())
        return sourceModel()->columnCount();
    return sourceModel()->columnCount(mapToSource(parent));
}

QVariant Categorizer::headerData(int section, Qt::Orientation orientation, int role) const 
{
    if (!sourceModel())
        return QVariant();
    return sourceModel()->headerData(section, orientation, role);
}

bool Categorizer::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (!sourceModel())
        return false;
    return sourceModel()->setHeaderData(section, orientation, value, role);
}

QVariant Categorizer::data(const QModelIndex &proxyIndex, int role) const 
{
    if (!proxyIndex.isValid() || !sourceModel())
        return QVariant();
    if (!proxyIndex.parent().isValid()){
        return dataForRoot(proxyIndex, role);
    }
    return sourceModel()->data(mapToSource(proxyIndex), role);
}

bool Categorizer::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!sourceModel() || !index.isValid() || !index.parent().isValid())
        return false;
    return sourceModel()->setData(mapToSource(index), value, role);
}

bool Categorizer::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if (!sourceModel() || !index.isValid() || !index.parent().isValid())
        return false;
    return sourceModel()->setItemData(mapToSource(index), roles);
}



QModelIndex Categorizer::mapFromSource(const QModelIndex &sourceIndex) const 
{
    if (!sourceIndex.isValid() || !sourceModel())
        return QModelIndex();
    Q_ASSERT(sourceIndex.model() == sourceModel());
    Q_D(const Categorizer);
    TreeRow* const proxyItem = d->m_mapping.value(sourceIndex, Q_NULLPTR);
    return d->indexForItem(proxyItem, sourceIndex.column());
}

QModelIndex Categorizer::mapToSource(const QModelIndex &proxyIndex) const 
{
    if (!proxyIndex.isValid() || !sourceModel() || !proxyIndex.parent().isValid())
        return QModelIndex();
    Q_ASSERT(proxyIndex.model() == this);
    Q_D(const Categorizer);
    const TreeRow* const itemIdx = d->itemForIndex(proxyIndex);
    if (!itemIdx)
        return QModelIndex();
    return itemIdx->columns().value(proxyIndex.column(), QPersistentModelIndex());
}

Qt::ItemFlags Categorizer::flags(const QModelIndex &index) const 
{
    if (!sourceModel() || !index.isValid() || !index.parent().isValid())
        return Qt::ItemIsEnabled;
    return sourceModel()->flags(mapToSource(index));
}

QModelIndex Categorizer::parent(const QModelIndex &index) const
{
    if (!index.isValid() || !sourceModel())
        return QModelIndex();
    Q_ASSERT(index.model() == this);
    Q_D(const Categorizer);
    const TreeRow* const itemIdx = d->itemForIndex(index);
    if(!itemIdx) 
        Q_ASSERT(false);
    return d->indexForItem(itemIdx->parent(), itemIdx->parentColumn());
}

bool Categorizer::removeColumns(int column, int count, const QModelIndex &parent) 
{
    return false;
/*
    if (!parent.isValid() || !sourceModel())
        return false;
    const QModelIndex sourceParent = mapToSource(parent);
    beginRemoveColumns(parent, column, column + count - 1);
    const bool result = sourceModel()->removeColumns(column, count, sourceParent);
    endRemoveColumns();
    return result;*/
}

bool Categorizer::moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count, const QModelIndex &destinationParent, int destinationChild) 
{
    return false;
/*
    if (sourceParent.isValid() || !sourceModel() || destinationParent.isValid())
        return false;
    const QModelIndex sourceSourceParent = mapToSource(sourceParent);
    const QModelIndex sourceDestinationParent = mapToSource(destinationParent);
    beginMoveColumns(sourceParent, sourceColumn, sourceColumn + count - 1, destinationParent, destinationChild);
    const bool result = sourceModel()->moveColumns(sourceSourceParent, sourceColumn, count, sourceDestinationParent, destinationChild);
    endMoveColumns();
    return result;*/
}

int Categorizer::keyColumn() const
{
    Q_D(const Categorizer);
    return d->m_keyColumn;
}

void Categorizer::setKeyColumn(int col) 
{
    Q_D(Categorizer);
    if (d->m_keyColumn == col)
        return;
    d->m_keyColumn = col;
    keyColumnChanged(col);
    d->rebuildMapping();
}

int Categorizer::keyRole() const
{
    Q_D(const Categorizer);
    return d->m_keyRole;
}

void Categorizer::setKeyRole(int role)
{
    Q_D(Categorizer);
    if (d->m_keyRole == role)
        return;
    d->m_keyRole = role;
    keyColumnChanged(role);
    d->rebuildMapping();
}

QVariant Categorizer::dataForRoot(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(!index.parent().isValid());
    Q_ASSERT(index.model() == this);
    if (index.column() == 0 && (role == Qt::DisplayRole || role == CategorizerPrivate::RootDataRole)) {
        Q_D(const Categorizer);
        return d->m_treeStructure.at(index.row())->category();
    }
    return QVariant();
}

bool Categorizer::sameKey(const QVariant& left, const QVariant& right) const
{
    return left == right;
}

