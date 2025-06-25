#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QHeaderView>
#include <QMap>
#include <QKeySequence>

#include "CADViewer.h"

namespace HybridCAD {

class KeyBindingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyBindingDialog(CADViewer* cadViewer, QWidget *parent = nullptr);
    ~KeyBindingDialog();

    QMap<KeyAction, QKeySequence> getKeyBindings() const;
    void setKeyBindings(const QMap<KeyAction, QKeySequence>& bindings);

private slots:
    void onResetToDefaults();
    void onAccept();
    void onReject();
    void onKeySequenceChanged();

private:
    void setupUI();
    void populateTable();
    QString getActionName(KeyAction action) const;
    QString getActionDescription(KeyAction action) const;

    CADViewer* m_cadViewer;
    QVBoxLayout* m_mainLayout;
    QTableWidget* m_keyBindingTable;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_resetButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    QMap<KeyAction, QKeySequence> m_keyBindings;
    QMap<KeyAction, QKeySequenceEdit*> m_keySequenceEdits;
};

} // namespace HybridCAD 