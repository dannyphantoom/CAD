#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace HybridCAD {

class CADViewer;
class PropertyPanel;
class TreeView;
class ToolManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu actions
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void exportFile();
    void importFile();
    
    // Edit menu actions
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void deleteSelected();
    void selectAll();
    
    // View menu actions
    void resetView();
    void frontView();
    void backView();
    void leftView();
    void rightView();
    void topView();
    void bottomView();
    void isometricView();
    void toggleWireframe();
    void toggleGrid();
    void toggleAxes();
    
    // Create menu actions
    void createBox();
    void createCylinder();
    void createSphere();
    void createCone();
    void createRectangle();
    void createCircle();
    void createLine(); // New action for creating a line
    void createSketch();
    
    // Mesh menu actions
    void enterMeshEditMode();
    void exitMeshEditMode();
    void subdivideMesh();
    void smoothMesh();
    void decimateMesh();
    
    // Boolean operations
    void booleanUnion();
    void booleanDifference();
    void booleanIntersection();
    
    // Tools
    void selectTool();
    void moveTool();
    void rotateTool();
    void scaleTool();
    void extrudeTool();
    
    // Window management
    void togglePropertyPanel();
    void toggleTreeView();
    void toggleToolbox();
    
    // Settings
    void openKeyBindingDialog();
    void openPreferencesDialog();
    
    // Help
    void about();

private slots:
    void updateStatusMessage(const QString& message);

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWindows();
    void setupLayoutAndConnections();
    
    void updateRecentFileActions();
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);
    
    // UI Components
    CADViewer *m_cadViewer;
    PropertyPanel *m_propertyPanel;
    TreeView *m_treeView;
    ToolManager *m_toolManager;
    
    // Docking widgets
    QDockWidget *m_propertyDock;
    QDockWidget *m_treeDock;
    QDockWidget *m_toolboxDock;
    
    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_createMenu;
    QMenu *m_meshMenu;
    QMenu *m_booleanMenu;
    QMenu *m_toolsMenu;
    QMenu *m_settingsMenu;
    QMenu *m_windowMenu;
    QMenu *m_helpMenu;
    QMenu *m_recentFilesMenu;
    
    // Toolbars
    QToolBar *m_fileToolBar;
    QToolBar *m_editToolBar;
    QToolBar *m_viewToolBar;
    QToolBar *m_createToolBar;
    QToolBar *m_toolsToolBar;
    
    // Actions - File
    QAction *m_newAct;
    QAction *m_openAct;
    QAction *m_saveAct;
    QAction *m_saveAsAct;
    QAction *m_exportAct;
    QAction *m_importAct;
    QAction *m_exitAct;
    QAction *m_recentFileActs[5];
    QAction *m_separatorAct;
    
    // Actions - Edit
    QAction *m_undoAct;
    QAction *m_redoAct;
    QAction *m_cutAct;
    QAction *m_copyAct;
    QAction *m_pasteAct;
    QAction *m_deleteAct;
    QAction *m_selectAllAct;
    
    // Actions - View
    QAction *m_resetViewAct;
    QAction *m_frontViewAct;
    QAction *m_backViewAct;
    QAction *m_leftViewAct;
    QAction *m_rightViewAct;
    QAction *m_topViewAct;
    QAction *m_bottomViewAct;
    QAction *m_isometricViewAct;
    QAction *m_wireframeAct;
    QAction *m_gridAct;
    QAction *m_axesAct;
    
    // Actions - Create
    QAction *m_createBoxAct;
    QAction *m_createCylinderAct;
    QAction *m_createSphereAct;
    QAction *m_createConeAct;
    QAction *m_createRectangleAct;
    QAction *m_createCircleAct;
    QAction *m_createLineAct; // New action for creating a line
    QAction *m_createSketchAct;
    
    // Actions - Mesh
    QAction *m_meshModeAct;
    QAction *m_subdivideAct;
    QAction *m_smoothAct;
    QAction *m_decimateAct;
    
    // Actions - Boolean
    QAction *m_unionAct;
    QAction *m_differenceAct;
    QAction *m_intersectionAct;
    
    // Actions - Tools
    QActionGroup *m_toolGroup;
    QAction *m_selectToolAct;
    QAction *m_moveToolAct;
    QAction *m_rotateToolAct;
    QAction *m_scaleToolAct;
    QAction *m_extrudeToolAct;
    
    // Actions - Settings
    QAction *m_keyBindingsAct;
    QAction *m_preferencesAct;
    
    // Actions - Window
    QAction *m_propertyPanelAct;
    QAction *m_treeViewAct;
    QAction *m_toolboxAct;
    
    // Actions - Help
    QAction *m_aboutAct;
    QAction *m_aboutQtAct;
    
    // Status bar
    QLabel *m_statusLabel;
    QLabel *m_coordinateLabel;
    
    // Current file
    QString m_currentFile;
    QStringList m_recentFiles;
};

} // namespace HybridCAD 