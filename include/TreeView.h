#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QtCore/QTimer>
#include <memory>
#include <unordered_map>

#include "CADTypes.h"

namespace HybridCAD {

class TreeView : public QWidget
{
    Q_OBJECT

public:
    explicit TreeView(QWidget *parent = nullptr);
    ~TreeView();
    
    void addObject(CADObjectPtr object);
    void removeObject(CADObjectPtr object);
    void updateObject(CADObjectPtr object);
    void clearObjects();
    void selectObject(CADObjectPtr object);
    void deselectAll();
    
    void setObjects(const CADObjectList& objects);
    QTreeWidgetItem* findItem(CADObjectPtr object);

signals:
    void objectSelected(CADObjectPtr object);
    void objectDeselected(CADObjectPtr object);
    void objectVisibilityChanged(CADObjectPtr object, bool visible);
    void objectRenamed(CADObjectPtr object, const QString& newName);
    void deleteRequested(CADObjectPtr object);
    void duplicateRequested(CADObjectPtr object);
    void groupRequested(const std::vector<CADObjectPtr>& objects);
    void ungroupRequested(CADObjectPtr group);

private slots:
    void onItemSelectionChanged();
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onVisibilityToggled();
    void onRenameRequested();
    void onDeleteRequested();
    void onDuplicateRequested();
    void onGroupRequested();
    void onUngroupRequested();
    void onExpandAll();
    void onCollapseAll();
    void onRefresh();

private:
    void setupUI();
    void createContextMenu();
    void updateItemIcon(QTreeWidgetItem* item, CADObjectPtr object);
    void updateItemText(QTreeWidgetItem* item, CADObjectPtr object);
    void updateItemVisibility(QTreeWidgetItem* item, CADObjectPtr object);
    
    QString getObjectTypeName(ObjectType type) const;
    QIcon getObjectTypeIcon(ObjectType type) const;
    
    // UI components
    QVBoxLayout* m_layout;
    QTreeWidget* m_treeWidget;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_expandAllButton;
    QPushButton* m_collapseAllButton;
    QPushButton* m_refreshButton;
    
    // Context menu
    QMenu* m_contextMenu;
    QAction* m_renameAction;
    QAction* m_deleteAction;
    QAction* m_duplicateAction;
    QAction* m_groupAction;
    QAction* m_ungroupAction;
    QAction* m_visibilityAction;
    
    // Object mapping
    std::unordered_map<CADObjectPtr, QTreeWidgetItem*> m_objectToItem;
    std::unordered_map<QTreeWidgetItem*, CADObjectPtr> m_itemToObject;
    
    // State
    bool m_updating;
    QTreeWidgetItem* m_contextMenuItem;
    
    // Tree columns
    enum Column {
        NAME_COLUMN = 0,
        TYPE_COLUMN = 1,
        VISIBILITY_COLUMN = 2
    };
};

} // namespace HybridCAD 