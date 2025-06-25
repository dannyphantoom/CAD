#include "MainWindow.h"
#include "CADViewer.h"
#include "PropertyPanel.h"
#include "TreeView.h"
#include "ToolManager.h"
#include "KeyBindingDialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>

namespace HybridCAD {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_cadViewer(nullptr)
    , m_propertyPanel(nullptr)
    , m_treeView(nullptr)
    , m_toolManager(nullptr)
{
    setWindowTitle("HybridCAD - Advanced CAD & Mesh Editor");
    setMinimumSize(1200, 800);
    resize(1600, 1000);
    
    // Initialize UI components
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();
    setupLayoutAndConnections();
    
    // Load settings
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Update recent files menu
    updateRecentFileActions();
    
    // Set initial state
    m_selectToolAct->setChecked(true);
    m_gridAct->setChecked(true);
    m_axesAct->setChecked(true);
}

MainWindow::~MainWindow()
{
    // Save settings
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // TODO: Check for unsaved changes
    event->accept();
}

void MainWindow::createActions()
{
    // File actions
    m_newAct = new QAction(tr("&New"), this);
    m_newAct->setShortcuts(QKeySequence::New);
    m_newAct->setStatusTip(tr("Create a new file"));
    connect(m_newAct, &QAction::triggered, this, &MainWindow::newFile);
    
    m_openAct = new QAction(tr("&Open..."), this);
    m_openAct->setShortcuts(QKeySequence::Open);
    m_openAct->setStatusTip(tr("Open an existing file"));
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openFile);
    
    m_saveAct = new QAction(tr("&Save"), this);
    m_saveAct->setShortcuts(QKeySequence::Save);
    m_saveAct->setStatusTip(tr("Save the document to disk"));
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveFile);
    
    m_saveAsAct = new QAction(tr("Save &As..."), this);
    m_saveAsAct->setShortcuts(QKeySequence::SaveAs);
    m_saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(m_saveAsAct, &QAction::triggered, this, &MainWindow::saveAsFile);
    
    m_exportAct = new QAction(tr("&Export..."), this);
    m_exportAct->setStatusTip(tr("Export to various formats"));
    connect(m_exportAct, &QAction::triggered, this, &MainWindow::exportFile);
    
    m_importAct = new QAction(tr("&Import..."), this);
    m_importAct->setStatusTip(tr("Import from various formats"));
    connect(m_importAct, &QAction::triggered, this, &MainWindow::importFile);
    
    m_exitAct = new QAction(tr("E&xit"), this);
    m_exitAct->setShortcuts(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Exit the application"));
    connect(m_exitAct, &QAction::triggered, this, &QWidget::close);
    
    // Edit actions
    m_undoAct = new QAction(tr("&Undo"), this);
    m_undoAct->setShortcuts(QKeySequence::Undo);
    connect(m_undoAct, &QAction::triggered, this, &MainWindow::undo);
    
    m_redoAct = new QAction(tr("&Redo"), this);
    m_redoAct->setShortcuts(QKeySequence::Redo);
    connect(m_redoAct, &QAction::triggered, this, &MainWindow::redo);
    
    m_cutAct = new QAction(QIcon(":/icons/cut.png"), tr("Cu&t"), this);
    m_cutAct->setShortcuts(QKeySequence::Cut);
    connect(m_cutAct, &QAction::triggered, this, &MainWindow::cut);
    
    m_copyAct = new QAction(QIcon(":/icons/copy.png"), tr("&Copy"), this);
    m_copyAct->setShortcuts(QKeySequence::Copy);
    connect(m_copyAct, &QAction::triggered, this, &MainWindow::copy);
    
    m_pasteAct = new QAction(QIcon(":/icons/paste.png"), tr("&Paste"), this);
    m_pasteAct->setShortcuts(QKeySequence::Paste);
    connect(m_pasteAct, &QAction::triggered, this, &MainWindow::paste);
    
    m_deleteAct = new QAction(QIcon(":/icons/delete.png"), tr("&Delete"), this);
    m_deleteAct->setShortcut(QKeySequence::Delete);
    connect(m_deleteAct, &QAction::triggered, this, &MainWindow::deleteSelected);
    
    m_selectAllAct = new QAction(tr("Select &All"), this);
    m_selectAllAct->setShortcuts(QKeySequence::SelectAll);
    connect(m_selectAllAct, &QAction::triggered, this, &MainWindow::selectAll);
    
    // View actions
    m_resetViewAct = new QAction(tr("&Reset View"), this);
    m_resetViewAct->setShortcut(QKeySequence(Qt::Key_Home));
    connect(m_resetViewAct, &QAction::triggered, this, &MainWindow::resetView);
    
    m_frontViewAct = new QAction(tr("&Front View"), this);
    m_frontViewAct->setShortcut(QKeySequence(Qt::Key_1));
    connect(m_frontViewAct, &QAction::triggered, this, &MainWindow::frontView);
    
    m_backViewAct = new QAction(tr("&Back View"), this);
    m_backViewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    connect(m_backViewAct, &QAction::triggered, this, &MainWindow::backView);
    
    m_leftViewAct = new QAction(tr("&Left View"), this);
    m_leftViewAct->setShortcut(QKeySequence(Qt::Key_3));
    connect(m_leftViewAct, &QAction::triggered, this, &MainWindow::leftView);
    
    m_rightViewAct = new QAction(tr("&Right View"), this);
    m_rightViewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
    connect(m_rightViewAct, &QAction::triggered, this, &MainWindow::rightView);
    
    m_topViewAct = new QAction(tr("&Top View"), this);
    m_topViewAct->setShortcut(QKeySequence(Qt::Key_7));
    connect(m_topViewAct, &QAction::triggered, this, &MainWindow::topView);
    
    m_bottomViewAct = new QAction(tr("B&ottom View"), this);
    m_bottomViewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_7));
    connect(m_bottomViewAct, &QAction::triggered, this, &MainWindow::bottomView);
    
    m_isometricViewAct = new QAction(tr("&Isometric View"), this);
    m_isometricViewAct->setShortcut(QKeySequence(Qt::Key_9));
    connect(m_isometricViewAct, &QAction::triggered, this, &MainWindow::isometricView);
    
    m_wireframeAct = new QAction(tr("&Wireframe"), this);
    m_wireframeAct->setCheckable(true);
    m_wireframeAct->setShortcut(QKeySequence(Qt::Key_Z));
    connect(m_wireframeAct, &QAction::triggered, this, &MainWindow::toggleWireframe);
    
    m_gridAct = new QAction(tr("&Grid"), this);
    m_gridAct->setCheckable(true);
    m_gridAct->setShortcut(QKeySequence(Qt::Key_G));
    connect(m_gridAct, &QAction::triggered, this, &MainWindow::toggleGrid);
    
    m_axesAct = new QAction(tr("&Axes"), this);
    m_axesAct->setCheckable(true);
    connect(m_axesAct, &QAction::triggered, this, &MainWindow::toggleAxes);
    
    // Create actions
    m_createBoxAct = new QAction(QIcon(":/icons/box.png"), tr("&Box"), this);
    m_createBoxAct->setStatusTip(tr("Create a box primitive"));
    connect(m_createBoxAct, &QAction::triggered, this, &MainWindow::createBox);
    
    m_createCylinderAct = new QAction(QIcon(":/icons/cylinder.png"), tr("&Cylinder"), this);
    m_createCylinderAct->setStatusTip(tr("Create a cylinder primitive"));
    connect(m_createCylinderAct, &QAction::triggered, this, &MainWindow::createCylinder);
    
    m_createSphereAct = new QAction(QIcon(":/icons/sphere.png"), tr("&Sphere"), this);
    m_createSphereAct->setStatusTip(tr("Create a sphere primitive"));
    connect(m_createSphereAct, &QAction::triggered, this, &MainWindow::createSphere);
    
    m_createConeAct = new QAction(QIcon(":/icons/cone.png"), tr("C&one"), this);
    m_createConeAct->setStatusTip(tr("Create a cone primitive"));
    connect(m_createConeAct, &QAction::triggered, this, &MainWindow::createCone);
    
    m_createRectangleAct = new QAction(QIcon(":/icons/rectangle.png"), tr("&Rectangle"), this);
    m_createRectangleAct->setStatusTip(tr("Create a rectangle primitive"));
    connect(m_createRectangleAct, &QAction::triggered, this, &MainWindow::createRectangle);
    
    m_createCircleAct = new QAction(QIcon(":/icons/circle.png"), tr("&Circle"), this);
    m_createCircleAct->setStatusTip(tr("Create a circle primitive"));
    connect(m_createCircleAct, &QAction::triggered, this, &MainWindow::createCircle);
    
    m_createLineAct = new QAction(QIcon(":/icons/line.png"), tr("&Line"), this);
    m_createLineAct->setStatusTip(tr("Create a line primitive"));
    connect(m_createLineAct, &QAction::triggered, this, &MainWindow::createLine);
    
    m_createSketchAct = new QAction(QIcon(":/icons/sketch.png"), tr("S&ketch"), this);
    m_createSketchAct->setStatusTip(tr("Create a 2D sketch"));
    connect(m_createSketchAct, &QAction::triggered, this, &MainWindow::createSketch);
    
    // Mesh actions
    m_meshModeAct = new QAction(tr("&Mesh Edit Mode"), this);
    m_meshModeAct->setCheckable(true);
    m_meshModeAct->setShortcut(QKeySequence(Qt::Key_Tab));
    connect(m_meshModeAct, &QAction::triggered, this, &MainWindow::enterMeshEditMode);
    
    m_subdivideAct = new QAction(tr("&Subdivide"), this);
    connect(m_subdivideAct, &QAction::triggered, this, &MainWindow::subdivideMesh);
    
    m_smoothAct = new QAction(tr("S&mooth"), this);
    connect(m_smoothAct, &QAction::triggered, this, &MainWindow::smoothMesh);
    
    m_decimateAct = new QAction(tr("&Decimate"), this);
    connect(m_decimateAct, &QAction::triggered, this, &MainWindow::decimateMesh);
    
    // Boolean actions
    m_unionAct = new QAction(QIcon(":/icons/union.png"), tr("&Union"), this);
    m_unionAct->setStatusTip(tr("Boolean union operation"));
    connect(m_unionAct, &QAction::triggered, this, &MainWindow::booleanUnion);
    
    m_differenceAct = new QAction(QIcon(":/icons/difference.png"), tr("&Difference"), this);
    m_differenceAct->setStatusTip(tr("Boolean difference operation"));
    connect(m_differenceAct, &QAction::triggered, this, &MainWindow::booleanDifference);
    
    m_intersectionAct = new QAction(QIcon(":/icons/intersection.png"), tr("&Intersection"), this);
    m_intersectionAct->setStatusTip(tr("Boolean intersection operation"));
    connect(m_intersectionAct, &QAction::triggered, this, &MainWindow::booleanIntersection);
    
    // Tool actions
    m_toolGroup = new QActionGroup(this);
    
    m_selectToolAct = new QAction(QIcon(":/icons/select.png"), tr("&Select"), this);
    m_selectToolAct->setCheckable(true);
    m_selectToolAct->setShortcut(QKeySequence(Qt::Key_S));
    m_toolGroup->addAction(m_selectToolAct);
    connect(m_selectToolAct, &QAction::triggered, this, &MainWindow::selectTool);
    
    m_moveToolAct = new QAction(QIcon(":/icons/move.png"), tr("&Move"), this);
    m_moveToolAct->setCheckable(true);
    m_moveToolAct->setShortcut(QKeySequence(Qt::Key_M));
    m_toolGroup->addAction(m_moveToolAct);
    connect(m_moveToolAct, &QAction::triggered, this, &MainWindow::moveTool);
    
    m_rotateToolAct = new QAction(QIcon(":/icons/rotate.png"), tr("&Rotate"), this);
    m_rotateToolAct->setCheckable(true);
    m_rotateToolAct->setShortcut(QKeySequence(Qt::Key_R));
    m_toolGroup->addAction(m_rotateToolAct);
    connect(m_rotateToolAct, &QAction::triggered, this, &MainWindow::rotateTool);
    
    m_scaleToolAct = new QAction(QIcon(":/icons/scale.png"), tr("S&cale"), this);
    m_scaleToolAct->setCheckable(true);
    m_scaleToolAct->setShortcut(QKeySequence(Qt::Key_C));
    m_toolGroup->addAction(m_scaleToolAct);
    connect(m_scaleToolAct, &QAction::triggered, this, &MainWindow::scaleTool);
    
    m_extrudeToolAct = new QAction(QIcon(":/icons/extrude.png"), tr("&Extrude"), this);
    m_extrudeToolAct->setCheckable(true);
    m_extrudeToolAct->setShortcut(QKeySequence(Qt::Key_E));
    m_toolGroup->addAction(m_extrudeToolAct);
    connect(m_extrudeToolAct, &QAction::triggered, this, &MainWindow::extrudeTool);
    
    // Window actions
    m_propertyPanelAct = new QAction(tr("&Property Panel"), this);
    m_propertyPanelAct->setCheckable(true);
    m_propertyPanelAct->setChecked(true);
    connect(m_propertyPanelAct, &QAction::triggered, this, &MainWindow::togglePropertyPanel);
    
    m_treeViewAct = new QAction(tr("&Tree View"), this);
    m_treeViewAct->setCheckable(true);
    m_treeViewAct->setChecked(true);
    connect(m_treeViewAct, &QAction::triggered, this, &MainWindow::toggleTreeView);
    
    m_toolboxAct = new QAction(tr("Tool&box"), this);
    m_toolboxAct->setCheckable(true);
    m_toolboxAct->setChecked(true);
    connect(m_toolboxAct, &QAction::triggered, this, &MainWindow::toggleToolbox);
    
    // Settings actions
    m_keyBindingsAct = new QAction(tr("&Key Bindings..."), this);
    m_keyBindingsAct->setStatusTip(tr("Customize keyboard shortcuts"));
    connect(m_keyBindingsAct, &QAction::triggered, this, &MainWindow::openKeyBindingDialog);
    
    // Help actions
    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, &QAction::triggered, this, &MainWindow::about);
    
    m_aboutQtAct = new QAction(tr("About &Qt"), this);
    m_aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(m_aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::createMenus()
{
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_newAct);
    m_fileMenu->addAction(m_openAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_saveAct);
    m_fileMenu->addAction(m_saveAsAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_importAct);
    m_fileMenu->addAction(m_exportAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAct);
    
    // Edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_undoAct);
    m_editMenu->addAction(m_redoAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_cutAct);
    m_editMenu->addAction(m_copyAct);
    m_editMenu->addAction(m_pasteAct);
    m_editMenu->addAction(m_deleteAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_selectAllAct);
    
    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_resetViewAct);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_frontViewAct);
    m_viewMenu->addAction(m_backViewAct);
    m_viewMenu->addAction(m_leftViewAct);
    m_viewMenu->addAction(m_rightViewAct);
    m_viewMenu->addAction(m_topViewAct);
    m_viewMenu->addAction(m_bottomViewAct);
    m_viewMenu->addAction(m_isometricViewAct);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_wireframeAct);
    m_viewMenu->addAction(m_gridAct);
    m_viewMenu->addAction(m_axesAct);
    
    // Create menu
    m_createMenu = menuBar()->addMenu(tr("&Create"));
    m_createMenu->addAction(m_createBoxAct);
    m_createMenu->addAction(m_createCylinderAct);
    m_createMenu->addAction(m_createSphereAct);
    m_createMenu->addAction(m_createConeAct);
    m_createMenu->addAction(m_createRectangleAct);
    m_createMenu->addAction(m_createCircleAct);
    m_createMenu->addAction(m_createLineAct);
    m_createMenu->addSeparator();
    m_createMenu->addAction(m_createSketchAct);
    
    // Mesh menu
    m_meshMenu = menuBar()->addMenu(tr("&Mesh"));
    m_meshMenu->addAction(m_meshModeAct);
    m_meshMenu->addSeparator();
    m_meshMenu->addAction(m_subdivideAct);
    m_meshMenu->addAction(m_smoothAct);
    m_meshMenu->addAction(m_decimateAct);
    
    // Boolean menu
    m_booleanMenu = menuBar()->addMenu(tr("&Boolean"));
    m_booleanMenu->addAction(m_unionAct);
    m_booleanMenu->addAction(m_differenceAct);
    m_booleanMenu->addAction(m_intersectionAct);
    
    // Tools menu
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_toolsMenu->addAction(m_selectToolAct);
    m_toolsMenu->addAction(m_moveToolAct);
    m_toolsMenu->addAction(m_rotateToolAct);
    m_toolsMenu->addAction(m_scaleToolAct);
    m_toolsMenu->addAction(m_extrudeToolAct);
    
    // Settings menu
    m_settingsMenu = menuBar()->addMenu(tr("&Settings"));
    m_settingsMenu->addAction(m_keyBindingsAct);
    
    // Window menu
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    m_windowMenu->addAction(m_propertyPanelAct);
    m_windowMenu->addAction(m_treeViewAct);
    m_windowMenu->addAction(m_toolboxAct);
    
    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAct);
    m_helpMenu->addAction(m_aboutQtAct);
}

void MainWindow::createToolBars()
{
    // File toolbar
    m_fileToolBar = addToolBar(tr("File"));
    m_fileToolBar->setObjectName("FileToolBar");
    m_fileToolBar->addAction(m_newAct);
    m_fileToolBar->addAction(m_openAct);
    m_fileToolBar->addAction(m_saveAct);
    
    // Edit toolbar
    m_editToolBar = addToolBar(tr("Edit"));
    m_editToolBar->setObjectName("EditToolBar");
    m_editToolBar->addAction(m_undoAct);
    m_editToolBar->addAction(m_redoAct);
    m_editToolBar->addSeparator();
    m_editToolBar->addAction(m_cutAct);
    m_editToolBar->addAction(m_copyAct);
    m_editToolBar->addAction(m_pasteAct);
    m_editToolBar->addAction(m_deleteAct);
    
    // View toolbar
    m_viewToolBar = addToolBar(tr("View"));
    m_viewToolBar->setObjectName("ViewToolBar");
    m_viewToolBar->addAction(m_wireframeAct);
    m_viewToolBar->addAction(m_gridAct);
    m_viewToolBar->addAction(m_axesAct);
    
    // Create toolbar
    m_createToolBar = addToolBar(tr("Create"));
    m_createToolBar->setObjectName("CreateToolBar");
    m_createToolBar->addAction(m_createBoxAct);
    m_createToolBar->addAction(m_createCylinderAct);
    m_createToolBar->addAction(m_createSphereAct);
    m_createToolBar->addAction(m_createConeAct);
    m_createToolBar->addAction(m_createRectangleAct);
    m_createToolBar->addAction(m_createCircleAct);
    m_createToolBar->addAction(m_createLineAct);
    m_createToolBar->addSeparator();
    m_createToolBar->addAction(m_unionAct);
    m_createToolBar->addAction(m_differenceAct);
    m_createToolBar->addAction(m_intersectionAct);
    
    // Tools toolbar
    m_toolsToolBar = addToolBar(tr("Tools"));
    m_toolsToolBar->setObjectName("ToolsToolBar");
    m_toolsToolBar->addAction(m_selectToolAct);
    m_toolsToolBar->addAction(m_moveToolAct);
    m_toolsToolBar->addAction(m_rotateToolAct);
    m_toolsToolBar->addAction(m_scaleToolAct);
    m_toolsToolBar->addAction(m_extrudeToolAct);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addWidget(m_statusLabel);
    
    m_coordinateLabel = new QLabel(tr("X: 0.00  Y: 0.00  Z: 0.00"));
    statusBar()->addPermanentWidget(m_coordinateLabel);
}

void MainWindow::createDockWindows()
{
    // Create central widget (CAD Viewer)
    m_cadViewer = new CADViewer(this);
    setCentralWidget(m_cadViewer);
    
    // Property panel dock
    m_propertyDock = new QDockWidget(tr("Properties"), this);
    m_propertyDock->setObjectName("PropertiesDock");
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_propertyPanel = new PropertyPanel(m_propertyDock);
    m_propertyDock->setWidget(m_propertyPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);
    
    // Tree view dock
    m_treeDock = new QDockWidget(tr("Scene Tree"), this);
    m_treeDock->setObjectName("SceneTreeDock");
    m_treeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_treeView = new TreeView(m_treeDock);
    m_treeDock->setWidget(m_treeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_treeDock);
    
    // Tool manager dock
    m_toolboxDock = new QDockWidget(tr("Toolbox"), this);
    m_toolboxDock->setObjectName("ToolboxDock");
    m_toolboxDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_toolManager = new ToolManager(m_toolboxDock);
    m_toolboxDock->setWidget(m_toolManager);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolboxDock);
    
    // Tabify the left docks
    tabifyDockWidget(m_treeDock, m_toolboxDock);
    m_treeDock->raise();
}

void MainWindow::setupLayoutAndConnections()
{
    // Connect CAD viewer signals
    connect(m_cadViewer, &CADViewer::objectSelected, m_propertyPanel, &PropertyPanel::setSelectedObject);
    connect(m_cadViewer, &CADViewer::objectSelected, m_treeView, &TreeView::selectObject);
    connect(m_cadViewer, &CADViewer::coordinatesChanged, this, [this](const QVector3D& pos) {
        m_coordinateLabel->setText(QString("X: %1  Y: %2  Z: %3")
                                  .arg(pos.x(), 0, 'f', 2)
                                  .arg(pos.y(), 0, 'f', 2)
                                  .arg(pos.z(), 0, 'f', 2));
    });
    
    // Connect grid/wireframe/axes toggle signals to update UI actions
    connect(m_cadViewer, &CADViewer::gridToggled, m_gridAct, &QAction::setChecked);
    connect(m_cadViewer, &CADViewer::wireframeToggled, m_wireframeAct, &QAction::setChecked);
    connect(m_cadViewer, &CADViewer::axesToggled, m_axesAct, &QAction::setChecked);
    
    // Connect ToolManager to CADViewer for grid control
    connect(m_toolManager, &ToolManager::gridSizeChanged, m_cadViewer, &CADViewer::setGridSize);
    connect(m_toolManager, &ToolManager::gridPlaneChanged, this, [this](int plane) {
        m_cadViewer->setGridPlane(static_cast<GridPlane>(plane));
    });
    
    // Connect tree view signals
    connect(m_treeView, &TreeView::objectSelected, m_cadViewer, &CADViewer::selectObject);
    connect(m_treeView, &TreeView::objectSelected, m_propertyPanel, &PropertyPanel::setSelectedObject);
    
    // Connect dock widget visibility to actions
    connect(m_propertyDock, &QDockWidget::visibilityChanged, m_propertyPanelAct, &QAction::setChecked);
    connect(m_treeDock, &QDockWidget::visibilityChanged, m_treeViewAct, &QAction::setChecked);
    connect(m_toolboxDock, &QDockWidget::visibilityChanged, m_toolboxAct, &QAction::setChecked);
}

// File menu implementations
void MainWindow::newFile()
{
    m_cadViewer->clearObjects();
    m_treeView->clearObjects();
    m_propertyPanel->clearSelection();
    setCurrentFile("");
    m_statusLabel->setText(tr("New file created"));
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open CAD File"), "",
        tr("CAD Files (*.cad *.step *.stp *.iges *.igs);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Implement file loading
        setCurrentFile(fileName);
        m_statusLabel->setText(tr("File opened: %1").arg(fileName));
    }
}

void MainWindow::saveFile()
{
    if (m_currentFile.isEmpty()) {
        saveAsFile();
    } else {
        // TODO: Implement file saving
        m_statusLabel->setText(tr("File saved: %1").arg(m_currentFile));
    }
}

void MainWindow::saveAsFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save CAD File"), "",
        tr("CAD Files (*.cad);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        setCurrentFile(fileName);
        saveFile();
    }
}

void MainWindow::exportFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export File"), "",
        tr("STEP Files (*.step *.stp);;IGES Files (*.iges *.igs);;STL Files (*.stl);;OBJ Files (*.obj)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Implement export
        m_statusLabel->setText(tr("File exported: %1").arg(fileName));
    }
}

void MainWindow::importFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import File"), "",
        tr("STEP Files (*.step *.stp);;IGES Files (*.iges *.igs);;STL Files (*.stl);;OBJ Files (*.obj);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Implement import
        m_statusLabel->setText(tr("File imported: %1").arg(fileName));
    }
}

// Edit menu implementations
void MainWindow::undo() { m_statusLabel->setText(tr("Undo")); }
void MainWindow::redo() { m_statusLabel->setText(tr("Redo")); }
void MainWindow::cut() { m_statusLabel->setText(tr("Cut")); }
void MainWindow::copy() { m_statusLabel->setText(tr("Copy")); }
void MainWindow::paste() { m_statusLabel->setText(tr("Paste")); }
void MainWindow::deleteSelected() { m_statusLabel->setText(tr("Delete selected")); }
void MainWindow::selectAll() { m_statusLabel->setText(tr("Select all")); }

// View menu implementations
void MainWindow::resetView() { if (m_cadViewer) m_cadViewer->resetView(); }
void MainWindow::frontView() { if (m_cadViewer) m_cadViewer->frontView(); }
void MainWindow::backView() { if (m_cadViewer) m_cadViewer->backView(); }
void MainWindow::leftView() { if (m_cadViewer) m_cadViewer->leftView(); }
void MainWindow::rightView() { if (m_cadViewer) m_cadViewer->rightView(); }
void MainWindow::topView() { if (m_cadViewer) m_cadViewer->topView(); }
void MainWindow::bottomView() { if (m_cadViewer) m_cadViewer->bottomView(); }
void MainWindow::isometricView() { if (m_cadViewer) m_cadViewer->isometricView(); }

void MainWindow::toggleWireframe() { 
    if (m_cadViewer) m_cadViewer->setWireframeMode(m_wireframeAct->isChecked()); 
}

void MainWindow::toggleGrid() { 
    if (m_cadViewer) m_cadViewer->setGridVisible(m_gridAct->isChecked()); 
}

void MainWindow::toggleAxes() { 
    if (m_cadViewer) m_cadViewer->setAxesVisible(m_axesAct->isChecked()); 
}

// Create menu implementations
void MainWindow::createBox() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_BOX);
        m_statusLabel->setText(tr("Click to place box"));
    }
}

void MainWindow::createCylinder() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_CYLINDER);
        m_statusLabel->setText(tr("Click to place cylinder"));
    }
}

void MainWindow::createSphere() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_SPHERE);
        m_statusLabel->setText(tr("Click to place sphere"));
    }
}

void MainWindow::createCone() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_CONE);
        m_statusLabel->setText(tr("Click to place cone"));
    }
}

void MainWindow::createRectangle() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_RECTANGLE);
        m_statusLabel->setText(tr("Click and drag to place rectangle"));
    }
}

void MainWindow::createCircle() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_CIRCLE);
        m_statusLabel->setText(tr("Click and drag to place circle"));
    }
}

void MainWindow::createLine() { 
    if (m_cadViewer) {
        m_cadViewer->setActiveTool(ActiveTool::PLACE_SHAPE);
        m_cadViewer->setShapeToPlace(ObjectType::PRIMITIVE_LINE);
        m_statusLabel->setText(tr("Click and drag to place line"));
    }
}

void MainWindow::createSketch() { m_statusLabel->setText(tr("Create sketch")); }

// Mesh menu implementations
void MainWindow::enterMeshEditMode() { m_statusLabel->setText(tr("Enter mesh edit mode")); }
void MainWindow::exitMeshEditMode() { m_statusLabel->setText(tr("Exit mesh edit mode")); }
void MainWindow::subdivideMesh() { m_statusLabel->setText(tr("Subdivide mesh")); }
void MainWindow::smoothMesh() { m_statusLabel->setText(tr("Smooth mesh")); }
void MainWindow::decimateMesh() { m_statusLabel->setText(tr("Decimate mesh")); }

// Boolean operations
void MainWindow::booleanUnion() { m_statusLabel->setText(tr("Boolean union")); }
void MainWindow::booleanDifference() { m_statusLabel->setText(tr("Boolean difference")); }
void MainWindow::booleanIntersection() { m_statusLabel->setText(tr("Boolean intersection")); }

// Tool implementations
void MainWindow::selectTool() { m_statusLabel->setText(tr("Select tool active")); }
void MainWindow::moveTool() { m_statusLabel->setText(tr("Move tool active")); }
void MainWindow::rotateTool() { m_statusLabel->setText(tr("Rotate tool active")); }
void MainWindow::scaleTool() { m_statusLabel->setText(tr("Scale tool active")); }
void MainWindow::extrudeTool() { m_statusLabel->setText(tr("Extrude tool active")); }

// Window management
void MainWindow::togglePropertyPanel() { m_propertyDock->setVisible(m_propertyPanelAct->isChecked()); }
void MainWindow::toggleTreeView() { m_treeDock->setVisible(m_treeViewAct->isChecked()); }
void MainWindow::toggleToolbox() { m_toolboxDock->setVisible(m_toolboxAct->isChecked()); }

// Settings
void MainWindow::openKeyBindingDialog()
{
    KeyBindingDialog dialog(m_cadViewer, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Key bindings are automatically applied in the dialog
        m_statusLabel->setText(tr("Key bindings updated"));
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About HybridCAD"),
        tr("<h2>HybridCAD 1.0</h2>"
           "<p>A powerful cross-platform CAD application that combines "
           "precision engineering with advanced mesh editing capabilities.</p>"
           "<p>Built with Qt6, OpenCASCADE, and modern C++.</p>"));
}

void MainWindow::updateRecentFileActions()
{
    // TODO: Implement recent files
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    m_currentFile = fileName;
    QString shownName = m_currentFile;
    if (m_currentFile.isEmpty())
        shownName = "untitled.cad";
    setWindowTitle(tr("%1[*] - %2").arg(shownName, tr("HybridCAD")));
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

} // namespace HybridCAD 