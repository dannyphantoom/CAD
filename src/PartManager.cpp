#include "PartManager.h"
#include <algorithm>
#include <GL/gl.h>

namespace HybridCAD {

// Assembly implementation
Assembly::Assembly(const std::string& name) 
    : CADObject(name), m_constraintsDirty(false) {
}

void Assembly::render() const {
    if (!m_visible) return;
    
    for (const auto& instance : m_partInstances) {
        if (instance.visible && instance.part) {
            // Apply transform and render part
            glPushMatrix();
            
            // Apply the transform matrix
            const auto& matrix = instance.transform.matrix;
            float glMatrix[16];
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    glMatrix[j * 4 + i] = matrix(i, j);
                }
            }
            glMultMatrixf(glMatrix);
            
            instance.part->render();
            glPopMatrix();
        }
    }
}

bool Assembly::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    for (const auto& instance : m_partInstances) {
        if (instance.visible && instance.part) {
            // For simplicity, check intersection without transform
            if (instance.part->intersects(rayOrigin, rayDirection)) {
                return true;
            }
        }
    }
    return false;
}

void Assembly::addPart(CADObjectPtr part, const std::string& instanceName) {
    if (!part) return;
    
    std::string name = instanceName.empty() ? part->getName() : instanceName;
    
    PartInstance instance(part, name);
    m_partInstances.push_back(instance);
    
    // Set the parent of the added part
    part->setParent(this);
    
    // If this assembly is nested, make it transparent and the added part opaque
    if (getParent()) {
        Material assemblyMaterial = getMaterial();
        assemblyMaterial.transparency = 0.5f; // Semi-transparent
        setMaterial(assemblyMaterial);
        
        Material partMaterial = part->getMaterial();
        partMaterial.transparency = 0.0f; // Opaque
        part->setMaterial(partMaterial);
    }
}

void Assembly::removePart(CADObjectPtr part) {
    m_partInstances.erase(
        std::remove_if(m_partInstances.begin(), m_partInstances.end(),
                      [part](const PartInstance& instance) { 
                          return instance.part == part; 
                      }),
        m_partInstances.end());
}

void Assembly::removePartInstance(const std::string& instanceName) {
    m_partInstances.erase(
        std::remove_if(m_partInstances.begin(), m_partInstances.end(),
                      [&instanceName](const PartInstance& instance) { 
                          return instance.instanceName == instanceName; 
                      }),
        m_partInstances.end());
}

PartInstance* Assembly::getPartInstance(const std::string& instanceName) {
    for (auto& instance : m_partInstances) {
        if (instance.instanceName == instanceName) {
            return &instance;
        }
    }
    return nullptr;
}

void Assembly::setPartTransform(const std::string& instanceName, const Transform& transform) {
    PartInstance* instance = getPartInstance(instanceName);
    if (instance) {
        instance->transform = transform;
        m_constraintsDirty = true;
    }
}

Transform Assembly::getPartTransform(const std::string& instanceName) const {
    for (const auto& instance : m_partInstances) {
        if (instance.instanceName == instanceName) {
            return instance.transform;
        }
    }
    return Transform();
}

void Assembly::addConstraint(const AssemblyConstraint& constraint) {
    m_constraints.push_back(constraint);
    m_constraintsDirty = true;
}

void Assembly::removeConstraint(size_t index) {
    if (index < m_constraints.size()) {
        m_constraints.erase(m_constraints.begin() + index);
        m_constraintsDirty = true;
    }
}

void Assembly::updateConstraint(size_t index, const AssemblyConstraint& constraint) {
    if (index < m_constraints.size()) {
        m_constraints[index] = constraint;
        m_constraintsDirty = true;
    }
}

bool Assembly::solveConstraints() {
    // Placeholder constraint solver
    m_constraintsDirty = false;
    return true;
}

void Assembly::updateAssembly() {
    if (m_constraintsDirty) {
        solveConstraints();
    }
}

Point3D Assembly::getBoundingBoxMin() const {
    if (m_partInstances.empty()) {
        return Point3D(0, 0, 0);
    }
    
    Point3D min(std::numeric_limits<double>::max(), 
                std::numeric_limits<double>::max(), 
                std::numeric_limits<double>::max());
    
    for (const auto& instance : m_partInstances) {
        if (instance.visible && instance.part) {
            // For simplicity, use basic bounding box calculation
            Point3D partMin(-1, -1, -1);
            Point3D partMax(1, 1, 1);
            
            min.x = std::min(min.x, partMin.x);
            min.y = std::min(min.y, partMin.y);
            min.z = std::min(min.z, partMin.z);
        }
    }
    
    return min;
}

Point3D Assembly::getBoundingBoxMax() const {
    if (m_partInstances.empty()) {
        return Point3D(0, 0, 0);
    }
    
    Point3D max(std::numeric_limits<double>::lowest(), 
                std::numeric_limits<double>::lowest(), 
                std::numeric_limits<double>::lowest());
    
    for (const auto& instance : m_partInstances) {
        if (instance.visible && instance.part) {
            // For simplicity, use basic bounding box calculation
            Point3D partMin(-1, -1, -1);
            Point3D partMax(1, 1, 1);
            
            max.x = std::max(max.x, partMax.x);
            max.y = std::max(max.y, partMax.y);
            max.z = std::max(max.z, partMax.z);
        }
    }
    
    return max;
}

bool Assembly::hasCollisions() const {
    // Placeholder collision detection
    return false;
}

std::vector<std::pair<std::string, std::string>> Assembly::getCollisions() const {
    // Placeholder collision detection
    return {};
}

void Assembly::applyConstraint(const AssemblyConstraint& constraint) {
    // Placeholder constraint application
}

bool Assembly::checkCollision(const PartInstance& instanceA, const PartInstance& instanceB) const {
    // Placeholder collision check
    return false;
}

// PartDocument implementation
PartDocument::PartDocument(const std::string& name) 
    : m_name(name), m_dirty(false), m_maxUndoLevels(50) {
}

void PartDocument::addObject(CADObjectPtr object) {
    if (object) {
        m_objects.push_back(object);
        setDirty(true);
    }
}

void PartDocument::removeObject(CADObjectPtr object) {
    m_objects.erase(
        std::remove(m_objects.begin(), m_objects.end(), object),
        m_objects.end());
    setDirty(true);
}

void PartDocument::clearObjects() {
    m_objects.clear();
    setDirty(true);
}

CADObjectPtr PartDocument::findObject(const std::string& name) const {
    for (const auto& object : m_objects) {
        if (object && object->getName() == name) {
            return object;
        }
    }
    return nullptr;
}

void PartDocument::beginUndoGroup(const std::string& description) {
    // Placeholder undo system
}

void PartDocument::endUndoGroup() {
    // Placeholder undo system
}

void PartDocument::addUndoCommand(const std::string& description) {
    // Placeholder undo system
}

bool PartDocument::canUndo() const {
    return !m_undoStack.empty();
}

bool PartDocument::canRedo() const {
    return !m_redoStack.empty();
}

void PartDocument::undo() {
    // Placeholder undo implementation
}

void PartDocument::redo() {
    // Placeholder redo implementation
}

void PartDocument::clearHistory() {
    m_undoStack.clear();
    m_redoStack.clear();
}

// PartManager implementation
PartManager::PartManager() {
}

PartManager::~PartManager() {
}

std::shared_ptr<PartDocument> PartManager::createDocument(const std::string& name) {
    auto document = std::make_shared<PartDocument>(name);
    m_documents.push_back(document);
    
    if (!m_activeDocument) {
        m_activeDocument = document;
    }
    
    return document;
}

std::shared_ptr<PartDocument> PartManager::openDocument(const std::string& filePath) {
    auto document = std::make_shared<PartDocument>("Document");
    document->setFilePath(filePath);
    
    if (loadFromFile(filePath, document)) {
        m_documents.push_back(document);
        m_activeDocument = document;
        return document;
    }
    
    return nullptr;
}

bool PartManager::saveDocument(std::shared_ptr<PartDocument> document) {
    if (!document || document->getFilePath().empty()) {
        return false;
    }
    
    return saveToFile(document->getFilePath(), document);
}

bool PartManager::saveDocumentAs(std::shared_ptr<PartDocument> document, const std::string& filePath) {
    if (!document) return false;
    
    if (saveToFile(filePath, document)) {
        document->setFilePath(filePath);
        document->setDirty(false);
        return true;
    }
    
    return false;
}

void PartManager::closeDocument(std::shared_ptr<PartDocument> document) {
    if (!document) return;
    
    m_documents.erase(
        std::remove(m_documents.begin(), m_documents.end(), document),
        m_documents.end());
    
    if (m_activeDocument == document) {
        m_activeDocument = m_documents.empty() ? nullptr : m_documents.front();
    }
}

void PartManager::setActiveDocument(std::shared_ptr<PartDocument> document) {
    if (std::find(m_documents.begin(), m_documents.end(), document) != m_documents.end()) {
        m_activeDocument = document;
    }
}

std::shared_ptr<Assembly> PartManager::createAssembly(const std::string& name) {
    return std::make_shared<Assembly>(name);
}

void PartManager::addToLibrary(CADObjectPtr part, const std::string& category) {
    if (!part) return;
    
    LibraryEntry entry;
    entry.part = part;
    entry.name = part->getName();
    entry.category = category;
    entry.description = "";
    entry.thumbnailPath = "";
    
    m_libraryParts.push_back(entry);
    m_libraryCategories[category].push_back(entry.name);
}

void PartManager::removeFromLibrary(const std::string& name) {
    m_libraryParts.erase(
        std::remove_if(m_libraryParts.begin(), m_libraryParts.end(),
                      [&name](const LibraryEntry& entry) { 
                          return entry.name == name; 
                      }),
        m_libraryParts.end());
}

CADObjectPtr PartManager::getFromLibrary(const std::string& name) const {
    for (const auto& entry : m_libraryParts) {
        if (entry.name == name) {
            return entry.part;
        }
    }
    return nullptr;
}

std::vector<std::string> PartManager::getLibraryCategories() const {
    std::vector<std::string> categories;
    for (const auto& pair : m_libraryCategories) {
        categories.push_back(pair.first);
    }
    return categories;
}

std::vector<std::string> PartManager::getLibraryParts(const std::string& category) const {
    auto it = m_libraryCategories.find(category);
    if (it != m_libraryCategories.end()) {
        return it->second;
    }
    return {};
}

void PartManager::saveAsTemplate(CADObjectPtr object, const std::string& name, const std::string& category) {
    if (!object) return;
    
    LibraryEntry entry;
    entry.part = object;
    entry.name = name;
    entry.category = category;
    entry.description = "";
    entry.thumbnailPath = "";
    
    m_templates.push_back(entry);
}

CADObjectPtr PartManager::createFromTemplate(const std::string& name) const {
    for (const auto& entry : m_templates) {
        if (entry.name == name) {
            return entry.part; // In a real implementation, this would create a copy
        }
    }
    return nullptr;
}

bool PartManager::importSTEP(const std::string& filePath, std::shared_ptr<PartDocument> document) {
    // Placeholder STEP import
    return false;
}

bool PartManager::exportSTEP(const std::string& filePath, std::shared_ptr<PartDocument> document) {
    // Placeholder STEP export
    return false;
}

bool PartManager::importIGES(const std::string& filePath, std::shared_ptr<PartDocument> document) {
    // Placeholder IGES import
    return false;
}

bool PartManager::exportIGES(const std::string& filePath, std::shared_ptr<PartDocument> document) {
    // Placeholder IGES export
    return false;
}

void PartManager::generateThumbnail(CADObjectPtr object, const std::string& imagePath) {
    // Placeholder thumbnail generation
}

void PartManager::optimizeForPerformance(CADObjectPtr object) {
    // Placeholder performance optimization
}

void PartManager::validateGeometry(CADObjectPtr object, std::vector<std::string>& issues) {
    issues.clear();
    
    if (!object) {
        issues.push_back("Null object");
        return;
    }
    
    // Basic validation
    if (object->getName().empty()) {
        issues.push_back("Object has no name");
    }
}

bool PartManager::loadFromFile(const std::string& filePath, std::shared_ptr<PartDocument> document) {
    // Placeholder file loading
    return false;
}

bool PartManager::saveToFile(const std::string& filePath, std::shared_ptr<PartDocument> document) {
    // Placeholder file saving
    return false;
}

std::string PartManager::generateUniqueObjectName(const std::string& baseName, std::shared_ptr<PartDocument> document) {
    if (!document) return baseName;
    
    std::string name = baseName;
    int counter = 1;
    
    while (document->findObject(name)) {
        name = baseName + "_" + std::to_string(counter++);
    }
    
    return name;
}

} // namespace HybridCAD 