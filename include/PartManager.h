#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "CADTypes.h"

namespace HybridCAD {

// Assembly constraint types
enum class ConstraintType {
    FIXED,
    COINCIDENT,
    PARALLEL,
    PERPENDICULAR,
    CONCENTRIC,
    DISTANCE,
    ANGLE,
    TANGENT
};

// Constraint between two parts
struct AssemblyConstraint {
    ConstraintType type;
    CADObjectPtr partA;
    CADObjectPtr partB;
    Point3D pointA;
    Point3D pointB;
    Vector3D directionA;
    Vector3D directionB;
    double value; // For distance and angle constraints
    bool enabled;
    
    AssemblyConstraint(ConstraintType t = ConstraintType::FIXED) 
        : type(t), value(0.0), enabled(true) {}
};

// Part instance in an assembly
struct PartInstance {
    CADObjectPtr part;
    Transform transform;
    std::string instanceName;
    bool visible;
    bool locked;
    
    PartInstance(CADObjectPtr p = nullptr, const std::string& name = "Instance") 
        : part(p), instanceName(name), visible(true), locked(false) {}
};

// Assembly class
class Assembly : public CADObject {
public:
    Assembly(const std::string& name = "Assembly");
    ~Assembly() = default;
    
    ObjectType getType() const override { return ObjectType::ASSEMBLY; }
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    
    // Part management
    void addPart(CADObjectPtr part, const std::string& instanceName = "");
    void removePart(CADObjectPtr part);
    void removePartInstance(const std::string& instanceName);
    
    const std::vector<PartInstance>& getPartInstances() const { return m_partInstances; }
    PartInstance* getPartInstance(const std::string& instanceName);
    
    // Transform management
    void setPartTransform(const std::string& instanceName, const Transform& transform);
    Transform getPartTransform(const std::string& instanceName) const;
    
    // Constraint management
    void addConstraint(const AssemblyConstraint& constraint);
    void removeConstraint(size_t index);
    void updateConstraint(size_t index, const AssemblyConstraint& constraint);
    
    const std::vector<AssemblyConstraint>& getConstraints() const { return m_constraints; }
    
    // Assembly operations
    bool solveConstraints();
    void updateAssembly();
    
    // Bounding box
    Point3D getBoundingBoxMin() const;
    Point3D getBoundingBoxMax() const;
    
    // Collision detection
    bool hasCollisions() const;
    std::vector<std::pair<std::string, std::string>> getCollisions() const;

private:
    std::vector<PartInstance> m_partInstances;
    std::vector<AssemblyConstraint> m_constraints;
    bool m_constraintsDirty;
    
    void applyConstraint(const AssemblyConstraint& constraint);
    bool checkCollision(const PartInstance& instanceA, const PartInstance& instanceB) const;
};

// Part document class
class PartDocument {
public:
    PartDocument(const std::string& name = "Document");
    ~PartDocument() = default;
    
    // Document management
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }
    
    const std::string& getFilePath() const { return m_filePath; }
    void setFilePath(const std::string& path) { m_filePath = path; }
    
    // Object management
    void addObject(CADObjectPtr object);
    void removeObject(CADObjectPtr object);
    void clearObjects();
    
    const CADObjectList& getObjects() const { return m_objects; }
    CADObjectPtr findObject(const std::string& name) const;
    
    // History and undo/redo
    void beginUndoGroup(const std::string& description);
    void endUndoGroup();
    void addUndoCommand(const std::string& description);
    
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearHistory();
    
    // Feature tree
    CADObjectPtr getRootObject() const { return m_rootObject; }
    void setRootObject(CADObjectPtr root) { m_rootObject = root; }

private:
    std::string m_name;
    std::string m_filePath;
    bool m_dirty;
    
    CADObjectList m_objects;
    CADObjectPtr m_rootObject;
    
    // Undo/Redo system
    struct UndoCommand {
        std::string description;
        // Store the state or operations needed to undo
    };
    
    std::vector<UndoCommand> m_undoStack;
    std::vector<UndoCommand> m_redoStack;
    int m_maxUndoLevels;
};

// Part manager class
class PartManager {
public:
    PartManager();
    ~PartManager();
    
    // Document management
    std::shared_ptr<PartDocument> createDocument(const std::string& name = "New Document");
    std::shared_ptr<PartDocument> openDocument(const std::string& filePath);
    bool saveDocument(std::shared_ptr<PartDocument> document);
    bool saveDocumentAs(std::shared_ptr<PartDocument> document, const std::string& filePath);
    void closeDocument(std::shared_ptr<PartDocument> document);
    
    std::shared_ptr<PartDocument> getActiveDocument() const { return m_activeDocument; }
    void setActiveDocument(std::shared_ptr<PartDocument> document);
    
    const std::vector<std::shared_ptr<PartDocument>>& getDocuments() const { return m_documents; }
    
    // Assembly management
    std::shared_ptr<Assembly> createAssembly(const std::string& name = "Assembly");
    
    // Part library management
    void addToLibrary(CADObjectPtr part, const std::string& category = "General");
    void removeFromLibrary(const std::string& name);
    CADObjectPtr getFromLibrary(const std::string& name) const;
    
    std::vector<std::string> getLibraryCategories() const;
    std::vector<std::string> getLibraryParts(const std::string& category) const;
    
    // Template management
    void saveAsTemplate(CADObjectPtr object, const std::string& name, const std::string& category = "General");
    CADObjectPtr createFromTemplate(const std::string& name) const;
    
    // Import/Export
    bool importSTEP(const std::string& filePath, std::shared_ptr<PartDocument> document);
    bool exportSTEP(const std::string& filePath, std::shared_ptr<PartDocument> document);
    bool importIGES(const std::string& filePath, std::shared_ptr<PartDocument> document);
    bool exportIGES(const std::string& filePath, std::shared_ptr<PartDocument> document);
    
    // Utility functions
    void generateThumbnail(CADObjectPtr object, const std::string& imagePath);
    void optimizeForPerformance(CADObjectPtr object);
    void validateGeometry(CADObjectPtr object, std::vector<std::string>& issues);

private:
    std::vector<std::shared_ptr<PartDocument>> m_documents;
    std::shared_ptr<PartDocument> m_activeDocument;
    
    // Part library
    struct LibraryEntry {
        CADObjectPtr part;
        std::string name;
        std::string category;
        std::string description;
        std::string thumbnailPath;
    };
    
    std::vector<LibraryEntry> m_libraryParts;
    std::unordered_map<std::string, std::vector<std::string>> m_libraryCategories;
    
    // Templates
    std::vector<LibraryEntry> m_templates;
    
    // Helper methods
    bool loadFromFile(const std::string& filePath, std::shared_ptr<PartDocument> document);
    bool saveToFile(const std::string& filePath, std::shared_ptr<PartDocument> document);
    std::string generateUniqueObjectName(const std::string& baseName, std::shared_ptr<PartDocument> document);
};

} // namespace HybridCAD 