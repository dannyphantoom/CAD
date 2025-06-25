#include "KeyBindingDialog.h"
#include <QHeaderView>
#include <QMessageBox>

namespace HybridCAD {

KeyBindingDialog::KeyBindingDialog(CADViewer* cadViewer, QWidget *parent)
    : QDialog(parent)
    , m_cadViewer(cadViewer)
{
    setWindowTitle("Customize Key Bindings");
    setModal(true);
    resize(600, 400);
    
    setupUI();
    populateTable();
}

KeyBindingDialog::~KeyBindingDialog() = default;

void KeyBindingDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Create table
    m_keyBindingTable = new QTableWidget(this);
    m_keyBindingTable->setColumnCount(3);
    QStringList headers;
    headers << "Action" << "Description" << "Key Binding";
    m_keyBindingTable->setHorizontalHeaderLabels(headers);
    
    // Set column widths
    m_keyBindingTable->horizontalHeader()->setStretchLastSection(true);
    m_keyBindingTable->setColumnWidth(0, 120);
    m_keyBindingTable->setColumnWidth(1, 250);
    
    m_mainLayout->addWidget(m_keyBindingTable);
    
    // Create buttons
    m_buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton("Reset to Defaults", this);
    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Cancel", this);
    
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_resetButton, &QPushButton::clicked, this, &KeyBindingDialog::onResetToDefaults);
    connect(m_okButton, &QPushButton::clicked, this, &KeyBindingDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &KeyBindingDialog::onReject);
}

void KeyBindingDialog::populateTable()
{
    // Get current key bindings from CADViewer
    auto defaults = m_cadViewer->getDefaultKeyBindings();
    m_keyBindings.clear();
    
    // Get all possible actions
    QList<KeyAction> actions;
    actions << KeyAction::TOGGLE_GRID
            << KeyAction::TOGGLE_WIREFRAME
            << KeyAction::TOGGLE_AXES
            << KeyAction::RESET_VIEW
            << KeyAction::FRONT_VIEW
            << KeyAction::BACK_VIEW
            << KeyAction::LEFT_VIEW
            << KeyAction::RIGHT_VIEW
            << KeyAction::TOP_VIEW
            << KeyAction::BOTTOM_VIEW
            << KeyAction::ISOMETRIC_VIEW
            << KeyAction::DELETE_SELECTED
            << KeyAction::SELECT_ALL
            << KeyAction::DESELECT_ALL
            << KeyAction::PLACE_SHAPE
            << KeyAction::SKETCH_LINE
            << KeyAction::SKETCH_RECTANGLE
            << KeyAction::SKETCH_CIRCLE
            << KeyAction::CANCEL_CURRENT_ACTION;
    
    m_keyBindingTable->setRowCount(actions.size());
    
    for (int i = 0; i < actions.size(); ++i) {
        KeyAction action = actions[i];
        
        // Get current binding from CADViewer
        QKeySequence currentBinding = m_cadViewer->getKeyBinding(action);
        if (currentBinding.isEmpty()) {
            currentBinding = defaults.value(action);
        }
        m_keyBindings[action] = currentBinding;
        
        // Action name
        QTableWidgetItem* nameItem = new QTableWidgetItem(getActionName(action));
        nameItem->setFlags(Qt::ItemIsEnabled);
        m_keyBindingTable->setItem(i, 0, nameItem);
        
        // Description
        QTableWidgetItem* descItem = new QTableWidgetItem(getActionDescription(action));
        descItem->setFlags(Qt::ItemIsEnabled);
        m_keyBindingTable->setItem(i, 1, descItem);
        
        // Key sequence editor
        QKeySequenceEdit* keyEdit = new QKeySequenceEdit(currentBinding, this);
        m_keySequenceEdits[action] = keyEdit;
        m_keyBindingTable->setCellWidget(i, 2, keyEdit);
        
        connect(keyEdit, &QKeySequenceEdit::keySequenceChanged, this, &KeyBindingDialog::onKeySequenceChanged);
    }
}

QString KeyBindingDialog::getActionName(KeyAction action) const
{
    switch (action) {
    case KeyAction::TOGGLE_GRID: return "Toggle Grid";
    case KeyAction::TOGGLE_WIREFRAME: return "Toggle Wireframe";
    case KeyAction::TOGGLE_AXES: return "Toggle Axes";
    case KeyAction::RESET_VIEW: return "Reset View";
    case KeyAction::FRONT_VIEW: return "Front View";
    case KeyAction::BACK_VIEW: return "Back View";
    case KeyAction::LEFT_VIEW: return "Left View";
    case KeyAction::RIGHT_VIEW: return "Right View";
    case KeyAction::TOP_VIEW: return "Top View";
    case KeyAction::BOTTOM_VIEW: return "Bottom View";
    case KeyAction::ISOMETRIC_VIEW: return "Isometric View";
    case KeyAction::DELETE_SELECTED: return "Delete Selected";
    case KeyAction::SELECT_ALL: return "Select All";
    case KeyAction::DESELECT_ALL: return "Deselect All";
    case KeyAction::PLACE_SHAPE: return "Place Shape";
    case KeyAction::SKETCH_LINE: return "Sketch Line";
    case KeyAction::SKETCH_RECTANGLE: return "Sketch Rectangle";
    case KeyAction::SKETCH_CIRCLE: return "Sketch Circle";
    case KeyAction::CANCEL_CURRENT_ACTION: return "Cancel Action";
    default: return "Unknown";
    }
}

QString KeyBindingDialog::getActionDescription(KeyAction action) const
{
    switch (action) {
    case KeyAction::TOGGLE_GRID: return "Show/hide the grid";
    case KeyAction::TOGGLE_WIREFRAME: return "Switch between solid and wireframe display";
    case KeyAction::TOGGLE_AXES: return "Show/hide coordinate axes";
    case KeyAction::RESET_VIEW: return "Reset camera to default position";
    case KeyAction::FRONT_VIEW: return "Switch to front view";
    case KeyAction::BACK_VIEW: return "Switch to back view";
    case KeyAction::LEFT_VIEW: return "Switch to left view";
    case KeyAction::RIGHT_VIEW: return "Switch to right view";
    case KeyAction::TOP_VIEW: return "Switch to top view";
    case KeyAction::BOTTOM_VIEW: return "Switch to bottom view";
    case KeyAction::ISOMETRIC_VIEW: return "Switch to isometric view";
    case KeyAction::DELETE_SELECTED: return "Delete selected objects";
    case KeyAction::SELECT_ALL: return "Select all objects";
    case KeyAction::DESELECT_ALL: return "Deselect all objects";
    case KeyAction::PLACE_SHAPE: return "Enter shape placement mode";
    case KeyAction::SKETCH_LINE: return "Start line sketching";
    case KeyAction::SKETCH_RECTANGLE: return "Start rectangle sketching";
    case KeyAction::SKETCH_CIRCLE: return "Start circle sketching";
    case KeyAction::CANCEL_CURRENT_ACTION: return "Cancel current operation";
    default: return "Unknown action";
    }
}

void KeyBindingDialog::onResetToDefaults()
{
    auto defaults = m_cadViewer->getDefaultKeyBindings();
    setKeyBindings(defaults);
}

void KeyBindingDialog::onAccept()
{
    // Update key bindings from editors
    for (auto it = m_keySequenceEdits.begin(); it != m_keySequenceEdits.end(); ++it) {
        m_keyBindings[it.key()] = it.value()->keySequence();
    }
    
    // Apply to CADViewer
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        m_cadViewer->setKeyBinding(it.key(), it.value());
    }
    
    // Save key bindings
    m_cadViewer->saveKeyBindings();
    
    accept();
}

void KeyBindingDialog::onReject()
{
    reject();
}

void KeyBindingDialog::onKeySequenceChanged()
{
    // Update internal mapping when key sequence changes
    QKeySequenceEdit* editor = qobject_cast<QKeySequenceEdit*>(sender());
    if (!editor) return;
    
    for (auto it = m_keySequenceEdits.begin(); it != m_keySequenceEdits.end(); ++it) {
        if (it.value() == editor) {
            m_keyBindings[it.key()] = editor->keySequence();
            break;
        }
    }
}

QMap<KeyAction, QKeySequence> KeyBindingDialog::getKeyBindings() const
{
    return m_keyBindings;
}

void KeyBindingDialog::setKeyBindings(const QMap<KeyAction, QKeySequence>& bindings)
{
    m_keyBindings = bindings;
    
    // Update UI
    for (auto it = bindings.begin(); it != bindings.end(); ++it) {
        if (m_keySequenceEdits.contains(it.key())) {
            m_keySequenceEdits[it.key()]->setKeySequence(it.value());
        }
    }
}

} // namespace HybridCAD 