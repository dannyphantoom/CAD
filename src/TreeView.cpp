#include "TreeView.h"
#include <QtCore/QMimeData>

namespace HybridCAD {

TreeView::TreeView(QWidget *parent)
    : QWidget(parent)
    , m_updating(false)
    , m_contextMenuItem(nullptr)
{
    setupUI();
    createContextMenu();
}

TreeView::~TreeView() {
}

void TreeView::addObject(CADObjectPtr object) {
    if (!object) return;
    
    QTreeWidgetItem* item = new QTreeWidgetItem();
    updateItemText(item, object);
    updateItemIcon(item, object);
    updateItemVisibility(item, object);
    
    m_treeWidget->addTopLevelItem(item);
    
    // Store mappings
    m_objectToItem[object] = item;
    m_itemToObject[item] = object;
}

void TreeView::removeObject(CADObjectPtr object) {
    if (!object) return;
    
    auto it = m_objectToItem.find(object);
    if (it != m_objectToItem.end()) {
        QTreeWidgetItem* item = it->second;
        
        // Remove from tree
        QTreeWidgetItem* parent = item->parent();
        if (parent) {
            parent->removeChild(item);
        } else {
            int index = m_treeWidget->indexOfTopLevelItem(item);
            if (index >= 0) {
                m_treeWidget->takeTopLevelItem(index);
            }
        }
        
        // Remove from mappings
        m_objectToItem.erase(it);
        m_itemToObject.erase(item);
        
        delete item;
    }
}

void TreeView::updateObject(CADObjectPtr object) {
    if (!object) return;
    
    auto it = m_objectToItem.find(object);
    if (it != m_objectToItem.end()) {
        QTreeWidgetItem* item = it->second;
        updateItemText(item, object);
        updateItemIcon(item, object);
        updateItemVisibility(item, object);
    }
}

void TreeView::clearObjects() {
    m_treeWidget->clear();
    m_objectToItem.clear();
    m_itemToObject.clear();
}

void TreeView::selectObject(CADObjectPtr object) {
    if (!object) return;
    
    auto it = m_objectToItem.find(object);
    if (it != m_objectToItem.end()) {
        QTreeWidgetItem* item = it->second;
        m_treeWidget->setCurrentItem(item);
    }
}

void TreeView::deselectAll() {
    m_treeWidget->clearSelection();
}

void TreeView::setObjects(const CADObjectList& objects) {
    clearObjects();
    
    for (const auto& object : objects) {
        addObject(object);
    }
}

QTreeWidgetItem* TreeView::findItem(CADObjectPtr object) {
    auto it = m_objectToItem.find(object);
    return (it != m_objectToItem.end()) ? it->second : nullptr;
}

void TreeView::onItemSelectionChanged() {
    if (m_updating) return;
    
    QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
    
    // Clear previous selection
    for (auto& pair : m_itemToObject) {
        if (pair.second) {
            pair.second->setSelected(false);
        }
    }
    
    // Set new selection
    for (QTreeWidgetItem* item : selectedItems) {
        auto it = m_itemToObject.find(item);
        if (it != m_itemToObject.end() && it->second) {
            it->second->setSelected(true);
            emit objectSelected(it->second);
        }
    }
}

void TreeView::onItemChanged(QTreeWidgetItem* item, int column) {
    if (m_updating || column != NAME_COLUMN) return;
    
    auto it = m_itemToObject.find(item);
    if (it != m_itemToObject.end() && it->second) {
        CADObjectPtr object = it->second;
        QString newName = item->text(NAME_COLUMN);
        
        if (newName != QString::fromStdString(object->getName())) {
            object->setName(newName.toStdString());
            emit objectRenamed(object, newName);
        }
    }
}

void TreeView::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (column == VISIBILITY_COLUMN) {
        onVisibilityToggled();
    } else {
        // Edit item name
        m_treeWidget->editItem(item, NAME_COLUMN);
    }
}

void TreeView::onCustomContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    m_contextMenuItem = item;
    
    if (item) {
        // Enable/disable context menu actions based on selection
        auto it = m_itemToObject.find(item);
        bool hasObject = (it != m_itemToObject.end() && it->second);
        
        m_renameAction->setEnabled(hasObject);
        m_deleteAction->setEnabled(hasObject);
        m_duplicateAction->setEnabled(hasObject);
        m_visibilityAction->setEnabled(hasObject);
        
        if (hasObject) {
            bool visible = it->second->isVisible();
            m_visibilityAction->setText(visible ? "Hide" : "Show");
        }
        
        // Show context menu
        m_contextMenu->exec(m_treeWidget->mapToGlobal(pos));
    }
}

void TreeView::onVisibilityToggled() {
    QTreeWidgetItem* item = m_contextMenuItem ? m_contextMenuItem : m_treeWidget->currentItem();
    if (!item) return;
    
    auto it = m_itemToObject.find(item);
    if (it != m_itemToObject.end() && it->second) {
        CADObjectPtr object = it->second;
        bool newVisibility = !object->isVisible();
        object->setVisible(newVisibility);
        
        updateItemVisibility(item, object);
        emit objectVisibilityChanged(object, newVisibility);
    }
}

void TreeView::onRenameRequested() {
    QTreeWidgetItem* item = m_contextMenuItem ? m_contextMenuItem : m_treeWidget->currentItem();
    if (item) {
        m_treeWidget->editItem(item, NAME_COLUMN);
    }
}

void TreeView::onDeleteRequested() {
    QTreeWidgetItem* item = m_contextMenuItem ? m_contextMenuItem : m_treeWidget->currentItem();
    if (!item) return;
    
    auto it = m_itemToObject.find(item);
    if (it != m_itemToObject.end() && it->second) {
        emit deleteRequested(it->second);
    }
}

void TreeView::onDuplicateRequested() {
    QTreeWidgetItem* item = m_contextMenuItem ? m_contextMenuItem : m_treeWidget->currentItem();
    if (!item) return;
    
    auto it = m_itemToObject.find(item);
    if (it != m_itemToObject.end() && it->second) {
        emit duplicateRequested(it->second);
    }
}

void TreeView::onGroupRequested() {
    QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
    std::vector<CADObjectPtr> objects;
    
    for (QTreeWidgetItem* item : selectedItems) {
        auto it = m_itemToObject.find(item);
        if (it != m_itemToObject.end() && it->second) {
            objects.push_back(it->second);
        }
    }
    
    if (objects.size() > 1) {
        emit groupRequested(objects);
    }
}

void TreeView::onUngroupRequested() {
    QTreeWidgetItem* item = m_contextMenuItem ? m_contextMenuItem : m_treeWidget->currentItem();
    if (!item) return;
    
    auto it = m_itemToObject.find(item);
    if (it != m_itemToObject.end() && it->second) {
        // Check if this is a group/assembly
        if (it->second->getType() == ObjectType::ASSEMBLY) {
            emit ungroupRequested(it->second);
        }
    }
}

void TreeView::onExpandAll() {
    m_treeWidget->expandAll();
}

void TreeView::onCollapseAll() {
    m_treeWidget->collapseAll();
}

void TreeView::onRefresh() {
    // Refresh the tree view
    m_updating = true;
    
    for (auto& pair : m_itemToObject) {
        if (pair.second) {
            updateObject(pair.second);
        }
    }
    
    m_updating = false;
}

void TreeView::setupUI() {
    m_layout = new QVBoxLayout(this);
    
    // Create tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "Name" << "Type" << "Visible");
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_treeWidget->setDefaultDropAction(Qt::MoveAction);
    
    // Create button layout
    m_buttonLayout = new QHBoxLayout();
    
    m_expandAllButton = new QPushButton("Expand All", this);
    m_collapseAllButton = new QPushButton("Collapse All", this);
    m_refreshButton = new QPushButton("Refresh", this);
    
    m_buttonLayout->addWidget(m_expandAllButton);
    m_buttonLayout->addWidget(m_collapseAllButton);
    m_buttonLayout->addWidget(m_refreshButton);
    m_buttonLayout->addStretch();
    
    // Add to main layout
    m_layout->addWidget(m_treeWidget);
    m_layout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &TreeView::onItemSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::itemChanged,
            this, &TreeView::onItemChanged);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &TreeView::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &TreeView::onCustomContextMenuRequested);
    
    connect(m_expandAllButton, &QPushButton::clicked,
            this, &TreeView::onExpandAll);
    connect(m_collapseAllButton, &QPushButton::clicked,
            this, &TreeView::onCollapseAll);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &TreeView::onRefresh);
}

void TreeView::createContextMenu() {
    m_contextMenu = new QMenu(this);
    
    m_renameAction = new QAction("Rename", this);
    m_deleteAction = new QAction("Delete", this);
    m_duplicateAction = new QAction("Duplicate", this);
    m_groupAction = new QAction("Group", this);
    m_ungroupAction = new QAction("Ungroup", this);
    m_visibilityAction = new QAction("Toggle Visibility", this);
    
    m_contextMenu->addAction(m_renameAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_duplicateAction);
    m_contextMenu->addAction(m_deleteAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_groupAction);
    m_contextMenu->addAction(m_ungroupAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_visibilityAction);
    
    // Connect actions
    connect(m_renameAction, &QAction::triggered, this, &TreeView::onRenameRequested);
    connect(m_deleteAction, &QAction::triggered, this, &TreeView::onDeleteRequested);
    connect(m_duplicateAction, &QAction::triggered, this, &TreeView::onDuplicateRequested);
    connect(m_groupAction, &QAction::triggered, this, &TreeView::onGroupRequested);
    connect(m_ungroupAction, &QAction::triggered, this, &TreeView::onUngroupRequested);
    connect(m_visibilityAction, &QAction::triggered, this, &TreeView::onVisibilityToggled);
}

void TreeView::updateItemIcon(QTreeWidgetItem* item, CADObjectPtr object) {
    if (!item || !object) return;
    
    QIcon icon = getObjectTypeIcon(object->getType());
    item->setIcon(NAME_COLUMN, icon);
}

void TreeView::updateItemText(QTreeWidgetItem* item, CADObjectPtr object) {
    if (!item || !object) return;
    
    item->setText(NAME_COLUMN, QString::fromStdString(object->getName()));
    item->setText(TYPE_COLUMN, getObjectTypeName(object->getType()));
}

void TreeView::updateItemVisibility(QTreeWidgetItem* item, CADObjectPtr object) {
    if (!item || !object) return;
    
    item->setText(VISIBILITY_COLUMN, object->isVisible() ? "Yes" : "No");
    
    // Make item appear grayed out if not visible
    QColor color = object->isVisible() ? Qt::black : Qt::gray;
    for (int col = 0; col < m_treeWidget->columnCount(); ++col) {
        item->setForeground(col, QBrush(color));
    }
}

QString TreeView::getObjectTypeName(ObjectType type) const {
    switch (type) {
        case ObjectType::PRIMITIVE_BOX: return "Box";
        case ObjectType::PRIMITIVE_CYLINDER: return "Cylinder";
        case ObjectType::PRIMITIVE_SPHERE: return "Sphere";
        case ObjectType::PRIMITIVE_CONE: return "Cone";
        case ObjectType::PRIMITIVE_LINE: return "Line";
        case ObjectType::SKETCH: return "Sketch";
        case ObjectType::EXTRUSION: return "Extrusion";
        case ObjectType::REVOLUTION: return "Revolution";
        case ObjectType::BOOLEAN_UNION: return "Union";
        case ObjectType::BOOLEAN_DIFFERENCE: return "Difference";
        case ObjectType::BOOLEAN_INTERSECTION: return "Intersection";
        case ObjectType::MESH: return "Mesh";
        case ObjectType::ASSEMBLY: return "Assembly";
    }
    return "Unknown";
}

QIcon TreeView::getObjectTypeIcon(ObjectType type) const {
    // For now, return empty icons
    // In a real implementation, you would load appropriate icons
    return QIcon();
}

} // namespace HybridCAD 