#pragma once

#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "CADTypes.h"

namespace HybridCAD {

// Mesh editing structures
struct Vertex {
    Point3D position;
    Vector3D normal;
    int id;
    bool selected;
    
    Vertex(const Point3D& pos = Point3D(), int vertexId = -1) 
        : position(pos), id(vertexId), selected(false) {}
};

struct Edge {
    int vertex1, vertex2;
    int id;
    bool selected;
    std::vector<int> adjacentFaces;
    
    Edge(int v1 = -1, int v2 = -1, int edgeId = -1) 
        : vertex1(v1), vertex2(v2), id(edgeId), selected(false) {}
};

struct MeshFace {
    std::vector<int> vertices;
    Vector3D normal;
    int id;
    bool selected;
    
    MeshFace(const std::vector<int>& verts = {}, int faceId = -1) 
        : vertices(verts), id(faceId), selected(false) {}
};

// Mesh selection modes
enum class SelectionMode {
    VERTEX,
    EDGE,
    FACE,
    OBJECT
};

// Mesh editing tools
enum class MeshTool {
    SELECT,
    EXTRUDE,
    INSET,
    KNIFE,
    LOOP_CUT,
    SUBDIVIDE,
    MERGE,
    SEPARATE,
    BRIDGE
};

class MeshObject : public CADObject {
public:
    MeshObject(const std::string& name = "Mesh");
    ~MeshObject() = default;
    
    ObjectType getType() const override { return ObjectType::MESH; }
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    
    // Mesh data access
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<Edge>& getEdges() const { return m_edges; }
    const std::vector<MeshFace>& getFaces() const { return m_faces; }
    
    std::vector<Vertex>& getVertices() { return m_vertices; }
    std::vector<Edge>& getEdges() { return m_edges; }
    std::vector<MeshFace>& getFaces() { return m_faces; }
    
    // Mesh creation from geometry
    void createFromTriangles(const std::vector<Triangle>& triangles);
    void createFromGeometry(const std::vector<Point3D>& vertices, const std::vector<Face>& faces);
    
    // Selection
    void selectVertex(int vertexId, bool addToSelection = false);
    void selectEdge(int edgeId, bool addToSelection = false);
    void selectFace(int faceId, bool addToSelection = false);
    void deselectAll();
    
    const std::unordered_set<int>& getSelectedVertices() const { return m_selectedVertices; }
    const std::unordered_set<int>& getSelectedEdges() const { return m_selectedEdges; }
    const std::unordered_set<int>& getSelectedFaces() const { return m_selectedFaces; }
    
    // Mesh validation and repair
    bool isValid() const;
    void recalculateNormals();
    void removeDuplicateVertices(float tolerance = 1e-6f);
    void removeUnusedVertices();
    
    // Bounding box
    Point3D getBoundingBoxMin() const override;
    Point3D getBoundingBoxMax() const override;

protected:
    std::vector<Vertex> m_vertices;
    std::vector<Edge> m_edges;
    std::vector<MeshFace> m_faces;
    
    std::unordered_set<int> m_selectedVertices;
    std::unordered_set<int> m_selectedEdges;
    std::unordered_set<int> m_selectedFaces;
    
    int m_nextVertexId;
    int m_nextEdgeId;
    int m_nextFaceId;
    
    void buildTopology();
    void updateNormals();
};

class MeshManager {
public:
    MeshManager();
    ~MeshManager();
    
    // Mesh creation
    std::shared_ptr<MeshObject> createMesh(const std::string& name = "Mesh");
    std::shared_ptr<MeshObject> createPrimitiveMesh(ObjectType type, const std::string& name = "Primitive");
    std::shared_ptr<MeshObject> convertToMesh(CADObjectPtr cadObject);
    
    // Selection modes
    void setSelectionMode(SelectionMode mode) { m_selectionMode = mode; }
    SelectionMode getSelectionMode() const { return m_selectionMode; }
    
    // Active tool
    void setActiveTool(MeshTool tool) { m_activeTool = tool; }
    MeshTool getActiveTool() const { return m_activeTool; }
    
    // Mesh editing operations
    bool extrudeFaces(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& faceIds, 
                     const Vector3D& direction, float distance);
    bool insetFaces(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& faceIds, 
                   float insetAmount);
    bool subdivideEdges(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& edgeIds);
    bool subdivideSelected(std::shared_ptr<MeshObject> mesh);
    
    // Vertex operations
    bool mergeVertices(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& vertexIds);
    bool dissolveVertices(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& vertexIds);
    
    // Edge operations
    bool extrudeEdges(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& edgeIds, 
                     const Vector3D& direction, float distance);
    bool bridgeEdgeLoops(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& edgeIds1,
                        const std::unordered_set<int>& edgeIds2);
    bool knifeProject(std::shared_ptr<MeshObject> mesh, const Point3D& start, const Point3D& end);
    
    // Smoothing and modifiers
    bool smoothMesh(std::shared_ptr<MeshObject> mesh, int iterations = 1, float factor = 0.5f);
    bool decimateMesh(std::shared_ptr<MeshObject> mesh, float ratio);
    bool applySubdivisionSurface(std::shared_ptr<MeshObject> mesh, int levels = 1);
    
    // Boolean operations on meshes
    std::shared_ptr<MeshObject> booleanUnion(std::shared_ptr<MeshObject> meshA, 
                                           std::shared_ptr<MeshObject> meshB);
    std::shared_ptr<MeshObject> booleanDifference(std::shared_ptr<MeshObject> meshA, 
                                                 std::shared_ptr<MeshObject> meshB);
    std::shared_ptr<MeshObject> booleanIntersection(std::shared_ptr<MeshObject> meshA, 
                                                   std::shared_ptr<MeshObject> meshB);
    
    // Utility functions
    bool selectByRay(std::shared_ptr<MeshObject> mesh, const Point3D& rayOrigin, 
                    const Vector3D& rayDirection);
    void clearSelection(std::shared_ptr<MeshObject> mesh);
    void invertSelection(std::shared_ptr<MeshObject> mesh);
    void selectAll(std::shared_ptr<MeshObject> mesh);
    
    // Import/Export
    bool importOBJ(const std::string& filename, std::shared_ptr<MeshObject> mesh);
    bool exportOBJ(const std::string& filename, std::shared_ptr<MeshObject> mesh);
    bool importSTL(const std::string& filename, std::shared_ptr<MeshObject> mesh);
    bool exportSTL(const std::string& filename, std::shared_ptr<MeshObject> mesh);

private:
    SelectionMode m_selectionMode;
    MeshTool m_activeTool;
    
    // Helper functions
    void calculateFaceNormal(MeshObject* mesh, int faceId);
    void calculateVertexNormal(MeshObject* mesh, int vertexId);
    std::vector<int> getAdjacentVertices(const MeshObject* mesh, int vertexId);
    std::vector<int> getAdjacentFaces(const MeshObject* mesh, int vertexId);
    bool isEdgeManifold(const MeshObject* mesh, int edgeId);
    
    // Subdivision algorithms
    void catmullClarkSubdivision(MeshObject* mesh);
    void loopSubdivision(MeshObject* mesh);
    
    // Mesh analysis
    bool isMeshManifold(const MeshObject* mesh);
    void findBoundaryEdges(const MeshObject* mesh, std::vector<int>& boundaryEdges);
    void findNonManifoldEdges(const MeshObject* mesh, std::vector<int>& nonManifoldEdges);
};

} // namespace HybridCAD 